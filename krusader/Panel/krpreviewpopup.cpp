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

#include "krpreviewpopup.h"

#include <algorithm>

// QtGui
#include <QPixmap>
#include <QPainter>
// QtWidgets
#include <QApplication>
#include <QStyleOptionMenuItem>
#include <QProxyStyle>

#include <KI18n/KLocalizedString>
#include <KIO/PreviewJob>

#include "../KViewer/krviewer.h"

class KrPreviewPopup::ProxyStyle : public QProxyStyle
{
public:
    ProxyStyle() : QProxyStyle(QApplication::style()) {}

    virtual QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                                   const QSize &contentsSize, const QWidget *widget = 0) const Q_DECL_OVERRIDE
    {
        if(type == QStyle::CT_MenuItem) {
            const QStyleOptionMenuItem *menuItem =
                    qstyleoption_cast<const QStyleOptionMenuItem*>(option);

            QFontMetrics fontMetrics(menuItem->font);
            QSize iconSize = menuItem->icon.actualSize(QSize(MAX_SIZE, MAX_SIZE));
            QSize textSize = QSize(fontMetrics.boundingRect(menuItem->text).width(),
                                   fontMetrics.height());

            return QSize(std::max(iconSize.width(), textSize.width()) + MARGIN*2,
                         iconSize.height() + textSize.height() + MARGIN*2);
        } else
            return QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
    }

    virtual void drawControl(ControlElement element, const QStyleOption *option,
                                    QPainter *painter, const QWidget *widget = 0 ) const Q_DECL_OVERRIDE
    {
        if(element == QStyle::CE_MenuItem) {
            painter->save();

            const QStyleOptionMenuItem *menuItem =
                    qstyleoption_cast<const QStyleOptionMenuItem*>(option);

            bool active = menuItem->state & State_Selected;

            QRect rect = menuItem->rect;

            if(active)
                painter->fillRect(rect, menuItem->palette.brush(QPalette::Highlight));

            rect.adjust(MARGIN, MARGIN, -MARGIN, -MARGIN);

            int textHeight = QFontMetrics(menuItem->font).height();

            QRect previewRect = rect;
            previewRect.setHeight(rect.height() - textHeight);
            QPixmap pixmap = menuItem->icon.pixmap(menuItem->icon.actualSize(QSize(MAX_SIZE, MAX_SIZE)));
            QProxyStyle::drawItemPixmap(painter, previewRect, Qt::AlignCenter, pixmap);

            QRect textRect = rect;
            textRect.setTop(previewRect.bottom() + 1);
            painter->setPen(active ? menuItem->palette.highlightedText().color() :
                                     menuItem->palette.buttonText().color());
            int textFlags = Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine |
                                Qt::AlignCenter;
            painter->drawText(textRect, textFlags, menuItem->text);

            painter->restore();
        } else
            QProxyStyle::drawControl(element, option, painter, widget);
    }
};


KrPreviewPopup::KrPreviewPopup() : jobStarted(false)
{
    prevNotAvailAction = addAction(i18n("Preview not available"));

    setStyle(new ProxyStyle());

    connect(this, SIGNAL(triggered(QAction*)), this, SLOT(view(QAction*)));
}

void KrPreviewPopup::showEvent(QShowEvent *event)
{
    QMenu::showEvent(event);

    if (!jobStarted) {
        QStringList allPlugins = KIO::PreviewJob::availablePlugins();
        KIO::PreviewJob *pjob = new KIO::PreviewJob(files, QSize(MAX_SIZE, MAX_SIZE), &allPlugins);
            pjob->setOverlayIconSize(0);
            pjob->setOverlayIconAlpha(1);
            pjob->setScaleType(KIO::PreviewJob::ScaledAndCached);
        connect(pjob, SIGNAL(gotPreview(KFileItem,QPixmap)),
                this, SLOT(addPreview(KFileItem,QPixmap)));
        jobStarted = true;
    }
}

void KrPreviewPopup::setUrls(const QList<QUrl> &urls)
{
    foreach(const QUrl &url, urls) {
        files.push_back(KFileItem(url));
    }
}

void KrPreviewPopup::addPreview(const KFileItem& file, const QPixmap& preview)
{
    if (prevNotAvailAction) {
        removeAction(prevNotAvailAction);
        delete prevNotAvailAction;
        prevNotAvailAction = 0;
    }

    QAction *act = addAction(file.text());
    act->setIcon(QIcon(preview));
    act->setData(QVariant::fromValue(file.url()));
}

void KrPreviewPopup::view(QAction *clicked)
{
    if (clicked && clicked->data().canConvert<QUrl>())
        KrViewer::view(clicked->data().value<QUrl>());
}

