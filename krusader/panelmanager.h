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

#ifndef PANELMANAGER_H
#define PANELMANAGER_H

#include "abstractpanelmanager.h"

// QtWidgets
#include <QWidget>
#include <QLayout>
#include <QGridLayout>
#include <QHBoxLayout>

#include <KConfigCore/KConfigGroup>

#include "paneltabbar.h"

class QStackedWidget;
class QToolButton;
class ListPanel;
class KrMainWindow;
class TabActions;

/**
 * Implements tabbed-browsing by managing a list of tabs and corresponding panels.
 */
class PanelManager: public QWidget, public AbstractPanelManager
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.krusader.PanelManager")

public:
    /**
     * PanelManager is created where once panels were created. It accepts three references to pointers
     * (self, other, active), which enables it to manage pointers held by the panels transparently.
     * It also receives a bool (left) which is true if the manager is the left one, or false otherwise.
     */
    PanelManager(QWidget *parent, KrMainWindow* mainWindow, bool left);

    void saveSettings(KConfigGroup config, bool saveHistory);
    void loadSettings(KConfigGroup config);
    int findTab(QUrl url);
    int tabCount() {
        return _tabbar->count();
    }
    int activeTab();
    void setActiveTab(int index);
    void moveTabToOtherSide();
    /** Refresh all tabs after config changes. */
    void reloadConfig();
    void layoutTabs();
    void setLeft(bool left) {
        _left = left;
    }
    void setOtherManager(PanelManager *other) {
        _otherManager = other;
    }

    // AbstractPanelManager implementation
    virtual bool isLeft() Q_DECL_OVERRIDE {
        return _left;
    }
    virtual AbstractPanelManager *otherManager() Q_DECL_OVERRIDE {
        return _otherManager;
    }
    virtual KrPanel *currentPanel() Q_DECL_OVERRIDE;
    virtual void newTab(const QUrl &url, KrPanel *nextTo) Q_DECL_OVERRIDE {
        slotNewTab(url, true, nextTo);
    }

signals:
    void draggingTab(PanelManager *from, QMouseEvent*);
    void draggingTabFinished(PanelManager *from, QMouseEvent*);
    void setActiveManager(PanelManager *manager);
    void pathChanged(ListPanel *panel);

public slots:
    /**
     * Called externally to start a new tab. Example of usage would be the "open in a new tab"
     * action, from the context-menu.
     */

    Q_SCRIPTABLE void newTab(const QString& url) {
        slotNewTab(QUrl::fromUserInput(url, QString(), QUrl::AssumeLocalFile));
    }
    Q_SCRIPTABLE void newTabs(const QStringList& urls);

    void slotNewTab(const QUrl &url, bool setCurrent = true, KrPanel *nextTo = 0);
    void slotNewTab();
    void slotLockTab();
    void slotNextTab();

    void slotPreviousTab();
    void slotCloseTab();
    void slotCloseTab(int index);
    void slotRecreatePanels();
    void slotCloseInactiveTabs();
    void slotCloseDuplicatedTabs();

protected slots:
    void slotCurrentTabChanged(int index);
    void activate();
    void slotDraggingTab(QMouseEvent *e) {
        emit draggingTab(this, e);
    }
    void slotDraggingTabFinished(QMouseEvent* e) {
        emit draggingTabFinished(this, e);
    }

private:
    void deletePanel(ListPanel *p);
    void updateTabbarPos();
    void tabsCountChanged();
    ListPanel* addPanel(bool setCurrent = true, KConfigGroup cfg = KConfigGroup(), KrPanel *nextTo = 0);
    ListPanel* createPanel(KConfigGroup cfg);
    void connectPanel(ListPanel *p);
    void disconnectPanel(ListPanel *p);

    PanelManager *_otherManager;
    TabActions *_actions;
    QGridLayout *_layout;
    QHBoxLayout *_barLayout;
    bool _left;
    PanelTabBar *_tabbar;
    QStackedWidget *_stack;
    QToolButton *_newTab;
    ListPanel *_self;
};


#endif // _PANEL_MANAGER_H
