#include "tanglepathsearch.h"

#include <algorithm>
#include <cmath>
#include "../graph/debruijnedge.h"
#include "../graph/debruijnnode.h"

namespace
{
QString baseName(DeBruijnNode *node)
{
    return node == 0 ? QString() : node->getNameWithoutSign();
}

DeBruijnNode *positiveNode(DeBruijnNode *node)
{
    if (node != 0 && node->isNegativeNode())
        return node->getReverseComplement();
    return node;
}

bool numericTagValue(DeBruijnNode *node, const QString &tag, double *value)
{
    if (node == 0)
        return false;
    bool ok = false;
    const double parsed = node->getGfaTagValue(tag).toDouble(&ok);
    if (!ok || !std::isfinite(parsed) || parsed < 0.0)
        return false;
    if (value != 0)
        *value = parsed;
    return true;
}
}

TanglePathParameters::TanglePathParameters()
    : beamSize(1000),
      perNodeBeam(20),
      maxCopy(8),
      tauMin(0.5),
      lambdaMissing(0.1),
      lambdaExtra(1.0),
      lambdaStep(0.001),
      beamHuberDelta(0.5),
      topK(10),
      coverageDispersion(0.25),
      cpHuberDelta(2.0),
      cpTauMin(0.9),
      cpSingleCopyCoverage(0.0),
      cpSingleCopyCoverageLocked(true),
      fullThreadFraction(0.4),
      contextFraction(0.6),
      contextMin(2),
      contextMax(4),
      asFraction(0.999),
      coverageWeight(0.75),
      readWeight(1.25),
      timeLimitSeconds(60.0),
      randomSeed(1)
{
}

TanglePathSearchResult::TanglePathSearchResult()
    : cancelled(false), relaxedCoverage(false), elapsedMs(0)
{
}

int TangleGraph::orientedId(const QString &name, QChar orientation) const
{
    if (!segmentIndex.contains(name))
        return -1;
    return 2 * segmentIndex.value(name) + (orientation == '-' ? 1 : 0);
}

QString TangleGraph::label(int id) const
{
    if (id < 0 || id / 2 >= segments.size())
        return QString();
    return segments[id / 2].name + (id % 2 == 0 ? "+" : "-");
}

bool TangleGraph::containsTransition(int left, int right) const
{
    if (left < 0 || left >= adjacency.size())
        return false;
    const QVector<TangleEdge> &edges = adjacency[left];
    for (int i = 0; i < edges.size(); ++i)
        if (edges[i].target == right)
            return true;
    return false;
}

int TangleGraph::targetOverlap(int left, int right) const
{
    if (left < 0 || left >= adjacency.size())
        return 0;
    const QVector<TangleEdge> &edges = adjacency[left];
    for (int i = 0; i < edges.size(); ++i)
        if (edges[i].target == right)
            return edges[i].targetOverlap;
    return 0;
}

QStringList commonNumericGfaTags(const std::vector<DeBruijnNode *> &selectedNodes)
{
    QHash<QString, DeBruijnNode *> nodesByName;
    for (size_t i = 0; i < selectedNodes.size(); ++i)
    {
        DeBruijnNode *node = positiveNode(selectedNodes[i]);
        if (node != 0)
            nodesByName.insert(node->getNameWithoutSign(), node);
    }

    if (nodesByName.isEmpty())
        return QStringList();

    QStringList candidates;
    QHash<QString, DeBruijnNode *>::const_iterator first = nodesByName.constBegin();
    const QStringList firstTags = first.value()->getGfaTagNames();
    for (int i = 0; i < firstTags.size(); ++i)
    {
        const QString &tag = firstTags[i];
        bool validForAll = true;
        QHash<QString, DeBruijnNode *>::const_iterator nodeIt = nodesByName.constBegin();
        while (nodeIt != nodesByName.constEnd())
        {
            if (!numericTagValue(nodeIt.value(), tag, 0))
            {
                validForAll = false;
                break;
            }
            ++nodeIt;
        }
        if (validForAll && !candidates.contains(tag))
            candidates << tag;
    }

    candidates.sort(Qt::CaseInsensitive);
    return candidates;
}

