#include "MainWindow.h"

#include "ActivationService.h"
#include "SettingsStore.h"
#include "WindowEnumerator.h"
#include "WinEventWatcher.h"

#include <QApplication>
#include <QButtonGroup>
#include <QHash>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <climits>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_watcher(new WinEventWatcher(this))
    , m_settings(new SettingsStore())
    , m_refreshTimer(new QTimer(this))
{
    setWindowTitle(QStringLiteral("PaneDeck"));
    resize(1280, 800);
    setWindowState(Qt::WindowMaximized);

    auto *central = new QWidget(this);
    central->setStyleSheet(QStringLiteral("background: #F3F3F3;"));
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    m_header = new QWidget(central);
    m_header->setFixedHeight(48);
    m_header->setAttribute(Qt::WA_StyledBackground, true);
    m_header->setStyleSheet(QStringLiteral(
        "background: #FFFFFF; border-bottom: 1px solid #E5E5E5;"));
    m_tabLayout = new QHBoxLayout(m_header);
    m_tabLayout->setContentsMargins(24, 0, 24, 0);
    m_tabLayout->setSpacing(4);
    m_tabLayout->addStretch(1);

    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);

    m_dashboard = new DashboardView(central);
    connect(m_dashboard, &DashboardView::windowActivated, this, &MainWindow::onWindowActivated);
    connect(m_dashboard, &DashboardView::orderChanged, this, &MainWindow::onOrderChanged);

    centralLayout->addWidget(m_header);
    centralLayout->addWidget(m_dashboard, 1);
    setCentralWidget(central);

    m_statusLeft = new QLabel(this);
    m_statusLeft->setStyleSheet(QStringLiteral("color: #5F6368; font-size: 11px;"));
    m_statusRight = new QLabel(
        QStringLiteral("Drag to reorder · Double-click / Enter to activate"), this);
    m_statusRight->setStyleSheet(QStringLiteral("color: #5F6368; font-size: 11px;"));
    statusBar()->setSizeGripEnabled(false);
    statusBar()->setStyleSheet(
        QStringLiteral("QStatusBar { background: #F3F3F3; border-top: 1px solid #E5E5E5; }"));
    statusBar()->addWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusRight, 0);

    connect(m_watcher, &WinEventWatcher::windowActivity, this, &MainWindow::refresh);
    m_watcher->start();

    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(750);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refresh);

    auto *refreshShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(refreshShortcut, &QShortcut::activated, this, &MainWindow::refresh);
}

MainWindow::~MainWindow()
{
    delete m_settings;
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    refresh();
}

void MainWindow::refresh()
{
    if (m_refreshing)
    {
        return;
    }

    if (m_lastRefresh.isValid() && m_lastRefresh.elapsed() < 750)
    {
        m_refreshPending = true;
        if (!m_refreshTimer->isActive())
        {
            m_refreshTimer->start();
        }
        return;
    }

    m_lastRefresh.start();
    m_refreshPending = false;
    m_refreshing = true;

    WindowEnumerator enumerator;
    const QVector<WindowInfo> windows = enumerator.capture();
    m_groups = buildGroups(windows);
    saveWindowOrder(m_groups);

    rebuildTabs();
    applyFilter(m_filterKey);

    int windowCount = 0;
    for (const WindowGroup &group : m_groups)
    {
        windowCount += group.windows.size();
    }
    updateStatus(windowCount, m_groups.size());

    m_refreshing = false;
}

