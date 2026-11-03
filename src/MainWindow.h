#pragma once

#include "DashboardView.h"

#include <QMainWindow>
#include <QStringList>
#include <QVector>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QButtonGroup;
class WinEventWatcher;
class SettingsStore;

/// Normal desktop window hosting the two-column dashboard.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void refresh();
    void onWindowActivated(quintptr handle);
    void onOrderChanged(const QString &applicationKey, const QStringList &identityOrder);

private:
    void rebuildTabs();
    void applyFilter(const QString &key);
    void saveWindowOrder(const QVector<WindowGroup> &groups);
    QVector<WindowGroup> buildGroups(const QVector<WindowInfo> &windows) const;
    void updateStatus(int windowCount, int appCount);

    QWidget *m_header = nullptr;
    QHBoxLayout *m_tabLayout = nullptr;
    QButtonGroup *m_tabGroup = nullptr;
    DashboardView *m_dashboard = nullptr;
    QLabel *m_statusLeft = nullptr;
    QLabel *m_statusRight = nullptr;
    WinEventWatcher *m_watcher = nullptr;
    SettingsStore *m_settings = nullptr;

    QVector<WindowGroup> m_groups;
    QString m_filterKey;
    QVector<QPushButton *> m_tabButtons;
    bool m_refreshing = false;
};

