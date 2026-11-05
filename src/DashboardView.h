#pragma once

#include "WindowInfo.h"

#include <QIcon>
#include <QScrollArea>
#include <QStringList>
#include <QVector>

class QHBoxLayout;
class QVBoxLayout;
class GroupWidget;

struct WindowGroup
{
    QString key;
    QString title;
    QIcon icon;
    QVector<WindowInfo> windows;
    bool caseGroup = false;
};

/// Two-column scrollable board of application groups.
class DashboardView : public QScrollArea
{
    Q_OBJECT

public:
    explicit DashboardView(QWidget *parent = nullptr);

    void setGroups(const QVector<WindowGroup> &groups);

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void windowActivated(quintptr handle);
    void orderChanged(const QString &applicationKey, const QStringList &identityOrder);

private:
    void clearGroups();

    QWidget *m_container = nullptr;
    QHBoxLayout *m_columns = nullptr;
    QVBoxLayout *m_leftColumn = nullptr;
    QVBoxLayout *m_rightColumn = nullptr;
    QVector<GroupWidget *> m_groupWidgets;
};


