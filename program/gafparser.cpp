//Copyright 2024

//This file is part of Bandage

//Bandage is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//Bandage is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with Bandage.  If not, see <http://www.gnu.org/licenses/>.

#include "gafparser.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace
{
QString normaliseNodeName(QString name, QChar orientation)
{
    QString trimmed = name.trimmed();
    if (trimmed.endsWith("+") || trimmed.endsWith("-"))
        trimmed.chop(1);

    return trimmed + orientation;
}


QStringList parseWalkWithArrows(const QString &pathField, QString * error)
{
    QString cleaned = pathField;
    cleaned.remove(','); // commas are optional separators in some outputs

    QStringList nodes;
    QString currentName;
    QChar currentOrientation;

    auto flushNode = [&]() -> bool
    {
        if (currentOrientation.isNull())
            return true;

        QString name = currentName.trimmed();
        if (name.isEmpty())
        {
            *error = "empty segment name in path";
            return false;
        }

        nodes << normaliseNodeName(name, currentOrientation == '>' ? '+' : '-');
        currentName.clear();
        return true;
    };

    for (int i = 0; i < cleaned.size(); ++i)
    {
        QChar ch = cleaned.at(i);
        if (ch == '>' || ch == '<')
        {
            if (!flushNode())
                return QStringList();
            currentOrientation = ch;
        }
        else if (ch != ';')
            currentName.append(ch);
    }

    if (!flushNode())
        return QStringList();

    if (nodes.isEmpty())
        *error = "no nodes found in path";

    return nodes;
}


QStringList parseWalkWithSuffixes(const QString &pathField, QString * error)
{
    QStringList parts = pathField.split(QRegularExpression("[,;]"), Qt::SkipEmptyParts);
    QStringList nodes;

    for (int i = 0; i < parts.size(); ++i)
    {
        QString part = parts[i].trimmed();
        if (part.length() < 2)
        {
            *error = "path entry too short";
            return QStringList();
        }

        QChar orientation = part.right(1).at(0);
        if (orientation != '+' && orientation != '-')
        {
            *error = "missing orientation (+/-) in path entry: " + part;
            return QStringList();
        }

        part.chop(1);
        nodes << normaliseNodeName(part, orientation);
    }

    return nodes;
}


QStringList parseGafPath(const QString &pathField, QString * error)
{
    QString trimmed = pathField.trimmed();
    if (trimmed == "" || trimmed == "*")
    {
        *error = "path field is empty";
        return QStringList();
    }

    // If the path contains explicit direction markers ('>' or '<'), parse using them.
    if (trimmed.contains(">") || trimmed.contains("<"))
        return parseWalkWithArrows(trimmed, error);

    // Otherwise, expect comma/semicolon separated entries with +/- suffix.
    return parseWalkWithSuffixes(trimmed, error);
}


int safeToInt(const QString &text)
{
    bool ok = false;
    int value = text.toInt(&ok);
    return ok ? value : -1;
}
}


GafParseResult parseGafFile(const QString &fileName)
{
    GafParseResult result;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        result.warnings << "Cannot open GAF file: " + fileName;
        return result;
    }

    QTextStream in(&file);
    int lineNumber = 0;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        ++lineNumber;

        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("#"))
            continue;

        QStringList fields = trimmed.split('\t');
        if (fields.size() < 6)
        {
            result.warnings << "Line " + QString::number(lineNumber) + ": not enough fields, skipped.";
            continue;
        }

        QString pathError;
        QStringList nodeNames = parseGafPath(fields[5], &pathError);
        if (nodeNames.isEmpty())
        {
            result.warnings << "Line " + QString::number(lineNumber) + ": failed to parse path (" + pathError + ").";
            continue;
        }

        QString bandagePathString = nodeNames.join(", ");
        QString pathFailure;
        Path path = Path::makeFromString(bandagePathString, false, &pathFailure);
        if (path.isEmpty())
        {
            if (pathFailure.length() == 0)
                pathFailure = "the nodes do not form a path";
            result.warnings << "Line " + QString::number(lineNumber) + ": invalid path (" + pathFailure + ").";
            continue;
        }

        GafAlignment alignment;
        alignment.queryName = fields[0];
        alignment.queryLength = fields.size() > 1 ? safeToInt(fields[1]) : -1;
        alignment.queryStart = fields.size() > 2 ? safeToInt(fields[2]) : -1;
        alignment.queryEnd = fields.size() > 3 ? safeToInt(fields[3]) : -1;
        alignment.strand = fields[4];
        alignment.rawPathField = fields[5];
        alignment.bandagePathString = bandagePathString;
        alignment.mappingQuality = fields.size() > 11 ? safeToInt(fields[11]) : -1;
        alignment.lineNumber = lineNumber;
        alignment.path = path;

        result.alignments.push_back(alignment);
    }

    if (result.alignments.isEmpty() && result.warnings.isEmpty())
        result.warnings << "No alignments were found in the file.";

    return result;
}
