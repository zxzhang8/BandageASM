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

#include "gafpathsdialog.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QComboBox>
#include <QRegularExpression>
#include <QHideEvent>
#include <QLabel>
#include <QIntValidator>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QAbstractSpinBox>
#include <QLineEdit>
#include <QScrollBar>
#include "../graph/debruijnnode.h"
#include "../graph/graphicsitemnode.h"
#include "../program/globals.h"
#include "../program/memory.h"
#include "../program/settings.h"
#include <QShowEvent>
#include "mygraphicsscene.h"
#include "mygraphicsview.h"

GafPathsTableView::GafPathsTableView(QWidget *parent)
    : QTableView(parent),
      m_pathColumn(-1)
{
}

void GafPathsTableView::setPathColumn(int col)
{
    m_pathColumn = col;
}

void GafPathsTableView::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint)
{
    if (index.column() == m_pathColumn)
    {
        int horizontalValue = horizontalScrollBar()->value();
        QTableView::scrollTo(index, hint);
        horizontalScrollBar()->setValue(horizontalValue);
        return;
    }

    QTableView::scrollTo(index, hint);
}

GafPathsModel::GafPathsModel(const QList<GafAlignment> * alignments, QObject * parent) :
    QAbstractTableModel(parent),
    m_alignments(alignments),
    m_pageSize(500),
    m_currentPage(0)
{
}

int GafPathsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_pageRows.size();
}

int GafPathsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 7;
}

QVariant GafPathsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    int row = index.row();
    if (row < 0 || row >= m_pageRows.size())
        return QVariant();

    int alignmentIndex = m_pageRows[row];
    if (alignmentIndex < 0 || alignmentIndex >= m_alignments->size())
        return QVariant();

    const GafAlignment &a = (*m_alignments)[alignmentIndex];
    switch (index.column())
    {
        case 0: return QString::number(a.lineNumber);
        case 1: return a.queryName;
        case 2: return a.strand;
        case 3: return (a.mappingQuality >= 0) ? QString::number(a.mappingQuality) : "";
        case 4: return QString::number(a.path.getNodeCount());
        case 5: return a.bandagePathString;
        case 6:
        {
            if (a.queryStart >= 0 && a.queryEnd >= 0 && a.queryLength > 0)
                return QString::number(a.queryStart) + "-" + QString::number(a.queryEnd) + " / " + QString::number(a.queryLength);
            if (a.queryStart >= 0 && a.queryEnd >= 0)
                return QString::number(a.queryStart) + "-" + QString::number(a.queryEnd);
            return "";
        }
        default:
            return QVariant();
    }
}

QVariant GafPathsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case 0: return "#";
            case 1: return "Query";
            case 2: return "Strand";
            case 3: return "MAPQ";
            case 4: return "Nodes";
            case 5: return "Path";
            case 6: return "Query range";
            default: return QVariant();
        }
    }

    return QVariant();
}

void GafPathsModel::setVisibleRows(const QList<int> &rows)
{
    m_visibleRows = rows;
    if (m_currentPage >= pageCount())
        m_currentPage = 0;
    rebuildPageRows();
}

void GafPathsModel::setPageSize(int size)
{
    int newSize = qMax(1, size);
    if (m_pageSize == newSize)
        return;
    m_pageSize = newSize;
    if (m_currentPage >= pageCount())
        m_currentPage = 0;
    rebuildPageRows();
}

void GafPathsModel::setCurrentPage(int page)
{
    int clamped = qBound(0, page, qMax(0, pageCount() - 1));
    if (m_currentPage == clamped)
        return;
    m_currentPage = clamped;
    rebuildPageRows();
}

int GafPathsModel::pageCount() const
{
    if (m_visibleRows.isEmpty())
        return 0;
    return (m_visibleRows.size() + m_pageSize - 1) / m_pageSize;
}

int GafPathsModel::alignmentIndexForRow(int row) const
{
    if (row < 0 || row >= m_pageRows.size())
        return -1;
    return m_pageRows[row];
}

void GafPathsModel::rebuildPageRows()
{
    beginResetModel();
    m_pageRows.clear();
    if (!m_visibleRows.isEmpty())
    {
        int start = m_currentPage * m_pageSize;
        int end = qMin(start + m_pageSize, m_visibleRows.size());
        for (int i = start; i < end; ++i)
            m_pageRows.push_back(m_visibleRows[i]);
    }
    endResetModel();
}

