/***************************************************************************
                         KgProtocols.h  -  description
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

#ifndef KGPROTOCOLS_H
#define KGPROTOCOLS_H

// QtWidgets
#include <QPushButton>

#include "konfiguratorpage.h"
#include "../GUI/krtreewidget.h"
#include "../GUI/krlistwidget.h"

class KgProtocols : public KonfiguratorPage
{
    Q_OBJECT

public:
    explicit KgProtocols(bool first, QWidget* parent = 0);

    virtual void loadInitialValues() Q_DECL_OVERRIDE;
    virtual void setDefaults() Q_DECL_OVERRIDE;
    virtual bool apply() Q_DECL_OVERRIDE;
    virtual bool isChanged() Q_DECL_OVERRIDE;

    static  void init();

public slots:
    void         slotDisableButtons();
    void         slotAddProtocol();
    void         slotRemoveProtocol();
    void         slotAddMime();
    void         slotRemoveMime();

protected:
    void         loadProtocols();
    void         loadMimes();
    void         addSpacer(QBoxLayout *parent);

    void         addProtocol(QString name, bool changeCurrent = false);
    void         removeProtocol(QString name);
    void         addMime(QString name, QString protocol);
    void         removeMime(QString name);

    KrTreeWidget *linkList;

    KrListWidget *protocolList;
    KrListWidget *mimeList;

    QPushButton *btnAddProtocol;
    QPushButton *btnRemoveProtocol;
    QPushButton *btnAddMime;
    QPushButton *btnRemoveMime;
};

#endif /* __KgProtocols_H__ */
