/***************************************************************************
                          diskusage.h  -  description
                             -------------------
    copyright            : (C) 2004 by Csaba Karai
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

#ifndef DISKUSAGE_H
#define DISKUSAGE_H

// QtCore
#include <QHash>
#include <QEvent>
#include <QTimer>
#include <QStack>
#include <QUrl>
// QtGui
#include <QResizeEvent>
#include <QKeyEvent>
#include <QPixmap>
// QtWidgets
#include <QDialog>
#include <QLabel>
#include <QStackedWidget>
#include <QScrollArea>

#include <KWidgetsAddons/KSqueezedTextLabel>

#include "filelightParts/fileTree.h"

#define VIEW_LINES      0
#define VIEW_DETAILED   1
#define VIEW_FILELIGHT  2
#define VIEW_LOADER     3

typedef QHash<QString, void *> Properties;

class DUListView;
class DULines;
class DUFilelight;
class QMenu;
class LoaderWidget;
class FileItem;
class FileSystem;

class DiskUsage : public QStackedWidget
{
    Q_OBJECT

public:
    explicit DiskUsage(QString confGroup, QWidget *parent = 0);
    ~DiskUsage();

    void       load(const QUrl &dirName);
    void       close();
    void       stopLoad();
    bool       isLoading()     {
        return loading;
    }

    void       setView(int view);
    int        getActiveView() {
        return activeView;
    }

    Directory* getDirectory(QString path);
    File *     getFile(QString path);

    QString    getConfigGroup() {
        return configGroup;
    }

    void *     getProperty(File *, QString);
    void       addProperty(File *, QString, void *);
    void       removeProperty(File *, QString);

    int        exclude(File *file, bool calcPercents = true, int depth = 0);
    void       includeAll();

    int        del(File *file, bool calcPercents = true, int depth = 0);

    QString    getToolTip(File *);

    void       rightClickMenu(const QPoint &, File *, QMenu * = 0, QString = QString());

    void       changeDirectory(Directory *dir);

    Directory* getCurrentDir();
    File*      getCurrentFile();

    QPixmap    getIcon(QString mime);

    QUrl       getBaseURL() {
        return baseURL;
    }

public slots:
    void       dirUp();
    void       clear();

signals:
    void       enteringDirectory(Directory *);
    void       clearing();
    void       changed(File *);
    void       changeFinished();
    void       deleted(File *);
    void       deleteFinished();
    void       status(QString);
    void       viewChanged(int);
    void       loadFinished(bool);
    void       newSearch();

protected slots:
    void       slotLoadDirectory();

protected:
    QHash< QString, Directory * > contentMap;
    QHash< File *, Properties *> propertyMap;

    Directory* currentDirectory;
    KIO::filesize_t currentSize;

    virtual void keyPressEvent(QKeyEvent *) Q_DECL_OVERRIDE;
    virtual bool event(QEvent *) Q_DECL_OVERRIDE;

    int        calculateSizes(Directory *dir = 0, bool emitSig = false, int depth = 0);
    int        calculatePercents(bool emitSig = false, Directory *dir = 0 , int depth = 0);
    int        include(Directory *dir, int depth = 0);
    void       createStatus();
    void       executeAction(int, File * = 0);

    QUrl       baseURL;             //< the base URL of loading

    DUListView                *listView;
    DULines                   *lineView;
    DUFilelight               *filelightView;
    LoaderWidget              *loaderView;

    Directory *root;

    int        activeView;

    QString    configGroup;

    bool       first;
    bool       loading;
    bool       abortLoading;
    bool       clearAfterAbort;
    bool       deleting;

    QStack<QString> directoryStack;
    QStack<Directory *> parentStack;

    FileSystem *searchFileSystem;
    FileItem *currentFileItem;
    QList<FileItem *> fileItems;
    Directory *currentParent;
    QString dirToCheck;

    int   fileNum;
    int   dirNum;
    int   viewBeforeLoad;

    QTimer loadingTimer;
};


class LoaderWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit LoaderWidget(QWidget *parent = 0);

    void init();
    void setCurrentURL(const QUrl &url);
    void setValues(int fileNum, int dirNum, KIO::filesize_t total);
    bool wasCancelled()  {
        return cancelled;
    }

public slots:
    void slotCancelled();

protected:
    QLabel *totalSize;
    QLabel *files;
    QLabel *directories;

    KSqueezedTextLabel *searchedDirectory;
    QWidget *widget;

    bool   cancelled;
};

#endif /* __DISK_USAGE_GUI_H__ */
