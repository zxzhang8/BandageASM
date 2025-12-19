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
    QLabel * m_warningLabel;

    void populateTable();
    void updateButtons();
    void showWarnings();

private slots:
    void onSelectionChanged();
    void highlightSelectedPaths();

signals:
    void selectionChanged();
    void highlightRequested();

protected:
    void hideEvent(QHideEvent * event) override;
    void showEvent(QShowEvent * event) override;
};

#endif // GAFPATHSDIALOG_H
