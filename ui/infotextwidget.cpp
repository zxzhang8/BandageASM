//Copyright 2017 Ryan Wick

//This file is part of Grovolve.

//Grovolve is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//Grovolve is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with Grovolve.  If not, see <http://www.gnu.org/licenses/>.


#include "infotextwidget.h"
#include <QPainter>
#include <QToolTip>
#include <QMouseEvent>


InfoTextWidget::InfoTextWidget(QWidget * parent) :
    QWidget(parent)
{
    setFixedSize(16, 16);
    setMouseTracking(true);
}

InfoTextWidget::InfoTextWidget(QWidget * parent, QString infoText) :
    QWidget(parent)
{
    setFixedSize(16, 16);
    setMouseTracking(true);
    setInfoText(infoText);
}

void InfoTextWidget::setInfoText(QString infoText)
{
    //Convert text to a rich text format, which will let QToolTip wrap the text.
    m_infoText = "<html>" + infoText + "</html>";
}


void InfoTextWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF circleRect(1.0, 1.0, width() - 2.0, height() - 2.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#1976d2"));
    painter.drawEllipse(circleRect);

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(qRound(height() * 0.9));
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignCenter, "!");
}


void InfoTextWidget::mousePressEvent(QMouseEvent * event)
{
    QToolTip::showText(event->globalPos(), m_infoText);
}