GafPathsDialog::GafPathsDialog(QWidget * parent,
                               const QString &fileName,
                               const GafParseResult &parseResult) :
    QWidget(parent),
    m_fileName(fileName),
    m_alignments(parseResult.alignments),
    m_warnings(parseResult.warnings),
    m_model(new GafPathsModel(&m_alignments, this)),
    m_table(new GafPathsTableView(this)),
    m_highlightButton(new QPushButton("Highlight selected paths", this)),
    m_highlightAllButton(new QPushButton("Highlight all paths", this)),
    m_filterButton(new QPushButton("Filter", this)),
    m_resetFilterButton(new QPushButton("Reset", this)),
    m_prevPageButton(new QPushButton("Prev", this)),
    m_nextPageButton(new QPushButton("Next", this)),
    m_mapqFilterSpinBox(new QSpinBox(this)),
    m_nodeFilterLineEdit(new QLineEdit(this)),
    m_nodeFilterModeComboBox(new QComboBox(this)),
    m_pageSizeSpinBox(new QSpinBox(this)),
    m_pageCurrentLineEdit(new QLineEdit(this)),
    m_pageTotalLabel(new QLabel(this)),
    m_warningLabel(new QLabel(this))
{
    setWindowTitle("GAF Paths");

    QVBoxLayout * layout = new QVBoxLayout(this);

    QLabel * title = new QLabel("File: " + fileName, this);
    title->setWordWrap(true);
    layout->addWidget(title);

    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    static_cast<GafPathsTableView *>(m_table)->setPathColumn(5);
    layout->addWidget(m_table);

    m_mapqFilterSpinBox->setRange(0, 1000);
    m_mapqFilterSpinBox->setValue(0);
    m_mapqFilterSpinBox->setPrefix("MAPQ ≥ ");
    m_mapqFilterSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_mapqFilterSpinBox->setFixedWidth(120);

    m_nodeFilterLineEdit->setPlaceholderText("Node name(s)");
    m_nodeFilterLineEdit->setFixedWidth(200);

    m_nodeFilterModeComboBox->addItem("Any");
    m_nodeFilterModeComboBox->addItem("All");
    m_nodeFilterModeComboBox->setFixedWidth(70);

    m_pageSizeSpinBox->setRange(10, 5000);
    m_pageSizeSpinBox->setValue(500);
    m_pageSizeSpinBox->setPrefix("Page size ");
    m_pageSizeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pageSizeSpinBox->setFixedWidth(140);

    QHBoxLayout * paginationLayout = new QHBoxLayout();
    paginationLayout->addStretch();
    paginationLayout->addWidget(m_pageSizeSpinBox);
    paginationLayout->addWidget(m_prevPageButton);
    paginationLayout->addWidget(m_nextPageButton);
    paginationLayout->addWidget(new QLabel("Page", this));
    m_pageCurrentLineEdit->setFixedWidth(50);
    m_pageCurrentLineEdit->setValidator(new QIntValidator(1, 1, m_pageCurrentLineEdit));
    paginationLayout->addWidget(m_pageCurrentLineEdit);
    m_pageTotalLabel->setMinimumWidth(60);
    paginationLayout->addWidget(m_pageTotalLabel);
    paginationLayout->addStretch();
    layout->addLayout(paginationLayout);

    QHBoxLayout * buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_highlightButton);
    buttonLayout->addWidget(m_highlightAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(new QLabel("Path includes:", this));
    buttonLayout->addWidget(m_nodeFilterLineEdit);
    buttonLayout->addWidget(m_nodeFilterModeComboBox);
    buttonLayout->addWidget(m_mapqFilterSpinBox);
    buttonLayout->addWidget(m_filterButton);
    buttonLayout->addWidget(m_resetFilterButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    m_warningLabel->setWordWrap(true);
    layout->addWidget(m_warningLabel);

    m_visibleRows.clear();
    for (int i = 0; i < m_alignments.size(); ++i)
        m_visibleRows << i;
    m_currentMapqThreshold = 0;
    m_nodeFilters.clear();
    m_nodeFilterMatchAll = false;

    populateTable();
    showWarnings();
    updateButtons();

    connect(m_table->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onSelectionChanged()));
    connect(m_highlightButton, SIGNAL(clicked()), this, SLOT(highlightSelectedPaths()));
    connect(m_highlightAllButton, SIGNAL(clicked()), this, SLOT(highlightAllPaths()));
    connect(m_filterButton, SIGNAL(clicked()), this, SLOT(filterByMapq()));
    connect(m_resetFilterButton, SIGNAL(clicked()), this, SLOT(resetMapqFilter()));
    connect(m_prevPageButton, SIGNAL(clicked()), this, SLOT(goToPreviousPage()));
    connect(m_nextPageButton, SIGNAL(clicked()), this, SLOT(goToNextPage()));
    connect(m_pageSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(pageSizeChanged(int)));
    connect(m_pageCurrentLineEdit, SIGNAL(returnPressed()), this, SLOT(pageCurrentEdited()));
}


