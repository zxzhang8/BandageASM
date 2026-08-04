#include "pathsearch.h"

#include <QHash>
#include <QQueue>
#include <QSet>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

#include "../graph/debruijnedge.h"
#include "../graph/debruijnnode.h"
#include "../graph/path.h"

namespace
{

struct GraphIndex
{
    QVector<DeBruijnNode *> nodeById;
    QHash<DeBruijnNode *, int> idByNode;
    QVector< QVector<int> > adjOut;
    QVector< QVector<int> > adjIn;
    QVector<bool> isEnd;
    QVector<int> minHopsToEnd;
    QVector<double> nodeBaseScore;
    double maxNodeBaseScore;
};

struct AcceptedPath
{
    QVector<int> nodes;
    QString key;
    double score;
};

QString makePathKey(const QVector<int> &nodes)
{
    QString key;
    key.reserve(nodes.size() * 6);
    for (int i = 0; i < nodes.size(); ++i)
    {
        key += QString::number(nodes[i]);
        key += ',';
    }
    return key;
}

inline bool isBitSet(const QVector<quint64> &bits, int bit)
{
    int word = bit / 64;
    int offset = bit % 64;
    return (bits[word] & (quint64(1) << offset)) != 0;
}

inline void setBit(QVector<quint64> &bits, int bit)
{
    int word = bit / 64;
    int offset = bit % 64;
    bits[word] |= (quint64(1) << offset);
}

double computeNodeBaseScore(const DeBruijnNode *node, int outDegree)
{
    double lengthReward = std::min(1.0, double(node->getLength()) / 10000.0);

    double readSupportReward = 0.0;
    if (node->hasReadSupportCount())
        readSupportReward = std::min(1.0, std::log(1.0 + double(node->getReadSupportCount())) / 10.0);

    double depthPenalty = std::min(1.0, std::abs(node->getDepth() - 1.0) / 10.0);
    double branchPenalty = std::min(1.0, std::max(0, outDegree - 1) / 5.0);

    return 0.20 * lengthReward +
           0.35 * readSupportReward -
           0.30 * depthPenalty -
           0.15 * branchPenalty;
}

bool shouldStopByTimeout(const PathSearchRequest &request, const QElapsedTimer &timer)
{
    return request.timeoutMs > 0 && timer.elapsed() >= request.timeoutMs;
}

GraphIndex buildIndex(const PathSearchRequest &request, PathSearchStats *stats)
{
    GraphIndex gi;

    gi.nodeById.reserve(request.allowedNodes.size());
    for (QSet<DeBruijnNode *>::const_iterator it = request.allowedNodes.constBegin();
         it != request.allowedNodes.constEnd(); ++it)
    {
        DeBruijnNode *node = *it;
        gi.idByNode.insert(node, gi.nodeById.size());
        gi.nodeById.push_back(node);
    }

    int n = gi.nodeById.size();
    gi.adjOut.resize(n);
    gi.adjIn.resize(n);
    gi.isEnd = QVector<bool>(n, false);
    gi.minHopsToEnd = QVector<int>(n, std::numeric_limits<int>::max());
    gi.nodeBaseScore = QVector<double>(n, 0.0);
    gi.maxNodeBaseScore = -std::numeric_limits<double>::infinity();

    for (int id = 0; id < n; ++id)
    {
        DeBruijnNode *node = gi.nodeById[id];
        std::vector<DeBruijnEdge *> edges = node->getLeavingEdges();
        QVector<int> next;
        next.reserve(int(edges.size()));
        for (size_t i = 0; i < edges.size(); ++i)
        {
            DeBruijnNode *to = edges[i]->getEndingNode();
            if (!gi.idByNode.contains(to))
                continue;
            int toId = gi.idByNode.value(to);
            next.push_back(toId);
            gi.adjIn[toId].push_back(id);
        }
        gi.adjOut[id] = next;
    }

    for (int i = 0; i < request.endNodes.size(); ++i)
    {
        DeBruijnNode *endNode = request.endNodes[i];
        if (!gi.idByNode.contains(endNode))
            continue;
        gi.isEnd[gi.idByNode.value(endNode)] = true;
    }

    QQueue<int> q;
    for (int id = 0; id < n; ++id)
    {
        if (!gi.isEnd[id])
            continue;
        gi.minHopsToEnd[id] = 0;
        q.enqueue(id);
    }

    while (!q.isEmpty())
    {
        int u = q.dequeue();
        int d = gi.minHopsToEnd[u];
        const QVector<int> &pred = gi.adjIn[u];
        for (int i = 0; i < pred.size(); ++i)
        {
            int p = pred[i];
            if (gi.minHopsToEnd[p] > d + 1)
            {
                gi.minHopsToEnd[p] = d + 1;
                q.enqueue(p);
            }
        }
    }

    for (int id = 0; id < n; ++id)
    {
        gi.nodeBaseScore[id] = computeNodeBaseScore(gi.nodeById[id], gi.adjOut[id].size());
        gi.maxNodeBaseScore = std::max(gi.maxNodeBaseScore, gi.nodeBaseScore[id]);

        std::sort(gi.adjOut[id].begin(), gi.adjOut[id].end(),
                  [&](int a, int b)
        {
            int ah = gi.minHopsToEnd[a];
            int bh = gi.minHopsToEnd[b];
            if (ah != bh)
                return ah < bh;
            if (gi.nodeBaseScore[a] != gi.nodeBaseScore[b])
                return gi.nodeBaseScore[a] > gi.nodeBaseScore[b];
            return a < b;
        });
    }

    if (!std::isfinite(gi.maxNodeBaseScore))
        gi.maxNodeBaseScore = 0.0;

    Q_UNUSED(stats);
    return gi;
}

QList<Path> convertCandidatesToPaths(const QVector<AcceptedPath> &accepted,
                                     const GraphIndex &gi)
{
    QVector<AcceptedPath> sorted = accepted;
    std::sort(sorted.begin(), sorted.end(),
              [](const AcceptedPath &a, const AcceptedPath &b)
    {
        if (a.score != b.score)
            return a.score > b.score;
        return a.key < b.key;
    });

    QList<Path> output;
    for (int i = 0; i < sorted.size(); ++i)
    {
        const QVector<int> &nodeIds = sorted[i].nodes;
        QList<DeBruijnNode *> nodes;
        nodes.reserve(nodeIds.size());
        for (int j = 0; j < nodeIds.size(); ++j)
            nodes.push_back(gi.nodeById[nodeIds[j]]);

        Path p = Path::makeFromOrderedNodes(nodes, false);
        if (!p.isEmpty())
            output.push_back(p);
    }

    return output;
}

PathSearchResult runEnumerateAll(const PathSearchRequest &request,
                                 const GraphIndex &gi,
                                 const QElapsedTimer &timer)
{
    PathSearchResult result;
    QVector<AcceptedPath> accepted;

    const int n = gi.nodeById.size();
    const int words = (n + 63) / 64;

    QVector<bool> isStart(n, false);
    for (int i = 0; i < request.startNodes.size(); ++i)
    {
        DeBruijnNode *startNode = request.startNodes[i];
        if (gi.idByNode.contains(startNode))
            isStart[gi.idByNode.value(startNode)] = true;
    }

    struct Frame
    {
        int node;
        int nextIndex;
    };

    for (int startId = 0; startId < n; ++startId)
    {
        if (!isStart[startId])
            continue;

        if (shouldStopByTimeout(request, timer))
        {
            result.stats.hitTimeout = true;
            break;
        }

        if (gi.minHopsToEnd[startId] == std::numeric_limits<int>::max())
        {
            ++result.stats.prunedByReachability;
            continue;
        }

        QVector<quint64> visited(words, 0);
        setBit(visited, startId);

        QVector<int> path;
        path.reserve(request.maxNodes);
        path.push_back(startId);

        QVector<Frame> stack;
        stack.reserve(request.maxNodes);
        stack.push_back(Frame{startId, 0});

        while (!stack.isEmpty())
        {
            if (shouldStopByTimeout(request, timer))
            {
                result.stats.hitTimeout = true;
                break;
            }

            Frame &frame = stack.last();
            int current = frame.node;

            if (gi.isEnd[current] && path.size() > 1)
            {
                AcceptedPath ap;
                ap.nodes = path;
                ap.key = makePathKey(ap.nodes);
                ap.score = 0.0;

                bool duplicate = false;
                for (int i = 0; i < accepted.size(); ++i)
                {
                    if (accepted[i].key == ap.key)
                    {
                        duplicate = true;
                        ++result.stats.dedupDropped;
                        break;
                    }
                }

                if (!duplicate)
                {
                    accepted.push_back(ap);
                    if (accepted.size() >= request.maxPaths)
                    {
                        result.stats.hitPathLimit = true;
                        break;
                    }
                }

                setBit(visited, current); // no-op intentional for symmetry
                stack.removeLast();
                path.removeLast();
                if (!stack.isEmpty())
                {
                    int leaving = current;
                    int word = leaving / 64;
                    int offset = leaving % 64;
                    visited[word] &= ~(quint64(1) << offset);
                }
                continue;
            }

            if (path.size() >= request.maxNodes)
            {
                ++result.stats.prunedByDepth;
                int leaving = current;
                stack.removeLast();
                path.removeLast();
                if (!stack.isEmpty())
                {
                    int word = leaving / 64;
                    int offset = leaving % 64;
                    visited[word] &= ~(quint64(1) << offset);
                }
                continue;
            }

            const QVector<int> &nexts = gi.adjOut[current];
            bool advanced = false;
            while (frame.nextIndex < nexts.size())
            {
                int next = nexts[frame.nextIndex++];
                if (isBitSet(visited, next))
                    continue;

                int projectedNodes = path.size() + 1 + gi.minHopsToEnd[next];
                if (gi.minHopsToEnd[next] == std::numeric_limits<int>::max())
                {
                    ++result.stats.prunedByReachability;
                    continue;
                }
                if (projectedNodes > request.maxNodes)
                {
                    ++result.stats.prunedByDepth;
                    continue;
                }

                setBit(visited, next);
                path.push_back(next);
                stack.push_back(Frame{next, 0});
                ++result.stats.expandedStates;
                advanced = true;
                break;
            }

            if (!advanced)
            {
                int leaving = current;
                stack.removeLast();
                path.removeLast();
                if (!stack.isEmpty())
                {
                    int word = leaving / 64;
                    int offset = leaving % 64;
                    visited[word] &= ~(quint64(1) << offset);
                }
            }

            if (result.stats.hitPathLimit)
                break;
        }

        if (result.stats.hitPathLimit || result.stats.hitTimeout)
            break;
    }

    result.paths = convertCandidatesToPaths(accepted, gi);
    result.stats.elapsedMs = timer.elapsed();
    return result;
}

PathSearchResult runTopK(const PathSearchRequest &request,
                         const GraphIndex &gi,
                         const QElapsedTimer &timer)
{
    PathSearchResult result;
    QVector<AcceptedPath> accepted;

    const int n = gi.nodeById.size();
    const int words = (n + 63) / 64;

    QVector<bool> isStart(n, false);
    for (int i = 0; i < request.startNodes.size(); ++i)
    {
        DeBruijnNode *startNode = request.startNodes[i];
        if (gi.idByNode.contains(startNode))
            isStart[gi.idByNode.value(startNode)] = true;
    }

    struct State
    {
        int node;
        int depth;
        int parent;
        double score;
        double upperBound;
        QVector<quint64> visited;
    };

    struct PQEntry
    {
        int stateIndex;
        double upperBound;
    };

    struct PQCmp
    {
        bool operator()(const PQEntry &a, const PQEntry &b) const
        {
            return a.upperBound < b.upperBound;
        }
    };

    std::priority_queue<PQEntry, std::vector<PQEntry>, PQCmp> pq;
    QVector<State> states;
    states.reserve(4096);

    for (int startId = 0; startId < n; ++startId)
    {
        if (!isStart[startId])
            continue;

        if (gi.minHopsToEnd[startId] == std::numeric_limits<int>::max())
        {
            ++result.stats.prunedByReachability;
            continue;
        }

        State st;
        st.node = startId;
        st.depth = 1;
        st.parent = -1;
        st.score = gi.nodeBaseScore[startId];
        st.upperBound = st.score + (request.maxNodes - st.depth) * gi.maxNodeBaseScore;
        st.visited = QVector<quint64>(words, 0);
        setBit(st.visited, startId);

        const int stateIndex = static_cast<int>(states.size());
        states.push_back(st);
        pq.push(PQEntry{stateIndex, st.upperBound});
    }

    auto kthScore = [&]() -> double
    {
        if (accepted.size() < request.topK)
            return -std::numeric_limits<double>::infinity();
        double minScore = accepted[0].score;
        for (int i = 1; i < accepted.size(); ++i)
            minScore = std::min(minScore, accepted[i].score);
        return minScore;
    };

    auto reconstructPathNodes = [&](int stateIndex) -> QVector<int>
    {
        QVector<int> rev;
        int cur = stateIndex;
        while (cur >= 0)
        {
            rev.push_back(states[cur].node);
            cur = states[cur].parent;
        }
        QVector<int> nodes;
        nodes.reserve(rev.size());
        for (int i = rev.size() - 1; i >= 0; --i)
            nodes.push_back(rev[i]);
        return nodes;
    };

    while (!pq.empty())
    {
        if (shouldStopByTimeout(request, timer))
        {
            result.stats.hitTimeout = true;
            break;
        }

        PQEntry top = pq.top();
        pq.pop();

        if (accepted.size() >= request.topK && top.upperBound <= kthScore())
        {
            ++result.stats.prunedByBound;
            break;
        }

        const State st = states[top.stateIndex];

        if (gi.isEnd[st.node] && st.depth > 1)
        {
            QVector<int> nodeIds = reconstructPathNodes(top.stateIndex);
            AcceptedPath candidate;
            candidate.nodes = nodeIds;
            candidate.key = makePathKey(nodeIds);
            candidate.score = st.score;

            int duplicateIndex = -1;
            for (int i = 0; i < accepted.size(); ++i)
            {
                if (accepted[i].key == candidate.key)
                {
                    duplicateIndex = i;
                    break;
                }
            }

            if (duplicateIndex >= 0)
            {
                ++result.stats.dedupDropped;
                if (candidate.score > accepted[duplicateIndex].score)
                    accepted[duplicateIndex] = candidate;
            }
            else if (accepted.size() < request.topK)
                accepted.push_back(candidate);
            else
            {
                int worstIndex = 0;
                double worstScore = accepted[0].score;
                for (int i = 1; i < accepted.size(); ++i)
                {
                    if (accepted[i].score < worstScore)
                    {
                        worstScore = accepted[i].score;
                        worstIndex = i;
                    }
                }

                if (candidate.score > worstScore)
                    accepted[worstIndex] = candidate;
            }

            continue;
        }

        if (st.depth >= request.maxNodes)
        {
            ++result.stats.prunedByDepth;
            continue;
        }

        const QVector<int> &nexts = gi.adjOut[st.node];
        for (int i = 0; i < nexts.size(); ++i)
        {
            int next = nexts[i];
            if (isBitSet(st.visited, next))
                continue;

            if (gi.minHopsToEnd[next] == std::numeric_limits<int>::max())
            {
                ++result.stats.prunedByReachability;
                continue;
            }

            int nextDepth = st.depth + 1;
            int projectedNodes = nextDepth + gi.minHopsToEnd[next];
            if (projectedNodes > request.maxNodes)
            {
                ++result.stats.prunedByDepth;
                continue;
            }

            State child;
            child.node = next;
            child.depth = nextDepth;
            child.parent = top.stateIndex;
            child.score = st.score + gi.nodeBaseScore[next];
            child.upperBound = child.score + (request.maxNodes - child.depth) * gi.maxNodeBaseScore;
            child.visited = st.visited;
            setBit(child.visited, next);

            const int childIndex = static_cast<int>(states.size());
            states.push_back(child);
            pq.push(PQEntry{childIndex, child.upperBound});
            ++result.stats.expandedStates;
        }
    }

    result.paths = convertCandidatesToPaths(accepted, gi);
    if (accepted.size() >= request.topK)
        result.stats.hitPathLimit = true;
    result.stats.elapsedMs = timer.elapsed();
    return result;
}

} // namespace

PathSearchResult PathSearchEngine::search(const PathSearchRequest &request)
{
    PathSearchResult empty;

    if (request.allowedNodes.isEmpty() || request.startNodes.isEmpty() || request.endNodes.isEmpty())
        return empty;

    PathSearchRequest req = request;
    req.maxNodes = std::max(2, req.maxNodes);
    req.topK = std::max(1, req.topK);
    req.maxPaths = std::max(1, req.maxPaths);

    QElapsedTimer timer;
    timer.start();

    PathSearchStats prepStats;
    GraphIndex gi = buildIndex(req, &prepStats);

    PathSearchResult result;
    if (req.mode == PATH_SEARCH_ENUMERATE_ALL)
        result = runEnumerateAll(req, gi, timer);
    else
        result = runTopK(req, gi, timer);

    result.stats.expandedStates += prepStats.expandedStates;
    result.stats.prunedByReachability += prepStats.prunedByReachability;
    result.stats.prunedByDepth += prepStats.prunedByDepth;
    result.stats.prunedByBound += prepStats.prunedByBound;
    result.stats.dedupDropped += prepStats.dedupDropped;

    return result;
}
