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

#include "krinterdetailedview.h"

// QtCore
#include <QDir>
#include <QHashIterator>
// QtWidgets
#include <QApplication>
#include <QDirModel>
#include <QHeaderView>
#include <QMenu>
#include <QToolTip>

#include <KConfigCore/KSharedConfig>
#include <KI18n/KLocalizedString>
#include <KIOWidgets/KDirLister>

#include "krviewfactory.h"
#include "krviewitemdelegate.h"
#include "krviewitem.h"
#include "listmodel.h"
#include "../FileSystem/krpermhandler.h"
#include "../defaults.h"
#include "../krglobal.h"
#include "krmousehandler.h"
#include "../krcolorcache.h"
#include "../GUI/krstyleproxy.h"


KrInterDetailedView::KrInterDetailedView(QWidget *parent, KrViewInstance &instance, KConfig *cfg):
        QTreeView(parent),
        KrInterView(instance, cfg, this),
        _autoResizeColumns(true)
{
    connect(_mouseHandler, SIGNAL(renameCurrentItem()), this, SLOT(renameCurrentItem()));
    setWidget(this);

    KConfigGroup grpSvr(_config, "Look&Feel");
    _viewFont = grpSvr.readEntry("Filelist Font", _FilelistFont);

    setModel(_model);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setAllColumnsShowFocus(true);
    setUniformRowHeights(true);

    setMouseTracking(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

    setSelectionMode(QAbstractItemView::NoSelection);
    setSelectionModel(new DummySelectionModel(_model, this));

    header()->installEventFilter(this);
    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(false);

    KrStyleProxy *style = new KrStyleProxy();
    style->setParent(this);
    setStyle(style);
    viewport()->setStyle(style); // for custom tooltip delay

    setItemDelegate(new KrViewItemDelegate(this));

    connect(header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(sectionResized(int,int,int)));
    connect(header(), SIGNAL(sectionMoved(int,int,int)), this, SLOT(sectionMoved(int,int,int)));
}

KrInterDetailedView::~KrInterDetailedView()
{
    delete _properties;
    _properties = 0;
    delete _operator;
    _operator = 0;
}

void KrInterDetailedView::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    if (_model->ready()) {
        KrViewItem * item = getKrViewItem(currentIndex());
        op()->emitCurrentChanged(item);
    }
    QTreeView::currentChanged(current, previous);
}

void KrInterDetailedView::doRestoreSettings(KConfigGroup grp)
{
    _autoResizeColumns = grp.readEntry("AutoResizeColumns", true);

    QByteArray savedState = grp.readEntry("Saved State", QByteArray());

    if (savedState.isEmpty()) {
        hideColumn(KrViewProperties::Type);
        hideColumn(KrViewProperties::Permissions);
        hideColumn(KrViewProperties::Owner);
        hideColumn(KrViewProperties::Group);
        hideColumn(KrViewProperties::Changed);
        hideColumn(KrViewProperties::Accessed);
        header()->resizeSection(KrViewProperties::Ext, QFontMetrics(_viewFont).width("tar.bz2  "));
        header()->resizeSection(KrViewProperties::KrPermissions, QFontMetrics(_viewFont).width("rwx  "));
        header()->resizeSection(KrViewProperties::Size, QFontMetrics(_viewFont).width("9") * 10);

        QDateTime tmp(QDate(2099, 12, 29), QTime(23, 59));
        QString desc = QLocale().toString(tmp, QLocale::ShortFormat) + "  ";

        header()->resizeSection(KrViewProperties::Modified, QFontMetrics(_viewFont).width(desc));
    } else {
        header()->restoreState(savedState);

        // do not show new columns by default; restoreState() shows columns not saved
        if (KrGlobal::sCurrentConfigVersion < KrGlobal::sConfigVersion) {
            hideColumn(KrViewProperties::Changed);
            hideColumn(KrViewProperties::Accessed);
        }

        _model->setExtensionEnabled(!isColumnHidden(KrViewProperties::Ext));
    }

    KrInterView::doRestoreSettings(grp);
}

void KrInterDetailedView::saveSettings(KConfigGroup grp, KrViewProperties::PropertyType properties)
{
    KrInterView::saveSettings(grp, properties);

    grp.writeEntry("AutoResizeColumns", _autoResizeColumns);

    if(properties & KrViewProperties::PropColumns) {
        QByteArray state = header()->saveState();
        grp.writeEntry("Saved State", state);
    }
}

int KrInterDetailedView::itemsPerPage()
{
    QRect rect = visualRect(currentIndex());
    if (!rect.isValid()) {
        for (int i = 0; i != _model->rowCount(); i++) {
            rect = visualRect(_model->index(i, 0));
            if (rect.isValid())
                break;
        }
    }
    if (!rect.isValid())
        return 0;
    int size = (height() - header()->height()) / rect.height();
    if (size < 0)
        size = 0;
    return size;
}