GafPathsDialog::~GafPathsDialog()
{
    g_memory->gafPathDialogIsVisible = false;

    //If no other path dialog is visible, clear query paths to remove highlighting.
    if (!g_memory->queryPathDialogIsVisible)
    {
        g_memory->queryPaths.clear();
        emit selectionChanged();
    }
}


void GafPathsDialog::showEvent(QShowEvent * event)
{
    g_memory->gafPathDialogIsVisible = true;
    QWidget::showEvent(event);
}


void GafPathsDialog::hideEvent(QHideEvent * event)
{
    g_memory->gafPathDialogIsVisible = false;

    if (!g_memory->queryPathDialogIsVisible)
    {
        g_memory->queryPaths.clear();
        emit selectionChanged();
    }

    QWidget::hideEvent(event);
}


void GafPathsDialog::populateTable()
{
    m_model->setVisibleRows(m_visibleRows);
    m_model->setPageSize(m_pageSizeSpinBox->value());
    m_model->setCurrentPage(0);
    updatePaginationControls();
    m_table->resizeColumnsToContents();
}


void GafPathsDialog::showWarnings()
{
    if (m_warnings.isEmpty())
    {
        m_warningLabel->setText("");
        return;
    }

    QString text = "The following records could not be loaded:<ul>";
    for (int i = 0; i < m_warnings.size(); ++i)
        text += "<li>" + m_warnings[i] + "</li>";
    text += "</ul>";
    m_warningLabel->setText(text);
}


void GafPathsDialog::updateButtons()
{
    bool hasSelection = m_table->selectionModel() != 0 &&
            m_table->selectionModel()->hasSelection();
    m_highlightButton->setEnabled(hasSelection);
    m_highlightAllButton->setEnabled(m_model->totalRows() > 0);
    m_filterButton->setEnabled(true);
    m_resetFilterButton->setEnabled(m_model->totalRows() != m_alignments.size() ||
                                    m_currentMapqThreshold != 0 ||
                                    !m_nodeFilters.isEmpty());
}


void GafPathsDialog::onSelectionChanged()
{
    updateButtons();
}


void GafPathsDialog::highlightSelectedPaths()
{
    if (m_table->selectionModel() == 0 || !m_table->selectionModel()->hasSelection())
    {
        QMessageBox::information(this, "No paths selected", "Select at least one path first.");
        return;
    }

    QModelIndexList selectedRows = m_table->selectionModel()->selectedRows();
    QList<int> alignmentIndices;
    for (int i = 0; i < selectedRows.size(); ++i)
    {
        int alignmentIndex = m_model->alignmentIndexForRow(selectedRows[i].row());
        if (alignmentIndex >= 0)
            alignmentIndices.push_back(alignmentIndex);
    }

    highlightPathsForAlignments(alignmentIndices);
}


void GafPathsDialog::highlightAllPaths()
{
    if (m_model->totalRows() == 0)
    {
        QMessageBox::information(this, "No paths to highlight", "No paths are visible with the current filters.");
        return;
    }

    highlightPathsForAlignments(m_model->visibleRows());
}

void GafPathsDialog::highlightPathsForAlignments(const QList<int> &alignmentIndices)
{
    g_memory->gafPathDialogIsVisible = true;
    g_memory->queryPaths.clear();

    g_graphicsView->scene()->blockSignals(true);
    g_graphicsView->scene()->clearSelection();

    QStringList nodesNotFound;

    for (int i = 0; i < alignmentIndices.size(); ++i)
    {
        int alignmentIndex = alignmentIndices[i];
        if (alignmentIndex < 0 || alignmentIndex >= m_alignments.size())
            continue;

        const GafAlignment &alignment = m_alignments[alignmentIndex];
        g_memory->queryPaths.push_back(alignment.path);

        QList<DeBruijnNode *> nodes = alignment.path.getNodes();
        for (int n = 0; n < nodes.size(); ++n)
        {
            DeBruijnNode * node = nodes[n];
            GraphicsItemNode * item = node->getGraphicsItemNode();
            if (item == 0 && !g_settings->doubleMode)
                item = node->getReverseComplement()->getGraphicsItemNode();

            if (item != 0)
                item->setSelected(true);
            else
                nodesNotFound << node->getName();
        }
    }

    g_graphicsView->scene()->blockSignals(false);

    emit selectionChanged();
    g_graphicsView->viewport()->update();

    if (!nodesNotFound.isEmpty())
    {
        QStringList unique = nodesNotFound;
        unique.removeDuplicates();
        QMessageBox::information(this, "Nodes not visible",
                                 "These nodes are not currently drawn, so they cannot be highlighted:\n" +
                                 unique.join(", ") + "\n\nRedraw with a larger scope and try again.");
    }

    emit highlightRequested();
}


