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

#include "selectednodespathswidget.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <cmath>
#include "mygraphicsview.h"
#include "../graph/assemblygraph.h"
#include "../graph/debruijnnode.h"
#include "../graph/graphicsitemnode.h"
#include "../program/globals.h"
#include "../program/memory.h"

namespace
{
class SelectedPathsTableWidget : public QTableWidget
{
public:
    explicit SelectedPathsTableWidget(QWidget * parent) : QTableWidget(parent) {}

protected:
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override
    {
        // Row selection should keep the row visible vertically without chasing the
        // current cell horizontally (which can otherwise jump to the last column).
        const int horizontalPosition = horizontalScrollBar()->value();
        QTableWidget::scrollTo(index, hint);
        horizontalScrollBar()->setValue(horizontalPosition);
    }
};
}

SelectedNodesPathsWidget::SelectedNodesPathsWidget(
        QWidget * parent,
        const QList<Path> &paths,
        const QString &algorithm,
        const QString &status,
        const QList<TanglePathCandidate> &candidateDetails) :
    QWidget(parent),
    m_paths(paths),
    m_algorithm(algorithm),
    m_status(status),
    m_candidateDetails(candidateDetails),
    m_infoLabel(new QLabel(this)),
    m_table(new SelectedPathsTableWidget(this)),
    m_highlightButton(new QPushButton("Highlight selected paths", this)),
    m_highlightAllButton(new QPushButton("Highlight all paths", this)),
    m_exportFastaButton(new QPushButton("Export FASTA", this))
{
    QVBoxLayout * layout = new QVBoxLayout(this);

    QLabel * title = new QLabel("Paths within selected nodes", this);
    title->setWordWrap(true);
    layout->addWidget(title);

    m_infoLabel->setWordWrap(true);
    layout->addWidget(m_infoLabel);

    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(QStringList() << "Nodes" << "Length\n(bp)"
                                       << "Read support\n(avg)" << "Path"
                                       << "Score / objective" << "Read evidence");
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_table);

    QHBoxLayout * buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_highlightButton);
    buttonLayout->addWidget(m_highlightAllButton);
    buttonLayout->addWidget(m_exportFastaButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    populateTable();
    updateButtons();

    connect(m_table, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
    connect(m_highlightButton, SIGNAL(clicked()), this, SLOT(highlightSelectedPaths()));
    connect(m_highlightAllButton, SIGNAL(clicked()), this, SLOT(highlightAllPaths()));
    connect(m_exportFastaButton, SIGNAL(clicked()), this, SLOT(exportSelectedPathSequence()));
}


SelectedNodesPathsWidget::~SelectedNodesPathsWidget()
{
    g_memory->selectedPathsDialogIsVisible = false;

    if (g_memory->clearQueryPaths(Memory::SELECTED_NODE_PATH_HIGHLIGHT))
        emit selectionChanged();
}


void SelectedNodesPathsWidget::showEvent(QShowEvent * event)
{
    g_memory->selectedPathsDialogIsVisible = true;
    QWidget::showEvent(event);
}


void SelectedNodesPathsWidget::hideEvent(QHideEvent * event)
{
    g_memory->selectedPathsDialogIsVisible = false;
    QWidget::hideEvent(event);
}


void SelectedNodesPathsWidget::populateTable()
{
    m_table->clearContents();
    m_table->setRowCount(m_paths.size());

    for (int row = 0; row < m_paths.size(); ++row)
    {
        const Path &path = m_paths[row];
        QString pathString = path.getString(true);
        QString length = formatIntForDisplay(path.getLength());
        QString nodeCount = QString::number(path.getNodeCount());
        QString readSupportAverageText = "N/A";
        QList<DeBruijnNode *> nodes = path.getNodes();
        long long readSupportSum = 0;
        int readSupportCount = 0;
        for (int nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
        {
            DeBruijnNode * node = nodes[nodeIndex];
            if (node != 0 && node->hasReadSupportCount())
            {
                readSupportSum += node->getReadSupportCount();
                ++readSupportCount;
            }
        }
        if (readSupportCount > 0)
        {
            double readSupportAverage = double(readSupportSum) / double(readSupportCount);
            readSupportAverageText = formatDoubleForDisplay(readSupportAverage, 2);
        }

        QTableWidgetItem * nodesItem = new QTableWidgetItem(nodeCount);
        QTableWidgetItem * lengthItem = new QTableWidgetItem(length);
        QTableWidgetItem * readSupportItem = new QTableWidgetItem(readSupportAverageText);
        QTableWidgetItem * pathItem = new QTableWidgetItem(pathString);
        QString scoreText = "N/A";
        QString evidenceText = "N/A";
        if (row < m_candidateDetails.size())
        {
            if (std::isfinite(m_candidateDetails[row].score))
                scoreText = formatDoubleForDisplay(m_candidateDetails[row].score, 6);
            if (std::isfinite(m_candidateDetails[row].weightedReadSupport))
                evidenceText = formatDoubleForDisplay(m_candidateDetails[row].weightedReadSupport, 4);
        }

        m_table->setItem(row, 0, nodesItem);
        m_table->setItem(row, 1, lengthItem);
        m_table->setItem(row, 2, readSupportItem);
        m_table->setItem(row, 3, pathItem);
        m_table->setItem(row, 4, new QTableWidgetItem(scoreText));
        m_table->setItem(row, 5, new QTableWidgetItem(evidenceText));
    }

    m_table->resizeColumnsToContents();

    QString info = "Found " + formatIntForDisplay(m_paths.size()) + " path(s).";
    if (!m_algorithm.isEmpty())
        info += " Algorithm: " + m_algorithm + ".";
    if (!m_status.isEmpty())
        info += " Status: " + m_status + ".";
    m_infoLabel->setText(info);
}


void SelectedNodesPathsWidget::updateButtons()
{
    m_highlightButton->setEnabled(!m_table->selectedItems().isEmpty());
    m_highlightAllButton->setEnabled(!m_paths.isEmpty());
    m_exportFastaButton->setEnabled(m_table->selectionModel() != 0 &&
                                   m_table->selectionModel()->selectedRows().size() == 1);
}


void SelectedNodesPathsWidget::onSelectionChanged()
{
    updateButtons();
}


void SelectedNodesPathsWidget::highlightSelectedPaths()
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


void SelectedNodesPathsWidget::highlightAllPaths()
{
    if (m_paths.isEmpty())
    {
        QMessageBox::information(this, "No paths to highlight", "No paths are available.");
        return;
    }

    QList<int> allRows;
    for (int i = 0; i < m_paths.size(); ++i)
        allRows << i;

    highlightPathsForRows(allRows);
}


void SelectedNodesPathsWidget::exportSelectedPathSequence()
{
    if (m_table->selectionModel() == 0)
        return;

    QModelIndexList selectedRows = m_table->selectionModel()->selectedRows();
    if (selectedRows.size() != 1)
    {
        QMessageBox::information(this, "Select one path", "Select a single path to export its sequence.");
        return;
    }

    exportPathSequence(selectedRows[0].row());
}


void SelectedNodesPathsWidget::exportPathSequence(int row)
{
    if (row < 0 || row >= m_paths.size())
        return;

    QString defaultFileNameAndPath = g_memory->rememberedPath + "/selected_node_path.fa";
    QString fileName = QFileDialog::getSaveFileName(this, "Export FASTA", defaultFileNameAndPath,
                                                    "FASTA (*.fa *.fasta);;All files (*)");
    if (fileName == "")
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Export FASTA", "Could not open file for writing:\n" + fileName);
        return;
    }

    const Path &path = m_paths[row];
    QByteArray sequence = path.getPathSequence();
    QTextStream out(&file);
    out << ">selected_node_path\n";
    out << AssemblyGraph::addNewlinesToSequence(sequence);

    QFileInfo fastaInfo(fileName);
    QString mdFileName = fastaInfo.absolutePath() + "/" + fastaInfo.completeBaseName() + ".md";
    QFile mdFile(mdFileName);
    if (mdFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream mdOut(&mdFile);
        mdOut << "Selected path info\n";
        mdOut << "Nodes: " << m_table->item(row, 0)->text() << "\n";
        mdOut << "Length (bp): " << m_table->item(row, 1)->text() << "\n";
        mdOut << "Read support (avg): " << m_table->item(row, 2)->text() << "\n";
        mdOut << "Path: " << m_table->item(row, 3)->text() << "\n";
        mdOut << "Algorithm: " << (m_algorithm.isEmpty() ? "Standard" : m_algorithm) << "\n";
        mdOut << "Score / objective: " << m_table->item(row, 4)->text() << "\n";
        mdOut << "Read evidence: " << m_table->item(row, 5)->text() << "\n";
    }
    else
    {
        QMessageBox::warning(this, "Export FASTA", "Could not open file for writing:\n" + mdFileName);
    }

    g_memory->rememberedPath = QFileInfo(fileName).absolutePath();
}


void SelectedNodesPathsWidget::highlightPathsForRows(const QList<int> &rows)
{
    g_memory->selectedPathsDialogIsVisible = true;

    // Selected-node paths use the graph's normal selection styling.  Clear
    // any path overlay so it cannot remain after the graph selection changes.
    g_memory->clearAllQueryPaths();

    g_graphicsView->scene()->blockSignals(true);
    g_graphicsView->scene()->clearSelection();

    QStringList nodesNotFound;

    for (int i = 0; i < rows.size(); ++i)
    {
        int row = rows[i];
        if (row < 0 || row >= m_paths.size())
            continue;

        const Path &path = m_paths[row];

        QList<DeBruijnNode *> nodes = path.getNodes();
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