QVector<WindowGroup> MainWindow::buildGroups(const QVector<WindowInfo> &windows) const
{
    QVector<WindowGroup> groups;
    QHash<QString, int> indexByKey;

    for (const WindowInfo &window : windows)
    {
        const QString key = window.executablePath.isEmpty()
            ? window.applicationName
            : window.executablePath;

        auto iterator = indexByKey.find(key);
        if (iterator == indexByKey.end())
        {
            WindowGroup group;
            group.key = key;
            group.title = window.applicationName;
            group.icon = window.icon;
            indexByKey.insert(key, groups.size());
            groups.append(group);
            iterator = indexByKey.find(key);
        }

        WindowGroup &group = groups[iterator.value()];
        group.windows.append(window);
    }

    for (WindowGroup &group : groups)
    {
        std::sort(group.windows.begin(), group.windows.end(),
                  [](const WindowInfo &left, const WindowInfo &right)
                  {
                      return QString::localeAwareCompare(left.title, right.title) < 0;
                  });

        const QStringList savedOrder = m_settings->windowOrder(group.key);
        if (!savedOrder.isEmpty())
        {
            QHash<QString, int> rank;
            for (int index = 0; index < savedOrder.size(); ++index)
            {
                rank.insert(savedOrder.at(index), index);
            }

            std::stable_sort(group.windows.begin(), group.windows.end(),
                             [&rank](const WindowInfo &left, const WindowInfo &right)
                             {
                                 const int leftRank = rank.contains(left.identityKey)
                                     ? rank.value(left.identityKey)
                                     : INT_MAX;
                                 const int rightRank = rank.contains(right.identityKey)
                                     ? rank.value(right.identityKey)
                                     : INT_MAX;
                                 if (leftRank != rightRank)
                                 {
                                     return leftRank < rightRank;
                                 }
                                 return QString::localeAwareCompare(left.title, right.title) < 0;
                             });
        }
    }

    std::sort(groups.begin(), groups.end(),
              [](const WindowGroup &left, const WindowGroup &right)
              {
                  if (left.windows.size() != right.windows.size())
                  {
                      return left.windows.size() > right.windows.size();
                  }
                  return QString::localeAwareCompare(left.title, right.title) < 0;
              });

    return groups;
}
void MainWindow::saveWindowOrder(const QVector<WindowGroup> &groups)
{
    for (const WindowGroup &group : groups)
    {
        if (m_settings->windowOrder(group.key).isEmpty())
        {
            QStringList order;
            order.reserve(group.windows.size());
            for (const WindowInfo &window : group.windows)
            {
                order.append(window.identityKey);
            }
            m_settings->setWindowOrder(group.key, order);
        }
    }
}

void MainWindow::rebuildTabs()
{
    if (!m_filterKey.isEmpty())
    {
        bool stillAvailable = false;
        for (const WindowGroup &group : m_groups)
        {
            if (group.key == m_filterKey && group.windows.size() > 10)
            {
                stillAvailable = true;
                break;
            }
        }
        if (!stillAvailable)
        {
            m_filterKey.clear();
        }
    }

    m_tabGroup->blockSignals(true);
    for (QPushButton *button : m_tabButtons)
    {
        m_tabGroup->removeButton(button);
        delete button;
    }
    m_tabButtons.clear();

    const auto addTab = [this](const QString &text, const QString &key)
    {
        auto *button = new QPushButton(text, m_header);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setObjectName(QStringLiteral("PaneDeckTab"));
        button->setStyleSheet(QStringLiteral(
            "QPushButton#PaneDeckTab { background: transparent; border: none;"
            " border-bottom: 2px solid transparent; padding: 0 10px; color: #5F6368;"
            " font-size: 13px; }"
            "QPushButton#PaneDeckTab:checked { color: #B26B1F;"
            " border-bottom: 2px solid #B26B1F; font-weight: 600; }"));
        m_tabGroup->addButton(button);
        m_tabLayout->insertWidget(m_tabLayout->count() - 1, button);
        connect(button, &QPushButton::clicked, this, [this, key]() { applyFilter(key); });
        m_tabButtons.append(button);
    };

    addTab(QStringLiteral("All"), QString());
    m_tabButtons.first()->setChecked(m_filterKey.isEmpty());

    for (const WindowGroup &group : m_groups)
    {
        if (group.windows.size() <= 10)
        {
            continue;
        }

        addTab(group.title + QStringLiteral("  ") + QString::number(group.windows.size()),
               group.key);
        m_tabButtons.last()->setChecked(group.key == m_filterKey);
    }

    m_tabGroup->blockSignals(false);
}

void MainWindow::applyFilter(const QString &key)
{
    m_filterKey = key;

    QVector<WindowGroup> visible;
    for (const WindowGroup &group : m_groups)
    {
        if (key.isEmpty() || group.key == key)
        {
            visible.append(group);
        }
    }

    m_dashboard->setGroups(visible);
}

void MainWindow::onWindowActivated(quintptr handle)
{
    ActivationService::activate(static_cast<WId>(handle));
    m_dashboard->setFocus();
}

void MainWindow::onOrderChanged(const QString &applicationKey, const QStringList &identityOrder)
{
    m_settings->setWindowOrder(applicationKey, identityOrder);
}

void MainWindow::updateStatus(int windowCount, int appCount)
{
    m_statusLeft->setText(QStringLiteral("%1 windows · %2 apps")
                              .arg(windowCount)
                              .arg(appCount));
}












