#include "GroupWidget.h"

#include <QFontMetrics>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QToolTip>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStringList>

#include <cstdlib>

namespace
{
constexpr int kHeaderHeight = 32;
constexpr int kRowHeight = 24;
constexpr int kRadius = 8;
constexpr int kLeftPadding = 16;
constexpr int kTextLeft = 40;
constexpr int kTitlePixelSize = 13;
constexpr int kPathPixelSize = 11;
constexpr int kCountPixelSize = 11;

QString elided(const QFontMetrics &metrics, const QString &text, int width)
{
    return metrics.elidedText(text, Qt::ElideRight, width);
}

QFont titleFont(const QWidget *widget, bool bold)
{
    QFont font = widget->font();
    font.setPixelSize(kTitlePixelSize);
    font.setWeight(bold ? QFont::DemiBold : QFont::Normal);
    return font;
}

QFont pathFont()
{
    QFont font(QStringLiteral("Consolas"));
    font.setPixelSize(kPathPixelSize);
    return font;
}
}

GroupWidget::GroupWidget(QString applicationKey, QString title, QIcon icon,
                         QVector<WindowInfo> windows, QWidget *parent)
    : QWidget(parent)
    , m_applicationKey(std::move(applicationKey))
    , m_title(std::move(title))
    , m_icon(std::move(icon))
    , m_windows(std::move(windows))
{
    setFixedHeight(heightForRows(m_windows.size()));
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::ArrowCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

int GroupWidget::heightForRows(int rows)
{
    return kHeaderHeight + rows * kRowHeight + 4;
}

QString GroupWidget::applicationKey() const
{
    return m_applicationKey;
}

QStringList GroupWidget::identityOrder() const
{
    QStringList order;
    order.reserve(m_windows.size());
    for (const WindowInfo &window : m_windows)
    {
        order.append(window.identityKey);
    }
    return order;
}

void GroupWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF box(0.5, 0.5, width() - 1.0, height() - 1.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 12));
    painter.drawRoundedRect(box.translated(0, 1), kRadius, kRadius);

    painter.setBrush(QColor(QStringLiteral("#FFFFFF")));
    painter.setPen(QPen(QColor(QStringLiteral("#E2E2E2")), 1.0));
    painter.drawRoundedRect(box, kRadius, kRadius);

    const QRect headerIconRect(kLeftPadding - 2, 8, 16, 16);
    m_icon.paint(&painter, headerIconRect, Qt::AlignCenter);

    const QFont headerFont = titleFont(this, true);
    painter.setFont(headerFont);
    painter.setPen(QColor(QStringLiteral("#1A1A1A")));
    const QFontMetrics headerMetrics(headerFont);
    const int countWidth = 40;
    const QString headerText = elided(headerMetrics, m_title,
                                      width() - kTextLeft - countWidth - 24);
    painter.drawText(QRect(kTextLeft, 2, width() - kTextLeft - countWidth - 24, 28),
                     Qt::AlignVCenter | Qt::AlignLeft, headerText);

    painter.setFont(this->font());
    painter.setPen(QColor(QStringLiteral("#8A8A8A")));
    painter.drawText(QRect(width() - countWidth - 16, 2, countWidth, 28),
                     Qt::AlignVCenter | Qt::AlignRight,
                     QString::number(m_windows.size()));

    painter.setPen(QPen(QColor(QStringLiteral("#F0F0F0")), 1.0));
    painter.drawLine(8, kHeaderHeight - 1, width() - 8, kHeaderHeight - 1);

    const QFont rowFont = titleFont(this, false);
    const QFont metricsPathFont = pathFont();
    const QFontMetrics rowMetrics(rowFont);
    const QFontMetrics pathMetrics(metricsPathFont);

    for (int row = 0; row < m_windows.size(); ++row)
    {
        const WindowInfo &window = m_windows.at(row);
        const int rowY = kHeaderHeight + row * kRowHeight;
        const QString &section = window.sectionTitle;
        const QString previousSection =
            row > 0 ? m_windows.at(row - 1).sectionTitle : QString();
        const bool sectionChanged = row > 0 && section != previousSection
            && (!section.isEmpty() || !previousSection.isEmpty());
        if (sectionChanged)
        {
            painter.setPen(QPen(QColor(QStringLiteral("#CDD3DB")), 2.0));
            painter.drawLine(kTextLeft, rowY, width() - 14, rowY);
        }
        const QRectF rowRect(6, rowY + 1, width() - 12, kRowHeight - 2);

        if (row == m_selectedRow)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(QStringLiteral("#EDF1F5")));
            painter.drawRoundedRect(rowRect, 4, 4);

            painter.setBrush(QColor(QStringLiteral("#B26B1F")));
            painter.drawRoundedRect(QRectF(8, rowY + 5, 3, 14), 1.5, 1.5);
        }
        else if (row == m_hoverRow && !m_dragging)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 6));
            painter.drawRoundedRect(rowRect, 4, 4);
        }

        window.icon.paint(&painter, QRect(kLeftPadding, rowY + 4, 16, 16), Qt::AlignCenter);

        painter.setFont(rowFont);
        painter.setPen(QColor(QStringLiteral("#1A1A1A")));

        const int available = width() - kTextLeft - 14;
        int titleWidth = available;
        QString titleText = window.title;

        if (!window.path.isEmpty())
        {
            titleWidth = qMin(360, static_cast<int>(available * 0.55));
            titleText = elided(rowMetrics, window.title, titleWidth);
            const int actualTitleWidth = rowMetrics.horizontalAdvance(titleText);
            const int pathX = kTextLeft + actualTitleWidth + 10;
            const int pathWidth = width() - pathX - 14;

            painter.drawText(QRect(kTextLeft, rowY, actualTitleWidth + 2, kRowHeight),
                             Qt::AlignVCenter | Qt::AlignLeft, titleText);

            if (pathWidth > 20)
            {
                painter.setFont(metricsPathFont);
                painter.setPen(QColor(QStringLiteral("#6B6B6B")));
                painter.drawText(QRect(pathX, rowY, pathWidth, kRowHeight),
                                 Qt::AlignVCenter | Qt::AlignLeft,
                                 pathMetrics.elidedText(window.path, Qt::ElideMiddle, pathWidth));
            }
        }
        else
        {
            painter.drawText(QRect(kTextLeft, rowY, available, kRowHeight),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             elided(rowMetrics, titleText, available));
        }
    }

    if (m_dragging && m_dropRow >= 0)
    {
        const int lineY = kHeaderHeight + m_dropRow * kRowHeight;
        painter.setPen(QPen(QColor(QStringLiteral("#B26B1F")), 2));
        painter.drawLine(12, lineY, width() - 12, lineY);
    }
}

