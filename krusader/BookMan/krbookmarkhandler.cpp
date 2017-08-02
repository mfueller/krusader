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

#include "krbookmarkhandler.h"
#include "kraddbookmarkdlg.h"

#include "../krglobal.h"
#include "../krslots.h"
#include "../kractions.h"
#include "../krmainwindow.h"
#include "../Dialogs/popularurls.h"
#include "../FileSystem/filesystem.h"
#include "../Panel/krpanel.h"
#include "../Panel/listpanelactions.h"

// QtCore
#include <QTextStream>
#include <QFile>
#include <QEvent>
#include <QStandardPaths>
// QtGui
#include <QMouseEvent>
#include <QCursor>

#include <KConfigCore/KSharedConfig>
#include <KI18n/KLocalizedString>
#include <KIconThemes/KIconLoader>
#include <KWidgetsAddons/KMessageBox>
#include <KXmlGui/KActionCollection>
#include <KBookmarks/KBookmarkManager>

#define SPECIAL_BOOKMARKS true

// ------------------------ for internal use
#define BOOKMARKS_FILE "krusader/krbookmarks.xml"
#define CONNECT_BM(X) { disconnect(X, SIGNAL(activated(QUrl)), 0, 0); connect(X, SIGNAL(activated(QUrl)), this, SLOT(slotActivated(QUrl))); }

KrBookmarkHandler::KrBookmarkHandler(KrMainWindow *mainWindow) : QObject(mainWindow->widget()),
        _mainWindow(mainWindow), _middleClick(false), _mainBookmarkPopup(0), _specialBookmarks()
{
    // create our own action collection and make the shortcuts apply only to parent
    _privateCollection = new KActionCollection(this);
    _collection = _mainWindow->actions();

    // create _root: father of all bookmarks. it is a dummy bookmark and never shown
    _root = new KrBookmark(i18n("Bookmarks"));
    _root->setParent(this);

    // load bookmarks
    importFromFile();

    // hack
    QString filename = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + BOOKMARKS_FILE;
    manager = KBookmarkManager::managerForFile(filename, QStringLiteral("krusader"));
    connect(manager, SIGNAL(changed(QString,QString)), this, SLOT(bookmarksChanged(QString,QString)));
}

KrBookmarkHandler::~KrBookmarkHandler()
{
    delete manager;
    delete _privateCollection;
}

void KrBookmarkHandler::bookmarkCurrent(QUrl url)
{
    QPointer<KrAddBookmarkDlg> dlg = new KrAddBookmarkDlg(_mainWindow->widget(), url);
    if (dlg->exec() == QDialog::Accepted) {
        KrBookmark *bm = new KrBookmark(dlg->name(), dlg->url(), _collection);
        addBookmark(bm, dlg->folder());
    }
    delete dlg;
}

void KrBookmarkHandler::addBookmark(KrBookmark *bm, KrBookmark *folder)
{
    if (folder == 0)
        folder = _root;

    // add to the list (bottom)
    folder->children().append(bm);

    exportToFile();
}

void KrBookmarkHandler::deleteBookmark(KrBookmark *bm)
{
    if (bm->isFolder())
        clearBookmarks(bm);   // remove the child bookmarks
    removeReferences(_root, bm);
    foreach(QWidget *w, bm->associatedWidgets())
    w->removeAction(bm);
    delete bm;

    exportToFile();
}

void KrBookmarkHandler::removeReferences(KrBookmark *root, KrBookmark *bmToRemove)
{
    int index = root->children().indexOf(bmToRemove);
    if (index >= 0)
        root->children().removeAt(index);

    QListIterator<KrBookmark *> it(root->children());
    while (it.hasNext()) {
        KrBookmark *bm = it.next();
        if (bm->isFolder())
            removeReferences(bm, bmToRemove);
    }
}

void KrBookmarkHandler::exportToFileBookmark(QDomDocument &doc, QDomElement &where, KrBookmark *bm)
{
    if (bm->isSeparator()) {
        QDomElement bookmark = doc.createElement("separator");
        where.appendChild(bookmark);
    } else {
        QDomElement bookmark = doc.createElement("bookmark");
        // url
        bookmark.setAttribute("href", bm->url().toDisplayString());
        // icon
        bookmark.setAttribute("icon", bm->iconName());
        // title
        QDomElement title = doc.createElement("title");
        title.appendChild(doc.createTextNode(bm->text()));
        bookmark.appendChild(title);

        where.appendChild(bookmark);
    }
}

