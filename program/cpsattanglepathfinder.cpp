#include "tanglepathsearch.h"

#ifndef BANDAGE_WITH_ORTOOLS

TanglePathSearchResult runCpSatTanglePathSearch(const TanglePathSearchRequest &,
                                                std::atomic_bool *)
{
    TanglePathSearchResult result;
    result.status = "ORTOOLS_UNAVAILABLE";
    result.errorMessage = "This developer build was compiled without OR-Tools.";
    return result;
}

#else

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <vector>
#include <QElapsedTimer>

#if defined(_MSC_VER) && !defined(OR_PROTO_DLL)
#define OR_PROTO_DLL __declspec(dllimport)
#endif

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/sat_parameters.pb.h"
#include "ortools/util/sorted_interval_list.h"
#include "ortools/util/time_limit.h"

namespace sat = operations_research::sat;
using operations_research::Domain;
using operations_research::TimeLimit;

namespace
{
typedef std::vector<int> IntPath;
const qint64 objectiveScale = 1000000;

struct ContextEvidence
{
    IntPath pattern;
    double fraction;
    int expectedCount;
};

struct ReadEvidence
{
    QString readId;
    int selectedAlignments;
    double confidence;
    std::vector<ContextEvidence> fullThreads;
    std::vector<ContextEvidence> contexts;
};

struct Bound
{
    int segment;
    int minimum;
    int maximum;
};

struct SolveAttempt
{
    sat::CpSolverStatus status;
    IntPath path;
    double objectiveValue;
    double bestBound;

    SolveAttempt()
        : status(sat::CpSolverStatus::UNKNOWN), objectiveValue(0.0), bestBound(0.0)
    {
    }
};

double median(std::vector<double> values)
{
    if (values.empty())
        return 1.0;
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

IntPath reverseComplement(const IntPath &path)
{
    IntPath result;
    result.reserve(path.size());
    for (IntPath::const_reverse_iterator it = path.rbegin(); it != path.rend(); ++it)
        result.push_back(*it ^ 1);
    return result;
}

IntPath canonicalPattern(const IntPath &path)
{
    const IntPath reverse = reverseComplement(path);
    return reverse < path ? reverse : path;
}

double selectionScore(const TangleReadAlignment &alignment)
{
    return alignment.hasAlignmentScore ? alignment.alignmentScore
                                       : alignment.residueMatches;
}

double alignedFraction(const TangleReadAlignment &alignment)
{
    if (alignment.queryLength <= 0)
        return 0.0;
    return std::max(0.0, std::min(1.0,
            double(alignment.queryEnd - alignment.queryStart) / alignment.queryLength));
}

std::vector<double> alignmentWeights(
        const std::vector<const TangleReadAlignment *> &retained,
        double asFraction)
{
    if (retained.empty())
        return std::vector<double>();
    double bestScore = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < retained.size(); ++i)
        bestScore = std::max(bestScore, selectionScore(*retained[i]));
    const double cutoff = bestScore >= 0.0 ? bestScore * asFraction
                                           : bestScore / asFraction;
    std::vector<double> result(retained.size());
    double total = 0.0;
    for (size_t i = 0; i < retained.size(); ++i)
    {
        const TangleReadAlignment &alignment = *retained[i];
        const double margin = std::max(0.0, selectionScore(alignment) - cutoff);
        const double specificity = alignment.identity * alignedFraction(alignment);
        result[i] = std::pow(margin + 1e-6, 2.0) * std::max(1e-6, specificity);
        total += result[i];
    }
    if (total <= 0.0)
        return std::vector<double>(retained.size(), 1.0 / retained.size());
    for (size_t i = 0; i < result.size(); ++i)
        result[i] /= total;
    return result;
}

std::vector<ReadEvidence> buildEvidence(const QVector<TangleReadAlignment> &alignments,
                                        const TanglePathParameters &parameters)
{
    std::map<QString, std::vector<const TangleReadAlignment *> > grouped;
    for (int i = 0; i < alignments.size(); ++i)
        if (alignments[i].path.size() >= 2)
            grouped[alignments[i].readId].push_back(&alignments[i]);

    std::map<QString, std::vector<const TangleReadAlignment *> > selected;
    for (std::map<QString, std::vector<const TangleReadAlignment *> >::iterator group = grouped.begin();
         group != grouped.end(); ++group)
    {
        int bestMapq = -1;
        for (size_t i = 0; i < group->second.size(); ++i)
            bestMapq = std::max(bestMapq, group->second[i]->mappingQuality);
        std::vector<const TangleReadAlignment *> layer;
        for (size_t i = 0; i < group->second.size(); ++i)
            if (group->second[i]->mappingQuality == bestMapq)
                layer.push_back(group->second[i]);

        double bestScore = -std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < layer.size(); ++i)
            bestScore = std::max(bestScore, selectionScore(*layer[i]));
        const double cutoff = bestScore >= 0.0 ? bestScore * parameters.asFraction
                                               : bestScore / parameters.asFraction;
        std::vector<const TangleReadAlignment *> &retained = selected[group->first];
        for (size_t i = 0; i < layer.size(); ++i)
            if (selectionScore(*layer[i]) >= cutoff)
                retained.push_back(layer[i]);
        std::sort(retained.begin(), retained.end(),
                  [](const TangleReadAlignment *left, const TangleReadAlignment *right)
                  {
                      if (selectionScore(*left) != selectionScore(*right))
                          return selectionScore(*left) > selectionScore(*right);
                      if (left->identity != right->identity)
                          return left->identity > right->identity;
                      return left->path < right->path;
                  });
    }

