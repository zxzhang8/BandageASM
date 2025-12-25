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

#ifndef GAFPATHSDIALOG_H
#define GAFPATHSDIALOG_H

#include <QList>
#include <QWidget>
#include "../program/gafparser.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QSpinBox;
class QLineEdit;
class QModelIndex;

class GafPathsTable : public QTableWidget
{
    Q_OBJECT

public:
    explicit GafPathsTable(QWidget *parent = 0);
    void setPathColumn(int col);

protected:
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;

private:
    int m_pathColumn;
};

class GafPathsDialog : public QWidget
{
    Q_OBJECT

public:
    explicit GafPathsDialog(QWidget * parent,
                            const QString &fileName,
                            const GafParseResult &parseResult);
    ~GafPathsDialog();

private:
    QString m_fileName;
    QList<GafAlignment> m_alignments;
    QStringList m_warnings;
    QTableWidget * m_table;
    QPushButton * m_highlightButton;
    QPushButton * m_highlightAllButton;
    QPushButton * m_filterButton;
    QPushButton * m_resetFilterButton;
    QSpinBox * m_mapqFilterSpinBox;
    QLineEdit * m_nodeFilterLineEdit;
    QLabel * m_warningLabel;
    QList<int> m_visibleRows;
    int m_currentMapqThreshold;
    QString m_nodeFilter;

    void populateTable();
    void applyMapqFilter();
    void resetFilter();
    void updateButtons();
    void showWarnings();
    void highlightPathsForRows(const QList<int> &rows);
    int alignmentIndexForRow(int row) const;

private slots:
    void onSelectionChanged();
    void highlightSelectedPaths();
    void highlightAllPaths();
    void filterByMapq();
    void resetMapqFilter();

signals:
    void selectionChanged();
    void highlightRequested();

protected:
    void hideEvent(QHideEvent * event) override;
    void showEvent(QShowEvent * event) override;
};

#endif // GAFPATHSDIALOG_H
