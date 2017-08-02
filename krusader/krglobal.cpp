/***************************************************************************
                          krglobal.cpp
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

                                               S o u r c e    F i l e

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "krglobal.h"

#include "krusader.h"
#include "krusaderview.h"
#include "panelmanager.h"

// QtCore
#include <QtGlobal>

#include <KXmlGui/KActionCollection>

KConfig *KrGlobal::config = 0;
KMountMan *KrGlobal::mountMan = 0;
KrBookmarkHandler *KrGlobal::bookman = 0;
KRslots* KrGlobal::slot = 0;
KIconLoader *KrGlobal::iconLoader = 0;
KrusaderView *KrGlobal::mainView = 0;
QWidget *KrGlobal::mainWindow = 0;
UserAction *KrGlobal::userAction = 0;
JobMan *KrGlobal::jobMan = 0;
// ListPanel *KrGlobal::activePanel = 0;
QKeySequence KrGlobal::copyShortcut;

const int KrGlobal::sConfigVersion;
int KrGlobal::sCurrentConfigVersion;

KrPanel *KrGlobal::activePanel()
{
    // active manager might not be set yet
    if(mainView->activeManager())
        return mainView->activeManager()->currentPanel();
    else
        return 0;
}

// void KrGlobal::enableAction(const char *name, bool enable)
// {
//     getAction(name)->setEnabled(enable);
// }
// 
// QAction* KrGlobal::getAction(const char *name)
// {
//     QAction *act = krApp->actionCollection()->action(name);
//     if(!act)
//         qFatal("no such action: %s", name);
//     return act;
// }