    std::map<QString, std::map<IntPath, int> > contextsByRead;
    std::map<IntPath, int> globalFrequency;
    for (std::map<QString, std::vector<const TangleReadAlignment *> >::const_iterator read = selected.begin();
         read != selected.end(); ++read)
    {
        std::map<IntPath, int> &contexts = contextsByRead[read->first];
        for (size_t a = 0; a < read->second.size(); ++a)
        {
            const QVector<int> &thread = read->second[a]->path;
            const int threadLength = static_cast<int>(thread.size());
            const int lastLength = std::min(parameters.contextMax, threadLength);
            for (int length = parameters.contextMin; length <= lastLength; ++length)
            {
                for (int start = 0; start + length <= threadLength; ++start)
                {
                    IntPath context;
                    for (int i = 0; i < length; ++i)
                        context.push_back(thread[start + i]);
                    ++contexts[context];
                }
            }
        }
        for (std::map<IntPath, int>::const_iterator context = contexts.begin();
             context != contexts.end(); ++context)
            globalFrequency[context->first] += context->second;
    }

    std::vector<ReadEvidence> evidence;
    for (std::map<QString, std::vector<const TangleReadAlignment *> >::const_iterator read = selected.begin();
         read != selected.end(); ++read)
    {
        if (read->second.empty())
            continue;
        ReadEvidence item;
        item.readId = read->first;
        item.selectedAlignments = static_cast<int>(read->second.size());
        item.confidence = 0.0;
        for (size_t i = 0; i < read->second.size(); ++i)
        {
            const TangleReadAlignment &alignment = *read->second[i];
            const double mappingConfidence = std::max(
                    0.05, 1.0 - std::pow(10.0, -alignment.mappingQuality / 10.0));
            item.confidence = std::max(item.confidence,
                    alignment.identity * alignedFraction(alignment) * mappingConfidence);
        }
        item.confidence = std::max(0.0, std::min(1.0, item.confidence));
        const double ambiguityPenalty = 1.0 /
                std::sqrt(static_cast<double>(read->second.size()));
        item.confidence = std::max(0.0, std::min(1.0,
                item.confidence * ambiguityPenalty));

        const std::vector<double> weights = alignmentWeights(
                    read->second, parameters.asFraction);
        std::map<IntPath, double> fullRaw;
        for (size_t i = 0; i < read->second.size(); ++i)
            fullRaw[IntPath(read->second[i]->path.begin(), read->second[i]->path.end())] += weights[i];
        double fullTotal = 0.0;
        for (std::map<IntPath, double>::const_iterator thread = fullRaw.begin();
             thread != fullRaw.end(); ++thread)
            fullTotal += thread->second;
        for (std::map<IntPath, double>::const_iterator thread = fullRaw.begin();
             thread != fullRaw.end(); ++thread)
            item.fullThreads.push_back({thread->first,
                fullTotal > 0.0 ? thread->second / fullTotal : 0.0, 1});

        std::map<IntPath, double> rawContexts;
        std::map<IntPath, int> expectedCounts;
        for (size_t i = 0; i < read->second.size(); ++i)
        {
            std::map<IntPath, int> localContexts;
            const QVector<int> &thread = read->second[i]->path;
            const int threadLength = static_cast<int>(thread.size());
            const int lastLength = std::min(parameters.contextMax, threadLength);
            for (int length = parameters.contextMin; length <= lastLength; ++length)
                for (int start = 0; start + length <= threadLength; ++start)
                {
                    IntPath context;
                    for (int offset = 0; offset < length; ++offset)
                        context.push_back(thread[start + offset]);
                    ++localContexts[context];
                }
            for (std::map<IntPath, int>::const_iterator context = localContexts.begin();
                 context != localContexts.end(); ++context)
            {
                expectedCounts[context->first] = std::max(expectedCounts[context->first],
                                                           context->second);
                rawContexts[context->first] += weights[i] * context->second *
                        std::pow(static_cast<double>(context->first.size() - 1), 2.0) /
                        std::max(1, globalFrequency[context->first]);
            }
        }
        double contextTotal = 0.0;
        for (std::map<IntPath, double>::const_iterator context = rawContexts.begin();
             context != rawContexts.end(); ++context)
            contextTotal += context->second;
        for (std::map<IntPath, double>::const_iterator context = rawContexts.begin();
             context != rawContexts.end(); ++context)
            if (contextTotal > 0.0)
                item.contexts.push_back({context->first,
                    context->second / contextTotal, expectedCounts[context->first]});
        evidence.push_back(item);
    }
    return evidence;
}

