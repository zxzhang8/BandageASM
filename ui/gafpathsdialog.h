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

#include <QAbstractTableModel>
#include <QList>
#include <QTableView>
#include <QWidget>
#include "../program/gafparser.h"

class QLabel;
class QPushButton;
class QSpinBox;
class QLineEdit;
class QComboBox;
class QLabel;
class QModelIndex;

class GafPathsTableView : public QTableView
{
    Q_OBJECT

public:
    explicit GafPathsTableView(QWidget *parent = 0);
    void setPathColumn(int col);

protected:
    void scrollTo(const QModelIndex &index,
                  QAbstractItemView::ScrollHint hint = QAbstractItemView::EnsureVisible) override;

private:
    int m_pathColumn;
};

class GafPathsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit GafPathsModel(const QList<GafAlignment> * alignments, QObject * parent = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setVisibleRows(const QList<int> &rows);
    void setPageSize(int size);
    void setCurrentPage(int page);
    int currentPage() const {return m_currentPage;}
    int pageCount() const;
    int totalRows() const {return m_visibleRows.size();}
    int alignmentIndexForRow(int row) const;
    QList<int> visibleRows() const {return m_visibleRows;}

private:
    const QList<GafAlignment> * m_alignments;
    QList<int> m_visibleRows;
    QList<int> m_pageRows;
    int m_pageSize;
    int m_currentPage;

    void rebuildPageRows();
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
    GafPathsModel * m_model;
    GafPathsTableView * m_table;
    QPushButton * m_highlightButton;
    QPushButton * m_highlightAllButton;
    QPushButton * m_filterButton;
    QPushButton * m_resetFilterButton;
    QPushButton * m_prevPageButton;
    QPushButton * m_nextPageButton;
    QSpinBox * m_mapqFilterSpinBox;
    QLineEdit * m_nodeFilterLineEdit;
    QComboBox * m_nodeFilterModeComboBox;
    QSpinBox * m_pageSizeSpinBox;
    QLabel * m_warningLabel;
    QLabel * m_pageLabel;
    QList<int> m_visibleRows;
    int m_currentMapqThreshold;
    QStringList m_nodeFilters;
    bool m_nodeFilterMatchAll;

    void populateTable();
    void applyMapqFilter();
    void resetFilter();
    void updateButtons();
    void updatePaginationControls();
    void showWarnings();
    void highlightPathsForAlignments(const QList<int> &alignmentIndices);

private slots:
    void onSelectionChanged();
    void highlightSelectedPaths();
    void highlightAllPaths();
    void filterByMapq();
    void resetMapqFilter();
    void goToNextPage();
    void goToPreviousPage();
    void pageSizeChanged(int value);

signals:
    void selectionChanged();
    void highlightRequested();

protected:
    void hideEvent(QHideEvent * event) override;
    void showEvent(QShowEvent * event) override;
};

#endif // GAFPATHSDIALOG_H
