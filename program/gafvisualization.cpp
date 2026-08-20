//Copyright 2026

#include "gafvisualization.h"

#include <QSet>
#include <QStringList>
#include <algorithm>
#include <cmath>

#include "../graph/debruijnedge.h"
#include "../graph/debruijnnode.h"

namespace
{
template <typename T>
void incrementElements(QHash<T *, quint64> *counts, const QSet<T *> &elements)
{
    typename QSet<T *>::const_iterator it = elements.constBegin();
    while (it != elements.constEnd())
    {
        (*counts)[*it] = counts->value(*it) + 1;
        ++it;
    }
}

void addAlignmentElements(const GafAlignment &alignment,
                          QSet<DeBruijnNode *> *nodes,
                          QSet<DeBruijnEdge *> *edges)
{
    const QList<DeBruijnNode *> pathNodes = alignment.path.getNodes();
    for (int i = 0; i < pathNodes.size(); ++i)
        nodes->insert(pathNodes[i]);

    const QList<DeBruijnEdge *> pathEdges = alignment.path.getEdges();
    for (int i = 0; i < pathEdges.size(); ++i)
        edges->insert(pathEdges[i]);
}

template <typename T>
quint64 largestValue(const QHash<T *, quint64> &counts)
{
    quint64 maximum = 0;
    typename QHash<T *, quint64>::const_iterator it = counts.constBegin();
    while (it != counts.constEnd())
    {
        maximum = std::max(maximum, it.value());
        ++it;
    }
    return maximum;
}

template <typename T>
quint64 largestCollapsedValue(const QHash<T *, quint64> &counts)
{
    quint64 maximum = 0;
    QSet<T *> visited;
    typename QHash<T *, quint64>::const_iterator it = counts.constBegin();
    while (it != counts.constEnd())
    {
        T *element = it.key();
        if (visited.contains(element))
        {
            ++it;
            continue;
        }

        T *reverse = element->getReverseComplement();
        quint64 total = it.value();
        visited.insert(element);
        if (reverse != 0 && reverse != element)
        {
            total += counts.value(reverse);
            visited.insert(reverse);
        }
        maximum = std::max(maximum, total);
        ++it;
    }
    return maximum;
}

void finaliseMaximums(GafVisualizationData *data)
{
    data->directedMaximum = std::max(largestValue(data->nodeSupport),
                                     largestValue(data->edgeSupport));
    data->collapsedMaximum = std::max(largestCollapsedValue(data->nodeSupport),
                                      largestCollapsedValue(data->edgeSupport));
}

QColor interpolate(const QColor &left, const QColor &right, double amount, int alpha)
{
    amount = std::max(0.0, std::min(1.0, amount));
    return QColor(qRound(left.red() + (right.red() - left.red()) * amount),
                  qRound(left.green() + (right.green() - left.green()) * amount),
                  qRound(left.blue() + (right.blue() - left.blue()) * amount),
                  alpha);
}

}

GafVisualizationData::GafVisualizationData()
    : countBasis(GAF_COUNT_RECORDS),
      alignmentCount(0),
      queryCount(0),
      directedMaximum(0),
      collapsedMaximum(0),
      cancelled(false)
{
}

quint64 GafVisualizationData::nodeCount(DeBruijnNode *node, bool doubleMode) const
{
    if (node == 0)
        return 0;
    quint64 count = nodeSupport.value(node);
    DeBruijnNode *reverse = node->getReverseComplement();
    if (!doubleMode && reverse != 0 && reverse != node)
        count += nodeSupport.value(reverse);
    return count;
}

quint64 GafVisualizationData::edgeCount(DeBruijnEdge *edge, bool doubleMode) const
{
    if (edge == 0)
        return 0;
    quint64 count = edgeSupport.value(edge);
    DeBruijnEdge *reverse = edge->getReverseComplement();
    if (!doubleMode && reverse != 0 && reverse != edge)
        count += edgeSupport.value(reverse);
    return count;
}

quint64 GafVisualizationData::maximum(bool doubleMode) const
{
    return doubleMode ? directedMaximum : collapsedMaximum;
}

GafVisualizationData buildGafVisualization(
        const QList<GafAlignment> &alignments,
        GafCountBasis basis,
        const std::atomic_bool *cancelled,
        const std::function<void (int, int)> &progress)
{
    GafVisualizationData data;
    data.countBasis = basis;
    data.alignmentCount = alignments.size();

    QSet<QString> queryNames;
    if (basis == GAF_COUNT_RECORDS)
    {
        for (int i = 0; i < alignments.size(); ++i)
        {
            if (cancelled != 0 && cancelled->load())
            {
                data.cancelled = true;
                return data;
            }
            QSet<DeBruijnNode *> nodes;
            QSet<DeBruijnEdge *> edges;
            addAlignmentElements(alignments[i], &nodes, &edges);
            incrementElements(&data.nodeSupport, nodes);
            incrementElements(&data.edgeSupport, edges);
            queryNames.insert(alignments[i].queryName);
            if (progress && i + 1 < alignments.size() &&
                    (i % 256 == 0 || i + 2 == alignments.size()))
                progress(i + 1, alignments.size());
        }
    }
    else
    {
        QHash<QString, QSet<DeBruijnNode *> > queryNodes;
        QHash<QString, QSet<DeBruijnEdge *> > queryEdges;
        for (int i = 0; i < alignments.size(); ++i)
        {
            if (cancelled != 0 && cancelled->load())
            {
                data.cancelled = true;
                return data;
            }
            addAlignmentElements(alignments[i], &queryNodes[alignments[i].queryName],
                                 &queryEdges[alignments[i].queryName]);
            if (progress && i + 1 < alignments.size() &&
                    (i % 256 == 0 || i + 2 == alignments.size()))
                progress(i + 1, alignments.size());
        }

        QHash<QString, QSet<DeBruijnNode *> >::const_iterator it = queryNodes.constBegin();
        while (it != queryNodes.constEnd())
        {
            if (cancelled != 0 && cancelled->load())
            {
                data.cancelled = true;
                return data;
            }
            incrementElements(&data.nodeSupport, it.value());
            incrementElements(&data.edgeSupport, queryEdges.value(it.key()));
            queryNames.insert(it.key());
            ++it;
        }
    }

    data.queryCount = queryNames.size();
    finaliseMaximums(&data);
    if (progress)
        progress(alignments.size(), alignments.size());
    return data;
}

QColor gafVisualizationColour(quint64 count, quint64 maximum, GafHeatScale scale, int alpha)
{
    if (count == 0 || maximum == 0)
        return QColor(0, 0, 0, 0);

    double amount;
    if (maximum == 1)
        amount = 1.0;
    else if (scale == GAF_HEAT_LOG)
        amount = std::log1p(double(count)) / std::log1p(double(maximum));
    else
        amount = double(count) / double(maximum);

    // Five anchors from the colour-blind-friendly Viridis palette.
    const QColor anchors[] = {QColor("#440154"), QColor("#3b528b"), QColor("#21918c"),
                              QColor("#5ec962"), QColor("#fde725")};
    const double position = amount * 4.0;
    const int left = std::min(3, int(std::floor(position)));
    return interpolate(anchors[left], anchors[left + 1], position - left, alpha);
}

QString gafCountBasisLabel(GafCountBasis basis)
{
    return basis == GAF_COUNT_UNIQUE_QUERIES ? "unique queries" : "records";
}