std::vector<Bound> visitBounds(const TangleGraph &graph, int sourceSegment,
                               int targetSegment, double singleCopy,
                               const TanglePathParameters &parameters, bool strict)
{
    std::vector<Bound> result;
    for (int segment = 0; segment < graph.segments.size(); ++segment)
    {
        if (segment == sourceSegment || segment == targetSegment)
            continue;
        const TangleSegment &item = graph.segments[segment];
        const int maximum = std::max(1, std::min(parameters.maxCopy,
                int(std::ceil(item.coverage / (singleCopy * parameters.cpTauMin)))));
        Bound bound;
        bound.segment = segment;
        bound.minimum = strict && item.coverage >= singleCopy * parameters.cpTauMin ? 1 : 0;
        bound.maximum = maximum;
        result.push_back(bound);
    }
    return result;
}

double singleCopyCoverage(const TanglePathSearchRequest &request,
                          int sourceSegment, int targetSegment)
{
    if (request.parameters.cpSingleCopyCoverage > 0.0)
        return request.parameters.cpSingleCopyCoverage;
    return median(std::vector<double>{
                      request.graph.segments[sourceSegment].coverage,
                      request.graph.segments[targetSegment].coverage});
}

std::vector<double> coverageTable(const TangleSegment &segment, double medianLength,
                                  int maximum, double singleCopy,
                                  const TanglePathParameters &parameters)
{
    const double sigma = parameters.coverageDispersion * singleCopy;
    const double lengthWeight = std::max(0.05, segment.length / std::max(1.0, medianLength));
    std::vector<double> result;
    for (int count = 0; count <= maximum; ++count)
        result.push_back(lengthWeight * huber((segment.coverage - count * singleCopy) / sigma,
                                             parameters.cpHuberDelta));
    return result;
}

