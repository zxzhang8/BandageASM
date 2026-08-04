#ifndef GRAPHLAYOUTIO_H
#define GRAPHLAYOUTIO_H

#include <QMap>
#include <QPointF>
#include <QString>
#include <vector>

class AssemblyGraph;

struct SavedGraphLayout
{
    bool doubleMode;
    QMap<QString, std::vector<QPointF> > nodePoints;

    SavedGraphLayout() : doubleMode(false) {}
};

QString graphLayoutFingerprint(const AssemblyGraph &graph);

bool saveGraphLayout(const QString &fileName,
                     const AssemblyGraph &graph,
                     const SavedGraphLayout &layout,
                     QString *errorMessage);

bool loadGraphLayout(const QString &fileName,
                     const AssemblyGraph &graph,
                     SavedGraphLayout *layout,
                     QString *errorMessage);

#endif // GRAPHLAYOUTIO_H