void KrInterDetailedView::updateView()
{
}

void KrInterDetailedView::setup()
{
    setSortMode(_properties->sortColumn, (_properties->sortOptions & KrViewProperties::Descending));
    setSortingEnabled(true);
}

void KrInterDetailedView::keyPressEvent(QKeyEvent *e)
{
    if (!e || !_model->ready())
        return ; // subclass bug
    if (handleKeyEvent(e))    // did the view class handled the event?
        return;
    QTreeView::keyPressEvent(e);
}

void KrInterDetailedView::mousePressEvent(QMouseEvent * ev)
{
    if (!_mouseHandler->mousePressEvent(ev))
        QTreeView::mousePressEvent(ev);
}

void KrInterDetailedView::mouseReleaseEvent(QMouseEvent * ev)
{
    if (!_mouseHandler->mouseReleaseEvent(ev))
        QTreeView::mouseReleaseEvent(ev);
}

void KrInterDetailedView::mouseDoubleClickEvent(QMouseEvent *ev)
{
    if (!_mouseHandler->mouseDoubleClickEvent(ev))
        QTreeView::mouseDoubleClickEvent(ev);
}

void KrInterDetailedView::mouseMoveEvent(QMouseEvent * ev)
{
    if (!_mouseHandler->mouseMoveEvent(ev))
        QTreeView::mouseMoveEvent(ev);
}

void KrInterDetailedView::wheelEvent(QWheelEvent *ev)
{
    if (!_mouseHandler->wheelEvent(ev))
        QTreeView::wheelEvent(ev);
}

void KrInterDetailedView::dragEnterEvent(QDragEnterEvent *ev)
{
    if (!_mouseHandler->dragEnterEvent(ev))
        QTreeView::dragEnterEvent(ev);
}

void KrInterDetailedView::dragMoveEvent(QDragMoveEvent *ev)
{
    QTreeView::dragMoveEvent(ev);
    _mouseHandler->dragMoveEvent(ev);
}

void KrInterDetailedView::dragLeaveEvent(QDragLeaveEvent *ev)
{
    if (!_mouseHandler->dragLeaveEvent(ev))
        QTreeView::dragLeaveEvent(ev);
}

void KrInterDetailedView::dropEvent(QDropEvent *ev)
{
    if (!_mouseHandler->dropEvent(ev))
        QTreeView::dropEvent(ev);
}

bool KrInterDetailedView::event(QEvent * e)
{
    _mouseHandler->otherEvent(e);
    return QTreeView::event(e);
}

void KrInterDetailedView::renameCurrentItem()
{
    QModelIndex cIndex = currentIndex();
    QModelIndex nameIndex = _model->index(cIndex.row(), KrViewProperties::Name);
    edit(nameIndex);
    updateEditorData();
    update(nameIndex);
}

bool KrInterDetailedView::eventFilter(QObject *object, QEvent *event)
{
    if (object == header()) {
        if (event->type() == QEvent::ContextMenu) {
            QContextMenuEvent *me = (QContextMenuEvent *)event;
            showContextMenu(me->globalPos());
            return true;
        } else if (event->type() == QEvent::Resize) {
            recalculateColumnSizes();
            return false;
        }
    }
    return false;
}

void KrInterDetailedView::showContextMenu(const QPoint & p)
{
    QMenu popup(this);
    popup.setTitle(i18n("Columns"));

    QVector<QAction*> actions;

    for(int i = KrViewProperties::Ext; i < KrViewProperties::MAX_COLUMNS; i++) {
        QString text = (_model->headerData(i, Qt::Horizontal)).toString();
        QAction *act = popup.addAction(text);
        act->setCheckable(true);
        act->setChecked(!header()->isSectionHidden(i));
        act->setData(i);
        actions.append(act);
    }

    popup.addSeparator();
    QAction *actAutoResize = popup.addAction(i18n("Automatically Resize Columns"));
    actAutoResize->setCheckable(true);
    actAutoResize->setChecked(_autoResizeColumns);

    QAction *res = popup.exec(p);
    if (res == 0)
        return;

    if(res == actAutoResize) {
        _autoResizeColumns = actAutoResize->isChecked();
        recalculateColumnSizes();
    } else {
        int column = res->data().toInt();

        if(header()->isSectionHidden(column))
            header()->showSection(column);
        else
            header()->hideSection(column);

        if(KrViewProperties::Ext == column)
            _model->setExtensionEnabled(!header()->isSectionHidden(KrViewProperties::Ext));
    }
    op()->settingsChanged(KrViewProperties::PropColumns);
}

