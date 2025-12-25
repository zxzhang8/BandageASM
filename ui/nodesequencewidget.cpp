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

#include "nodesequencewidget.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include "../graph/assemblygraph.h"
#include "../graph/debruijnnode.h"

NodeSequenceWidget::NodeSequenceWidget(QWidget * parent, DeBruijnNode * node) :
    QWidget(parent),
    m_node(node),
    m_infoLabel(new QLabel(this)),
    m_sequenceEdit(new QPlainTextEdit(this))
{
    QVBoxLayout * layout = new QVBoxLayout(this);

    QString nodeName = node ? node->getName() : "Unknown";
    QLabel * title = new QLabel("Node sequence", this);
    title->setWordWrap(true);
    layout->addWidget(title);

    QLabel * nodeLabel = new QLabel("Node: " + nodeName, this);
    nodeLabel->setWordWrap(true);
    layout->addWidget(nodeLabel);

    int length = node ? node->getLength() : 0;
    QLabel * lengthLabel = new QLabel("Length: " + QString::number(length) + " bp", this);
    layout->addWidget(lengthLabel);

    QString infoText = "";
    QByteArray sequence;
    if (node)
    {
        sequence = node->getSequence();
        if (node->sequenceIsMissing())
            infoText = "Sequence is missing in the input; showing Ns to match length.";
    }
    m_infoLabel->setText(infoText);
    m_infoLabel->setWordWrap(true);
    layout->addWidget(m_infoLabel);

    m_sequenceEdit->setReadOnly(true);
    m_sequenceEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    if (!sequence.isEmpty())
    {
        QByteArray wrapped = AssemblyGraph::addNewlinesToSequence(sequence);
        m_sequenceEdit->setPlainText(QString::fromLatin1(wrapped));
    }
    layout->addWidget(m_sequenceEdit);
}
