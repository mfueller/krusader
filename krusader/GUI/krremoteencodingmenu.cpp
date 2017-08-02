/*****************************************************************************
 * Copyright (C) 2005 Csaba Karai <cskarai@freemail.hu>                      *
 * based on KRemoteEncodingPlugin from Dawit Alemayehu <adawit@kde.org>      *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This package is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this package; if not, write to the Free Software               *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA *
 *****************************************************************************/

#include "krremoteencodingmenu.h"

// QtCore
#include <QDebug>
// QtWidgets
#include <QMenu>

#include <KXmlGui/KActionCollection>
#include <KCodecs/KCharsets>
#include <KConfigCore/KConfig>
#include <KConfigCore/KConfigGroup>
#include <KI18n/KLocalizedString>
#include <KIO/Scheduler>
#include <KIOCore/KProtocolManager>

#include "../krglobal.h"
#include "../Panel/krpanel.h"
#include "../Panel/panelfunc.h"
#include "../kicons.h"

#define DATA_KEY    QString::fromLatin1("Charset")

KrRemoteEncodingMenu::KrRemoteEncodingMenu(const QString &text, const QString &icon, KActionCollection *parent) :
        KActionMenu(QIcon::fromTheme(icon), text, parent), settingsLoaded(false)
{
    connect(menu(), SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()));

    parent->addAction("changeremoteencoding", this);
}

void KrRemoteEncodingMenu::slotAboutToShow()
{
    if (!settingsLoaded)
        loadSettings();

    // uncheck everything
    QList<QAction *> acts = menu()->actions();
    foreach(QAction *act, acts)
    act->setChecked(false);

    QString charset = currentCharacterSet();
    if (!charset.isEmpty()) {
        int id = 1;
        QStringList::Iterator it;
        for (it = encodingNames.begin(); it != encodingNames.end(); ++it, ++id)
            if ((*it).indexOf(charset) != -1)
                break;

        bool found = false;

        foreach(QAction *act, acts) {
            if (act->data().canConvert<int> ()) {
                int idr = act->data().toInt();

                if (idr == id) {
                    act->setChecked(found = true);
                    break;
                }
            }
        }

        if (!found)
            qWarning() << Q_FUNC_INFO << "could not find entry for charset=" << charset;
    } else {
        foreach(QAction *act, acts) {
            if (act->data().canConvert<int> ()) {
                int idr = act->data().toInt();

                if (idr == -2) {
                    act->setChecked(true);
                    break;
                }
            }
        }
    }
}

QString KrRemoteEncodingMenu::currentCharacterSet()
{
    QUrl currentURL = ACTIVE_PANEL->virtualPath();
    return KProtocolManager::charsetFor(currentURL);
}

void KrRemoteEncodingMenu::loadSettings()
{
    settingsLoaded = true;
    encodingNames = KCharsets::charsets()->descriptiveEncodingNames();

    QMenu *qmenu = menu();
    disconnect(qmenu, SIGNAL(triggered(QAction*)), this, SLOT(slotTriggered(QAction*)));
    connect(qmenu, SIGNAL(triggered(QAction*)), this, SLOT(slotTriggered(QAction*)));
    qmenu->clear();

    QStringList::ConstIterator it;
    int count = 0;
    QAction *act;

    for (it = encodingNames.constBegin(); it != encodingNames.constEnd(); ++it) {
        act = qmenu->addAction(*it);
        act->setData(QVariant(++count));
        act->setCheckable(true);
    }
    qmenu->addSeparator();

    act = qmenu->addAction(i18n("Reload"));
    act->setCheckable(true);
    act->setData(QVariant(-1));

    act = qmenu->addAction(i18nc("Default encoding", "Default"));
    act->setCheckable(true);
    act->setData(QVariant(-2));
}

void KrRemoteEncodingMenu::slotTriggered(QAction * act)
{
    if (!act || !act->data().canConvert<int> ())
        return;

    int id = act->data().toInt();

    switch (id) {
    case -1:
        slotReload();
        return;
    case -2:
        chooseDefault();
        return;
    default:
        chooseEncoding(encodingNames[id - 1]);
    }
}

void KrRemoteEncodingMenu::chooseEncoding(QString encoding)
{
    QUrl currentURL = ACTIVE_PANEL->virtualPath();

    KConfig config(("kio_" + currentURL.scheme() + "rc").toLatin1());
    QString host = currentURL.host();

    QString charset = KCharsets::charsets()->encodingForName(encoding);

    KConfigGroup group(&config, host);
    group.writeEntry(DATA_KEY, charset);
    config.sync();

    // Update the io-slaves...
    updateKIOSlaves();
}

void KrRemoteEncodingMenu::slotReload()
{
    loadSettings();
}

void KrRemoteEncodingMenu::chooseDefault()
{
    QUrl currentURL = ACTIVE_PANEL->virtualPath();

    // We have no choice but delete all higher domain level
    // settings here since it affects what will be matched.
    KConfig config(("kio_" + currentURL.scheme() + "rc").toLatin1());

    QStringList partList = currentURL.host().split('.', QString::SkipEmptyParts);
    if (!partList.isEmpty()) {
        partList.erase(partList.begin());

        QStringList domains;
        // Remove the exact name match...
        domains << currentURL.host();

        while (partList.count()) {
            if (partList.count() == 2)
                if (partList[0].length() <= 2 && partList[1].length() == 2)
                    break;

            if (partList.count() == 1)
                break;

            domains << partList.join(".");
            partList.erase(partList.begin());
        }

        for (QStringList::Iterator it = domains.begin(); it != domains.end(); ++it) {
            //qDebug() << "Domain to remove: " << *it;
            if (config.hasGroup(*it))
                config.deleteGroup(*it);
            else if (config.group("").hasKey(*it))
                config.group("").deleteEntry(*it);       //don't know what group name is supposed to be XXX
        }
    }
    config.sync();

    updateKIOSlaves();
}


void KrRemoteEncodingMenu::updateKIOSlaves()
{
    KIO::Scheduler::emitReparseSlaveConfiguration();

    // Reload the page with the new charset
    QTimer::singleShot(500, ACTIVE_FUNC, SLOT(refresh()));
}

