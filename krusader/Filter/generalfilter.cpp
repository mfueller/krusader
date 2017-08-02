/***************************************************************************
                      generalfilter.cpp  -  description
                             -------------------
    copyright            : (C) 2003 + by Shie Erlich & Rafi Yanai & Csaba Karai
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

#include "generalfilter.h"
#include "filtertabs.h"
#include "../krglobal.h"
#include "../krservices.h"
#include "../FileSystem/filesystem.h"

// QtWidgets
#include <QPushButton>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>

#include <KCodecs/KCharsets>
#include <KConfigCore/KSharedConfig>
#include <KI18n/KLocalizedString>
#include <KIconThemes/KIconLoader>
#include <KWidgetsAddons/KMessageBox>

typedef struct {
    const char *description;
    const char *regExp;
    int cursorAdjustment;
} term;

static const term items[] = {
    { I18N_NOOP("Any Character"),                 ".",        0 },
    { I18N_NOOP("Start of Line"),                 "^",        0 },
    { I18N_NOOP("End of Line"),                   "$",        0 },
    { I18N_NOOP("Set of Characters"),             "[]",       -1 },
    { I18N_NOOP("Repeats, Zero or More Times"),   "*",        0 },
    { I18N_NOOP("Repeats, One or More Times"),    "+",        0 },
    { I18N_NOOP("Optional"),                      "?",        0 },
    { I18N_NOOP("Escape"),                        "\\",       0 },
    { I18N_NOOP("TAB"),                           "\\t",      0 },
    { I18N_NOOP("Newline"),                       "\\n",      0 },
    { I18N_NOOP("Carriage Return"),               "\\r",      0 },
    { I18N_NOOP("White Space"),                   "\\s",      0 },
    { I18N_NOOP("Digit"),                         "\\d",      0 },
};

class RegExpAction : public QAction
{
public:
    RegExpAction(QObject *parent, const QString &text, const QString &regExp, int cursor)
            : QAction(text, parent), mText(text), mRegExp(regExp), mCursor(cursor) {
    }

    QString text() const {
        return mText;
    }
    QString regExp() const {
        return mRegExp;
    }
    int cursor() const {
        return mCursor;
    }

private:
    QString mText;
    QString mRegExp;
    int mCursor;
};

GeneralFilter::GeneralFilter(FilterTabs *tabs, int properties, QWidget *parent,
                             QStringList extraOptions) :
        QWidget(parent),
        profileManager(0), fltTabs(tabs)
{
    QGridLayout *filterLayout = new QGridLayout(this);
    filterLayout->setSpacing(6);
    filterLayout->setContentsMargins(11, 11, 11, 11);

    this->properties = properties;

    // Options for name filtering

    QGroupBox *nameGroup = new QGroupBox(this);
    nameGroup->setTitle(i18n("File Name"));
    QGridLayout *nameGroupLayout = new QGridLayout(nameGroup);
    nameGroupLayout->setAlignment(Qt::AlignTop);
    nameGroupLayout->setSpacing(6);
    nameGroupLayout->setContentsMargins(11, 11, 11, 11);

    searchForCase = new QCheckBox(nameGroup);
    searchForCase->setText(i18n("&Case sensitive"));
    searchForCase->setChecked(false);
    nameGroupLayout->addWidget(searchForCase, 1, 2);

    QLabel *searchForLabel = new QLabel(nameGroup);
    searchForLabel->setText(i18n("Search &for:"));
    nameGroupLayout->addWidget(searchForLabel, 0, 0);

    searchFor = new KHistoryComboBox(false, nameGroup/*, "searchFor"*/);
    QSizePolicy searchForPolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    searchForPolicy.setHeightForWidth(searchFor->sizePolicy().hasHeightForWidth());
    searchFor->setSizePolicy(searchForPolicy);
    searchFor->setEditable(true);
    searchFor->setDuplicatesEnabled(false);
    searchFor->setMaxCount(25);
    searchFor->setMinimumContentsLength(10);
    searchForLabel->setBuddy(searchFor);
    nameGroupLayout->addWidget(searchFor, 0, 1, 1, 2);

    QString s = "<p><img src='toolbar|find'></p>" + i18n("<p>The filename filtering criteria is defined here.</p><p>You can make use of wildcards. Multiple patterns are separated by space (means logical OR) and patterns are excluded from the search using the pipe symbol.</p><p>If the pattern is ended with a slash (<code>*pattern*/</code>), that means that pattern relates to recursive search of folders.<ul><li><code>pattern</code> - means to search those files/folders that name is <code>pattern</code>, recursive search goes through all subfolders independently of the value of <code>pattern</code></li><li><code>pattern/</code> - means to search all files/folders, but recursive search goes through/excludes the folders that name is <code>pattern</code></li></ul></p><p>It is allowed to use quotation marks for names that contain space. Filter <code>\"Program&nbsp;Files\"</code> searches out those files/folders that name is <code>Program&nbsp;Files</code>.</p><p>Examples:</p><ul><li><code>*.o</code></li><li><code>*.h *.c\?\?</code></li><li><code>*.cpp *.h | *.moc.cpp</code></li><li><code>* | .svn/ .git/</code></li></ul><p><b>Note</b>: the search term '<code>text</code>' is equivalent to '<code>*text*</code>'.</p>");
    searchFor->setWhatsThis(s);
    searchForLabel->setWhatsThis(s);

    QLabel *searchType = new QLabel(nameGroup);
    searchType->setText(i18n("&Of type:"));
    nameGroupLayout->addWidget(searchType, 1, 0);

    ofType = new KComboBox(false, nameGroup);
    ofType->setObjectName("ofType");
    QSizePolicy ofTypePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ofTypePolicy.setHeightForWidth(ofType->sizePolicy().hasHeightForWidth());
    ofType->setSizePolicy(ofTypePolicy);
    ofType->setEditable(false);
    searchType->setBuddy(ofType);
    ofType->addItem(i18n("All Files"));
    ofType->addItem(i18n("Archives"));
    ofType->addItem(i18n("Folders"));
    ofType->addItem(i18n("Image Files"));
    ofType->addItem(i18n("Text Files"));
    ofType->addItem(i18n("Video Files"));
    ofType->addItem(i18n("Audio Files"));
    connect(ofType, SIGNAL(currentIndexChanged(int)), this, SLOT(slotDisable()));

    nameGroupLayout->addWidget(ofType, 1, 1);
    filterLayout->addWidget(nameGroup, 0, 0);

    middleLayout = new QHBoxLayout();
    middleLayout->setSpacing(6);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    QSpacerItem* middleSpacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Fixed);
    middleLayout->addItem(middleSpacer);

    if (properties & FilterTabs::HasProfileHandler) {
        // The profile handler

        QGroupBox *profileHandler = new QGroupBox(this);
        profileHandler->setTitle(i18n("&Profile handler"));
        QGridLayout *profileLayout = new QGridLayout(profileHandler);
        profileLayout->setAlignment(Qt::AlignTop);
        profileLayout->setSpacing(6);
        profileLayout->setContentsMargins(11, 11, 11, 11);

        profileListBox = new KrListWidget(profileHandler);
        profileLayout->addWidget(profileListBox, 0, 0, 4, 1);

        profileAddBtn = new QPushButton(profileHandler);
        KStandardGuiItem::assign(profileAddBtn, KStandardGuiItem::Add);
        profileLayout->addWidget(profileAddBtn, 0, 1);

        profileLoadBtn = new QPushButton(i18n("&Load"), profileHandler);
        profileLoadBtn->setEnabled(false);
        profileLayout->addWidget(profileLoadBtn, 1, 1);

        profileOverwriteBtn = new QPushButton(profileHandler);
        profileOverwriteBtn->setEnabled(false);
        KStandardGuiItem::assign(profileOverwriteBtn, KStandardGuiItem::Overwrite);
        profileLayout->addWidget(profileOverwriteBtn, 2, 1);

        profileRemoveBtn = new QPushButton(profileHandler);
        profileRemoveBtn->setEnabled(false);
        KStandardGuiItem::assign(profileRemoveBtn, KStandardGuiItem::Remove);
        profileLayout->addWidget(profileRemoveBtn, 3, 1);

        profileManager = new ProfileManager("SelectionProfile", this);
        profileManager->hide();

        middleLayout->addWidget(profileHandler);

        refreshProfileListBox();
    }

    if (properties & FilterTabs::HasSearchIn) {
        // Options for search in

        QGroupBox *searchInGroup = new QGroupBox(this);
        searchInGroup->setTitle(i18n("Searc&h in"));
        QGridLayout *searchInLayout = new QGridLayout(searchInGroup);
        searchInLayout->setAlignment(Qt::AlignTop);
        searchInLayout->setSpacing(6);
        searchInLayout->setContentsMargins(11, 11, 11, 11);

        searchIn = new KURLListRequester(KURLListRequester::RequestDirs, searchInGroup);
        searchInLayout->addWidget(searchIn, 0, 0);
        connect(searchIn, SIGNAL(changed()), this, SLOT(slotDisable()));

        middleLayout->addWidget(searchInGroup);
    }

    if (properties & FilterTabs::HasDontSearchIn) {
        // Options for don't search in

        QGroupBox *dontSearchInGroup = new QGroupBox(this);
        dontSearchInGroup->setTitle(i18n("&Do not search in"));
        QGridLayout *dontSearchInLayout = new QGridLayout(dontSearchInGroup);
        dontSearchInLayout->setAlignment(Qt::AlignTop);
        dontSearchInLayout->setSpacing(6);
        dontSearchInLayout->setContentsMargins(11, 11, 11, 11);

        dontSearchIn = new KURLListRequester(KURLListRequester::RequestDirs, dontSearchInGroup);
        dontSearchInLayout->addWidget(dontSearchIn, 0, 0);

        middleLayout->addWidget(dontSearchInGroup);
    }

    filterLayout->addLayout(middleLayout, 1, 0);

    // Options for containing text

    QGroupBox *containsGroup = new QGroupBox(this);
    containsGroup->setTitle(i18n("Containing text"));
    QGridLayout *containsLayout = new QGridLayout(containsGroup);
    containsLayout->setAlignment(Qt::AlignTop);
    containsLayout->setSpacing(6);
    containsLayout->setContentsMargins(11, 11, 11, 11);

    QHBoxLayout *containsTextLayout = new QHBoxLayout();
    containsTextLayout->setSpacing(6);
    containsTextLayout->setContentsMargins(0, 0, 0, 0);

    containsLabel = new QLabel(containsGroup);
    QSizePolicy containsLabelPolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    containsLabelPolicy.setHeightForWidth(containsLabel->sizePolicy().hasHeightForWidth());
    containsLabel->setSizePolicy(containsLabelPolicy);
    containsLabel->setText(i18n("&Text:"));
    containsTextLayout->addWidget(containsLabel);

    containsText = new KHistoryComboBox(false, containsGroup/*, "containsText"*/);
    QSizePolicy containsTextPolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    containsTextPolicy.setHeightForWidth(containsText->sizePolicy().hasHeightForWidth());
    containsText->setSizePolicy(containsTextPolicy);
    containsText->setDuplicatesEnabled(false);
    containsText->setMaxCount(25);
    containsText->setMinimumContentsLength(10);
    containsTextLayout->addWidget(containsText);
    containsLabel->setBuddy(containsText);

    containsRegExp = new QToolButton(containsGroup);
    containsRegExp->setPopupMode(QToolButton::MenuButtonPopup);
    containsRegExp->setCheckable(true);
    containsRegExp->setText(i18n("RegExp"));
    // Populate the popup menu.
    QMenu *patterns = new QMenu(containsRegExp);
    for (int i = 0; (unsigned)i < sizeof(items) / sizeof(items[0]); i++) {
        patterns->addAction(new RegExpAction(patterns, i18n(items[i].description),
                                             items[i].regExp, items[i].cursorAdjustment));
    }
    connect(containsRegExp, SIGNAL(toggled(bool)), this, SLOT(slotDisable()));
    connect(containsRegExp, SIGNAL(triggered(QAction*)), this, SLOT(slotRegExpTriggered(QAction*)));
    containsRegExp->setMenu(patterns);
    patterns->setEnabled(false);

    containsTextLayout->addWidget(containsRegExp);

    containsLayout->addLayout(containsTextLayout, 0, 0);

    QHBoxLayout *containsCbsLayout = new QHBoxLayout();
    containsCbsLayout->setSpacing(6);
    containsCbsLayout->setContentsMargins(0, 0, 0, 0);

    encLabel = new QLabel(i18n("Encoding:"), containsGroup);
    containsCbsLayout->addWidget(encLabel);
    contentEncoding = new KComboBox(containsGroup);
    contentEncoding->setEditable(false);
    contentEncoding->addItem(i18nc("Default encoding", "Default"));
    contentEncoding->addItems(KCharsets::charsets()->descriptiveEncodingNames());
    containsCbsLayout->addWidget(contentEncoding);

    QSpacerItem* cbSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    containsCbsLayout->addItem(cbSpacer);

    containsWholeWord = new QCheckBox(containsGroup);
    QSizePolicy containsWholeWordPolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    containsWholeWordPolicy.setHeightForWidth(containsWholeWord->sizePolicy().hasHeightForWidth());
    containsWholeWord->setSizePolicy(containsWholeWordPolicy);
    containsWholeWord->setText(i18n("&Match whole word only"));
    containsWholeWord->setChecked(false);
    containsCbsLayout->addWidget(containsWholeWord);

    containsTextCase = new QCheckBox(containsGroup);
    QSizePolicy containsTextCasePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    containsTextCasePolicy.setHeightForWidth(containsTextCase->sizePolicy().hasHeightForWidth());
    containsTextCase->setSizePolicy(containsTextCasePolicy);
    containsTextCase->setText(i18n("Cas&e sensitive"));
    containsTextCase->setChecked(true);
    containsCbsLayout->addWidget(containsTextCase);

    containsLayout->addLayout(containsCbsLayout, 1, 0);

    filterLayout->addWidget(containsGroup, 2, 0);

    QHBoxLayout *recurseLayout = new QHBoxLayout();
    recurseLayout->setSpacing(6);
    recurseLayout->setContentsMargins(0, 0, 0, 0);
    QSpacerItem* recurseSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    recurseLayout->addItem(recurseSpacer);

    if (properties & FilterTabs::HasRecurseOptions) {
        // Options for recursive searching
        searchInDirs = new QCheckBox(this);
        searchInDirs->setText(i18n("Search in s&ub folders"));
        searchInDirs->setChecked(true);
        recurseLayout->addWidget(searchInDirs);

        searchInArchives = new QCheckBox(this);
        searchInArchives->setText(i18n("Search in arch&ives"));
        recurseLayout->addWidget(searchInArchives);

        followLinks = new QCheckBox(this);
        followLinks->setText(i18n("Follow &links"));
        recurseLayout->addWidget(followLinks);

    }
    filterLayout->addLayout(recurseLayout, 3, 0);

    for(int i = 0; i < extraOptions.length(); i++) {
        QCheckBox *option = new QCheckBox(this);
        option->setText(extraOptions[i]);
        recurseLayout->addWidget(option);
        this->extraOptions.insert(extraOptions[i], option);
    }

    // Connection table

    if (properties & FilterTabs::HasProfileHandler) {
        connect(profileAddBtn,       SIGNAL(clicked()) , this, SLOT(slotAddBtnClicked()));
        connect(profileLoadBtn,      SIGNAL(clicked()) , this, SLOT(slotLoadBtnClicked()));
        connect(profileOverwriteBtn, SIGNAL(clicked()) , this, SLOT(slotOverwriteBtnClicked()));
        connect(profileRemoveBtn,    SIGNAL(clicked()) , this, SLOT(slotRemoveBtnClicked()));
        connect(profileListBox,      SIGNAL(itemDoubleClicked(QListWidgetItem*)) , this, SLOT(slotProfileDoubleClicked(QListWidgetItem*)));
        connect(profileManager,      SIGNAL(loadFromProfile(QString)), fltTabs, SLOT(loadFromProfile(QString)));
        connect(profileManager,      SIGNAL(saveToProfile(QString)), fltTabs, SLOT(saveToProfile(QString)));
    }

    connect(searchFor, SIGNAL(activated(QString)), searchFor, SLOT(addToHistory(QString)));
    connect(containsText, SIGNAL(activated(QString)), containsText, SLOT(addToHistory(QString)));

    // load the completion and history lists
    // ==> search for
    KConfigGroup group(krConfig, "Search");
    QStringList list = group.readEntry("SearchFor Completion", QStringList());
    searchFor->completionObject()->setItems(list);
    list = group.readEntry("SearchFor History", QStringList());
    searchFor->setHistoryItems(list);
    // ==> grep
    list = group.readEntry("ContainsText Completion", QStringList());
    containsText->completionObject()->setItems(list);
    list = group.readEntry("ContainsText History", QStringList());
    containsText->setHistoryItems(list);

    setTabOrder(searchFor, containsText);    // search for -> content
    setTabOrder(containsText, searchType);    // content -> search type

    slotDisable();
}

