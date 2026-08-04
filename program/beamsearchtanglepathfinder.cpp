#include "tanglepathsearch.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <map>
#include <numeric>
#include <tuple>
#include <vector>
#include <QElapsedTimer>

namespace
{
typedef std::vector<int> IntPath;

struct BeamState
{
    int current;
    IntPath path;
    std::vector<int> visits;
    double score;
    double optimisticScore;
};

struct BeamCandidate
{
    IntPath path;
    std::vector<int> visits;
    double score;
    double coverageMad;
    double explainedLengthFraction;
    double copyAgreement;
};

double median(std::vector<double> values)
{
    if (values.empty())
        return std::numeric_limits<double>::quiet_NaN();
    std::sort(values.begin(), values.end());
    const size_t middle = values.size() / 2;
    if (values.size() % 2)
        return values[middle];
    return (values[middle - 1] + values[middle]) / 2.0;
}

double huber(double value, double delta)
{
    const double absolute = std::abs(value);
    if (absolute <= delta)
        return 0.5 * value * value;
    return delta * (absolute - 0.5 * delta);
}

double nodePenalty(const TangleSegment &segment, int count, double singleCopy,
                   int expectedCount, const TanglePathParameters &parameters)
{
    if (count == 0)
        return parameters.lambdaMissing * segment.length;
    const double observed = std::max(segment.coverage, singleCopy * 1e-6);
    const double logRatio = std::log(observed / (count * singleCopy));
    const double coveragePenalty = segment.length * huber(logRatio, parameters.beamHuberDelta);
    const int extra = std::max(0, count - expectedCount);
    return coveragePenalty + parameters.lambdaExtra * segment.length * extra * extra;
}

std::vector<bool> reachableToTarget(const TangleGraph &graph, int targetSegment)
{
    std::vector<std::vector<int> > reverse(graph.adjacency.size());
    for (int node = 0; node < graph.adjacency.size(); ++node)
    {
        const QVector<TangleEdge> &edges = graph.adjacency[node];
        for (int edge = 0; edge < edges.size(); ++edge)
            reverse[edges[edge].target].push_back(node);
    }

    const int plus = 2 * targetSegment;
    std::vector<bool> reachable(graph.adjacency.size(), false);
    std::deque<int> queue;
    queue.push_back(plus);
    queue.push_back(plus ^ 1);
    reachable[plus] = true;
    reachable[plus ^ 1] = true;
    while (!queue.empty())
    {
        const int node = queue.front();
        queue.pop_front();
        const std::vector<int> &previousNodes = reverse[node];
        for (size_t i = 0; i < previousNodes.size(); ++i)
        {
            const int previous = previousNodes[i];
            if (!reachable[previous])
            {
                reachable[previous] = true;
                queue.push_back(previous);
            }
        }
    }
    return reachable;
}

bool stateLess(const BeamState &left, const BeamState &right)
{
    if (left.optimisticScore != right.optimisticScore)
        return left.optimisticScore < right.optimisticScore;
    if (left.score != right.score)
        return left.score < right.score;
    if (left.path.size() != right.path.size())
        return left.path.size() < right.path.size();
    return left.path < right.path;
}

bool candidateLess(const BeamCandidate &left, const BeamCandidate &right)
{
    if (left.score != right.score)
        return left.score < right.score;
    if (left.path.size() != right.path.size())
        return left.path.size() < right.path.size();
    return left.path < right.path;
}

std::tuple<double, double, double> candidateMetrics(
        const TangleGraph &graph,
        const std::vector<int> &internalSegments,
        const std::vector<int> &expected,
        const std::vector<int> &visits,
        double singleCopy)
{
    std::vector<double> differences;
    double totalLength = 0.0;
    double explainedLength = 0.0;
    int visited = 0;
    int agreed = 0;
    for (size_t i = 0; i < internalSegments.size(); ++i)
    {
        const TangleSegment &segment = graph.segments[internalSegments[i]];
        totalLength += segment.length;
        if (visits[i] > 0)
        {
            differences.push_back(std::abs(segment.coverage / visits[i] - singleCopy));
            explainedLength += segment.length;
            ++visited;
            if (visits[i] == expected[i])
                ++agreed;
        }
    }
    return std::make_tuple(median(differences),
                           totalLength > 0.0 ? explainedLength / totalLength : 1.0,
                           visited > 0 ? double(agreed) / visited
                                       : std::numeric_limits<double>::quiet_NaN());
}
}

