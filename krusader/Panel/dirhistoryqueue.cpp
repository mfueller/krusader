/*****************************************************************************
 * Copyright (C) 2004 Shie Erlich <erlich@users.sourceforge.net>             *
 * Copyright (C) 2004 Rafi Yanai <yanai@users.sourceforge.net>               *
 * Copyright (C) 2010 Jan Lepper <dehtris@yahoo.de>                          *
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

#include "dirhistoryqueue.h"

#include "krpanel.h"
#include "PanelView/krview.h"
#include "../defaults.h"
#include "../krservices.h"

// QtCore
#include <QDir>

DirHistoryQueue::DirHistoryQueue(KrPanel *panel) :
    _panel(panel), _currentPos(0)
{
}

DirHistoryQueue::~DirHistoryQueue() {}

void DirHistoryQueue::clear()
{
    _urlQueue.clear();
    _currentItems.clear();
    _currentPos = 0;
}

QUrl DirHistoryQueue::currentUrl()
{
    if(_urlQueue.count())
        return _urlQueue[_currentPos];
    else
        return QUrl();
}

void DirHistoryQueue::setCurrentUrl(const QUrl &url)
{
    if(_urlQueue.count())
        _urlQueue[_currentPos] = url;
}

QString DirHistoryQueue::currentItem()
{
    if(count())
        return _currentItems[_currentPos];
    else
        return QString();
}

void DirHistoryQueue::saveCurrentItem()
{
    // if the filesystem-url hasn't been refreshed yet,
    // avoid saving current item for the wrong url
    if(count() &&  _panel->virtualPath().matches(_urlQueue[_currentPos], QUrl::StripTrailingSlash))
        _currentItems[_currentPos] = _panel->view->getCurrentItem();
}

void DirHistoryQueue::add(QUrl url, QString currentItem)
{
    url.setPath(QDir::cleanPath(url.path()));

    if(_urlQueue.isEmpty()) {
        _urlQueue.push_front(url);
        _currentItems.push_front(currentItem);
        return;
    }

    if(_urlQueue[_currentPos].matches(url, QUrl::StripTrailingSlash)) {
        _currentItems[_currentPos] = currentItem;
        return;
    }

    for (int i = 0; i < _currentPos; i++) {
        _urlQueue.pop_front();
        _currentItems.pop_front();
    }

    _currentPos = 0;

    // do we have room for another ?
    if (_urlQueue.count() > 12) { // FIXME: use user-defined size
        // no room - remove the oldest entry
        _urlQueue.pop_back();
        _currentItems.pop_back();
    }

    saveCurrentItem();
    _urlQueue.push_front(url);
    _currentItems.push_front(currentItem);
}

bool DirHistoryQueue::gotoPos(int pos)
{
    if(pos >= 0 && pos < _urlQueue.count()) {
        saveCurrentItem();
        _currentPos = pos;
        return true;
    }
    return false;
}

bool DirHistoryQueue::goBack()
{
    return gotoPos(_currentPos + 1);
}

bool DirHistoryQueue::goForward()
{
    return gotoPos(_currentPos - 1);
}

void DirHistoryQueue::save(KConfigGroup cfg)
{
    saveCurrentItem();

    QList<QUrl> urls;
    foreach(const QUrl &url, _urlQueue) {
        // make sure no passwords are permanently stored
        QUrl safeUrl(url);
        safeUrl.setPassword(QString());
        urls << safeUrl;
    }

    cfg.writeEntry("Entrys", KrServices::toStringList(urls));
    cfg.writeEntry("CurrentItems", _currentItems);
    cfg.writeEntry("CurrentIndex", _currentPos);
}

bool DirHistoryQueue::restore(KConfigGroup cfg)
{
    clear();
    _urlQueue = KrServices::toUrlList(cfg.readEntry("Entrys", QStringList()));
    _currentItems = cfg.readEntry("CurrentItems", QStringList());
    if(!_urlQueue.count() || _urlQueue.count() != _currentItems.count()) {
        clear();
        return false;
    }
    _currentPos = cfg.readEntry("CurrentIndex", 0);
    if(_currentPos >= _urlQueue.count() || _currentPos < 0)
        _currentPos  = 0;

    return true;
}
