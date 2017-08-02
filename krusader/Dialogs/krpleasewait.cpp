/***************************************************************************
                                krpleasewait.cpp
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

#include "krpleasewait.h"

// QtCore
#include <QTimer>
#include <QDateTime>
// QtGui
#include <QCloseEvent>
// QtWidgets
#include <QApplication>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>

#include <KI18n/KLocalizedString>
#include <KWidgetsAddons/KCursor>

#include "../krglobal.h"

KRPleaseWait::KRPleaseWait(QString msg, QWidget *parent, int count, bool cancel):
        QProgressDialog(cancel ? 0 : parent) , inc(true)
{
    setModal(!cancel);

    timer = new QTimer(this);
    setWindowTitle(i18n("Krusader::Wait"));

    setMinimumDuration(500);
    setAutoClose(false);
    setAutoReset(false);

    connect(timer, SIGNAL(timeout()), this, SLOT(cycleProgress()));

    QProgressBar* progress = new QProgressBar(this);
    progress->setMaximum(count);
    progress->setMinimum(0);
    setBar(progress);

    QLabel* label = new QLabel(this);
    setLabel(label);

    QPushButton* btn = new QPushButton(i18n("&Cancel"), this);
    setCancelButton(btn);

    btn->setEnabled(canClose = cancel);
    setLabelText(msg);

    show();
}

void KRPleaseWait::closeEvent(QCloseEvent * e)
{
    if (canClose) {
        emit canceled();
        e->accept();
    } else              /* if cancel is not allowed, we disable */
        e->ignore();         /* the window closing [x] also */
}

void KRPleaseWait::incProgress(int howMuch)
{
    setValue(value() + howMuch);
}

void KRPleaseWait::cycleProgress()
{
    if (inc) setValue(value() + 1);
    else     setValue(value() - 1);
    if (value() >= 9) inc = false;
    if (value() <= 0) inc = true;
}

KRPleaseWaitHandler::KRPleaseWaitHandler(QWidget *parentWindow)
    : QObject(parentWindow), _parentWindow(parentWindow), job(), dlg(0)
{
}

void KRPleaseWaitHandler::stopWait()
{
    if (dlg != 0) delete dlg;
    dlg = 0;
    cycleMutex = incMutex = false;
    // return cursor to normal arrow
    _parentWindow->setCursor(Qt::ArrowCursor);
}


void KRPleaseWaitHandler::startWaiting(QString msg, int count , bool cancel)
{
    if (dlg == 0) {
        dlg = new KRPleaseWait(msg , _parentWindow, count, cancel);
        connect(dlg, SIGNAL(canceled()), this, SLOT(killJob()));
    }
    incMutex = cycleMutex = _wasCancelled = false;
    dlg->setValue(0);

    dlg->setLabelText(msg);
    if (count == 0) {
        dlg->setMaximum(10);
        cycle = true;
        cycleProgress();
    } else {
        dlg->setMaximum(count);
        cycle = false;
    }
}

void KRPleaseWaitHandler::cycleProgress()
{
    if (cycleMutex) return;
    cycleMutex = true;
    if (dlg) dlg->cycleProgress();
    if (cycle) QTimer::singleShot(2000, this, SLOT(cycleProgress()));
    cycleMutex = false;
}

void KRPleaseWaitHandler::killJob()
{
    if (!job.isNull()) job->kill(KJob::EmitResult);
    stopWait();
    _wasCancelled = true;
}

void KRPleaseWaitHandler::setJob(KIO::Job* j)
{
    job = j;
}

void KRPleaseWaitHandler::incProgress(int i)
{
    if (incMutex) return;
    incMutex = true;
    if (dlg) dlg->incProgress(i);
    incMutex = false;
}

