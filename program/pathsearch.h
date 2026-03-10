#ifndef PATHSEARCH_H
#define PATHSEARCH_H

#include <QList>
#include <QSet>
#include <QVector>
#include <QString>
#include <QElapsedTimer>
#include "../graph/path.h"

class DeBruijnNode;

enum PathSearchMode
{
    PATH_SEARCH_TOP_K,
    PATH_SEARCH_ENUMERATE_ALL
};

struct PathSearchRequest
{
    QSet<DeBruijnNode *> allowedNodes;
    QList<DeBruijnNode *> startNodes;
    QList<DeBruijnNode *> endNodes;
    int maxNodes;
    PathSearchMode mode;
    int topK;
    int maxPaths;
    int timeoutMs;
};

struct PathSearchStats
{
    qint64 expandedStates;
    qint64 prunedByReachability;
    qint64 prunedByDepth;
    qint64 prunedByBound;
    qint64 dedupDropped;
    qint64 elapsedMs;
    bool hitPathLimit;
    bool hitTimeout;

    PathSearchStats()
        : expandedStates(0),
          prunedByReachability(0),
          prunedByDepth(0),
          prunedByBound(0),
          dedupDropped(0),
          elapsedMs(0),
          hitPathLimit(false),
          hitTimeout(false)
    {
    }
};

struct PathSearchResult
{
    QList<Path> paths;
    PathSearchStats stats;
};

class PathSearchEngine
{
public:
    static PathSearchResult search(const PathSearchRequest &request);
};

#endif // PATHSEARCH_H
