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

    clearGroups();

    int leftHeight = 0;
    int rightHeight = 0;

    for (const WindowGroup &group : groups)
    {
        auto *widget = new GroupWidget(group.key, group.title, group.icon, group.windows,
                                       m_container);
        connect(widget, &GroupWidget::windowActivated, this, &DashboardView::windowActivated);
        connect(widget, &GroupWidget::orderChanged, this, &DashboardView::orderChanged);

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

    verticalScrollBar()->setValue(previousScroll);
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
    for (GroupWidget *widget : m_groupWidgets)
    {
        m_leftColumn->removeWidget(widget);
        m_rightColumn->removeWidget(widget);
        widget->hide();
        widget->deleteLater();
    }
    m_groupWidgets.clear();
}




