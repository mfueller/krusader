/***************************************************************************
                             kmountman.h
                          -------------------
 begin                : Thu May 4 2000
 copyright            : (C) 2000 by Shie Erlich & Rafi Yanai
 e-mail               : krusader@users.sourceforge.net
 web site             : http://krusader.sourceforge.net
---------------------------------------------------------------------------
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
#ifndef KMOUNTMAN_H
#define KMOUNTMAN_H

// QtCore
#include <QExplicitlySharedDataPointer>
#include <QObject>
#include <QString>
#include <QPointer>
#include <QUrl>
// QtWidgets
#include <QWidget>
#include <QAction>

#include <KIO/Job>
#include <KIO/Global>
#include <KIOCore/KMountPoint>

#include <Solid/Device>
#include <Solid/SolidNamespace>

class KMountManGUI;
class KToolBarPopupAction;

class KMountMan : public QObject
{
    Q_OBJECT
    friend class KMountManGUI;

public:
    enum mntStatus {DOESNT_EXIST, NOT_MOUNTED, MOUNTED};

    inline bool operational() {
        return Operational;
    } // check this 1st

    void mount(QString mntPoint, bool blocking = true); // this is probably what you need for mount
    void unmount(QString mntPoint, bool blocking = true); // this is probably what you need for unmount
    mntStatus getStatus(QString mntPoint);    // return the status of a mntPoint (if any)
    void eject(QString mntPoint);
    bool ejectable(QString path);
    bool removable(QString path);
    bool removable(Solid::Device d);
    QString convertSize(KIO::filesize_t size);
    bool invalidFilesystem(QString type);
    bool networkFilesystem(QString type);
    bool nonmountFilesystem(QString type, QString mntPoint);
    QAction *action() {
        return (QAction *) _action;
    }

    explicit KMountMan(QWidget *parent);
    ~KMountMan();

    // NOTE: this function needs some time (~50msec)
    QString findUdiForPath(QString path, const Solid::DeviceInterface::Type &expType = Solid::DeviceInterface::Unknown);
    QString pathForUdi(QString udi);

public slots:
    void mainWindow();                        // opens up the GUI
    void autoMount(QString path);             // just call it before refreshing into a dir
    void delayedPerformAction(QAction *);
    void performAction();
    void quickList();

protected slots:
    void jobResult(KJob *job);
    void slotTeardownDone(Solid::ErrorType error, QVariant errorData, const QString &udi);
    void slotSetupDone(Solid::ErrorType error, QVariant errorData, const QString &udi);

protected:
    // used internally
    static QExplicitlySharedDataPointer<KMountPoint> findInListByMntPoint(KMountPoint::List &lst, QString value);
    void toggleMount(QString mntPoint);
    void emitRefreshPanel(const QUrl &url) {
        emit refreshPanel(url);
    }

signals:
    void refreshPanel(const QUrl &);

private:
    QString *_actions;
    KToolBarPopupAction *_action;

    bool Operational;   // if false, something went terribly wrong on startup
    bool waiting; // used to block krusader while waiting for (un)mount operation
    KMountManGUI *mountManGui;
    // the following is the FS type
    QStringList invalid_fs;
    QStringList nonmount_fs;
    QStringList network_fs;
    // the following is the FS name
    QStringList nonmount_fs_mntpoint;
    QPointer<QWidget> parentWindow;
};

#endif
