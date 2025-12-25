//Copyright 2024

//This file is part of Bandage.

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

#ifndef SELECTEDNODESPATHSWIDGET_H
#define SELECTEDNODESPATHSWIDGET_H

#include <QList>
#include <QWidget>
#include "../graph/path.h"

class QLabel;
class QPushButton;
class QTableWidget;

class SelectedNodesPathsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SelectedNodesPathsWidget(QWidget * parent, const QList<Path> &paths);
    ~SelectedNodesPathsWidget();

private:
    QList<Path> m_paths;
    QLabel * m_infoLabel;
    QTableWidget * m_table;
    QPushButton * m_highlightButton;
    QPushButton * m_highlightAllButton;
    QPushButton * m_exportFastaButton;

    void populateTable();
    void updateButtons();
    void highlightPathsForRows(const QList<int> &rows);
    void exportPathSequence(int row);

private slots:
    void onSelectionChanged();
    void highlightSelectedPaths();
    void highlightAllPaths();
    void exportSelectedPathSequence();

signals:
    void selectionChanged();
    void highlightRequested();

protected:
    void hideEvent(QHideEvent * event) override;
    void showEvent(QShowEvent * event) override;
};

#endif // SELECTEDNODESPATHSWIDGET_H
