//Copyright 2026

#ifndef GAFVISUALIZATION_H
#define GAFVISUALIZATION_H

#include <QColor>
#include <QHash>
#include <QList>
#include <QtGlobal>
#include <atomic>
#include <functional>

#include "gafparser.h"

class DeBruijnNode;
class DeBruijnEdge;

enum GafCountBasis
{
    GAF_COUNT_RECORDS = 0,
    GAF_COUNT_UNIQUE_QUERIES = 1
};

enum GafHeatScale
{
    GAF_HEAT_LOG = 0,
    GAF_HEAT_LINEAR = 1
};

struct GafVisualizationData
{
    GafVisualizationData();

    QHash<DeBruijnNode *, quint64> nodeSupport;
    QHash<DeBruijnEdge *, quint64> edgeSupport;
    GafCountBasis countBasis;
    int alignmentCount;
    int queryCount;
    quint64 directedMaximum;
    quint64 collapsedMaximum;
    bool cancelled;

    bool isEmpty() const {return nodeSupport.isEmpty() && edgeSupport.isEmpty();}
    quint64 nodeCount(DeBruijnNode *node, bool doubleMode) const;
    quint64 edgeCount(DeBruijnEdge *edge, bool doubleMode) const;
    quint64 maximum(bool doubleMode) const;
};

GafVisualizationData buildGafVisualization(
        const QList<GafAlignment> &alignments,
        GafCountBasis basis,
        const std::atomic_bool *cancelled = 0,
        const std::function<void (int, int)> &progress = std::function<void (int, int)>());

QColor gafVisualizationColour(quint64 count, quint64 maximum, GafHeatScale scale,
                              int alpha = 255);
QString gafCountBasisLabel(GafCountBasis basis);

#endif // GAFVISUALIZATION_H
