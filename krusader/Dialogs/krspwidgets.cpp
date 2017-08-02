/***************************************************************************
                                krspwidgets.cpp
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

#include "krspwidgets.h"
#include "../krglobal.h"
#include "../kicons.h"
#include "../Filter/filtertabs.h"
#include "../GUI/krlistwidget.h"

// QtCore
#include <QEvent>
// QtGui
#include <QBitmap>
// QtWidgets
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <qnamespace.h>		// missing ?

#include <KCompletion/KComboBox>
#include <KCompletion/KHistoryComboBox>
#include <KConfigCore/KSharedConfig>
#include <KI18n/KLocalizedString>
#include <KIconThemes/KIconLoader>
#include <KWidgetsAddons/KCursor>

///////////////////// initiation of the static members ////////////////////////
QStringList KRSpWidgets::maskList;

///////////////////////////////////////////////////////////////////////////////

KRSpWidgets::KRSpWidgets()
{
}

KRQuery KRSpWidgets::getMask(QString caption, bool nameOnly, QWidget * parent)
{
    if (!nameOnly) {
        return FilterTabs::getQuery(parent);
    } else {
        QPointer<KRMaskChoiceSub> p = new KRMaskChoiceSub(parent);
        p->setWindowTitle(caption);
        p->exec();
        QString selection = p->selection->currentText();
        delete p;
        if (selection.isEmpty()) {
            return KRQuery();
        } else {
            return KRQuery(selection);
        }
    }
}

/////////////////////////// newFTP ////////////////////////////////////////
QUrl KRSpWidgets::newFTP()
{
    QPointer<newFTPSub> p = new newFTPSub();
    p->exec();
    QString uri = p->url->currentText();
    if (uri.isEmpty()) {
        delete p;
        return QUrl(); // empty url
    }

    QString protocol = p->prefix->currentText();
    protocol.truncate(protocol.length() - 3); // remove the trailing ://

    QString username = p->username->text().simplified();
    QString password = p->password->text().simplified();

    int uriStart = uri.lastIndexOf('@'); /* lets the user enter user and password in the URI field */
    if (uriStart != -1) {
        QString uriUser = uri.left(uriStart);
        QString uriPsw;
        uri = uri.mid(uriStart + 1);

        int pswStart = uriUser.indexOf(':'); /* getting the password name from the URL */
        if (pswStart != -1) {
            uriPsw = uriUser.mid(pswStart + 1);
            uriUser = uriUser.left(pswStart);
        }

        if (!uriUser.isEmpty()) { /* handling the ftp proxy username and password also */
            username = username.isEmpty() ? uriUser : username + '@' + uriUser;
        }

        if (!uriPsw.isEmpty()) { /* handling the ftp proxy username and password also */
            password = password.isEmpty() ? uriPsw : password + '@' + uriPsw;
        }
    }

    QString host = uri; /* separating the hostname and path from the uri */
    QString path;
    int pathStart = uri.indexOf("/");
    if (pathStart != -1) {
        path = host.mid(pathStart);
        host = host.left(pathStart);
    }

    /* setting the parameters of the URL */
    QUrl url;
    url.setScheme(protocol);
    url.setHost(host);
    url.setPath(path);
    if (protocol == "ftp" || protocol == "fish" || protocol == "sftp") {
        url.setPort(p->port->cleanText().toInt());
    }
    if (!username.isEmpty()) {
        url.setUserName(username);
    }
    if (!password.isEmpty()) {
        url.setPassword(password);
    }

    delete p;
    return url;
}

newFTPSub::newFTPSub() : newFTPGUI(0)
{
    url->setFocus();
    setGeometry(krMainWindow->x() + krMainWindow->width() / 2 - width() / 2,
                krMainWindow->y() + krMainWindow->height() / 2 - height() / 2, width(), height());
}

void newFTPSub::accept()
{
    url->addToHistory(url->currentText());
    // save the history and completion list when the history combo is
    // destroyed
    KConfigGroup group(krConfig, "Private");
    QStringList list = url->completionObject()->items();
    group.writeEntry("newFTP Completion list", list);
    list = url->historyItems();
    group.writeEntry("newFTP History list", list);
    QString protocol = prefix->currentText();
    group.writeEntry("newFTP Protocol", protocol);

    newFTPGUI::accept();
}

void newFTPSub::reject()
{
    url->lineEdit()->setText("");
    newFTPGUI::reject();
}

/////////////////////////// KRMaskChoiceSub ///////////////////////////////
KRMaskChoiceSub::KRMaskChoiceSub(QWidget * parent) : KRMaskChoice(parent)
{
    PixmapLabel1->setPixmap(krLoader->loadIcon("edit-select", KIconLoader::Desktop, 32));
    label->setText(i18n("Enter a selection:"));
    // the predefined selections list
    KConfigGroup group(krConfig, "Private");
    QStringList lst = group.readEntry("Predefined Selections", QStringList());
    if (lst.size() > 0) preSelections->addItems(lst);
    // the combo-box tweaks
    selection->setDuplicatesEnabled(false);
    selection->addItems(KRSpWidgets::maskList);
    selection->lineEdit()->setText("*");
    selection->lineEdit()->selectAll();
    selection->setFocus();
}

void KRMaskChoiceSub::reject()
{
    selection->clear();
    KRMaskChoice::reject();
}

void KRMaskChoiceSub::accept()
{
    bool add = true;
    // make sure we don't have that already
    for (int i = 0; i != KRSpWidgets::maskList.count(); i++)
        if (KRSpWidgets::maskList[ i ].simplified() == selection->currentText().simplified()) {
            // break if we found one such as this
            add = false;
            break;
        }

    if (add)
        KRSpWidgets::maskList.insert(0, selection->currentText().toLocal8Bit());
    // write down the predefined selections list
    QStringList list;

    for (int j = 0; j != preSelections->count(); j++) {
        QListWidgetItem *i = preSelections->item(j);
        list.append(i->text());
    }

    KConfigGroup group(krConfig, "Private");
    group.writeEntry("Predefined Selections", list);
    KRMaskChoice::accept();
}

void KRMaskChoiceSub::addSelection()
{
    QString temp = selection->currentText();
    bool itemExists = false;

    // check if the selection already exists
    for (int j = 0; j != preSelections->count(); j++) {
        QListWidgetItem *i = preSelections->item(j);

        if (i->text() == temp) {
            itemExists = true;
            break;
        }
    }

    if (!temp.isEmpty() && !itemExists) {
        preSelections->addItem(selection->currentText());
        preSelections->update();
    }
}

void KRMaskChoiceSub::deleteSelection()
{
    delete preSelections->currentItem();
    preSelections->update();
}

void KRMaskChoiceSub::clearSelections()
{
    preSelections->clear();
    preSelections->update();
}

void KRMaskChoiceSub::acceptFromList(QListWidgetItem *i)
{
    selection->addItem(i->text(), 0);
    accept();
}

void KRMaskChoiceSub::currentItemChanged(QListWidgetItem *i)
{
    if (i)
        selection->setEditText(i->text());
}
