#pragma once

#include <QString>
#include <QVector>

/// One Explorer window input for path classification.
struct ExplorerPathInfo
{
    QString title;
    QString path;
};

/// Per-window path facts and the related-path section it belongs to.
struct PathGrouping
{
    QString normalizedPath;
    QString parentPath;
    QString pathLabel;
    QString sortPath;
    QString caseId;
    QString sectionKey;
    QString sectionLabel;
};

/// Path rules shared by the dashboard and its unit tests. Everything here is
/// pure: no window handles, no settings, no UI.
namespace PathClassifier
{
QString normalize(const QString &path);
QString leafName(const QString &path);
QString parentPath(const QString &path);
QString folderDisplay(const QString &path);
QString displayLabel(const QString &path, const QString &title);
QString caseIdFromPath(const QString &path);
bool isRootParent(const QString &parentPath);

/// Classifies Explorer paths into related-path sections:
///  * a case number shared by two or more windows wins;
///  * otherwise an identical, non-root parent folder shared by two or more
///    windows wins;
///  * otherwise the row stays unsectioned.
QVector<PathGrouping> classify(const QVector<ExplorerPathInfo> &windows);
}
