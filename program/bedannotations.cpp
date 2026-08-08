//Copyright 2026

#include "bedannotations.h"

#include "../graph/assemblygraph.h"
#include "../graph/debruijnnode.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace
{
const QColor DEFAULT_BED_COLOUR(63, 160, 230);
const int MAX_EXAMPLES = 5;

bool parseInteger(const QString &text, qint64 *value)
{
    bool ok = false;
    const qint64 parsed = text.toLongLong(&ok);
    if (ok)
        *value = parsed;
    return ok;
}

void addExample(QStringList *examples, int lineNumber, const QString &detail)
{
    if (examples->size() < MAX_EXAMPLES)
        examples->append(QString("line %1: %2").arg(lineNumber).arg(detail));
}

bool parseRgb(const QString &text, QColor *colour)
{
    if (text.isEmpty() || text == "." || text == "0")
    {
        *colour = DEFAULT_BED_COLOUR;
        return true;
    }

    const QStringList components = text.split(',', Qt::KeepEmptyParts);
    if (components.size() != 3)
        return false;

    int values[3];
    for (int i = 0; i < 3; ++i)
    {
        bool ok = false;
        values[i] = components[i].toInt(&ok);
        if (!ok || values[i] < 0 || values[i] > 255)
            return false;
    }
    *colour = QColor(values[0], values[1], values[2]);
    return true;
}

bool parseCommaSeparatedIntegers(QString text, int expectedCount, QVector<qint64> *values)
{
    values->clear();
    if (expectedCount == 0)
        return text.trimmed().isEmpty();

    if (text.endsWith(','))
        text.chop(1);
    const QStringList fields = text.split(',', Qt::KeepEmptyParts);
    if (fields.size() != expectedCount)
        return false;

    values->reserve(expectedCount);
    for (const QString &field : fields)
    {
        qint64 value = 0;
        if (!parseInteger(field, &value))
            return false;
        values->append(value);
    }
    return true;
}

DeBruijnNode *oppositeNode(DeBruijnNode *node)
{
    return node == nullptr ? nullptr : node->getReverseComplement();
}
}

QString BedLoadResult::warningSummary() const
{
    QStringList parts;
    if (malformedCount > 0)
        parts << QString("%1 malformed").arg(malformedCount);
    if (unmatchedCount > 0)
        parts << QString("%1 unmatched node%2").arg(unmatchedCount).arg(unmatchedCount == 1 ? "" : "s");
    if (outOfRangeCount > 0)
        parts << QString("%1 outside node bounds").arg(outOfRangeCount);

    QString summary;
    if (!parts.isEmpty())
        summary = QString("Skipped %1 BED record%2: %3.")
                      .arg(skippedCount())
                      .arg(skippedCount() == 1 ? "" : "s")
                      .arg(parts.join(", "));

    QStringList examples;
    examples << malformedExamples << unmatchedExamples << outOfRangeExamples;
    if (!examples.isEmpty())
        summary += "\n\nExamples:\n" + examples.join('\n');
    return summary;
}