void KrBookmarkHandler::exportToFileFolder(QDomDocument &doc, QDomElement &parent, KrBookmark *folder)
{
    QListIterator<KrBookmark *> it(folder->children());
    while (it.hasNext()) {
        KrBookmark *bm = it.next();

        if (bm->isFolder()) {
            QDomElement newFolder = doc.createElement("folder");
            newFolder.setAttribute("icon", bm->iconName());
            parent.appendChild(newFolder);
            QDomElement title = doc.createElement("title");
            title.appendChild(doc.createTextNode(bm->text()));
            newFolder.appendChild(title);
            exportToFileFolder(doc, newFolder, bm);
        } else {
            exportToFileBookmark(doc, parent, bm);
        }
    }
}

// export to file using the xbel standard
//
//  <xbel>
//    <bookmark href="http://developer.kde.org"><title>Developer Web Site</title></bookmark>
//    <folder folded="no">
//      <title>Title of this folder</title>
//      <bookmark icon="kde" href="http://www.kde.org"><title>KDE Web Site</title></bookmark>
//      <folder toolbar="yes">
//        <title>My own bookmarks</title>
//        <bookmark href="http://www.koffice.org"><title>KOffice Web Site</title></bookmark>
//        <separator/>
//        <bookmark href="http://www.kdevelop.org"><title>KDevelop Web Site</title></bookmark>
//      </folder>
//    </folder>
//  </xbel>
void KrBookmarkHandler::exportToFile()
{
    QDomDocument doc("xbel");
    QDomElement root = doc.createElement("xbel");
    doc.appendChild(root);

    exportToFileFolder(doc, root, _root);
    if (!doc.firstChild().isProcessingInstruction()) {
        // adding: <?xml version="1.0" encoding="UTF-8" ?> if not already present
        QDomProcessingInstruction instr = doc.createProcessingInstruction("xml",
                                          "version=\"1.0\" encoding=\"UTF-8\" ");
        doc.insertBefore(instr, doc.firstChild());
    }


    QString filename = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + BOOKMARKS_FILE;
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << doc.toString();
        file.close();
    } else {
        KMessageBox::error(_mainWindow->widget(), i18n("Unable to write to %1", filename), i18n("Error"));
    }
}

bool KrBookmarkHandler::importFromFileBookmark(QDomElement &e, KrBookmark *parent, QString path, QString *errorMsg)
{
    QString url, name, icon;
    // verify tag
    if (e.tagName() != "bookmark") {
        *errorMsg = i18n("%1 instead of %2", e.tagName(), QLatin1String("bookmark"));
        return false;
    }
    // verify href
    if (!e.hasAttribute("href")) {
        *errorMsg = i18n("missing tag %1", QLatin1String("href"));
        return false;
    } else url = e.attribute("href");
    // verify title
    QDomElement te = e.firstChild().toElement();
    if (te.tagName() != "title") {
        *errorMsg = i18n("missing tag %1", QLatin1String("title"));
        return false;
    } else name = te.text();
    // do we have an icon?
    if (e.hasAttribute("icon")) {
        icon = e.attribute("icon");
    }
    // ok: got name and url, let's add a bookmark
    KrBookmark *bm = KrBookmark::getExistingBookmark(path + name, _collection);
    if (!bm) {
        bm = new KrBookmark(name, QUrl(url), _collection, icon, path + name);
        parent->children().append(bm);
    }

    return true;
}

bool KrBookmarkHandler::importFromFileFolder(QDomNode &first, KrBookmark *parent, QString path, QString *errorMsg)
{
    QString name;
    QDomNode n = first;
    while (!n.isNull()) {
        QDomElement e = n.toElement();
        if (e.tagName() == "bookmark") {
            if (!importFromFileBookmark(e, parent, path, errorMsg))
                return false;
        } else if (e.tagName() == "folder") {
            QString iconName = "";
            if (e.hasAttribute("icon")) iconName = e.attribute("icon");
            // the title is the first child of the folder
            QDomElement tmp = e.firstChild().toElement();
            if (tmp.tagName() != "title") {
                *errorMsg = i18n("missing tag %1", QLatin1String("title"));
                return false;
            } else name = tmp.text();
            KrBookmark *folder = new KrBookmark(name, iconName);
            parent->children().append(folder);

            QDomNode nextOne = tmp.nextSibling();
            if (!importFromFileFolder(nextOne, folder, path + name + '/', errorMsg))
                return false;
        } else if (e.tagName() == "separator") {
            parent->children().append(KrBookmark::separator());
        }
        n = n.nextSibling();
    }
    return true;
}


