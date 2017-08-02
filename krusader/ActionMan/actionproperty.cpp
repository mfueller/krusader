/*****************************************************************************
 * Copyright (C) 2004-2007 Jonas Bähr <jonas.baehr@web.de>                   *
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

#include "actionproperty.h"
#include "addplaceholderpopup.h"

#include "../UserAction/useraction.h"
#include "../UserAction/kraction.h"
#include "../krusader.h"
#include "../krglobal.h"

// QtWidgets
#include <QFileDialog>
#include <QInputDialog>

#include <KI18n/KLocalizedString>
#include <KIconThemes/KIconLoader>
#include <KWidgetsAddons/KMessageBox>
#include <KXmlGui/KActionCollection>


#define ICON(N)  KIconLoader::global()->loadIcon(N, KIconLoader::Small)

ActionProperty::ActionProperty(QWidget *parent, KrAction *action)
        : QWidget(parent), _modified(false)
{

    setupUi(this);

    if (action) {
        _action = action;
        updateGUI(_action);
    }

    ButtonAddPlaceholder->setIcon(ICON("list-add"));
    ButtonAddStartpath->setIcon(ICON("document-open"));

    // fill with all existing categories
    cbCategory->addItems(krUserAction->allCategories());

    connect(ButtonAddPlaceholder, SIGNAL(clicked()), this, SLOT(addPlaceholder()));
    connect(ButtonAddStartpath, SIGNAL(clicked()), this, SLOT(addStartpath()));
    connect(ButtonNewProtocol, SIGNAL(clicked()), this, SLOT(newProtocol()));
    connect(ButtonEditProtocol, SIGNAL(clicked()), this, SLOT(editProtocol()));
    connect(ButtonRemoveProtocol, SIGNAL(clicked()), this, SLOT(removeProtocol()));
    connect(ButtonAddPath, SIGNAL(clicked()), this, SLOT(addPath()));
    connect(ButtonEditPath, SIGNAL(clicked()), this, SLOT(editPath()));
    connect(ButtonRemovePath, SIGNAL(clicked()), this, SLOT(removePath()));
    connect(ButtonAddMime, SIGNAL(clicked()), this, SLOT(addMime()));
    connect(ButtonEditMime, SIGNAL(clicked()), this, SLOT(editMime()));
    connect(ButtonRemoveMime, SIGNAL(clicked()), this, SLOT(removeMime()));
    connect(ButtonNewFile, SIGNAL(clicked()), this, SLOT(newFile()));
    connect(ButtonEditFile, SIGNAL(clicked()), this, SLOT(editFile()));
    connect(ButtonRemoveFile, SIGNAL(clicked()), this, SLOT(removeFile()));
    connect(KeyButtonShortcut, SIGNAL(keySequenceChanged(QKeySequence)), this, SLOT(changedShortcut(QKeySequence)));
    // track modifications:
    connect(leDistinctName, SIGNAL(textChanged(QString)), SLOT(setModified()));
    connect(leTitle, SIGNAL(textChanged(QString)), SLOT(setModified()));
    connect(ButtonIcon, SIGNAL(iconChanged(QString)), SLOT(setModified()));
    connect(cbCategory, SIGNAL(currentTextChanged(QString)), SLOT(setModified()));
    connect(leTooltip, SIGNAL(textChanged(QString)), SLOT(setModified()));
    connect(textDescription, SIGNAL(textChanged()), SLOT(setModified()));
    connect(leCommandline, SIGNAL(textChanged(QString)), SLOT(setModified()));
    connect(leStartpath, SIGNAL(textChanged(QString)), SLOT(setModified()));
    connect(chkSeparateStdError, SIGNAL(clicked()), SLOT(setModified()));
    connect(radioCollectOutput, SIGNAL(clicked()), SLOT(setModified()));
    connect(radioNormal, SIGNAL(clicked()), SLOT(setModified()));
    connect(radioTE, SIGNAL(clicked()), SLOT(setModified()));
    connect(radioTerminal, SIGNAL(clicked()), SLOT(setModified()));
    connect(radioLocal, SIGNAL(clicked()), SLOT(setModified()));
    connect(radioUrl, SIGNAL(clicked()), SLOT(setModified()));
    connect(KeyButtonShortcut, SIGNAL(keySequenceChanged(QKeySequence)), SLOT(setModified()));
    connect(chkEnabled, SIGNAL(clicked()), SLOT(setModified()));
    connect(leDifferentUser, SIGNAL(textChanged(QString)), SLOT(setModified()));
    connect(chkDifferentUser, SIGNAL(clicked()), SLOT(setModified()));
    connect(chkConfirmExecution, SIGNAL(clicked()), SLOT(setModified()));
    connect(chkSeparateStdError, SIGNAL(clicked()), SLOT(setModified()));
    // The modified-state of the ShowOnly-lists is tracked in the access-functions below
}

ActionProperty::~ActionProperty()
{
}

void ActionProperty::changedShortcut(const QKeySequence& shortcut)
{
    KeyButtonShortcut->setKeySequence(shortcut);
}


void ActionProperty::clear()
{
    _action = 0;

    // This prevents the changed-signal from being emitted during the GUI-update
    _modified = true; // The real state is set at the end of this function.

    leDistinctName->clear();
    cbCategory->clearEditText();
    leTitle->clear();
    leTooltip->clear();
    textDescription->clear();
    leCommandline->clear();
    leStartpath->clear();
    KeyButtonShortcut->clearKeySequence();

    lbShowonlyProtocol->clear();
    lbShowonlyPath->clear();
    lbShowonlyMime->clear();
    lbShowonlyFile->clear();

    chkSeparateStdError->setChecked(false);
    radioNormal->setChecked(true);

    radioLocal->setChecked(true);

    chkEnabled->setChecked(true);

    chkConfirmExecution->setChecked(false);

    ButtonIcon->resetIcon();

    leDifferentUser->clear();
    chkDifferentUser->setChecked(false);

    setModified(false);
}

void ActionProperty::updateGUI(KrAction *action)
{
    if (action)
        _action = action;
    if (! _action)
        return;

    // This prevents the changed-signal from being emitted during the GUI-update.
    _modified = true; // The real state is set at the end of this function.

    leDistinctName->setText(_action->objectName());
    cbCategory->lineEdit()->setText(_action->category());
    leTitle->setText(_action->text());
    leTooltip->setText(_action->toolTip());
    textDescription->setText(_action->whatsThis());
    leCommandline->setText(_action->command());
    leCommandline->home(false);
    leStartpath->setText(_action->startpath());
    KeyButtonShortcut->setKeySequence(_action->shortcut());

    lbShowonlyProtocol->clear();
    lbShowonlyProtocol->addItems(_action->showonlyProtocol());
    lbShowonlyPath->clear();
    lbShowonlyPath->addItems(_action->showonlyPath());
    lbShowonlyMime->clear();
    lbShowonlyMime->addItems(_action->showonlyMime());
    lbShowonlyFile->clear();
    lbShowonlyFile->addItems(_action->showonlyFile());

    chkSeparateStdError->setChecked(false);
    switch (_action->execType()) {
    case KrAction::CollectOutputSeparateStderr:
        chkSeparateStdError->setChecked(true);
        radioCollectOutput->setChecked(true);
        break;
    case KrAction::CollectOutput:
        radioCollectOutput->setChecked(true);
        break;
    case KrAction::Terminal:
        radioTerminal->setChecked(true);
        break;
    case KrAction::RunInTE:
        radioTE->setChecked(true);
        break;
    default: // case KrAction::Normal:
        radioNormal->setChecked(true);
        break;
    }

    if (_action->acceptURLs())
        radioUrl->setChecked(true);
    else
        radioLocal->setChecked(true);

    chkEnabled->setChecked(_action->isVisible());

    chkConfirmExecution->setChecked(_action->confirmExecution());

    if (! _action->icon().isNull())
        ButtonIcon->setIcon(_action->icon());
    else
        ButtonIcon->resetIcon();

    leDifferentUser->setText(_action->user());
    if (_action->user().isEmpty())
        chkDifferentUser->setChecked(false);
    else
        chkDifferentUser->setChecked(true);

    setModified(false);
}

void ActionProperty::updateAction(KrAction *action)
{
    if (action)
        _action = action;
    if (! _action)
        return;

    if (_action->category() != cbCategory->currentText()) {
        _action->setCategory(cbCategory->currentText());
        // Update the category-list
        cbCategory->clear();
        cbCategory->addItems(krUserAction->allCategories());
        cbCategory->lineEdit()->setText(_action->category());
    }

    _action->setObjectName(leDistinctName->text());
    _action->setText(leTitle->text());
    _action->setToolTip(leTooltip->text());
    _action->setWhatsThis(textDescription->toPlainText());
    _action->setCommand(leCommandline->text());
    _action->setStartpath(leStartpath->text());
    _action->setShortcut(KeyButtonShortcut->keySequence());

    QStringList list;

    for (int i1 = 0; i1 != lbShowonlyProtocol->count(); i1++) {
        QListWidgetItem* lbi = lbShowonlyProtocol->item(i1);

        list << lbi->text();
    }
    _action->setShowonlyProtocol(list);

    list = QStringList();
    for (int i1 = 0; i1 != lbShowonlyPath->count(); i1++) {
        QListWidgetItem* lbi = lbShowonlyPath->item(i1);

        list << lbi->text();
    }
    _action->setShowonlyPath(list);

    list = QStringList();
    for (int i1 = 0; i1 != lbShowonlyMime->count(); i1++) {
        QListWidgetItem* lbi = lbShowonlyMime->item(i1);

        list << lbi->text();
    }
    _action->setShowonlyMime(list);

    list = QStringList();
    for (int i1 = 0; i1 != lbShowonlyFile->count(); i1++) {
        QListWidgetItem* lbi = lbShowonlyFile->item(i1);

        list << lbi->text();
    }
    _action->setShowonlyFile(list);

    if (radioCollectOutput->isChecked() && chkSeparateStdError->isChecked())
        _action->setExecType(KrAction::CollectOutputSeparateStderr);
    else if (radioCollectOutput->isChecked() && ! chkSeparateStdError->isChecked())
        _action->setExecType(KrAction::CollectOutput);
    else if (radioTerminal->isChecked())
        _action->setExecType(KrAction::Terminal);
    else if (radioTE->isChecked())
        _action->setExecType(KrAction::RunInTE);
    else
        _action->setExecType(KrAction::Normal);

    if (radioUrl->isChecked())
        _action->setAcceptURLs(true);
    else
        _action->setAcceptURLs(false);

    _action->setEnabled(chkEnabled->isChecked());
    _action->setVisible(chkEnabled->isChecked());

    _action->setConfirmExecution(chkConfirmExecution->isChecked());

    _action->setIcon(QIcon::fromTheme(ButtonIcon->icon()));
    _action->setIconName(ButtonIcon->icon());

    _action->setUser(leDifferentUser->text());

    setModified(false);
}

void ActionProperty::addPlaceholder()
{
    AddPlaceholderPopup popup(this);
    QString exp = popup.getPlaceholder(mapToGlobal(
                                           QPoint(
                                               ButtonAddPlaceholder->pos().x() + ButtonAddPlaceholder->width() + 6, // 6 is the default margin
                                               ButtonAddPlaceholder->pos().y()
                                           )
                                       ));
    leCommandline->insert(exp);
}


void ActionProperty::addStartpath()
{
    QString folder = QFileDialog::getExistingDirectory(this);
    if (!folder.isEmpty()) {
        leStartpath->setText(folder);
    }
}


void ActionProperty::newProtocol()
{
    bool ok;

    QString currentText;
    if (lbShowonlyProtocol->currentItem())
        currentText = lbShowonlyProtocol->currentItem()->text();

    QString text = QInputDialog::getText(this, i18n("New protocol"), i18n("Set a protocol:"),
                                         QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        lbShowonlyProtocol->addItems(text.split(';'));
        setModified();
    }
}

void ActionProperty::editProtocol()
{
    if (lbShowonlyProtocol->currentItem() == 0)
        return;

    bool ok;

    QString currentText = lbShowonlyProtocol->currentItem()->text();

    QString text = QInputDialog::getText(this, i18n("Edit Protocol"), i18n("Set another protocol:"),
                                         QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        lbShowonlyProtocol->currentItem()->setText(text);
        setModified();
    }
}

void ActionProperty::removeProtocol()
{
    if (lbShowonlyProtocol->currentItem() != 0) {
        delete lbShowonlyProtocol->currentItem();
        setModified();
    }
}

void ActionProperty::addPath()
{
    QString folder = QFileDialog::getExistingDirectory(this);
    if (!folder.isEmpty()) {
        lbShowonlyPath->addItem(folder);
        setModified();
    }
}

void ActionProperty::editPath()
{
    if (lbShowonlyPath->currentItem() == 0)
        return;

    bool ok;

    QString currentText = lbShowonlyPath->currentItem()->text();

    QString text = QInputDialog::getText(this, i18n("Edit Path"), i18n("Set another path:"),
                                         QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        lbShowonlyPath->currentItem()->setText(text);
        setModified();
    }
}

void ActionProperty::removePath()
{
    if (lbShowonlyPath->currentItem() != 0) {
        delete lbShowonlyPath->currentItem();
        setModified();
    }
}

void ActionProperty::addMime()
{
    bool ok;

    QString currentText;
    if (lbShowonlyMime->currentItem())
        currentText = lbShowonlyMime->currentItem()->text();

    QString text = QInputDialog::getText(this, i18n("New MIME Type"), i18n("Set a MIME type:"),
                                         QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        lbShowonlyMime->addItems(text.split(';'));
        setModified();
    }
}

void ActionProperty::editMime()
{
    if (lbShowonlyMime->currentItem() == 0)
        return;

    bool ok;

    QString currentText = lbShowonlyMime->currentItem()->text();

    QString text = QInputDialog::getText(this, i18n("Edit MIME Type"), i18n("Set another MIME type:"),
                                         QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        lbShowonlyMime->currentItem()->setText(text);
        setModified();
    }
}

void ActionProperty::removeMime()
{
    if (lbShowonlyMime->currentItem() != 0) {
        delete lbShowonlyMime->currentItem();
        setModified();
    }
}

void ActionProperty::newFile()
{
    bool ok;

    QString currentText;
    if (lbShowonlyFile->currentItem())
        currentText = lbShowonlyFile->currentItem()->text();

    QString text = QInputDialog::getText(this, i18n("New File Name"), i18n("Set a file name:"),
                                         QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        lbShowonlyFile->addItems(text.split(';'));
        setModified();
    }
}

void ActionProperty::editFile()
{
    if (lbShowonlyFile->currentItem() == 0)
        return;

    bool ok;

    QString currentText = lbShowonlyFile->currentItem()->text();

    QString text = QInputDialog::getText(this, i18n("Edit File Name"), i18n("Set another file name:"),
                                         QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        lbShowonlyFile->currentItem()->setText(text);
        setModified();
    }
}

void ActionProperty::removeFile()
{
    if (lbShowonlyFile->currentItem() != 0) {
        delete lbShowonlyFile->currentItem();
        setModified();
    }
}


bool ActionProperty::validProperties()
{
    if (leDistinctName->text().simplified().isEmpty()) {
        KMessageBox::error(this, i18n("Please set a unique name for the useraction"));
        leDistinctName->setFocus();
        return false;
    }
    if (leTitle->text().simplified().isEmpty()) {
        KMessageBox::error(this, i18n("Please set a title for the menu entry"));
        leTitle->setFocus();
        return false;
    }
    if (leCommandline->text().simplified().isEmpty()) {
        KMessageBox::error(this, i18n("Command line is empty"));
        leCommandline->setFocus();
        return false;
    }
    if (leDistinctName->isEnabled())
        if (krApp->actionCollection()->action(leDistinctName->text())) {
            KMessageBox::error(this,
                               i18n("There already is an action with this name.\n"
                                    "If you do not have such a useraction the name is used by Krusader for an internal action.")
                              );
            leDistinctName->setFocus();
            return false;
        }

    return true;
}

void ActionProperty::setModified(bool m)
{
    if (m && !_modified) {   // emit only when the state _changes_to_true_,
        emit changed();
    }
    _modified = m;
}