BedLoadResult parseBedAnnotationsFile(const QString &fileName, AssemblyGraph &graph)
{
    BedLoadResult result;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        result.fatalError = QString("Could not open %1: %2")
                                .arg(QFileInfo(fileName).fileName(), file.errorString());
        return result;
    }

    QTextStream stream(&file);
    int lineNumber = 0;
    while (!stream.atEnd())
    {
        const QString rawLine = stream.readLine();
        ++lineNumber;
        const QString trimmed = rawLine.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') ||
                trimmed.startsWith("track ", Qt::CaseInsensitive) ||
                trimmed.startsWith("browser ", Qt::CaseInsensitive))
            continue;

        const QStringList columns = rawLine.split('\t', Qt::KeepEmptyParts);
        auto malformed = [&](const QString &detail) {
            ++result.malformedCount;
            addExample(&result.malformedExamples, lineNumber, detail);
        };

        if (columns.size() < 3)
        {
            malformed("fewer than three tab-separated columns");
            continue;
        }
        if (columns.size() == 7 || columns.size() == 10 || columns.size() == 11)
        {
            malformed("incomplete optional BED columns");
            continue;
        }

        qint64 start = 0;
        qint64 end = 0;
        if (!parseInteger(columns[1], &start) || !parseInteger(columns[2], &end) ||
                start < 0 || end <= start)
        {
            malformed("invalid half-open start/end coordinates");
            continue;
        }

        if (columns.size() >= 5 && !columns[4].isEmpty() && columns[4] != ".")
        {
            qint64 score = 0;
            if (!parseInteger(columns[4], &score) || score < 0 || score > 1000)
            {
                malformed("score must be an integer from 0 to 1000");
                continue;
            }
        }

        QString strand = columns.size() >= 6 ? columns[5].trimmed() : ".";
        if (strand.isEmpty())
            strand = ".";
        if (strand != "+" && strand != "-" && strand != ".")
        {
            malformed("strand must be +, - or .");
            continue;
        }

        BedAnnotation annotation;
        annotation.start = start;
        annotation.end = end;
        annotation.name = columns.size() >= 4 && columns[3] != "." ? columns[3] : QString();
        annotation.colour = DEFAULT_BED_COLOUR;

        if (columns.size() >= 8)
        {
            if (!parseInteger(columns[6], &annotation.thickStart) ||
                    !parseInteger(columns[7], &annotation.thickEnd) ||
                    annotation.thickStart < start || annotation.thickEnd < annotation.thickStart ||
                    annotation.thickEnd > end)
            {
                malformed("invalid thickStart/thickEnd coordinates");
                continue;
            }
            annotation.hasThick = annotation.thickEnd > annotation.thickStart;
        }

        if (columns.size() >= 9 && !parseRgb(columns[8].trimmed(), &annotation.colour))
        {
            malformed("itemRgb must be 0 or three comma-separated values from 0 to 255");
            continue;
        }

        if (columns.size() >= 12)
        {
            qint64 blockCount64 = 0;
            if (!parseInteger(columns[9], &blockCount64) || blockCount64 < 0 || blockCount64 > 1000000)
            {
                malformed("invalid blockCount");
                continue;
            }
            const int blockCount = static_cast<int>(blockCount64);
            QVector<qint64> sizes;
            QVector<qint64> relativeStarts;
            if (!parseCommaSeparatedIntegers(columns[10], blockCount, &sizes) ||
                    !parseCommaSeparatedIntegers(columns[11], blockCount, &relativeStarts))
            {
                malformed("blockCount does not match blockSizes/blockStarts");
                continue;
            }

            bool validBlocks = true;
            for (int i = 0; i < blockCount; ++i)
            {
                if (sizes[i] <= 0 || relativeStarts[i] < 0 || relativeStarts[i] > end - start)
                {
                    validBlocks = false;
                    break;
                }
                const qint64 blockStart = start + relativeStarts[i];
                if (sizes[i] > end - blockStart)
                {
                    validBlocks = false;
                    break;
                }
                const qint64 blockEnd = blockStart + sizes[i];
                annotation.blocks.append({blockStart, blockEnd});
            }
            if (!validBlocks)
            {
                malformed("a BED block is outside the main interval");
                continue;
            }
        }

        const QString resolvedName = graph.getNodeNameFromString(columns[0].trimmed());
        DeBruijnNode *node = graph.m_deBruijnGraphNodes.value(resolvedName, nullptr);
        if (node == nullptr)
        {
            ++result.unmatchedCount;
            addExample(&result.unmatchedExamples, lineNumber,
                       QString("node '%1' is not in the graph").arg(columns[0].trimmed()));
            continue;
        }
        if (strand == "-")
            node = oppositeNode(node);

        if (node == nullptr || end > node->getLength())
        {
            ++result.outOfRangeCount;
            addExample(&result.outOfRangeExamples, lineNumber,
                       QString("interval [%1, %2) exceeds node '%3' (length %4)")
                           .arg(start).arg(end).arg(columns[0].trimmed())
                           .arg(node == nullptr ? 0 : node->getLength()));
            continue;
        }

        result.annotations[node].append(annotation);
        ++result.validCount;
    }

    if (result.validCount == 0)
    {
        result.fatalError = "The BED file contains no valid annotations for the current graph.";
        const QString details = result.warningSummary();
        if (!details.isEmpty())
            result.fatalError += "\n\n" + details;
    }
    return result;
}

void clearBedAnnotations(AssemblyGraph &graph)
{
    for (DeBruijnNode *node : graph.m_deBruijnGraphNodes)
        node->clearBedAnnotations();
}

void replaceBedAnnotations(AssemblyGraph &graph, const BedLoadResult &result)
{
    clearBedAnnotations(graph);
    for (auto iterator = result.annotations.constBegin(); iterator != result.annotations.constEnd(); ++iterator)
        iterator.key()->setBedAnnotations(iterator.value());
}