void KrBookmarkHandler::importFromFile()
{
    clearBookmarks(_root);

    QString filename = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + BOOKMARKS_FILE;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return; // no bookmarks file

    QString errorMsg;
    QDomNode n;
    QDomElement e;
    QDomDocument doc("xbel");
    if (!doc.setContent(&file, &errorMsg)) {
        goto BM_ERROR;
    }
    // iterate through the document: first child should be "xbel" (skip all until we find it)
    n = doc.firstChild();
    while (!n.isNull() && n.toElement().tagName() != "xbel")
        n = n.nextSibling();

    if (n.isNull() || n.toElement().tagName() != "xbel") {
        errorMsg = i18n("%1 does not seem to be a valid bookmarks file", filename);
        goto BM_ERROR;
    } else n = n.firstChild(); // skip the xbel part
    importFromFileFolder(n, _root, "", &errorMsg);
    goto BM_SUCCESS;

BM_ERROR:
    KMessageBox::error(_mainWindow->widget(), i18n("Error reading bookmarks file: %1", errorMsg), i18n("Error"));

BM_SUCCESS:
    file.close();
}

void KrBookmarkHandler::populate(QMenu *menu)
{
    _mainBookmarkPopup = menu;
    menu->clear();
    _specialBookmarks.clear();
    buildMenu(_root, menu);
}

void KrBookmarkHandler::buildMenu(KrBookmark *parent, QMenu *menu)
{
    static int inSecondaryMenu = 0; // used to know if we're on the top menu

    // run the loop twice, in order to put the folders on top. stupid but easy :-)
    // note: this code drops the separators put there by the user
    QListIterator<KrBookmark *> it(parent->children());
    while (it.hasNext()) {
        KrBookmark *bm = it.next();

        if (!bm->isFolder()) continue;
        QMenu *newMenu = new QMenu(menu);
        newMenu->setIcon(QIcon(krLoader->loadIcon(bm->iconName(), KIconLoader::Small)));
        newMenu->setTitle(bm->text());
        QAction *menuAction = menu->addMenu(newMenu);
        QVariant v;
        v.setValue<KrBookmark *>(bm);
        menuAction->setData(v);

        ++inSecondaryMenu;
        buildMenu(bm, newMenu);
        --inSecondaryMenu;
    }

    it.toFront();
    while (it.hasNext()) {
        KrBookmark *bm = it.next();
        if (bm->isFolder()) continue;
        if (bm->isSeparator()) {
            menu->addSeparator();
            continue;
        }
        menu->addAction(bm);
        CONNECT_BM(bm);
    }

    if (!inSecondaryMenu) {
        KConfigGroup group(krConfig, "Private");
        bool hasPopularURLs = group.readEntry("BM Popular URLs", true);
        bool hasTrash       = group.readEntry("BM Trash",        true);
        bool hasLan         = group.readEntry("BM Lan",          true);
        bool hasVirtualFS   = group.readEntry("BM Virtual FS",   true);
        bool hasJumpback    = group.readEntry("BM Jumpback",     true);

        if (hasPopularURLs) {
            menu->addSeparator();

            // add the popular links submenu
            QMenu *newMenu = new QMenu(menu);
            newMenu->setTitle(i18n("Popular URLs"));
            newMenu->setIcon(QIcon(krLoader->loadIcon("folder-bookmark", KIconLoader::Small)));
            QAction *bmfAct  = menu->addMenu(newMenu);
            _specialBookmarks.append(bmfAct);
            // add the top 15 urls
#define MAX 15
            QList<QUrl> list = _mainWindow->popularUrls()->getMostPopularUrls(MAX);
            QList<QUrl>::Iterator it;
            for (it = list.begin(); it != list.end(); ++it) {
                QString name;
                if ((*it).isLocalFile()) name = (*it).path();
                else name = (*it).toDisplayString();
                // note: these bookmark are put into the private collection
                // as to not spam the general collection
                KrBookmark *bm = KrBookmark::getExistingBookmark(name, _privateCollection);
                if (!bm)
                    bm = new KrBookmark(name, *it, _privateCollection);
                newMenu->addAction(bm);
                CONNECT_BM(bm);
            }

            newMenu->addSeparator();
            newMenu->addAction(krPopularUrls);
            newMenu->installEventFilter(this);
        }

        // do we need to add special bookmarks?
        if (SPECIAL_BOOKMARKS) {
            if (hasTrash || hasLan || hasVirtualFS || hasJumpback)
                menu->addSeparator();

            KrBookmark *bm;

            // note: special bookmarks are not kept inside the _bookmarks list and added ad-hoc
            if (hasTrash) {
                bm = KrBookmark::trash(_collection);
                menu->addAction(bm);
                _specialBookmarks.append(bm);
                CONNECT_BM(bm);
            }

            if (hasLan) {
                bm = KrBookmark::lan(_collection);
                menu->addAction(bm);
                _specialBookmarks.append(bm);
                CONNECT_BM(bm);
            }

            if (hasVirtualFS) {
                bm = KrBookmark::virt(_collection);
                menu->addAction(bm);
                _specialBookmarks.append(bm);
                CONNECT_BM(bm);
            }

            if (hasJumpback) {
                // add the jump-back button
                ListPanelActions *actions = _mainWindow->listPanelActions();
                menu->addAction(actions->actJumpBack);
                _specialBookmarks.append(actions->actJumpBack);
                menu->addSeparator();
                menu->addAction(actions->actSetJumpBack);
                _specialBookmarks.append(actions->actSetJumpBack);
            }
        }

        if (!hasJumpback)
            menu->addSeparator();

        menu->addAction(KrActions::actAddBookmark);
        _specialBookmarks.append(KrActions::actAddBookmark);
        QAction *bmAct = menu->addAction(krLoader->loadIcon("bookmarks", KIconLoader::Small),
                                         i18n("Manage Bookmarks"), manager, SLOT(slotEditBookmarks()));
        _specialBookmarks.append(bmAct);

        // make sure the menu is connected to us
        disconnect(menu, SIGNAL(triggered(QAction*)), 0, 0);
    }

    menu->installEventFilter(this);
}

