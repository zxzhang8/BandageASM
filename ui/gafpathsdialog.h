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
#include <QFutureWatcher>
#include <atomic>
#include <memory>
#include "../program/gafparser.h"
#include "../program/gafvisualization.h"

class QLabel;
class QPushButton;
class QSpinBox;
class QLineEdit;
class QComboBox;
class QLabel;
class QModelIndex;
class QProgressDialog;

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
    QString fileName() const {return m_fileName;}
    int alignmentCount() const {return m_alignments.size();}
    QList<GafAlignment> allAlignments() const {return m_alignments;}
    QList<GafAlignment> filteredAlignments() const;
    bool writeFilteredGaf(const QString &fileName, QString * errorMessage = 0) const;

private:
    QString m_fileName;
    QList<GafAlignment> m_alignments;
    QStringList m_warnings;
    GafPathsModel * m_model;
    GafPathsTableView * m_table;
    QPushButton * m_highlightButton;
    QPushButton * m_highlightAllButton;
    QPushButton * m_clearHighlightButton;
    QPushButton * m_saveFilteredButton;
    QPushButton * m_visualizeButton;
    QPushButton * m_clearVisualizationButton;
    QPushButton * m_filterButton;
    QPushButton * m_resetFilterButton;
    QPushButton * m_prevPageButton;
    QPushButton * m_nextPageButton;
    QSpinBox * m_mapqFilterSpinBox;
    QSpinBox * m_nodeCountFilterSpinBox;
    QLineEdit * m_nodeFilterLineEdit;
    QComboBox * m_nodeFilterModeComboBox;
    QComboBox * m_countBasisComboBox;
    QComboBox * m_heatScaleComboBox;
    QSpinBox * m_pageSizeSpinBox;
    QLineEdit * m_pageCurrentLineEdit;
    QLabel * m_pageTotalLabel;
    QLabel * m_warningLabel;
    QLabel * m_visualizationStatusLabel;
    QLabel * m_visualizationLegendLabel;
    QList<int> m_visibleRows;
    QList<int> m_visibleRowsBase;
    int m_currentMapqThreshold;
    int m_currentNodeCountThreshold;
    QStringList m_nodeFilters;
    int m_nodeFilterMode;
    bool m_queryRangeSorted;
    int m_filterRevision;
    int m_visualizedFilterRevision;
    int m_visualizationBuildRevision;
    bool m_visualizationRunning;
    QFutureWatcher<GafVisualizationData> * m_visualizationWatcher;
    QProgressDialog * m_visualizationProgress;
    std::shared_ptr<std::atomic_bool> m_visualizationCancelled;

    void populateTable();
    void applyMapqFilter();
    void resetFilter();
    void updateButtons();
    void updatePaginationControls();
    void showWarnings();
    void highlightPathsForAlignments(const QList<int> &alignmentIndices);
    void markVisualizationOutOfDate();
    void updateVisualizationControls();
    void updateVisualizationLegend();

private slots:
    void onSelectionChanged();
    void highlightSelectedPaths();
    void highlightAllPaths();
    void clearHighlighting();
    void saveFilteredGaf();
    void filterByMapq();
    void resetMapqFilter();
    void goToNextPage();
    void goToPreviousPage();
    void pageSizeChanged(int value);
    void pageCurrentEdited();
    void handleHeaderClicked(int section);
    void visualizeGaf();
    void clearVisualization();
    void visualizationFinished();
    void visualizationScaleChanged(int index);
    void visualizationBasisChanged(int index);

signals:
    void selectionChanged();
    void highlightRequested();
    void clearHighlightRequested();
    void visualizationChanged();
    void visualizationRequested();

protected:
    void hideEvent(QHideEvent * event) override;
    void showEvent(QShowEvent * event) override;
};

#endif // GAFPATHSDIALOG_H
