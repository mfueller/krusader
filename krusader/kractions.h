/***************************************************************************
                                kractions.h
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

#ifndef KRACTIONS_H
#define KRACTIONS_H

// QtCore
#include <QObject>
// QtWidgets
#include <QAction>

#include <KWidgetsAddons/KToggleAction>

class Krusader;

class KrActions : public QObject
{
    Q_OBJECT
public:
    explicit KrActions(QObject *parent) : QObject(parent) {}

    // Actions
    static QAction *actCompare;
    static QAction *actCmdlinePopup, *actLocate, *actSwitchFullScreenTE;
    static QAction *actDiskUsage, *actSavePosition;
    static QAction *actHomeTerminal, *actProfiles;
    static QAction *actMountMan, *actNewTool, *actSwapPanels, *actSwapSides;
    static QAction *actKonfigurator, *actToolsSetup, *actFind;
    static QAction *actRemoteEncoding;
    static QAction *actSelectColorMask, *actMultiRename, *actOpenLeftBm, *actOpenRightBm, *actAddBookmark;
    static QAction *actSplit;
    static QAction *actCombine;
    static QAction *actUserMenu;
    static QAction *actManageUseractions;
#ifdef SYNCHRONIZER_ENABLED
    static QAction *actSyncDirs;
#endif
    static QAction *actVerticalMode;
    static QAction *actEmptyTrash, *actTrashBin;
    static QAction *actPopularUrls;
    static KToggleAction *actToggleTerminal;
    static QAction *actSelectNewerAndSingle, *actSelectNewer, *actSelectSingle,
    *actSelectDifferentAndSingle, *actSelectDifferent;
    static QAction *actF10Quit;
    /** actions for setting the execution mode of commands from commanddline */
    static QAction *actExecStartAndForget,
    *actExecCollectSeparate, *actExecCollectTogether,
    *actExecTerminalExternal, *actExecTerminalEmbedded;
    static KToggleAction *actToggleFnkeys, *actToggleCmdline,
    *actShowStatusBar, *actToggleHidden, *actCompareDirs;

    static QAction **compareArray[];
    /** actions for setting the execution mode of commands from commanddline */
    static QAction **execTypeArray[];

    /** JobMan toolbar actions */
    static QAction *actJobProgress;
    static QAction *actJobControl;
    static QAction *actJobMode;
    static QAction *actJobUndo;


#ifdef __KJSEMBED__
    static QAction *actShowJSConsole;
#endif

    static void setupActions(Krusader *krusader);
};

// krusader's actions - things krusader can do!
#define krHomeTerm        KrActions::actHomeTerminal   // open terminal@home dir
#define krRemoteEncoding  KrActions::actRemoteEncoding // remote encoding menu
#define krMountMan        KrActions::actMountMan       // run Mount-manager
#define krNewTool         KrActions::actNewTool        // Add a new tool to menu
#define krKonfigurator    KrActions::actKonfigurator
#define krToolsSetup      KrActions::actToolsSetup     // setup the tools menu
#define krRoot            KrActions::actRoot
#define krFind            KrActions::actFind           // find files
#define krMultiRename     KrActions::actMultiRename
//#define krToggleSortByExt KrActions::actToggleSortByExt// Sort by extension
#define krSwitchFullScreenTE KrActions::actSwitchFullScreenTE
#define krCmdlinePopup    KrActions::actCmdlinePopup
#define krSplit           KrActions::actSplit
#define krCombine         KrActions::actCombine
#define krUserMenu        KrActions::actUserMenu
#define krPopularUrls     KrActions::actPopularUrls

#ifdef __KJSEMBED__
#define krJSConsole  KrActions::actShowJSConsole
#endif

#endif
