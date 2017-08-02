/*****************************************************************************
 * Copyright (C) 2002 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2002 Rafi Yanai <yanai@users.sourceforge.net>               *
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

#include "krbookmark.h"
#include "../krglobal.h"
#include "../Archive/krarchandler.h"
#include "../FileSystem/krtrashhandler.h"

#include <KI18n/KLocalizedString>
#include <KIconThemes/KIconLoader>
#include <KXmlGui/KActionCollection>

#define BM_NAME(X)  (QString("Bookmark:")+X)

static const char* NAME_TRASH = I18N_NOOP("Trash bin");
static const char* NAME_VIRTUAL = I18N_NOOP("Virtual Filesystem");
static const char* NAME_LAN = I18N_NOOP("Local Network");

KrBookmark::KrBookmark(QString name, QUrl url, KActionCollection *parent, QString icon, QString actionName) :
        QAction(parent), _url(url), _icon(icon), _folder(false), _separator(false), _autoDelete(true)
{
    QString actName = actionName.isNull() ? BM_NAME(name) : BM_NAME(actionName);
    setText(name);
    parent->addAction(actName, this);
    connect(this, SIGNAL(triggered()), this, SLOT(activatedProxy()));

    // do we have an icon?
    if (!icon.isEmpty())
        setIcon(QIcon::fromTheme(icon));
    else {
        // what kind of a url is it?
        if (_url.isLocalFile()) {
            setIcon(QIcon::fromTheme("folder"));
        } else { // is it an archive?
            if (KRarcHandler::isArchive(_url))
                setIcon(QIcon::fromTheme("application-x-tar"));
            else setIcon(QIcon::fromTheme("folder-html"));
        }
    }
}

KrBookmark::KrBookmark(QString name, QString icon) :
        QAction(QIcon::fromTheme(icon), name, 0), _icon(icon), _folder(true), _separator(false), _autoDelete(false)
{
    setIcon(QIcon::fromTheme(icon == "" ? "folder" : icon));
}

KrBookmark::~KrBookmark()
{
    if (_autoDelete) {
        QListIterator<KrBookmark *> it(_children);
        while (it.hasNext())
            delete it.next();
        _children.clear();
    }
}

KrBookmark* KrBookmark::getExistingBookmark(QString actionName, KActionCollection *collection)
{
    return static_cast<KrBookmark*>(collection->action(BM_NAME(actionName)));
}

KrBookmark* KrBookmark::trash(KActionCollection *collection)
{
    KrBookmark *bm = getExistingBookmark(i18n(NAME_TRASH), collection);
    if (!bm)
        bm = new KrBookmark(i18n(NAME_TRASH), QUrl("trash:/"), collection);

    bm->setIcon(krLoader->loadIcon(KrTrashHandler::trashIcon(), KIconLoader::Small));
    return bm;
}

KrBookmark* KrBookmark::virt(KActionCollection *collection)
{
    KrBookmark *bm = getExistingBookmark(i18n(NAME_VIRTUAL), collection);
    if (!bm) {
        bm = new KrBookmark(i18n(NAME_VIRTUAL), QUrl("virt:/"), collection);
        bm->setIcon(krLoader->loadIcon("document-open-remote", KIconLoader::Small));
    }
    return bm;
}

KrBookmark* KrBookmark::lan(KActionCollection *collection)
{
    KrBookmark *bm = getExistingBookmark(i18n(NAME_LAN), collection);
    if (!bm) {
        bm = new KrBookmark(i18n(NAME_LAN), QUrl("remote:/"), collection);
        bm->setIcon(krLoader->loadIcon("network-workgroup", KIconLoader::Small));
    }
    return bm;
}

KrBookmark* KrBookmark::separator()
{
    KrBookmark *bm = new KrBookmark("");
    bm->_separator = true;
    bm->_folder = false;
    return bm;
}


void KrBookmark::activatedProxy()
{
    emit activated(url());
}