GeneralFilter::~GeneralFilter()
{
    // save the history combos
    // ==> search for
    QStringList list = searchFor->completionObject()->items();
    KConfigGroup group(krConfig, "Search");
    group.writeEntry("SearchFor Completion", list);
    list = searchFor->historyItems();
    group.writeEntry("SearchFor History", list);
    // ==> grep text
    list = containsText->completionObject()->items();
    group.writeEntry("ContainsText Completion", list);
    list = containsText->historyItems();
    group.writeEntry("ContainsText History", list);

    krConfig->sync();
}

bool GeneralFilter::isExtraOptionChecked(QString name)
{
    QCheckBox *option = extraOptions[name];
    return option ? option->isChecked() : false;
}

void GeneralFilter::checkExtraOption(QString name, bool check)
{
    QCheckBox *option = extraOptions[name];
    if(option)
        option->setChecked(check);
}

void GeneralFilter::queryAccepted()
{
    searchFor->addToHistory(searchFor->currentText());
    containsText->addToHistory(containsText->currentText());
}

void GeneralFilter::refreshProfileListBox()
{
    profileListBox->clear();
    profileListBox->addItems(ProfileManager::availableProfiles("SelectionProfile"));

    if (profileListBox->count() != 0) {
        profileLoadBtn->setEnabled(true);
        profileOverwriteBtn->setEnabled(true);
        profileRemoveBtn->setEnabled(true);
    } else {
        profileLoadBtn->setEnabled(false);
        profileOverwriteBtn->setEnabled(false);
        profileRemoveBtn->setEnabled(false);
    }
}