void KrInterDetailedView::sectionResized(int /*column*/, int oldSize, int newSize)
{
    // *** taken from dolphin ***
    // If the user changes the size of the headers, the autoresize feature should be
    // turned off. As there is no dedicated interface to find out whether the header
    // section has been resized by the user or by a resize event, another approach is used.
    // Attention: Take care when changing the if-condition to verify that there is no
    // regression in combination with bug 178630 (see fix in comment #8).
    if ((QApplication::mouseButtons() & Qt::LeftButton) && header()->underMouse()) {
        _autoResizeColumns = false;
        op()->settingsChanged(KrViewProperties::PropColumns);
    }

    if (oldSize == newSize || !_model->ready())
        return;

    recalculateColumnSizes();
}

void KrInterDetailedView::sectionMoved(int /*logicalIndex*/, int /*oldVisualIndex*/, int /*newVisualIndex*/)
{
    op()->settingsChanged(KrViewProperties::PropColumns);
}

void KrInterDetailedView::recalculateColumnSizes()
{
    if(!_autoResizeColumns)
        return;
    int sum = 0;
    for (int i = 0; i != _model->columnCount(); i++) {
        if (!isColumnHidden(i))
            sum += header()->sectionSize(i);
    }

    if (sum != header()->width()) {
        int delta = sum - header()->width();
        int nameSize = header()->sectionSize(KrViewProperties::Name);
        if (nameSize - delta > 20)
            header()->resizeSection(KrViewProperties::Name, nameSize - delta);
    }
}

bool KrInterDetailedView::viewportEvent(QEvent * event)
{
    if (event->type() == QEvent::ToolTip) {
        // only show tooltip if column is not wide enough to show all text. In this case the column
        // data text is abbreviated and the full text is shown as tooltip, see ListModel::data().

        QHelpEvent *he = static_cast<QHelpEvent*>(event);
        const QModelIndex index = indexAt(he->pos());
        // name column has a detailed tooltip
        if (index.isValid() && index.column() != KrViewProperties::Name) {
            int width = header()->sectionSize(index.column());
            QString text = index.data(Qt::DisplayRole).toString();

            int textWidth = QFontMetrics(_viewFont).width(text);

            const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
            textWidth += 2 * textMargin;

            QVariant decor = index.data(Qt::DecorationRole);
            if (decor.isValid() && decor.type() == QVariant::Pixmap) {
                QPixmap p = decor.value<QPixmap>();
                textWidth += p.width() + 2 * textMargin;
            }

            if (textWidth <= width) {
                QToolTip::hideText();
                event->accept();
                return true;
            }
        }
    }
    return QTreeView::viewportEvent(event);
}

void KrInterDetailedView::drawRow(QPainter *painter, const QStyleOptionViewItem &options,
                                  const QModelIndex &index) const
{
    QTreeView::drawRow(painter, options, index);
    // (may) draw dashed line border around current item row. This is done internally in
    // QTreeView::drawRow() only when panel is focused, we have to repeat it here.
    if (index == currentIndex() && drawCurrent()) {
        QStyleOptionFocusRect o;
        o.backgroundColor = options.palette.color(QPalette::Normal, QPalette::Background);

        const QRect focusRect(0, options.rect.y(), header()->length(), options.rect.height());
        o.rect = style()->visualRect(layoutDirection(), viewport()->rect(), focusRect);

        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
    }
}

void KrInterDetailedView::setSortMode(KrViewProperties::ColumnType sortColumn, bool descending)
{
    Qt::SortOrder sortDir = descending ? Qt::DescendingOrder : Qt::AscendingOrder;
    sortByColumn(sortColumn, sortDir);
}

void KrInterDetailedView::setFileIconSize(int size)
{
    KrView::setFileIconSize(size);
    setIconSize(QSize(fileIconSize(), fileIconSize()));
}

QRect KrInterDetailedView::itemRect(const FileItem *item)
{
    QRect r = visualRect(_model->fileItemIndex(item));
    r.setLeft(0);
    r.setWidth(header()->length());
    return r;
}

void KrInterDetailedView::copySettingsFrom(KrView *other)
{
    if(other->instance() == instance()) { // the other view is of the same type
        KrInterDetailedView *v = static_cast<KrInterDetailedView*>(other);
        _autoResizeColumns = v->_autoResizeColumns;
        header()->restoreState(v->header()->saveState());
        _model->setExtensionEnabled(!isColumnHidden(KrViewProperties::Ext));
        recalculateColumnSizes();
        setFileIconSize(v->fileIconSize());
    }
}
