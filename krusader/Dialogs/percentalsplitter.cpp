/***************************************************************************
                       percentalsplitter.h  -  description
                             -------------------
    copyright            : (C) 2006 + by Csaba Karai
    e-mail               : krusader@users.sourceforge.net
    web site             : http://krusader.sourceforge.net
 ---------------------------------------------------------------------------
  Description
 ***************************************************************************

  A

     db   dD d8888b. db    db .d8888.  .d8b.  d8888b. d88888b d8888b.
     88 ,8P' 88  `8D 88    88 88'  YP d8' `8b 88  `8D 88'     88  `8D
     88,8P   88oobY' 88    88 `8bo.   88ooo88 88   88 88ooooo 88oobY'
     88`8b   88`8b   88    88   `Y8b. 88~~~88 88   88 88~~~~~ 88`8b
     88 `88. 88 `88. 88b  d88 db   8D 88   88 88  .8D 88.     88 `88.
     YP   YD 88   YD ~Y8888P' `8888Y' YP   YP Y8888D' Y88888P 88   YD

                                                     H e a d e r    F i l e

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "percentalsplitter.h"

// QtCore
#include <QList>
// QtGui
#include <QPainter>
#include <QCursor>
// QtWidgets
#include <QApplication>
#include <QLabel>
#include <QFrame>
#include <QToolTip>

PercentalSplitter::PercentalSplitter(QWidget * parent) : QSplitter(parent), label(0), opaqueOldPos(-1)
{
    connect(this, SIGNAL(splitterMoved(int,int)), SLOT(slotSplitterMoved(int,int)));
}

PercentalSplitter::~PercentalSplitter()
{
}

QString PercentalSplitter::toolTipString(int p)
{
    QList<int> values = sizes();
    if (values.count() != 0) {
        int sum = 0;

        QListIterator<int> it(values);
        while (it.hasNext())
            sum += it.next();

        if (sum == 0)
            sum++;

        int percent = (int)(((double)p / (double)(sum)) * 10000. + 0.5);
        return QString("%1.%2%3").arg(percent / 100).arg((percent / 10) % 10).arg(percent % 10) + '%';
    }
    return QString();
}

void PercentalSplitter::slotSplitterMoved(int p, int index)
{
    handle(index) -> setToolTip(toolTipString(p));

    QToolTip::showText(QCursor::pos(), toolTipString(p) , this);
}

void PercentalSplitter::showEvent(QShowEvent * event)
{
    QList<int> values = sizes();

    for (int i = 0; i != count(); i++) {
        int p = 0;
        for (int j = 0; j < i; j++)
            p += values[ j ];

        handle(i) -> setToolTip(toolTipString(p));
    }

    QSplitter::showEvent(event);
}