void GeneralFilter::slotAddBtnClicked()
{
    profileManager->newProfile(searchFor->currentText().simplified());
    refreshProfileListBox();
}

void GeneralFilter::slotOverwriteBtnClicked()
{
    QListWidgetItem *item = profileListBox->currentItem();
    if (item != 0)
        profileManager->overwriteProfile(item->text());
}

void GeneralFilter::slotRemoveBtnClicked()
{
    QListWidgetItem *item = profileListBox->currentItem();
    if (item != 0) {
        profileManager->deleteProfile(item->text());
        refreshProfileListBox();
    }
}

void GeneralFilter::slotProfileDoubleClicked(QListWidgetItem *item)
{
    if (item != 0) {
        QString profileName = item->text();
        profileManager->loadProfile(profileName);
        fltTabs->close(true);
    }
}

void GeneralFilter::slotLoadBtnClicked()
{
    QListWidgetItem *item = profileListBox->currentItem();
    if (item != 0)
        profileManager->loadProfile(item->text());
}

void GeneralFilter::slotDisable()
{
    bool state = containsRegExp->isChecked();
    bool global = ofType->currentText() != i18n("Folders");
    bool remoteOnly = false;
    if (properties & FilterTabs::HasSearchIn) {
        QList<QUrl> urlList = searchIn->urlList();
        remoteOnly = urlList.count() != 0;
        foreach(const QUrl &url, urlList)
        if (url.scheme() == "file")
            remoteOnly = false;
    }

    containsWholeWord->setEnabled(!state && global);
    containsRegExp->menu()->setEnabled(state && global);
    encLabel->setEnabled(global);
    contentEncoding->setEnabled(global);
    containsTextCase->setEnabled(global);
    containsRegExp->setEnabled(global);
    if (properties & FilterTabs::HasRecurseOptions)
        searchInArchives->setEnabled(global && !remoteOnly);
    containsLabel->setEnabled(global);
    containsText->setEnabled(global);
}