void GafPathsDialog::filterByMapq()
{
    m_currentMapqThreshold = m_mapqFilterSpinBox->value();
    QString rawFilter = m_nodeFilterLineEdit->text().trimmed();
    m_nodeFilters = rawFilter.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    m_nodeFilterMatchAll = (m_nodeFilterModeComboBox->currentIndex() == 1);
    applyMapqFilter();
}


void GafPathsDialog::resetMapqFilter()
{
    m_currentMapqThreshold = 0;
    resetFilter();
}


void GafPathsDialog::applyMapqFilter()
{
    m_visibleRows.clear();
    for (int i = 0; i < m_alignments.size(); ++i)
    {
        int mapq = m_alignments[i].mappingQuality;
        bool mapqPass = (m_currentMapqThreshold <= 0 || mapq >= m_currentMapqThreshold);
        if (!mapqPass)
            continue;

        if (m_nodeFilters.isEmpty())
        {
            m_visibleRows << i;
            continue;
        }

        const QList<DeBruijnNode *> nodes = m_alignments[i].path.getNodes();
        bool matched = false;
        bool allMatched = true;
        for (int f = 0; f < m_nodeFilters.size(); ++f)
        {
            const QString filter = m_nodeFilters[f];
            bool filterHasSign = filter.endsWith('+') || filter.endsWith('-');
            bool filterMatched = false;
            for (int n = 0; n < nodes.size(); ++n)
            {
                const QString nodeName = nodes[n]->getName();
                if (filterHasSign)
                {
                    if (nodeName == filter)
                    {
                        filterMatched = true;
                        break;
                    }
                }
                else if (nodes[n]->getNameWithoutSign() == filter)
                {
                    filterMatched = true;
                    break;
                }
            }

            if (m_nodeFilterMatchAll)
            {
                if (!filterMatched)
                {
                    allMatched = false;
                    break;
                }
            }
            else if (filterMatched)
            {
                matched = true;
                break;
            }
        }

        if ((m_nodeFilterMatchAll && allMatched) || (!m_nodeFilterMatchAll && matched))
            m_visibleRows << i;
    }

    populateTable();
    showWarnings();
    updateButtons();
}


void GafPathsDialog::resetFilter()
{
    m_visibleRows.clear();
    for (int i = 0; i < m_alignments.size(); ++i)
        m_visibleRows << i;
    m_currentMapqThreshold = 0;
    m_mapqFilterSpinBox->setValue(0);
    m_nodeFilters.clear();
    m_nodeFilterLineEdit->setText("");
    m_nodeFilterModeComboBox->setCurrentIndex(0);
    populateTable();
    showWarnings();
    updateButtons();
}

void GafPathsDialog::updatePaginationControls()
{
    int pageCount = m_model->pageCount();
    int currentPage = m_model->currentPage();

    m_prevPageButton->setEnabled(currentPage > 0);
    m_nextPageButton->setEnabled(currentPage + 1 < pageCount);

    int safePageCount = qMax(1, pageCount);
    m_pageTotalLabel->setText("/ " + QString::number(pageCount));
    m_pageCurrentLineEdit->setEnabled(pageCount > 0);
    m_pageCurrentLineEdit->blockSignals(true);
    m_pageCurrentLineEdit->setText(pageCount == 0 ? "0" : QString::number(currentPage + 1));
    QIntValidator * validator = qobject_cast<QIntValidator *>(const_cast<QValidator *>(m_pageCurrentLineEdit->validator()));
    if (validator != 0)
        validator->setRange(1, safePageCount);
    m_pageCurrentLineEdit->blockSignals(false);
}

void GafPathsDialog::goToNextPage()
{
    m_model->setCurrentPage(m_model->currentPage() + 1);
    updatePaginationControls();
    updateButtons();
}

void GafPathsDialog::goToPreviousPage()
{
    m_model->setCurrentPage(m_model->currentPage() - 1);
    updatePaginationControls();
    updateButtons();
}

void GafPathsDialog::pageSizeChanged(int value)
{
    m_model->setPageSize(value);
    updatePaginationControls();
    updateButtons();
}

void GafPathsDialog::pageCurrentEdited()
{
    bool ok = false;
    int value = m_pageCurrentLineEdit->text().toInt(&ok);
    if (!ok)
        return;
    m_model->setCurrentPage(value - 1);
    updatePaginationControls();
    updateButtons();
}