SolveAttempt buildAndSolve(const TanglePathSearchRequest &request,
                           const std::vector<ReadEvidence> &evidence,
                           int sourceSegment, int targetSegment, bool strict,
                           std::atomic_bool *cancelled)
{
    const TangleGraph &graph = request.graph;
    const TanglePathParameters &parameters = request.parameters;
    const double singleCopy = singleCopyCoverage(request, sourceSegment, targetSegment);
    const std::vector<Bound> bounds = visitBounds(graph, sourceSegment, targetSegment,
                                                  singleCopy, parameters, strict);
    int maxPositions = 2;
    for (size_t i = 0; i < bounds.size(); ++i)
        maxPositions += bounds[i].maximum;
    const int orientedCount = 2 * graph.segments.size();
    const int padId = orientedCount;
    const int sourcePlus = 2 * sourceSegment;
    const int targetPlus = 2 * targetSegment;

    std::set<std::pair<int, int> > allowedPairs;
    for (int left = 0; left < graph.adjacency.size(); ++left)
    {
        const QVector<TangleEdge> &edges = graph.adjacency[left];
        for (int i = 0; i < edges.size(); ++i)
        {
            const int right = edges[i].target;
            if (graph.orientedToSegment[left] == targetSegment ||
                graph.orientedToSegment[right] == sourceSegment)
                continue;
            allowedPairs.insert(std::make_pair(left, right));
        }
    }
    allowedPairs.insert(std::make_pair(targetPlus, padId));
    allowedPairs.insert(std::make_pair(targetPlus ^ 1, padId));
    allowedPairs.insert(std::make_pair(padId, padId));

    sat::CpModelBuilder model;
    std::vector<sat::IntVar> positions;
    for (int i = 0; i < maxPositions; ++i)
        positions.push_back(model.NewIntVar(Domain(0, padId)));
    auto firstTable = model.AddAllowedAssignments({positions.front()});
    firstTable.AddTuple({sourcePlus});
    firstTable.AddTuple({sourcePlus ^ 1});
    model.AddEquality(positions.back(), padId);
    for (int position = 0; position + 1 < maxPositions; ++position)
    {
        auto table = model.AddAllowedAssignments(
                    {positions[position], positions[position + 1]});
        for (std::set<std::pair<int, int> >::const_iterator pair = allowedPairs.begin();
             pair != allowedPairs.end(); ++pair)
            table.AddTuple({pair->first, pair->second});
    }

    std::vector<sat::IntVar> countVars;
    for (size_t boundIndex = 0; boundIndex < bounds.size(); ++boundIndex)
    {
        const Bound &bound = bounds[boundIndex];
        std::vector<sat::BoolVar> flags;
        const int plus = 2 * bound.segment;
        for (size_t position = 0; position < positions.size(); ++position)
        {
            const sat::BoolVar flag = model.NewBoolVar();
            auto table = model.AddAllowedAssignments(
                        {positions[position], sat::IntVar(flag)});
            for (int value = 0; value <= padId; ++value)
                table.AddTuple({value, value == plus || value == (plus ^ 1) ? 1 : 0});
            flags.push_back(flag);
        }
        const sat::IntVar count = model.NewIntVar(Domain(bound.minimum, bound.maximum));
        model.AddEquality(count, sat::LinearExpr::Sum(flags));
        countVars.push_back(count);
    }

    std::vector<double> lengths;
    for (size_t i = 0; i < bounds.size(); ++i)
        lengths.push_back(graph.segments[bounds[i].segment].length);
    const double medianLength = median(lengths);
    std::vector<sat::IntVar> coverageTerms;
    for (size_t i = 0; i < bounds.size(); ++i)
    {
        const std::vector<double> table = coverageTable(graph.segments[bounds[i].segment],
                medianLength, bounds[i].maximum, singleCopy, parameters);
        std::vector<int64_t> scaled;
        for (size_t value = 0; value < table.size(); ++value)
            scaled.push_back(std::llround(table[value] * objectiveScale));
        const std::pair<std::vector<int64_t>::iterator, std::vector<int64_t>::iterator> range =
                std::minmax_element(scaled.begin(), scaled.end());
        const sat::IntVar cost = model.NewIntVar(Domain(*range.first, *range.second));
        model.AddElement(countVars[i], scaled, cost);
        coverageTerms.push_back(cost);
    }

    std::map<IntPath, sat::IntVar> encodedPatterns;
    std::function<sat::IntVar(const IntPath &)> patternOccurrences = [&](const IntPath &input)
    {
        const IntPath key = canonicalPattern(input);
        std::map<IntPath, sat::IntVar>::const_iterator found = encodedPatterns.find(key);
        if (found != encodedPatterns.end())
            return found->second;

        std::vector<IntPath> alternatives;
        alternatives.push_back(input);
        alternatives.push_back(reverseComplement(input));
        std::sort(alternatives.begin(), alternatives.end());
        alternatives.erase(std::unique(alternatives.begin(), alternatives.end()), alternatives.end());
        std::vector<sat::IntVar> alternativeCounts;
        for (size_t alternative = 0; alternative < alternatives.size(); ++alternative)
        {
            std::vector<sat::IntVar> starts;
            for (int start = 0; start + int(alternatives[alternative].size()) <= maxPositions; ++start)
            {
                const sat::BoolVar match = model.NewBoolVar();
                for (size_t offset = 0; offset < alternatives[alternative].size(); ++offset)
                    model.AddEquality(positions[start + offset], alternatives[alternative][offset])
                            .OnlyEnforceIf(match);
                starts.push_back(sat::IntVar(match));
            }
            const sat::IntVar alternativeCount = model.NewIntVar(
                        Domain(0, starts.size()));
            std::vector<sat::IntVar> startIntegers;
            for (size_t i = 0; i < starts.size(); ++i)
                startIntegers.push_back(sat::IntVar(starts[i]));
            model.AddEquality(alternativeCount,
                              sat::LinearExpr::Sum(startIntegers));
            alternativeCounts.push_back(alternativeCount);
        }
        const sat::IntVar occurrenceCount = model.NewIntVar(
                    Domain(0, maxPositions));
        model.AddMaxEquality(occurrenceCount, alternativeCounts);
        encodedPatterns.insert(std::make_pair(key, occurrenceCount));
        return occurrenceCount;
    };

    std::vector<sat::LinearExpr> readRewards;
    for (size_t read = 0; read < evidence.size(); ++read)
    {
        for (size_t thread = 0; thread < evidence[read].fullThreads.size(); ++thread)
        {
            const ContextEvidence &threadEvidence = evidence[read].fullThreads[thread];
            const sat::IntVar occurrences = patternOccurrences(threadEvidence.pattern);
            const int expected = std::max(1, threadEvidence.expectedCount);
            const sat::IntVar capped = model.NewIntVar(Domain(0, expected));
            model.AddLessOrEqual(capped, occurrences);
            const qint64 reward = std::llround(evidence[read].confidence *
                    parameters.fullThreadFraction * threadEvidence.fraction /
                    expected * parameters.readWeight * objectiveScale);
            if (reward != 0)
                readRewards.push_back(sat::IntVar(capped) * reward);
        }
        for (size_t context = 0; context < evidence[read].contexts.size(); ++context)
        {
            const ContextEvidence &contextEvidence = evidence[read].contexts[context];
            const sat::IntVar occurrences = patternOccurrences(contextEvidence.pattern);
            const int expected = std::max(1, contextEvidence.expectedCount);
            const sat::IntVar capped = model.NewIntVar(Domain(0, expected));
            model.AddLessOrEqual(capped, occurrences);
            const qint64 reward = std::llround(evidence[read].confidence *
                    parameters.contextFraction * contextEvidence.fraction / expected *
                    parameters.readWeight * objectiveScale);
            if (reward != 0)
                readRewards.push_back(sat::IntVar(capped) * reward);
        }
    }

    std::vector<sat::BoolVar> activeFlags;
    for (size_t i = 0; i < positions.size(); ++i)
    {
        const sat::BoolVar active = model.NewBoolVar();
        model.AddNotEqual(positions[i], padId).OnlyEnforceIf(active);
        model.AddEquality(positions[i], padId).OnlyEnforceIf(active.Not());
        activeFlags.push_back(active);
    }

    sat::LinearExpr objective;
    const qint64 coverageMultiplier = std::llround(parameters.coverageWeight * 1000);
    for (size_t i = 0; i < coverageTerms.size(); ++i)
        objective += coverageTerms[i] * coverageMultiplier;
    for (size_t i = 0; i < readRewards.size(); ++i)
        objective -= readRewards[i] * 1000;
    objective += sat::LinearExpr::Sum(activeFlags) * 1000;
    objective -= 1000;
    model.Minimize(objective);

    sat::SatParameters solverParameters;
    solverParameters.set_max_time_in_seconds(parameters.timeLimitSeconds);
    solverParameters.set_num_search_workers(1);
    solverParameters.set_random_seed(parameters.randomSeed);
    sat::Model solverModel;
    solverModel.Add(sat::NewSatParameters(solverParameters));
    if (cancelled != 0)
        solverModel.GetOrCreate<TimeLimit>()->RegisterExternalBooleanAsLimit(cancelled);
    const sat::CpSolverResponse response = sat::SolveCpModel(model.Build(), &solverModel);

    SolveAttempt result;
    result.status = response.status();
    result.objectiveValue = response.objective_value();
    result.bestBound = response.best_objective_bound();
    if (response.status() == sat::CpSolverStatus::OPTIMAL ||
        response.status() == sat::CpSolverStatus::FEASIBLE)
    {
        for (size_t i = 0; i < positions.size(); ++i)
        {
            const int value = sat::SolutionIntegerValue(response, positions[i]);
            if (value == padId)
                break;
            result.path.push_back(value);
        }
    }
    return result;
}

