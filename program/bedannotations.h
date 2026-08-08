//Copyright 2026

#ifndef BEDANNOTATIONS_H
#define BEDANNOTATIONS_H

#include <QColor>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

class AssemblyGraph;
class DeBruijnNode;

struct BedBlock
{
    qint64 start = 0;
    qint64 end = 0;
};

struct BedAnnotation
{
    qint64 start = 0;
    qint64 end = 0;
    QString name;
    QColor colour = QColor(63, 160, 230);
    bool hasThick = false;
    qint64 thickStart = 0;
    qint64 thickEnd = 0;
    QVector<BedBlock> blocks;
};

struct BedLoadResult
{
    QHash<DeBruijnNode *, QList<BedAnnotation>> annotations;
    int validCount = 0;
    int malformedCount = 0;
    int unmatchedCount = 0;
    int outOfRangeCount = 0;
    QStringList malformedExamples;
    QStringList unmatchedExamples;
    QStringList outOfRangeExamples;
    QString fatalError;

    bool hasValidAnnotations() const { return fatalError.isEmpty() && validCount > 0; }
    int skippedCount() const { return malformedCount + unmatchedCount + outOfRangeCount; }
    QString warningSummary() const;
};

BedLoadResult parseBedAnnotationsFile(const QString &fileName, AssemblyGraph &graph);
void clearBedAnnotations(AssemblyGraph &graph);
void replaceBedAnnotations(AssemblyGraph &graph, const BedLoadResult &result);

#endif // BEDANNOTATIONS_H
