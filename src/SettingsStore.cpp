#include "SettingsStore.h"

#include <QSettings>

QStringList SettingsStore::windowOrder(const QString &applicationKey) const
{
    QSettings settings;
    return settings.value(QStringLiteral("order/windows/") + applicationKey).toStringList();
}

void SettingsStore::setWindowOrder(const QString &applicationKey, const QStringList &order)
{
    QSettings settings;
    settings.setValue(QStringLiteral("order/windows/") + applicationKey, order);
}

QStringList SettingsStore::groupOrder() const
{
    QSettings settings;
    return settings.value(QStringLiteral("order/groups")).toStringList();
}

void SettingsStore::setGroupOrder(const QStringList &order)
{
    QSettings settings;
    settings.setValue(QStringLiteral("order/groups"), order);
}
