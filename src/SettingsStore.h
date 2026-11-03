#pragma once

#include <QStringList>

/// Persists user ordering with QSettings.
class SettingsStore
{
public:
    QStringList windowOrder(const QString &applicationKey) const;
    void setWindowOrder(const QString &applicationKey, const QStringList &order);
    QStringList groupOrder() const;
    void setGroupOrder(const QStringList &order);
};
