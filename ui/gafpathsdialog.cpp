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
#include "../graph/debruijnnode.h"
#include "../graph/graphicsitemnode.h"
#include "../program/globals.h"
#include "../program/memory.h"
#include "../program/settings.h"
#include <QShowEvent>
#include "mygraphicsscene.h"
#include "mygraphicsview.h"

GafPathsDialog::GafPathsDialog(QWidget * parent,
                               const QString &fileName,
                               const GafParseResult &parseResult) :
    QWidget(parent),
    m_fileName(fileName),
    m_alignments(parseResult.alignments),
    m_warnings(parseResult.warnings),
    m_table(new QTableWidget(this)),
    m_highlightButton(new QPushButton("Highlight selected paths", this)),
    m_warningLabel(new QLabel(this))
{
    setWindowTitle("GAF Paths");

    QVBoxLayout * layout = new QVBoxLayout(this);

    QLabel * title = new QLabel("File: " + fileName, this);
    title->setWordWrap(true);
    layout->addWidget(title);

    m_table->setColumnCount(7);
    QStringList headers;
    headers << "#" << "Query" << "Strand" << "Nodes" << "Path" << "Query range" << "MAPQ";
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table);

    QHBoxLayout * buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_highlightButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    m_warningLabel->setWordWrap(true);
    layout->addWidget(m_warningLabel);

    populateTable();
    showWarnings();
    updateButtons();

    connect(m_table, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
    connect(m_highlightButton, SIGNAL(clicked()), this, SLOT(highlightSelectedPaths()));
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
    m_table->setRowCount(m_alignments.size());

    for (int i = 0; i < m_alignments.size(); ++i)
    {
        const GafAlignment &a = m_alignments[i];

        QTableWidgetItem * indexItem = new QTableWidgetItem(QString::number(a.lineNumber));
        QTableWidgetItem * queryItem = new QTableWidgetItem(a.queryName);
        QTableWidgetItem * strandItem = new QTableWidgetItem(a.strand);
        QTableWidgetItem * nodeCountItem = new QTableWidgetItem(QString::number(a.path.getNodeCount()));
        QTableWidgetItem * pathItem = new QTableWidgetItem(a.bandagePathString);

        QString queryRange = "";
        if (a.queryStart >= 0 && a.queryEnd >= 0 && a.queryLength > 0)
            queryRange = QString::number(a.queryStart) + "-" + QString::number(a.queryEnd) + " / " + QString::number(a.queryLength);
        else if (a.queryStart >= 0 && a.queryEnd >= 0)
            queryRange = QString::number(a.queryStart) + "-" + QString::number(a.queryEnd);
        QTableWidgetItem * rangeItem = new QTableWidgetItem(queryRange);

        QString mapqString = (a.mappingQuality >= 0) ? QString::number(a.mappingQuality) : "";
        QTableWidgetItem * mapqItem = new QTableWidgetItem(mapqString);

        indexItem->setData(Qt::UserRole, i);

        m_table->setItem(i, 0, indexItem);
        m_table->setItem(i, 1, queryItem);
        m_table->setItem(i, 2, strandItem);
        m_table->setItem(i, 3, nodeCountItem);
        m_table->setItem(i, 4, pathItem);
        m_table->setItem(i, 5, rangeItem);
        m_table->setItem(i, 6, mapqItem);
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

    g_memory->gafPathDialogIsVisible = true;
    g_memory->queryPaths.clear();

    g_graphicsView->scene()->blockSignals(true);
    g_graphicsView->scene()->clearSelection();

    QStringList nodesNotFound;

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

    for (int i = 0; i < selectedRows.size(); ++i)
    {
        int row = selectedRows[i];
        if (row < 0 || row >= m_alignments.size())
            continue;

        const GafAlignment &alignment = m_alignments[row];
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
