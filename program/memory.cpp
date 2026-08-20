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


#include "memory.h"

#include <QDir>

Memory::Memory()
{
    rememberedPath = QDir::homePath();

    commandLineCommand = NO_COMMAND;

    pathDialogIsVisible = false;
    queryPathDialogIsVisible = false;
    gafPathDialogIsVisible = false;
    selectedPathsDialogIsVisible = false;
    pathHighlightSource = NO_PATH_HIGHLIGHT;
    m_gafHeatScale = GAF_HEAT_LOG;
    m_gafVisualizationVisible = false;

    userSpecifiedPath = Path();
    userSpecifiedPathString = "";
    userSpecifiedPathCircular = false;

    distancePathSearchQuery1 = "";
    distancePathSearchQuery2 = "";
    distancePathSearchQuery1Path = "";
    distancePathSearchQuery2Path = "";

    terminalWidth = 80;
}



//This function clears all memory that is particular to a graph.  It should be
//called whenever a new graph is loaded.
void Memory::clearGraphSpecificMemory()
{
    userSpecifiedPath = Path();
    userSpecifiedPathString = "";
    userSpecifiedPathCircular = false;
    clearAllQueryPaths();
    clearGafVisualization();
    queryPathDialogIsVisible = false;
    gafPathDialogIsVisible = false;
    selectedPathsDialogIsVisible = false;

    distanceSearchResults.clear();
    distancePathSearchQuery1 = "";
    distancePathSearchQuery2 = "";
    distancePathSearchQuery1Path = "";
    distancePathSearchQuery2Path = "";
}


void Memory::setGafVisualization(const GafVisualizationData &data)
{
    m_gafVisualization = data;
    m_gafVisualizationVisible = !data.cancelled;
}


bool Memory::clearGafVisualization()
{
    const bool changed = m_gafVisualizationVisible || !m_gafVisualization.isEmpty();
    m_gafVisualization = GafVisualizationData();
    m_gafVisualizationVisible = false;
    return changed;
}


void Memory::setQueryPaths(const QList<Path> &paths, PathHighlightSource source)
{
    queryPaths = paths;
    pathHighlightSource = queryPaths.isEmpty() ? NO_PATH_HIGHLIGHT : source;
}


bool Memory::clearQueryPaths(PathHighlightSource source)
{
    if (pathHighlightSource != source)
        return false;

    return clearAllQueryPaths();
}


bool Memory::clearAllQueryPaths()
{
    bool changed = !queryPaths.isEmpty() || pathHighlightSource != NO_PATH_HIGHLIGHT;
    queryPaths.clear();
    pathHighlightSource = NO_PATH_HIGHLIGHT;
    return changed;
}


bool Memory::queryPathHighlightIsVisible() const
{
    if (queryPaths.isEmpty())
        return false;

    if (pathHighlightSource == BLAST_QUERY_PATH_HIGHLIGHT)
        return queryPathDialogIsVisible;

    return pathHighlightSource == GAF_PATH_HIGHLIGHT ||
            pathHighlightSource == SELECTED_NODE_PATH_HIGHLIGHT;
}