int patternOccurrences(const IntPath &path, const IntPath &pattern)
{
    const IntPath reverse = reverseComplement(pattern);
    const auto count = [&](const IntPath &target)
    {
        if (target.size() > path.size())
            return 0;
        int total = 0;
        for (size_t start = 0; start + target.size() <= path.size(); ++start)
            if (std::equal(target.begin(), target.end(), path.begin() + start))
                ++total;
        return total;
    };
    return std::max(count(pattern), count(reverse));
}

double weightedReadSupport(const std::vector<ReadEvidence> &evidence,
                           const IntPath &path,
                           const TanglePathParameters &parameters)
{
    double confidenceTotal = 0.0;
    double fullWeight = 0.0;
    double contextWeight = 0.0;
    for (size_t read = 0; read < evidence.size(); ++read)
    {
        double fullFraction = 0.0;
        for (size_t thread = 0; thread < evidence[read].fullThreads.size(); ++thread)
        {
            const ContextEvidence &item = evidence[read].fullThreads[thread];
            fullFraction += item.fraction * std::min(
                        double(patternOccurrences(path, item.pattern)) /
                        std::max(1, item.expectedCount), 1.0);
        }
        double contextFraction = 0.0;
        for (size_t context = 0; context < evidence[read].contexts.size(); ++context)
        {
            const ContextEvidence &item = evidence[read].contexts[context];
            contextFraction += item.fraction * std::min(
                        double(patternOccurrences(path, item.pattern)) /
                        std::max(1, item.expectedCount), 1.0);
        }
        confidenceTotal += evidence[read].confidence;
        fullWeight += evidence[read].confidence * fullFraction;
        contextWeight += evidence[read].confidence * contextFraction;
    }
    if (confidenceTotal <= 0.0)
        return 0.0;
    return parameters.fullThreadFraction * fullWeight / confidenceTotal +
            parameters.contextFraction * contextWeight / confidenceTotal;
}

