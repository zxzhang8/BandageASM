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


#ifndef NODESEQUENCEWIDGET_H
#define NODESEQUENCEWIDGET_H

#include <QWidget>

class DeBruijnNode;
class QLabel;
class QPlainTextEdit;

class NodeSequenceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NodeSequenceWidget(QWidget * parent, DeBruijnNode * node);

private:
    DeBruijnNode * m_node;
    QLabel * m_infoLabel;
    QPlainTextEdit * m_sequenceEdit;
};

#endif // NODESEQUENCEWIDGET_H