int GroupWidget::rowAt(const QPoint &position) const
{
    if (position.y() < kHeaderHeight)
    {
        return -1;
    }

    const int row = (position.y() - kHeaderHeight) / kRowHeight;
    if (row < 0 || row >= m_windows.size())
    {
        return -1;
    }
    return row;
}

void GroupWidget::selectRow(int row)
{
    if (row == m_selectedRow)
    {
        return;
    }
    m_selectedRow = row;
    emit selectionChanged();
    update();
}

void GroupWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    const int row = rowAt(event->pos());
    m_pressRow = row;
    m_pressY = event->pos().y();
    m_dragging = false;
    m_dropRow = row;

    if (row >= 0)
    {
        setFocus(Qt::MouseFocusReason);
        selectRow(row);
    }
}

void GroupWidget::mouseMoveEvent(QMouseEvent *event)
{
    const int row = rowAt(event->pos());

    if ((event->buttons() & Qt::LeftButton) != 0 && m_pressRow >= 0
        && std::abs(event->pos().y() - m_pressY) > 5)
    {
        m_dragging = true;
        m_dropRow = row >= 0 ? row : qBound(0, (event->pos().y() - kHeaderHeight) / kRowHeight,
                                            m_windows.size() - 1);
        update();
        return;
    }

    if (row != m_hoverRow)
    {
        m_hoverRow = row;
        update();
    }
}

void GroupWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging)
    {
        if (m_dropRow >= 0 && m_pressRow >= 0 && m_dropRow != m_pressRow)
        {
            moveRow(m_pressRow, m_dropRow);
            emit orderChanged(m_applicationKey, identityOrder());
            selectRow(m_dropRow);
        }
        m_dragging = false;
        m_dropRow = -1;
        update();
    }

    m_pressRow = -1;
}

void GroupWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const int row = rowAt(event->pos());
    if (row >= 0)
    {
        emit windowActivated(static_cast<quintptr>(m_windows.at(row).handle));
    }
}

bool GroupWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
    {
        auto *helpEvent = static_cast<QHelpEvent *>(event);
        const int row = rowAt(helpEvent->pos());
        if (row >= 0 && row < m_windows.size() && !m_windows.at(row).path.isEmpty())
        {
            QToolTip::showText(helpEvent->globalPos(), m_windows.at(row).path, this);
        }
        else
        {
            QToolTip::hideText();
            event->ignore();
        }
        return true;
    }

    return QWidget::event(event);
}

void GroupWidget::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && m_selectedRow >= 0 && m_selectedRow < m_windows.size())
    {
        emit windowActivated(static_cast<quintptr>(m_windows.at(m_selectedRow).handle));
        return;
    }

    QWidget::keyPressEvent(event);
}

void GroupWidget::leaveEvent(QEvent *event)
{
    m_hoverRow = -1;
    update();
    QWidget::leaveEvent(event);
}

void GroupWidget::moveRow(int from, int to)
{
    if (from < 0 || to < 0 || from >= m_windows.size() || to >= m_windows.size())
    {
        return;
    }

    m_windows.move(from, to);
    update();
}





