//Copyright 2017 Ryan Wick

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


#ifndef MEMORY_H
#define MEMORY_H

#include "../program/globals.h"
#include <QList>
#include <QStringList>
#include "../graph/querydistance.h"
#include "../graph/path.h"
#include "gafvisualization.h"

class Memory
{
public:
    enum PathHighlightSource
    {
        NO_PATH_HIGHLIGHT,
        BLAST_QUERY_PATH_HIGHLIGHT,
        GAF_PATH_HIGHLIGHT,
        SELECTED_NODE_PATH_HIGHLIGHT
    };

    Memory();
    void clearGraphSpecificMemory();
    void setQueryPaths(const QList<Path> &paths, PathHighlightSource source);
    bool clearQueryPaths(PathHighlightSource source);
    bool clearAllQueryPaths();
    bool queryPathHighlightIsVisible() const;
    void setGafVisualization(const GafVisualizationData &data);
    bool clearGafVisualization();
    bool gafVisualizationIsVisible() const {return m_gafVisualizationVisible;}
    const GafVisualizationData &gafVisualization() const {return m_gafVisualization;}
    void setGafHeatScale(GafHeatScale scale) {m_gafHeatScale = scale;}
    GafHeatScale gafHeatScale() const {return m_gafHeatScale;}

    QString rememberedPath;

    CommandLineCommand commandLineCommand;

    bool pathDialogIsVisible;
    bool queryPathDialogIsVisible;
    bool gafPathDialogIsVisible;
    bool selectedPathsDialogIsVisible;

    //These store the user input in the 'Specify exact path...' dialog so it is
    //retained between uses.
    Path userSpecifiedPath;
    QString userSpecifiedPathString;
    bool userSpecifiedPathCircular;

    //These store the results of a distance search between two queries.
    QList<QueryDistance> distanceSearchResults;

    //These store the last used distance path search queries/paths.
    QString distancePathSearchQuery1;
    QString distancePathSearchQuery2;
    QString distancePathSearchQuery1Path;
    QString distancePathSearchQuery2Path;

    //This stores the currently selected query path in a query path dialog.
    QList<Path> queryPaths;
    PathHighlightSource pathHighlightSource;

    GafVisualizationData m_gafVisualization;
    GafHeatScale m_gafHeatScale;
    bool m_gafVisualizationVisible;

    int terminalWidth;
};

#endif // MEMORY_H
