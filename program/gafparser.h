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

#ifndef GAFPARSER_H
#define GAFPARSER_H

#include <QList>
#include <QString>
#include <QStringList>
#include "../graph/path.h"

struct GafAlignment
{
    QString queryName;
    QString strand;
    QString rawPathField;
    QString bandagePathString;
    int lineNumber;
    int queryStart;
    int queryEnd;
    int queryLength;
    int mappingQuality;
    Path path;
};

struct GafParseResult
{
    QList<GafAlignment> alignments;
    QStringList warnings;
    bool isEmpty() const {return alignments.isEmpty();}
};

GafParseResult parseGafFile(const QString &fileName);

#endif // GAFPARSER_H