void GeneralFilter::slotRegExpTriggered(QAction * act)
{
    if (act == 0)
        return;
    RegExpAction *regAct = dynamic_cast<RegExpAction *>(act);
    if (regAct == 0)
        return;
    containsText->lineEdit()->insert(regAct->regExp());
    containsText->lineEdit()->setCursorPosition(containsText->lineEdit()->cursorPosition() + regAct->cursor());
    containsText->lineEdit()->setFocus();
}

bool GeneralFilter::getSettings(FilterSettings &s)
{
    // check that we have (at least) what to search, and where to search in
    if (searchFor->currentText().simplified().isEmpty()) {
        KMessageBox::error(this , i18n("No search criteria entered."));
        searchFor->setFocus();
        return false;
    }

    s.searchFor = searchFor->currentText().trimmed();
    s.searchForCase = searchForCase->isChecked();

    if (ofType->currentText() != i18n("All Files"))
        s.mimeType = ofType->currentText();

    if (containsText->isEnabled()) {
        s.containsText = containsText->currentText();
        s.containsTextCase = containsTextCase->isChecked();
        s.containsWholeWord = containsWholeWord->isChecked();
        s.containsRegExp = containsRegExp->isChecked();
    }

    if (contentEncoding->currentIndex() != 0)
        s.contentEncoding =
            KCharsets::charsets()->encodingForName(contentEncoding->currentText());

    if (properties & FilterTabs::HasRecurseOptions) {
        s.recursive = searchInDirs->isChecked();
        s.searchInArchives = searchInArchives->isChecked();
        s.followLinks = followLinks->isChecked();
    }

    if (properties & FilterTabs::HasSearchIn) {
        s.searchIn = searchIn->urlList();
        if (s.searchIn.isEmpty()) { // we need a place to search in
            KMessageBox::error(this , i18n("Please specify a location to search in."));
            searchIn->lineEdit()->setFocus();
            return false;
        }
    }

    if (properties & FilterTabs::HasDontSearchIn)
        s.dontSearchIn = dontSearchIn->urlList();

    return true;
}

