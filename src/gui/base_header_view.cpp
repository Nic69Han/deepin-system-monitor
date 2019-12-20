#include <cmath>

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyleHelper>
#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QtMath>

#include "base_header_view.h"

static const int kSpacingMargin = 4;
// recommend size
// static const QSize kDropDownSize {10, 6};
static const QSize kDropDownSize {11, 10};

BaseHeaderView::BaseHeaderView(Qt::Orientation orientation, QWidget *parent)
    : DHeaderView(orientation, parent)
{
    viewport()->setAutoFillBackground(false);
}

QSize BaseHeaderView::sizeHint() const
{
    return QSize(width(), 36 + m_spacing);
}

int BaseHeaderView::sectionSizeHint(int logicalIndex) const
{
    QStyleOptionHeader option;
    initStyleOption(&option);
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    QFontMetrics fm(DApplication::font());
    QString buf = model()->headerData(logicalIndex, Qt::Horizontal, Qt::DisplayRole).toString();
    if (sortIndicatorSection() == logicalIndex) {
        return fm.width(buf) + margin * 3 + kDropDownSize.width();
    } else {
        return fm.width(buf) + margin * 2;
    }
}

void BaseHeaderView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.save();

    DPalette::ColorGroup cg;
#ifdef ENABLE_INACTIVE_DISPLAY
    QWidget *wnd = DApplication::activeWindow();
    if (!wnd) {
        cg = DPalette::Inactive;
    } else {
        cg = DPalette::Active;
    }
#else
    cg = DPalette::Active;
#endif

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());

    QBrush bgBrush(palette.color(cg, DPalette::Base));

    QStyleOptionHeader option;
    initStyleOption(&option);
    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);

    QRect rect = viewport()->rect();
    QRectF clipRect(rect.x(), rect.y(), rect.width(), rect.height() * 2);
    QRectF subRect(rect.x(), rect.y() + rect.height(), rect.width(), rect.height());
    QPainterPath clipPath, subPath;
    clipPath.addRoundedRect(clipRect, radius, radius);
    subPath.addRect(subRect);
    clipPath = clipPath.subtracted(subPath);

    painter.fillPath(clipPath, bgBrush);

    painter.restore();
    DHeaderView::paintEvent(event);
}

void BaseHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(1);

    DPalette::ColorGroup cg;
#ifdef ENABLE_INACTIVE_DISPLAY
    QWidget *wnd = DApplication::activeWindow();
    if (!wnd) {
        cg = DPalette::Inactive;
    } else {
        cg = DPalette::Active;
    }
#else
    cg = DPalette::Active;
#endif

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());

    QStyleOptionHeader option;
    initStyleOption(&option);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    // title
    QRect contentRect(rect.x(), rect.y(), rect.width(), rect.height() - m_spacing);
    QRect hSpacingRect(rect.x(), contentRect.height(), rect.width(),
                       rect.height() - contentRect.height());

    QBrush contentBrush(palette.color(cg, DPalette::Base));
    // horizontal spacing
    //    QBrush hSpacingBrush(palette.color(cg, DPalette::FrameBorder));
    QBrush hSpacingBrush(
        DApplicationHelper::instance()->palette(this).color(DPalette::FrameBorder));
    // vertical spacing
    QBrush vSpacingBrush(palette.color(cg, DPalette::FrameBorder));
    QRectF vSpacingRect(rect.x(), rect.y() + kSpacingMargin, m_spacing,
                        rect.height() - kSpacingMargin * 2);
    QBrush clearBrush(palette.color(cg, DPalette::Window));

    painter->fillRect(hSpacingRect, clearBrush);
    painter->fillRect(hSpacingRect, hSpacingBrush);

    if (visualIndex(logicalIndex) > 0) {
        painter->fillRect(vSpacingRect, clearBrush);
        painter->fillRect(vSpacingRect, vSpacingBrush);
    }

    QPen forground;
    forground.setColor(palette.color(cg, DPalette::Text));
    QRect textRect;
    if (sortIndicatorSection() == logicalIndex) {
        textRect = {contentRect.x() + margin, contentRect.y(),
                    contentRect.width() - margin * 3 - kDropDownSize.width(), contentRect.height()};
    } else {
        textRect = {contentRect.x() + margin, contentRect.y(), contentRect.width() - margin,
                    contentRect.height()};
    }
    QString title = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
    int align = model()->headerData(logicalIndex, orientation(), Qt::TextAlignmentRole).toInt();
    painter->setPen(forground);
    painter->drawText(textRect, static_cast<int>(align), title);

    // sort indicator
    if (isSortIndicatorShown() && logicalIndex == sortIndicatorSection()) {
        QRect sortIndicator(textRect.x() + textRect.width() + margin,
                            textRect.y() + qCeil((textRect.height() - kDropDownSize.height()) / 2.),
                            kDropDownSize.width(), kDropDownSize.height());
        option.rect = sortIndicator;
        if (sortIndicatorOrder() == Qt::DescendingOrder) {
            style->drawPrimitive(DStyle::PE_IndicatorArrowDown, &option, painter, this);
        } else if (sortIndicatorOrder() == Qt::AscendingOrder) {
            style->drawPrimitive(DStyle::PE_IndicatorArrowUp, &option, painter, this);
        }
    }

    painter->restore();
}