void KrBookmarkHandler::clearBookmarks(KrBookmark *root)
{
    QList<KrBookmark *>::iterator it = root->children().begin();
    while (it != root->children().end()) {
        KrBookmark *bm = *it;

        if (bm->isFolder())
            clearBookmarks(bm);
        else {
            foreach(QWidget *w, bm->associatedWidgets())
            w->removeAction(bm);
            delete bm;
        }

        it = root->children().erase(it);
    }
}

void KrBookmarkHandler::bookmarksChanged(const QString&, const QString&)
{
    importFromFile();
}

bool KrBookmarkHandler::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        switch (static_cast<QMouseEvent*>(ev)->button()) {
        case Qt::RightButton:
            _middleClick = false;
            if (obj->inherits("QMenu")) {
                QMenu * menu = static_cast<QMenu*>(obj);
                QAction * act = menu->actionAt(static_cast<QMouseEvent*>(ev)->pos());

                if (obj == _mainBookmarkPopup && _specialBookmarks.contains(act)) {
                    rightClickOnSpecialBookmark();
                    return true;
                }

                KrBookmark * bm = dynamic_cast<KrBookmark*>(act);
                if (bm != 0) {
                    rightClicked(menu, bm);
                    return true;
                } else if (act && act->data().canConvert<KrBookmark *>()) {
                    KrBookmark * bm = act->data().value<KrBookmark *> ();
                    rightClicked(menu, bm);
                }
            }
        case Qt::LeftButton:
            _middleClick = false;
            break;
        case Qt::MidButton:
            _middleClick = true;
            break;
        default:
            break;
        }
    }
    return QObject::eventFilter(obj, ev);
}

#define POPULAR_URLS_ID        100100
#define TRASH_ID               100101
#define LAN_ID                 100103
#define VIRTUAL_FS_ID          100102
#define JUMP_BACK_ID           100104

