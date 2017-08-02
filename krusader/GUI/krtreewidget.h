/***************************************************************************
                                krtreewidget.h
                             -------------------
    copyright            : (C) 2008+ by Csaba Karai
    email                : krusader@users.sourceforge.net
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

#ifndef KRTREEWIDGET_H
#define KRTREEWIDGET_H

// QtWidgets
#include <QAbstractScrollArea>
#include <QTreeWidget>

class KrTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit KrTreeWidget(QWidget * parent);
    void setStretchingColumn(int col)                {
        _stretchingColumn = col;
    }

    QModelIndex indexOf(QTreeWidgetItem * item, int col = 0) {
        return indexFromItem(item, col);
    }
    QTreeWidgetItem * item(const QModelIndex & ndx)          {
        return itemFromIndex(ndx);
    }

signals:
    void itemRightClicked(QTreeWidgetItem * it, const QPoint & pos, int column);

protected:
    virtual bool event(QEvent * event) Q_DECL_OVERRIDE;

private:
    int  _stretchingColumn;
    bool _inResize;
};

#endif /* KRTREEWIDGET_H */