double comparablePathObjective(const TanglePathSearchRequest &request,
                               const std::vector<ReadEvidence> &evidence,
                               const IntPath &path, int sourceSegment,
                               int targetSegment, double *weightedSupport)
{
    const TangleGraph &graph = request.graph;
    const TanglePathParameters &parameters = request.parameters;
    const double singleCopy = singleCopyCoverage(request, sourceSegment, targetSegment);
    const std::vector<Bound> bounds = visitBounds(graph, sourceSegment,
                                                  targetSegment, singleCopy,
                                                  parameters, false);
    std::vector<int> visits(graph.segments.size(), 0);
    for (size_t i = 0; i < path.size(); ++i)
        ++visits[graph.orientedToSegment[path[i]]];

    std::vector<double> lengths;
    for (size_t i = 0; i < bounds.size(); ++i)
        lengths.push_back(graph.segments[bounds[i].segment].length);
    const double medianLength = median(lengths);
    double coverageNll = 0.0;
    for (size_t i = 0; i < bounds.size(); ++i)
    {
        const Bound &bound = bounds[i];
        const std::vector<double> table = coverageTable(
                    graph.segments[bound.segment], medianLength,
                    bound.maximum, singleCopy, parameters);
        coverageNll += table[visits[bound.segment]];
    }

    double confidenceTotal = 0.0;
    for (size_t i = 0; i < evidence.size(); ++i)
        confidenceTotal += evidence[i].confidence;
    const double support = weightedReadSupport(evidence, path, parameters);
    if (weightedSupport != 0)
        *weightedSupport = support;
    return parameters.coverageWeight * coverageNll +
            parameters.readWeight * confidenceTotal * (1.0 - support) +
            1e-6 * std::max(0, static_cast<int>(path.size()) - 1);
}
}