bool buildTangleGraph(const std::vector<DeBruijnNode *> &selectedNodes,
                      const QString &coverageTag,
                      TangleGraph *graph,
                      QStringList *invalidCoverageNodes,
                      QString *errorMessage)
{
    if (graph == 0)
        return false;
    *graph = TangleGraph();
    if (invalidCoverageNodes != 0)
        invalidCoverageNodes->clear();
    if (errorMessage != 0)
        errorMessage->clear();

    QHash<QString, DeBruijnNode *> positiveNodes;
    for (size_t i = 0; i < selectedNodes.size(); ++i)
    {
        DeBruijnNode *node = positiveNode(selectedNodes[i]);
        if (node != 0)
            positiveNodes.insert(node->getNameWithoutSign(), node);
    }
    QStringList names = positiveNodes.keys();
    names.sort(Qt::CaseInsensitive);
    if (names.size() < 2)
    {
        if (errorMessage != 0)
            *errorMessage = "Select at least two different nodes.";
        return false;
    }

    for (int i = 0; i < names.size(); ++i)
    {
        DeBruijnNode *node = positiveNodes.value(names[i]);
        double coverage = 0.0;
        if (!numericTagValue(node, coverageTag, &coverage))
        {
            if (invalidCoverageNodes != 0)
                *invalidCoverageNodes << names[i];
            continue;
        }
        TangleSegment segment;
        segment.name = names[i];
        segment.length = node->getLength();
        segment.coverage = coverage;
        graph->segmentIndex.insert(segment.name, graph->segments.size());
        graph->segments << segment;
    }

    if (invalidCoverageNodes != 0 && !invalidCoverageNodes->isEmpty())
    {
        if (errorMessage != 0)
            *errorMessage = "The selected coverage tag is missing or non-numeric on some nodes.";
        *graph = TangleGraph();
        return false;
    }

    graph->orientedToSegment.resize(2 * graph->segments.size());
    graph->adjacency.resize(2 * graph->segments.size());
    for (int i = 0; i < graph->segments.size(); ++i)
    {
        graph->orientedToSegment[2 * i] = i;
        graph->orientedToSegment[2 * i + 1] = i;
    }

    for (int segmentIndex = 0; segmentIndex < graph->segments.size(); ++segmentIndex)
    {
        const QString &name = graph->segments[segmentIndex].name;
        DeBruijnNode *positive = positiveNodes.value(name);
        DeBruijnNode *orientations[2] = {positive, positive == 0 ? 0 : positive->getReverseComplement()};
        for (int orientation = 0; orientation < 2; ++orientation)
        {
            DeBruijnNode *node = orientations[orientation];
            if (node == 0)
                continue;
            const std::vector<DeBruijnEdge *> *edges = node->getEdgesPointer();
            for (size_t edgeIndex = 0; edgeIndex < edges->size(); ++edgeIndex)
            {
                DeBruijnEdge *edge = (*edges)[edgeIndex];
                if (edge->getStartingNode() != node)
                    continue;
                DeBruijnNode *targetNode = edge->getEndingNode();
                const QString targetBase = baseName(targetNode);
                if (!graph->segmentIndex.contains(targetBase))
                    continue;
                const int left = graph->orientedId(name, node->getSign().at(0));
                const int right = graph->orientedId(targetBase, targetNode->getSign().at(0));
                TangleEdge snapshotEdge;
                snapshotEdge.target = right;
                snapshotEdge.targetOverlap = edge->getOverlap();
                graph->adjacency[left] << snapshotEdge;
            }
        }
    }

    for (int i = 0; i < graph->adjacency.size(); ++i)
    {
        QVector<TangleEdge> &edges = graph->adjacency[i];
        std::sort(edges.begin(), edges.end());
        edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    }
    return true;
}

QVector<TangleReadAlignment> extractTangleReadAlignments(
        const QList<GafAlignment> &alignments,
        const TangleGraph &graph,
        int *discardedAlignmentCount)
{
    QVector<TangleReadAlignment> result;
    int discarded = 0;
    for (int alignmentIndex = 0; alignmentIndex < alignments.size(); ++alignmentIndex)
    {
        const GafAlignment &alignment = alignments[alignmentIndex];
        if (!alignment.hasCpSatMetrics())
        {
            ++discarded;
            continue;
        }

        QVector<int> run;
        const QList<DeBruijnNode *> pathNodes = alignment.path.getNodes();
        for (int nodeIndex = 0; nodeIndex <= pathNodes.size(); ++nodeIndex)
        {
            DeBruijnNode *node = nodeIndex < pathNodes.size() ? pathNodes[nodeIndex] : 0;
            const QString name = baseName(node);
            const bool inside = node != 0 && graph.segmentIndex.contains(name);
            if (inside)
            {
                const int id = graph.orientedId(name, node->getSign().at(0));
                if (!run.isEmpty() && !graph.containsTransition(run.last(), id))
                {
                    if (run.size() >= 2)
                    {
                        TangleReadAlignment clipped;
                        clipped.readId = alignment.queryName;
                        clipped.queryLength = alignment.queryLength;
                        clipped.queryStart = alignment.queryStart;
                        clipped.queryEnd = alignment.queryEnd;
                        clipped.residueMatches = alignment.residueMatches;
                        clipped.blockLength = alignment.blockLength;
                        clipped.mappingQuality = alignment.mappingQuality;
                        clipped.hasAlignmentScore = alignment.hasAlignmentScore;
                        clipped.alignmentScore = alignment.alignmentScore;
                        clipped.identity = alignment.identity;
                        clipped.path = run;
                        result << clipped;
                    }
                    run.clear();
                }
                run << id;
            }
            else
            {
                if (run.size() >= 2)
                {
                    TangleReadAlignment clipped;
                    clipped.readId = alignment.queryName;
                    clipped.queryLength = alignment.queryLength;
                    clipped.queryStart = alignment.queryStart;
                    clipped.queryEnd = alignment.queryEnd;
                    clipped.residueMatches = alignment.residueMatches;
                    clipped.blockLength = alignment.blockLength;
                    clipped.mappingQuality = alignment.mappingQuality;
                    clipped.hasAlignmentScore = alignment.hasAlignmentScore;
                    clipped.alignmentScore = alignment.alignmentScore;
                    clipped.identity = alignment.identity;
                    clipped.path = run;
                    result << clipped;
                }
                run.clear();
            }
        }
    }
    if (discardedAlignmentCount != 0)
        *discardedAlignmentCount = discarded;
    return result;
}