TanglePathSearchResult runBeamTanglePathSearch(const TanglePathSearchRequest &request,
                                               std::atomic_bool *cancelled)
{
    QElapsedTimer timer;
    timer.start();
    TanglePathSearchResult result;
    result.status = "BEAM_SEARCH";
    const TangleGraph &graph = request.graph;
    const TanglePathParameters &parameters = request.parameters;

    if (request.source == request.target)
        result.errorMessage = "Start and end nodes must be different.";
    else if (!graph.segmentIndex.contains(request.source) ||
             !graph.segmentIndex.contains(request.target))
        result.errorMessage = "Start or end node is outside the selected subgraph.";
    if (!result.errorMessage.isEmpty())
        return result;

    const int sourceSegment = graph.segmentIndex.value(request.source);
    const int targetSegment = graph.segmentIndex.value(request.target);
    const double sourceCoverage = graph.segments[sourceSegment].coverage;
    const double targetCoverage = graph.segments[targetSegment].coverage;
    if (sourceCoverage <= 0.0 || targetCoverage <= 0.0)
    {
        result.errorMessage = "Start and end coverage must be greater than zero.";
        return result;
    }
    const double singleCopy = median(std::vector<double>{sourceCoverage, targetCoverage});

    std::vector<int> internalSegments;
    QHash<int, int> internalIndex;
    for (int segment = 0; segment < graph.segments.size(); ++segment)
    {
        if (segment == sourceSegment || segment == targetSegment)
            continue;
        internalIndex.insert(segment, internalSegments.size());
        internalSegments.push_back(segment);
    }

    std::vector<int> expected;
    std::vector<int> maximum;
    std::vector<std::vector<double> > penalties;
    std::vector<std::vector<double> > optimisticTables;
    for (size_t i = 0; i < internalSegments.size(); ++i)
    {
        const TangleSegment &segment = graph.segments[internalSegments[i]];
        const double ratio = segment.coverage / singleCopy;
        const int expectedCount = std::max(1, int(std::floor(ratio + 0.5)));
        const int maxVisits = std::max(1, std::min(parameters.maxCopy,
                int(std::ceil(segment.coverage / (singleCopy * parameters.tauMin)))));
        expected.push_back(expectedCount);
        maximum.push_back(maxVisits);
        std::vector<double> table;
        for (int count = 0; count <= maxVisits; ++count)
            table.push_back(nodePenalty(segment, count, singleCopy, expectedCount, parameters));
        penalties.push_back(table);

        std::vector<double> optimistic(table.size());
        double bestFuture = std::numeric_limits<double>::infinity();
        for (int count = maxVisits; count >= 0; --count)
        {
            bestFuture = std::min(bestFuture, table[count]);
            optimistic[count] = bestFuture;
        }
        optimisticTables.push_back(optimistic);
    }

    double initialScore = 0.0;
    double initialOptimistic = 0.0;
    for (size_t i = 0; i < penalties.size(); ++i)
    {
        initialScore += penalties[i][0];
        initialOptimistic += optimisticTables[i][0];
    }

    const std::vector<bool> reachable = reachableToTarget(graph, targetSegment);
    std::vector<BeamState> beam;
    const int sourcePlus = 2 * sourceSegment;
    for (int orientation = 0; orientation < 2; ++orientation)
    {
        const int id = sourcePlus ^ orientation;
        if (reachable[id])
        {
            BeamState state;
            state.current = id;
            state.path.push_back(id);
            state.visits.assign(internalSegments.size(), 0);
            state.score = initialScore;
            state.optimisticScore = initialOptimistic;
            beam.push_back(state);
        }
    }
    if (beam.empty())
    {
        result.errorMessage = "No oriented route connects the selected boundaries.";
        return result;
    }

    int maxSteps = 10;
    for (size_t i = 0; i < maximum.size(); ++i)
        maxSteps += maximum[i];
    std::map<IntPath, BeamCandidate> completed;

    for (int step = 1; step <= maxSteps; ++step)
    {
        if (cancelled != 0 && cancelled->load())
        {
            result.cancelled = true;
            result.status = "CANCELLED";
            result.elapsedMs = timer.elapsed();
            return result;
        }

        std::map<std::pair<int, std::vector<int> >, BeamState> expanded;
        for (size_t stateIndex = 0; stateIndex < beam.size(); ++stateIndex)
        {
            const BeamState &state = beam[stateIndex];
            const QVector<TangleEdge> &edges = graph.adjacency[state.current];
            for (int edgeIndex = 0; edgeIndex < edges.size(); ++edgeIndex)
            {
                const int next = edges[edgeIndex].target;
                const int nextSegment = graph.orientedToSegment[next];
                if (!reachable[next] || nextSegment == sourceSegment)
                    continue;
                IntPath newPath = state.path;
                newPath.push_back(next);
                if (nextSegment == targetSegment)
                {
                    double mad;
                    double explained;
                    double agreement;
                    std::tie(mad, explained, agreement) = candidateMetrics(
                                graph, internalSegments, expected, state.visits, singleCopy);
                    BeamCandidate candidate;
                    candidate.path = newPath;
                    candidate.visits = state.visits;
                    candidate.score = state.score + parameters.lambdaStep;
                    candidate.coverageMad = mad;
                    candidate.explainedLengthFraction = explained;
                    candidate.copyAgreement = agreement;
                    std::map<IntPath, BeamCandidate>::iterator found = completed.find(candidate.path);
                    if (found == completed.end() || candidateLess(candidate, found->second))
                        completed[candidate.path] = candidate;
                    continue;
                }

                if (!internalIndex.contains(nextSegment))
                    continue;
                const int index = internalIndex.value(nextSegment);
                const int oldCount = state.visits[index];
                const int newCount = oldCount + 1;
                if (newCount > maximum[index])
                    continue;

                BeamState child;
                child.current = next;
                child.path = newPath;
                child.visits = state.visits;
                child.visits[index] = newCount;
                child.score = state.score - penalties[index][oldCount] +
                        penalties[index][newCount] + parameters.lambdaStep;
                child.optimisticScore = state.optimisticScore -
                        optimisticTables[index][oldCount] +
                        optimisticTables[index][newCount] + parameters.lambdaStep;
                const std::pair<int, std::vector<int> > signature(next, child.visits);
                std::map<std::pair<int, std::vector<int> >, BeamState>::iterator found =
                        expanded.find(signature);
                if (found == expanded.end() || stateLess(child, found->second))
                    expanded[signature] = child;
            }
        }
        if (expanded.empty())
            break;

        std::map<int, std::vector<BeamState> > groups;
        for (std::map<std::pair<int, std::vector<int> >, BeamState>::iterator it = expanded.begin();
             it != expanded.end(); ++it)
            groups[it->second.current].push_back(it->second);

        std::vector<BeamState> diverse;
        for (std::map<int, std::vector<BeamState> >::iterator group = groups.begin();
             group != groups.end(); ++group)
        {
            std::vector<BeamState> &states = group->second;
            std::sort(states.begin(), states.end(), stateLess);
            const size_t keep = std::min<size_t>(parameters.perNodeBeam, states.size());
            diverse.insert(diverse.end(), states.begin(), states.begin() + keep);
        }
        std::sort(diverse.begin(), diverse.end(), stateLess);
        if (diverse.size() > size_t(parameters.beamSize))
            diverse.resize(parameters.beamSize);
        beam.swap(diverse);

        if (completed.size() >= size_t(parameters.topK))
        {
            std::vector<const BeamCandidate *> ranked;
            for (std::map<IntPath, BeamCandidate>::const_iterator it = completed.begin();
                 it != completed.end(); ++it)
                ranked.push_back(&it->second);
            std::sort(ranked.begin(), ranked.end(),
                      [](const BeamCandidate *left, const BeamCandidate *right)
                      { return candidateLess(*left, *right); });
            if (!beam.empty() && beam.front().optimisticScore >= ranked[parameters.topK - 1]->score)
                break;
        }
    }

    std::vector<BeamCandidate> candidates;
    for (std::map<IntPath, BeamCandidate>::const_iterator it = completed.begin();
         it != completed.end(); ++it)
        candidates.push_back(it->second);
    std::sort(candidates.begin(), candidates.end(), candidateLess);
    if (candidates.size() > size_t(parameters.topK))
        candidates.resize(parameters.topK);
    if (candidates.empty())
    {
        result.errorMessage = "No path was found within the coverage-derived copy limits.";
        result.elapsedMs = timer.elapsed();
        return result;
    }

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        TanglePathCandidate output;
        for (size_t node = 0; node < candidates[i].path.size(); ++node)
            output.orientedNodeNames << graph.label(candidates[i].path[node]);
        output.score = candidates[i].score;
        output.coverageMad = candidates[i].coverageMad;
        output.explainedLengthFraction = candidates[i].explainedLengthFraction;
        output.copyAgreement = candidates[i].copyAgreement;
        output.weightedReadSupport = std::numeric_limits<double>::quiet_NaN();
        result.candidates << output;
    }
    result.status = "OK";
    result.elapsedMs = timer.elapsed();
    return result;
}