void KrBookmarkHandler::rightClickOnSpecialBookmark()
{
    KConfigGroup group(krConfig, "Private");
    bool hasPopularURLs = group.readEntry("BM Popular URLs", true);
    bool hasTrash       = group.readEntry("BM Trash",      true);
    bool hasLan         = group.readEntry("BM Lan",          true);
    bool hasVirtualFS   = group.readEntry("BM Virtual FS",   true);
    bool hasJumpback    = group.readEntry("BM Jumpback",     true);

    QMenu menu(_mainBookmarkPopup);
    menu.setTitle(i18n("Enable special bookmarks"));

    QAction *act;

    act = menu.addAction(i18n("Popular URLs"));
    act->setData(QVariant(POPULAR_URLS_ID));
    act->setCheckable(true);
    act->setChecked(hasPopularURLs);
    act = menu.addAction(i18n("Trash bin"));
    act->setData(QVariant(TRASH_ID));
    act->setCheckable(true);
    act->setChecked(hasTrash);
    act = menu.addAction(i18n("Local Network"));
    act->setData(QVariant(LAN_ID));
    act->setCheckable(true);
    act->setChecked(hasLan);
    act = menu.addAction(i18n("Virtual Filesystem"));
    act->setData(QVariant(VIRTUAL_FS_ID));
    act->setCheckable(true);
    act->setChecked(hasVirtualFS);
    act = menu.addAction(i18n("Jump back"));
    act->setData(QVariant(JUMP_BACK_ID));
    act->setCheckable(true);
    act->setChecked(hasJumpback);

    connect(_mainBookmarkPopup, SIGNAL(highlighted(int)), &menu, SLOT(close()));
    connect(_mainBookmarkPopup, SIGNAL(activated(int)), &menu, SLOT(close()));

    int result = -1;
    QAction *res = menu.exec(QCursor::pos());
    if (res && res->data().canConvert<int>())
        result = res->data().toInt();

    bool doCloseMain = true;

    switch (result) {
    case POPULAR_URLS_ID:
        group.writeEntry("BM Popular URLs", !hasPopularURLs);
        break;
    case TRASH_ID:
        group.writeEntry("BM Trash", !hasTrash);
        break;
    case LAN_ID:
        group.writeEntry("BM Lan", !hasLan);
        break;
    case VIRTUAL_FS_ID:
        group.writeEntry("BM Virtual FS", !hasVirtualFS);
        break;
    case JUMP_BACK_ID:
        group.writeEntry("BM Jumpback", !hasJumpback);
        break;
    default:
        doCloseMain = false;
        break;
    }

    menu.close();

    if (doCloseMain && _mainBookmarkPopup)
        _mainBookmarkPopup->close();
}

#define OPEN_ID           100200
#define OPEN_NEW_TAB_ID   100201
#define DELETE_ID         100202

void KrBookmarkHandler::rightClicked(QMenu *menu, KrBookmark * bm)
{
    QMenu popup(_mainBookmarkPopup);
    QAction * act;

    if (!bm->isFolder()) {
        act = popup.addAction(krLoader->loadIcon("document-open", KIconLoader::Panel), i18n("Open"));
        act->setData(QVariant(OPEN_ID));
        act = popup.addAction(krLoader->loadIcon("tab-new", KIconLoader::Panel), i18n("Open in a new tab"));
        act->setData(QVariant(OPEN_NEW_TAB_ID));
        popup.addSeparator();
    }
    act = popup.addAction(krLoader->loadIcon("edit-delete", KIconLoader::Panel), i18n("Delete"));
    act->setData(QVariant(DELETE_ID));

    connect(menu, SIGNAL(highlighted(int)), &popup, SLOT(close()));
    connect(menu, SIGNAL(activated(int)), &popup, SLOT(close()));

    int result = -1;
    QAction *res = popup.exec(QCursor::pos());
    if (res && res->data().canConvert<int> ())
        result = res->data().toInt();

    popup.close();
    if (_mainBookmarkPopup && result >= OPEN_ID && result <= DELETE_ID) {
        _mainBookmarkPopup->close();
    }

    switch (result) {
    case OPEN_ID:
        SLOTS->refresh(bm->url());
        break;
    case OPEN_NEW_TAB_ID:
        _mainWindow->activeManager()->newTab(bm->url());
        break;
    case DELETE_ID:
        deleteBookmark(bm);
        break;
    }
}

// used to monitor middle clicks. if mid is found, then the
// bookmark is opened in a new tab. ugly, but easier than overloading
// KAction and KActionCollection.
void KrBookmarkHandler::slotActivated(const QUrl &url)
{
    if (_mainBookmarkPopup && !_mainBookmarkPopup->isHidden())
        _mainBookmarkPopup->close();
    if (_middleClick)
        _mainWindow->activeManager()->newTab(url);
    else
        SLOTS->refresh(url);
}

