/***************************************************************************
                                krglobal.h
                           -------------------
    begin                : Thu May 4 2000
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

#ifndef KRGLOBAL_H
#define KRGLOBAL_H

// QtGui
#include <QKeySequence>

#include <KConfigCore/KConfigGroup>

class KConfig;
class KMountMan;
class KrBookmarkHandler;
class KRslots;
class KIconLoader;
class KrusaderView;
class UserAction;
class JobMan;
class QWidget;
class KrPanel;

// global references to frequently used objects

class KrGlobal
{
public:
    static KConfig *config;    // allow everyone to access the config
    static KMountMan *mountMan;  // krusader's Mount Manager
    static KrBookmarkHandler *bookman;
    static KRslots *slot;
    static KIconLoader *iconLoader; // the app's icon loader
    static KrusaderView *mainView;  // The GUI
    static QWidget *mainWindow;
    static UserAction *userAction;
    static JobMan *jobMan;
//     static ListPanel  *activePanel;
    static KrPanel *activePanel();

    //HACK - used by [ListerTextArea|KrSearchDialog|LocateDlg]:keyPressEvent()
    static QKeySequence copyShortcut;

//     static void enableAction(const char *name, bool enable);
//     static QAction *getAction(const char *name);

    /** Version of saved configuration. Use this to detect configuration updates. */
    static const int sConfigVersion = 1;
    static int sCurrentConfigVersion;
};

#define krConfig     KrGlobal::config
#define krMtMan      (*(KrGlobal::mountMan))
#define krBookMan    KrGlobal::bookman
#define SLOTS        KrGlobal::slot
#define krLoader     KrGlobal::iconLoader
#define MAIN_VIEW    KrGlobal::mainView
#define krMainWindow KrGlobal::mainWindow
#define krUserAction KrGlobal::userAction
#define krJobMan     KrGlobal::jobMan

#define ACTIVE_PANEL (KrGlobal::activePanel())

#define ACTIVE_MNG   (MAIN_VIEW->activeManager())
#define ACTIVE_FUNC  (ACTIVE_PANEL->func)
#define OTHER_MNG  (MAIN_VIEW->inactiveManager())
#define OTHER_PANEL (ACTIVE_PANEL->otherPanel())
#define OTHER_FUNC (OTHER_PANEL->func)
#define LEFT_PANEL (MAIN_VIEW->leftPanel())
#define LEFT_FUNC  (LEFT_PANEL->func)
#define LEFT_MNG  (MAIN_VIEW->leftManager())
#define RIGHT_PANEL  (MAIN_VIEW->rightPanel())
#define RIGHT_FUNC (RIGHT_PANEL->func)
#define RIGHT_MNG  (MAIN_VIEW->rightManager())

// #define krEnableAction(name, enable) (KrGlobal::enableAction(#name, (enable)))
// #define krGetAction(name) (KrGlobal::getAction(#name))

#endif
