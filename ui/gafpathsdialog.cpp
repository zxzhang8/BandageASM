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
#include <QHideEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
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

GafPathsTable::GafPathsTable(QWidget *parent)
    : QTableWidget(parent),
      m_pathColumn(-1)
{
}

void GafPathsTable::setPathColumn(int col)
{
    m_pathColumn = col;
}

void GafPathsTable::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    if (index.column() == m_pathColumn)
    {
        int horizontalValue = horizontalScrollBar()->value();
        QTableWidget::scrollTo(index, hint);
        horizontalScrollBar()->setValue(horizontalValue);
        return;
    }

    QTableWidget::scrollTo(index, hint);
}

GafPathsDialog::GafPathsDialog(QWidget * parent,
                               const QString &fileName,
                               const GafParseResult &parseResult) :
    QWidget(parent),
    m_fileName(fileName),
    m_alignments(parseResult.alignments),
    m_warnings(parseResult.warnings),
    m_table(new GafPathsTable(this)),
    m_highlightButton(new QPushButton("Highlight selected paths", this)),
    m_highlightAllButton(new QPushButton("Highlight all paths", this)),
    m_filterButton(new QPushButton("Filter", this)),
    m_resetFilterButton(new QPushButton("Reset", this)),
    m_mapqFilterSpinBox(new QSpinBox(this)),
    m_nodeFilterLineEdit(new QLineEdit(this)),
    m_warningLabel(new QLabel(this))
{
    setWindowTitle("GAF Paths");

    QVBoxLayout * layout = new QVBoxLayout(this);

    QLabel * title = new QLabel("File: " + fileName, this);
    title->setWordWrap(true);
    layout->addWidget(title);

    m_table->setColumnCount(7);
    QStringList headers;
    headers << "#" << "Query" << "Strand" << "MAPQ" << "Nodes" << "Path" << "Query range";
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    static_cast<GafPathsTable *>(m_table)->setPathColumn(5);
    layout->addWidget(m_table);

    m_mapqFilterSpinBox->setRange(0, 1000);
    m_mapqFilterSpinBox->setValue(0);
    m_mapqFilterSpinBox->setPrefix("MAPQ ≥ ");
    m_mapqFilterSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_mapqFilterSpinBox->setFixedWidth(120);

    m_nodeFilterLineEdit->setPlaceholderText("Node name");
    m_nodeFilterLineEdit->setFixedWidth(140);

    QHBoxLayout * buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_highlightButton);
    buttonLayout->addWidget(m_highlightAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(new QLabel("Path includes:", this));
    buttonLayout->addWidget(m_nodeFilterLineEdit);
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
    m_nodeFilter.clear();

    populateTable();
    showWarnings();
    updateButtons();

    connect(m_table, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
    connect(m_highlightButton, SIGNAL(clicked()), this, SLOT(highlightSelectedPaths()));
    connect(m_highlightAllButton, SIGNAL(clicked()), this, SLOT(highlightAllPaths()));
    connect(m_filterButton, SIGNAL(clicked()), this, SLOT(filterByMapq()));
    connect(m_resetFilterButton, SIGNAL(clicked()), this, SLOT(resetMapqFilter()));
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
    m_table->clearContents();
    m_table->setRowCount(m_visibleRows.size());

    for (int row = 0; row < m_visibleRows.size(); ++row)
    {
        int index = alignmentIndexForRow(row);
        if (index < 0 || index >= m_alignments.size())
            continue;

        const GafAlignment &a = m_alignments[index];

        QTableWidgetItem * indexItem = new QTableWidgetItem(QString::number(a.lineNumber));
        QTableWidgetItem * queryItem = new QTableWidgetItem(a.queryName);
        QTableWidgetItem * strandItem = new QTableWidgetItem(a.strand);
        QString mapqString = (a.mappingQuality >= 0) ? QString::number(a.mappingQuality) : "";
        QTableWidgetItem * mapqItem = new QTableWidgetItem(mapqString);
        QTableWidgetItem * nodeCountItem = new QTableWidgetItem(QString::number(a.path.getNodeCount()));
        QTableWidgetItem * pathItem = new QTableWidgetItem(a.bandagePathString);

        QString queryRange = "";
        if (a.queryStart >= 0 && a.queryEnd >= 0 && a.queryLength > 0)
            queryRange = QString::number(a.queryStart) + "-" + QString::number(a.queryEnd) + " / " + QString::number(a.queryLength);
        else if (a.queryStart >= 0 && a.queryEnd >= 0)
            queryRange = QString::number(a.queryStart) + "-" + QString::number(a.queryEnd);
        QTableWidgetItem * rangeItem = new QTableWidgetItem(queryRange);

        indexItem->setData(Qt::UserRole, index);

        m_table->setItem(row, 0, indexItem);
        m_table->setItem(row, 1, queryItem);
        m_table->setItem(row, 2, strandItem);
        m_table->setItem(row, 3, mapqItem);
        m_table->setItem(row, 4, nodeCountItem);
        m_table->setItem(row, 5, pathItem);
        m_table->setItem(row, 6, rangeItem);
    }

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
    m_highlightButton->setEnabled(!m_table->selectedItems().isEmpty());
    m_highlightAllButton->setEnabled(!m_visibleRows.isEmpty());
    m_filterButton->setEnabled(true);
    m_resetFilterButton->setEnabled(m_visibleRows.size() != m_alignments.size() ||
                                    m_currentMapqThreshold != 0 ||
                                    !m_nodeFilter.isEmpty());
}


void GafPathsDialog::onSelectionChanged()
{
    updateButtons();
}


void GafPathsDialog::highlightSelectedPaths()
{
    QList<QTableWidgetSelectionRange> selection = m_table->selectedRanges();
    if (selection.isEmpty())
    {
        QMessageBox::information(this, "No paths selected", "Select at least one path first.");
        return;
    }

    QList<int> selectedRows;
    for (int i = 0; i < selection.size(); ++i)
    {
        QTableWidgetSelectionRange range = selection[i];
        for (int row = range.topRow(); row <= range.bottomRow(); ++row)
        {
            if (!selectedRows.contains(row))
                selectedRows.push_back(row);
        }
    }

    highlightPathsForRows(selectedRows);
}


void GafPathsDialog::highlightAllPaths()
{
    if (m_visibleRows.isEmpty())
    {
        QMessageBox::information(this, "No paths to highlight", "No paths are visible with the current filters.");
        return;
    }

    QList<int> allRows;
    for (int i = 0; i < m_visibleRows.size(); ++i)
        allRows << i;

    highlightPathsForRows(allRows);
}


void GafPathsDialog::highlightPathsForRows(const QList<int> &rows)
{
    g_memory->gafPathDialogIsVisible = true;
    g_memory->queryPaths.clear();

    g_graphicsView->scene()->blockSignals(true);
    g_graphicsView->scene()->clearSelection();

    QStringList nodesNotFound;

    for (int i = 0; i < rows.size(); ++i)
    {
        int row = rows[i];
        int alignmentIndex = alignmentIndexForRow(row);
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
    m_nodeFilter = m_nodeFilterLineEdit->text().trimmed();
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

        if (m_nodeFilter.isEmpty())
        {
            m_visibleRows << i;
            continue;
        }

        const QList<DeBruijnNode *> nodes = m_alignments[i].path.getNodes();
        bool matched = false;
        bool filterHasSign = m_nodeFilter.endsWith('+') || m_nodeFilter.endsWith('-');
        for (int n = 0; n < nodes.size(); ++n)
        {
            const QString nodeName = nodes[n]->getName();
            if (filterHasSign)
            {
                if (nodeName == m_nodeFilter)
                {
                    matched = true;
                    break;
                }
            }
            else if (nodes[n]->getNameWithoutSign() == m_nodeFilter)
            {
                matched = true;
                break;
            }
        }

        if (matched)
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
    m_nodeFilter.clear();
    m_nodeFilterLineEdit->setText("");
    populateTable();
    showWarnings();
    updateButtons();
}


int GafPathsDialog::alignmentIndexForRow(int row) const
{
    if (row < 0 || row >= m_visibleRows.size())
        return -1;
    return m_visibleRows[row];
}
