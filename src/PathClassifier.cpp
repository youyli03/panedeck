#include "PathClassifier.h"

#include <QHash>
#include <QSet>
#include <QStringList>

namespace
{
QString withoutTrailingSeparators(QString path)
{
    while (path.size() > 1 && path.endsWith(QLatin1Char('\\')))
    {
        path.chop(1);
    }
    return path;
}

/// Collapse repeated separators while keeping the two leading separators of a
/// UNC path. Shell usually returns clean paths, but pasted/manual paths do not.
QString collapseSeparators(const QString &path)
{
    QString result;
    result.reserve(path.size());

    int start = 0;
    if (path.startsWith(QStringLiteral("\\\\")))
    {
        result.append(QStringLiteral("\\\\"));
        start = 2;
    }

    bool previousSeparator = false;
    for (int index = start; index < path.size(); ++index)
    {
        const QChar character = path.at(index);
        if (character == QLatin1Char('\\'))
        {
            if (previousSeparator)
            {
                continue;
            }
            previousSeparator = true;
        }
        else
        {
            previousSeparator = false;
        }
        result.append(character);
    }

    return result;
}

/// Canonical form used for grouping and sorting. Long-path prefixes are
/// stripped, separators are native, and trailing separators are dropped except
/// on drive roots such as "F:\".
QString canonicalPath(const QString &path)
{
    QString normalized = path;
    normalized.replace(QLatin1Char('/'), QLatin1Char('\\'));

    if (normalized.startsWith(QStringLiteral("\\\\?\\UNC\\"), Qt::CaseInsensitive))
    {
        normalized = QStringLiteral("\\\\") + normalized.mid(8);
    }
    else if (normalized.startsWith(QStringLiteral("\\\\?\\")))
    {
        // Only drive paths are safe to strip; Volume{GUID}/GLOBALROOT device
        // paths need their namespace for a correct path and no grouping.
        const QString rest = normalized.mid(4);
        if (rest.size() >= 2 && rest.at(1) == QLatin1Char(':'))
        {
            normalized = rest;
        }
    }
    else if (normalized.startsWith(QStringLiteral("\\\\.\\")))
    {
        const QString rest = normalized.mid(4);
        if (rest.size() >= 2 && rest.at(1) == QLatin1Char(':'))
        {
            normalized = rest;
        }
    }

    normalized = collapseSeparators(normalized);

    if (normalized.size() > 3)
    {
        if (normalized.at(1) == QLatin1Char(':') && normalized.at(2) == QLatin1Char('\\'))
        {
            bool onlySeparators = true;
            for (int index = 3; index < normalized.size(); ++index)
            {
                if (normalized.at(index) != QLatin1Char('\\'))
                {
                    onlySeparators = false;
                    break;
                }
            }
            if (onlySeparators)
            {
                return normalized.left(3);
            }
        }

        normalized = withoutTrailingSeparators(normalized);
    }
    return normalized;
}

QStringList segmentsOf(const QString &path)
{
    return path.split(QLatin1Char('\\'), Qt::SkipEmptyParts);
}

bool isDigits(const QString &value)
{
    if (value.isEmpty())
    {
        return false;
    }

    for (const QChar character : value)
    {
        // Case numbers are ASCII; Arabic-Indic or full-width digits are just
        // folder-name text and must not be treated as ids.
        if (character < QLatin1Char('0') || character > QLatin1Char('9'))
        {
            return false;
        }
    }
    return true;
}

bool validDateParts(int year, int month, int day)
{
    return year >= 1990 && year <= 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

/// Rejects date-looking folder names (20240101, 2024-01-01 without the dashes,
/// 202401, 240101, 2024011) so unrelated dated folders are not paired as a
/// case. Folders below a "case"/"cases" directory override this check.
bool looksLikeDateSegment(const QString &value)
{
    if (!isDigits(value))
    {
        return false;
    }

    if (value.size() == 8)
    {
        return validDateParts(value.left(4).toInt(), value.mid(4, 2).toInt(),
                              value.mid(6, 2).toInt());
    }

    if (value.size() == 7)
    {
        const int year = value.left(4).toInt();
        if (year < 1990 || year > 2100)
        {
            return false;
        }

        const int monthFirst = value.mid(4, 1).toInt();
        const int dayFirst = value.mid(5, 2).toInt();
        if (monthFirst >= 1 && monthFirst <= 12 && dayFirst >= 1 && dayFirst <= 31)
        {
            return true;
        }

        const int monthSecond = value.mid(4, 2).toInt();
        const int daySecond = value.mid(6, 1).toInt();
        return monthSecond >= 1 && monthSecond <= 12 && daySecond >= 1 && daySecond <= 31;
    }

    if (value.size() == 6)
    {
        // YYYYMM
        if (validDateParts(value.left(4).toInt(), value.mid(4, 2).toInt(), 1))
        {
            return true;
        }

        // YYMMDD
        return validDateParts(2000 + value.left(2).toInt(), value.mid(2, 2).toInt(),
                              value.mid(4, 2).toInt());
    }

    return false;
}

bool isCaseParent(const QString &segment)
{
    return segment.compare(QStringLiteral("case"), Qt::CaseInsensitive) == 0
        || segment.compare(QStringLiteral("cases"), Qt::CaseInsensitive) == 0;
}

/// Device/volume namespaces (\\?\Volume{...}, \\.\COM1, ...) have no usable
/// folder hierarchy; leave them ungrouped.
bool isDevicePath(const QString &canonical)
{
    return canonical.startsWith(QStringLiteral("\\\\?\\"))
        || canonical.startsWith(QStringLiteral("\\\\.\\"));
}

QString parentPathImpl(const QString &canonical)
{
    if (isDevicePath(canonical))
    {
        return {};
    }

    if (canonical.size() == 3 && canonical.at(1) == QLatin1Char(':')
        && canonical.at(2) == QLatin1Char('\\'))
    {
        return {};
    }

    // A server/share root has no meaningful parent folder.
    if (canonical.startsWith(QStringLiteral("\\\\"))
        && segmentsOf(canonical).size() <= 2)
    {
        return {};
    }

    const int separator = canonical.lastIndexOf(QLatin1Char('\\'));
    if (separator <= 0)
    {
        return {};
    }

    const QString parent = canonical.left(separator);
    if (parent.startsWith(QStringLiteral("\\\\"))
        && segmentsOf(parent).size() < 2)
    {
        return {};
    }
    return parent;
}

QString leafNameImpl(const QString &canonical)
{
    const int separator = canonical.lastIndexOf(QLatin1Char('\\'));
    if (separator < 0)
    {
        return {};
    }

    const QString leaf = canonical.mid(separator + 1);
    if (leaf.size() == 2 && leaf.at(1) == QLatin1Char(':'))
    {
        return {};
    }
    return leaf;
}

bool isRootParentImpl(const QString &parent)
{
    if (parent.isEmpty())
    {
        return true;
    }

    if (parent.size() == 2 && parent.at(1) == QLatin1Char(':'))
    {
        return true;
    }

    if (parent.startsWith(QStringLiteral("\\\\")))
    {
        return segmentsOf(parent).size() <= 2;
    }

    return false;
}

QString displayLabelImpl(const QString &canonical, const QString &title)
{
    const QString leaf = leafNameImpl(canonical);
    if (!leaf.isEmpty() && title.compare(leaf, Qt::CaseInsensitive) == 0)
    {
        const QString parent = parentPathImpl(canonical);
        if (!parent.isEmpty())
        {
            return parent + QLatin1Char('\\');
        }
    }
    return canonical;
}

struct CaseCandidate
{
    QString id;
    bool parentIsCase = false;
    bool dateLike = false;
    int score = -1;
};

QVector<CaseCandidate> caseCandidates(const QString &canonical)
{
    QVector<CaseCandidate> candidates;
    if (isDevicePath(canonical))
    {
        return candidates;
    }

    const QStringList segments = segmentsOf(canonical);
    const bool unc = canonical.startsWith(QStringLiteral("\\\\"));
    // The first two segments of a UNC path are the server and share names,
    // never case folders (a numeric host/share must not become a case id).
    const int firstIndex = unc ? 2 : 0;

    for (int index = firstIndex; index < segments.size(); ++index)
    {
        const QString &segment = segments.at(index);
        if (segment.size() < 6 || segment.size() > 8 || !isDigits(segment))
        {
            continue;
        }

        CaseCandidate candidate;
        candidate.id = segment;
        candidate.parentIsCase =
            index > firstIndex && isCaseParent(segments.at(index - 1));
        candidate.dateLike = looksLikeDateSegment(segment);
        // A case/cases parent is decisive; otherwise a longer id wins (an
        // 8-digit case beats a 6-digit numeric subfolder), and for equal
        // lengths the deepest segment wins, i.e. the folder closest to the
        // window's own leaf. Depth is capped so it can never outweigh length.
        candidate.score = (candidate.parentIsCase ? 1000 : 0) + segment.size() * 10
            + qMin(index, 9);
        candidates.append(candidate);
    }

    return candidates;
}

CaseCandidate bestCandidate(const QVector<CaseCandidate> &candidates, bool allowDateLike)
{
    CaseCandidate best;
    for (const CaseCandidate &candidate : candidates)
    {
        if (!allowDateLike && candidate.dateLike && !candidate.parentIsCase)
        {
            continue;
        }

        if (candidate.score > best.score)
        {
            best = candidate;
        }
    }
    return best;
}

QString caseIdImpl(const QString &canonical)
{
    return bestCandidate(caseCandidates(canonical), false).id;
}
}

QString PathClassifier::normalize(const QString &path)
{
    return canonicalPath(path);
}

QString PathClassifier::leafName(const QString &path)
{
    return leafNameImpl(canonicalPath(path));
}

QString PathClassifier::parentPath(const QString &path)
{
    return parentPathImpl(canonicalPath(path));
}

QString PathClassifier::folderDisplay(const QString &path)
{
    if (path.isEmpty() || path.endsWith(QLatin1Char('\\')))
    {
        return path;
    }
    return path + QLatin1Char('\\');
}

QString PathClassifier::displayLabel(const QString &path, const QString &title)
{
    return displayLabelImpl(canonicalPath(path), title);
}

QString PathClassifier::caseIdFromPath(const QString &path)
{
    return caseIdImpl(canonicalPath(path));
}

bool PathClassifier::isRootParent(const QString &parentPath)
{
    return isRootParentImpl(withoutTrailingSeparators(parentPath));
}

QVector<PathGrouping> PathClassifier::classify(const QVector<ExplorerPathInfo> &windows)
{
    QVector<PathGrouping> groupings(windows.size());
    QHash<QString, int> caseCounts;
    QHash<QString, int> parentCounts;
    QVector<CaseCandidate> lenientCandidates(windows.size());
    QSet<QString> confirmedCaseIds;

    for (int index = 0; index < windows.size(); ++index)
    {
        const QString canonical = canonicalPath(windows.at(index).path);
        PathGrouping &grouping = groupings[index];
        grouping.normalizedPath = canonical;
        grouping.sortPath = canonical;

        if (isDevicePath(canonical))
        {
            // Keep the full device path; no folder hierarchy to classify.
            grouping.pathLabel = canonical;
            continue;
        }

        grouping.parentPath = parentPathImpl(canonical);
        grouping.pathLabel = displayLabelImpl(canonical, windows.at(index).title);

        const QVector<CaseCandidate> candidates = caseCandidates(canonical);
        const CaseCandidate strict = bestCandidate(candidates, false);
        lenientCandidates[index] = bestCandidate(candidates, true);
        grouping.caseId = strict.id;

        // A number under case/cases confirms itself even when it looks like a
        // date; sibling folders with that number may then join it.
        if (strict.parentIsCase && !strict.id.isEmpty())
        {
            confirmedCaseIds.insert(strict.id);
        }
    }

    for (int index = 0; index < groupings.size(); ++index)
    {
        if (groupings.at(index).caseId.isEmpty()
            && confirmedCaseIds.contains(lenientCandidates.at(index).id))
        {
            groupings[index].caseId = lenientCandidates.at(index).id;
        }
    }

    for (const PathGrouping &grouping : groupings)
    {
        if (!grouping.caseId.isEmpty())
        {
            caseCounts[grouping.caseId] += 1;
        }
    }

    for (const PathGrouping &grouping : groupings)
    {
        if (!grouping.caseId.isEmpty() && caseCounts.value(grouping.caseId) > 1)
        {
            continue;
        }

        // A drive root or a share root is too broad to count as "related".
        if (grouping.parentPath.isEmpty() || isRootParentImpl(grouping.parentPath))
        {
            continue;
        }

        parentCounts[grouping.parentPath.toLower()] += 1;
    }

    for (PathGrouping &grouping : groupings)
    {
        if (!grouping.caseId.isEmpty() && caseCounts.value(grouping.caseId) > 1)
        {
            grouping.sectionKey = QStringLiteral("C")
                + grouping.caseId.rightJustified(8, QLatin1Char('0')) + QLatin1Char(':')
                + grouping.caseId;
            grouping.sectionLabel = QStringLiteral("Case ") + grouping.caseId;
            continue;
        }

        if (grouping.parentPath.isEmpty() || isRootParentImpl(grouping.parentPath))
        {
            continue;
        }

        if (parentCounts.value(grouping.parentPath.toLower()) > 1)
        {
            grouping.sectionKey = QStringLiteral("P") + grouping.parentPath.toLower();
            grouping.sectionLabel = grouping.parentPath + QLatin1Char('\\');
            if (grouping.pathLabel.compare(grouping.sectionLabel, Qt::CaseInsensitive) == 0)
            {
                grouping.pathLabel.clear();
            }
        }
    }

    return groupings;
}