TanglePathSearchResult runCpSatTanglePathSearch(const TanglePathSearchRequest &request,
                                                std::atomic_bool *cancelled)
{
    QElapsedTimer timer;
    timer.start();
    TanglePathSearchResult result;
    result.status = "CP_SAT";
    if (!request.graph.segmentIndex.contains(request.source) ||
        !request.graph.segmentIndex.contains(request.target))
    {
        result.errorMessage = "Start or end node is outside the selected subgraph.";
        return result;
    }
    if (request.readAlignments.isEmpty())
    {
        result.errorMessage = "No multi-node GAF evidence remains in the selected subgraph.";
        return result;
    }

    const int sourceSegment = request.graph.segmentIndex.value(request.source);
    const int targetSegment = request.graph.segmentIndex.value(request.target);
    if (request.graph.segments[sourceSegment].coverage <= 0.0 ||
        request.graph.segments[targetSegment].coverage <= 0.0)
    {
        result.errorMessage = "Start and end coverage must be greater than zero.";
        return result;
    }

    const std::vector<ReadEvidence> evidence = buildEvidence(request.readAlignments,
                                                             request.parameters);
    if (evidence.empty())
    {
        result.errorMessage = "The selected GAF records contain no usable multi-node read evidence.";
        return result;
    }

    SolveAttempt attempt = buildAndSolve(request, evidence, sourceSegment, targetSegment,
                                         true, cancelled);
    if (attempt.status == sat::CpSolverStatus::INFEASIBLE)
    {
        result.relaxedCoverage = true;
        attempt = buildAndSolve(request, evidence, sourceSegment, targetSegment,
                                false, cancelled);
    }
    if (cancelled != 0 && cancelled->load())
    {
        result.cancelled = true;
        result.status = "CANCELLED";
        result.elapsedMs = timer.elapsed();
        return result;
    }
    if (attempt.status != sat::CpSolverStatus::OPTIMAL &&
        attempt.status != sat::CpSolverStatus::FEASIBLE)
    {
        result.status = QString::fromStdString(sat::CpSolverStatus_Name(attempt.status));
        result.errorMessage = "CP-SAT returned " + result.status + ".";
        result.elapsedMs = timer.elapsed();
        return result;
    }

    TanglePathCandidate candidate;
    for (size_t i = 0; i < attempt.path.size(); ++i)
        candidate.orientedNodeNames << request.graph.label(attempt.path[i]);
    candidate.score = comparablePathObjective(request, evidence, attempt.path,
                                               sourceSegment, targetSegment,
                                               &candidate.weightedReadSupport);
    candidate.coverageMad = std::numeric_limits<double>::quiet_NaN();
    candidate.explainedLengthFraction = std::numeric_limits<double>::quiet_NaN();
    candidate.copyAgreement = std::numeric_limits<double>::quiet_NaN();
    result.candidates << candidate;
    result.status = QString::fromStdString(sat::CpSolverStatus_Name(attempt.status));
    if (result.relaxedCoverage)
        result.status += "_RELAXED_COVERAGE";
    result.elapsedMs = timer.elapsed();
    return result;
}

#endif // BANDAGE_WITH_ORTOOLS
