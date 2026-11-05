#include "DashboardView.h"

#include "GroupWidget.h"

#include <QHBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>

DashboardView::DashboardView(QWidget *parent)
    : QScrollArea(parent)
{
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::NoFocus);
    viewport()->setFocusPolicy(Qt::NoFocus);
    setStyleSheet(QStringLiteral("QScrollArea { background: #F3F3F3; }"));
    viewport()->setStyleSheet(QStringLiteral("background: #F3F3F3;"));

    m_container = new QWidget(this);
    m_container->setStyleSheet(QStringLiteral("background: #F3F3F3;"));
    setWidget(m_container);

    m_columns = new QHBoxLayout(m_container);
    m_columns->setContentsMargins(24, 16, 24, 0);
    m_columns->setSpacing(16);

    m_leftColumn = new QVBoxLayout();
    m_leftColumn->setContentsMargins(0, 0, 0, 0);
    m_leftColumn->setSpacing(8);
    m_leftColumn->setAlignment(Qt::AlignTop);
    m_leftColumn->addStretch(1);

    m_rightColumn = new QVBoxLayout();
    m_rightColumn->setContentsMargins(0, 0, 0, 0);
    m_rightColumn->setSpacing(8);
    m_rightColumn->setAlignment(Qt::AlignTop);
    m_rightColumn->addStretch(1);

    m_columns->addLayout(m_leftColumn, 1);
    m_columns->addLayout(m_rightColumn, 1);
}

void DashboardView::setGroups(const QVector<WindowGroup> &groups)
{
    const int previousScroll = verticalScrollBar()->value();

    QWidget *previousFocus = focusWidget();
    bool groupHadFocus = false;
    QString focusedGroupKey;
    for (GroupWidget *widget : m_groupWidgets)
    {
        if (previousFocus != nullptr
            && (previousFocus == widget || widget->isAncestorOf(previousFocus)))
        {
            groupHadFocus = true;
            focusedGroupKey = widget->applicationKey();
            break;
        }
    }

    clearGroups();

    int leftHeight = 0;
    int rightHeight = 0;

    for (const WindowGroup &group : groups)
    {
        auto *widget = new GroupWidget(group.key, group.title, group.icon, group.windows,
                                       m_container);
        connect(widget, &GroupWidget::windowActivated, this, &DashboardView::windowActivated);
        connect(widget, &GroupWidget::orderChanged, this, &DashboardView::orderChanged);
        connect(widget, &GroupWidget::selectionChanged, this,
                [this, widget](quintptr handle)
                {
                    // Only one window is "the current context" at a time.
                    if (m_selectedWidget != nullptr && m_selectedWidget != widget)
                    {
                        m_selectedWidget->clearSelection();
                    }
                    m_selectedWidget = handle != 0 ? widget : nullptr;
                    m_selectedHandle = static_cast<WId>(handle);
                    if (handle != 0 && widget->hasFocus())
                    {
                        revealRow(widget);
                    }
                });

        const int groupHeight = widget->height() + 8;
        if (leftHeight <= rightHeight)
        {
            m_leftColumn->insertWidget(m_leftColumn->count() - 1, widget);
            leftHeight += groupHeight;
        }
        else
        {
            m_rightColumn->insertWidget(m_rightColumn->count() - 1, widget);
            rightHeight += groupHeight;
        }

        m_groupWidgets.append(widget);
    }

    // Content changes (new folders appearing, titles changing) rebuild every
    // row; keep the window the user was on selected, and keep the keyboard
    // focus in the board, so context does not jump on a refresh.
    GroupWidget *selectedWidget = nullptr;
    if (m_selectedHandle != 0)
    {
        for (GroupWidget *widget : m_groupWidgets)
        {
            if (widget->selectByHandle(m_selectedHandle))
            {
                selectedWidget = widget;
                break;
            }
        }

        if (selectedWidget == nullptr)
        {
            m_selectedHandle = 0;
            m_selectedWidget = nullptr;
        }
    }

    if (groupHadFocus)
    {
        // Focus follows the group the user was in, not necessarily the group
        // that still holds the selected row (e.g. after tabbing to a group).
        GroupWidget *focusTarget = nullptr;
        for (GroupWidget *widget : m_groupWidgets)
        {
            if (widget->applicationKey() == focusedGroupKey)
            {
                focusTarget = widget;
                break;
            }
        }

        if (focusTarget == nullptr)
        {
            focusTarget = selectedWidget;
        }
        if (focusTarget == nullptr && !m_groupWidgets.isEmpty())
        {
            focusTarget = m_groupWidgets.first();
        }
        if (focusTarget != nullptr)
        {
            focusTarget->setFocus(Qt::OtherFocusReason);
        }
    }

    if (m_container->layout() != nullptr)
    {
        m_container->layout()->activate();
    }
    verticalScrollBar()->setValue(previousScroll);

    // Rows above may have shifted (or a group moved columns): keep the
    // restored selection visible when the board owns the keyboard.
    if (selectedWidget != nullptr && selectedWidget->hasFocus())
    {
        revealRow(selectedWidget);
    }
}

void DashboardView::focusFirstGroup()
{
    if (!m_groupWidgets.isEmpty())
    {
        m_groupWidgets.first()->setFocus(Qt::OtherFocusReason);
    }
}

void DashboardView::focusSelectedGroup()
{
    if (m_selectedHandle != 0)
    {
        for (GroupWidget *widget : m_groupWidgets)
        {
            if (widget->selectByHandle(m_selectedHandle))
            {
                widget->setFocus(Qt::OtherFocusReason);
                revealRow(widget);
                return;
            }
        }
    }

    focusFirstGroup();
}

void DashboardView::revealRow(GroupWidget *widget)
{
    const QRect row = widget->selectedRowRect();
    if (row.isEmpty())
    {
        return;
    }

    const QPoint topLeft = widget->mapTo(m_container, row.topLeft());
    const int margin = 6;
    const int rowTop = topLeft.y();
    const int rowBottom = rowTop + row.height();
    const int visibleTop = verticalScrollBar()->value();
    const int visibleBottom = visibleTop + viewport()->height();

    if (rowTop < visibleTop + margin)
    {
        verticalScrollBar()->setValue(rowTop - margin);
    }
    else if (rowBottom > visibleBottom - margin)
    {
        verticalScrollBar()->setValue(rowBottom - viewport()->height() + margin);
    }
}

void DashboardView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    if (m_columns != nullptr)
    {
        const int sideMargin = qMax(24, (viewport()->width() - 1600) / 2);
        m_columns->setContentsMargins(sideMargin, 16, sideMargin, 0);
    }
}

void DashboardView::clearGroups()
{
    m_selectedWidget = nullptr;
    for (GroupWidget *widget : m_groupWidgets)
    {
        m_leftColumn->removeWidget(widget);
        m_rightColumn->removeWidget(widget);
        widget->hide();
        widget->deleteLater();
    }
    m_groupWidgets.clear();
}