void GeneralFilter::applySettings(const FilterSettings &s)
{
    searchFor->setEditText(s.searchFor);
    searchForCase->setChecked(s.searchForCase);

    setComboBoxValue(ofType, s.mimeType);

    containsText->setEditText(s.containsText);
    containsTextCase->setChecked(s.containsTextCase);
    containsWholeWord->setChecked(s.containsWholeWord);
    containsRegExp->setChecked(s.containsRegExp);

    setComboBoxValue(contentEncoding,
            KCharsets::charsets()->descriptionForEncoding(s.contentEncoding));

    if (properties & FilterTabs::HasRecurseOptions) {
        searchInDirs->setChecked(s.recursive);
        searchInArchives->setChecked(s.searchInArchives);
        followLinks->setChecked(s.followLinks);
    }

    if (properties & FilterTabs::HasSearchIn) {
        searchIn->lineEdit()->clear();
        searchIn->listBox()->clear();
        searchIn->listBox()->addItems(KrServices::toStringList(s.searchIn));
    }

    if (properties & FilterTabs::HasDontSearchIn) {
        dontSearchIn->lineEdit()->clear();
        dontSearchIn->listBox()->clear();
        dontSearchIn->listBox()->addItems(KrServices::toStringList(s.dontSearchIn));
    }
}

