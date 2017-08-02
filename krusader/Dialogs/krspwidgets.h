/***************************************************************************
                                krspwidgets.h
                             -------------------
    copyright            : (C) 2000 by Shie Erlich & Rafi Yanai
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
#ifndef KRSPWIDGETS_H
#define KRSPWIDGETS_H

// QtCore
#include <QEvent>
// QtGui
#include <QMouseEvent>

#include <KCompletion/KLineEdit>
#include <KNotifications/KPassivePopup>

#include "krmaskchoice.h"
#include "newftpgui.h"
#include "../FileSystem/krquery.h"

class KRMaskChoiceSub;

class KRSpWidgets
{
    friend class KRMaskChoiceSub;

public:
    KRSpWidgets();

    static KRQuery getMask(QString caption, bool nameOnly = false, QWidget * parent = 0); // get file-mask for (un)selecting files
    static QUrl newFTP();

private:
    static QStringList maskList;  // used by KRMaskChoiceSub
};

/////////////////////////// newFTPSub ///////////////////////////////////////
class newFTPSub : public newFTPGUI
{
public:
    newFTPSub();

protected:
    void reject();
    void accept();
};

/////////////////////////// KRMaskChoiceSub /////////////////////////////////
// Inherits KRMaskChoice's generated code to fully implement the functions //
/////////////////////////////////////////////////////////////////////////////
class KRMaskChoiceSub : public KRMaskChoice
{
public:
    explicit KRMaskChoiceSub(QWidget * parent = 0);

public slots:
    void addSelection();
    void deleteSelection();
    void clearSelections();
    void acceptFromList(QListWidgetItem *i);
    void currentItemChanged(QListWidgetItem *i);

protected:
    void reject();
    void accept();
};

#endif
