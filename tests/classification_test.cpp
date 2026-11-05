#include "PathClassifier.h"

#include <QString>
#include <QVector>

#include <cstdio>
#include <initializer_list>

namespace
{
int gFailures = 0;

void checkTrue(int line, const char *expression, bool value)
{
    if (!value)
    {
        ++gFailures;
        std::fprintf(stderr, "line %d: FAILED %s\n", line, expression);
    }
}

void checkEqual(int line, const char *expression, const QString &actual, const QString &expected)
{
    if (actual != expected)
    {
        ++gFailures;
        std::fprintf(stderr, "line %d: FAILED %s\n  expected: %s\n  actual:   %s\n", line,
                     expression, expected.toUtf8().constData(), actual.toUtf8().constData());
    }
}

QVector<PathGrouping> classify(std::initializer_list<ExplorerPathInfo> windows)
{
    QVector<ExplorerPathInfo> input;
    input.reserve(static_cast<int>(windows.size()));
    for (const ExplorerPathInfo &window : windows)
    {
        input.append(window);
    }
    return PathClassifier::classify(input);
}

const PathGrouping &at(const QVector<PathGrouping> &groupings, int index)
{
    return groupings.at(index);
}
}

#define CHECK(condition) checkTrue(__LINE__, #condition, (condition))
#define CHECK_EQ(actual, expected) checkEqual(__LINE__, #actual, (actual), (expected))

int main()
{
    // Canonical paths and path parts.
    CHECK_EQ(PathClassifier::normalize("f:/Foo/bar/"), QStringLiteral("f:\\Foo\\bar"));
    CHECK_EQ(PathClassifier::normalize("G:\\"), QStringLiteral("G:\\"));
    CHECK_EQ(PathClassifier::normalize(QStringLiteral("G:\\\\")), QStringLiteral("G:\\"));
    CHECK_EQ(PathClassifier::normalize(QStringLiteral("F:\\\\a\\\\b")), QStringLiteral("F:\\a\\b"));
    CHECK_EQ(PathClassifier::normalize(QStringLiteral("\\\\server\\\\share")),
             QStringLiteral("\\\\server\\share"));
    CHECK_EQ(PathClassifier::normalize("\\\\?\\F:\\dir\\"), QStringLiteral("F:\\dir"));
    CHECK_EQ(PathClassifier::normalize("\\\\?\\UNC\\server\\share\\dir"),
             QStringLiteral("\\\\server\\share\\dir"));
    CHECK_EQ(PathClassifier::normalize("\\\\.\\F:\\x"), QStringLiteral("F:\\x"));

    CHECK_EQ(PathClassifier::leafName("F:\\project\\x"), QStringLiteral("x"));
    CHECK_EQ(PathClassifier::leafName("F:\\project\\x\\"), QStringLiteral("x"));
    CHECK_EQ(PathClassifier::leafName("G:\\"), QString());
    CHECK_EQ(PathClassifier::leafName("\\\\server\\share"), QStringLiteral("share"));

    CHECK_EQ(PathClassifier::parentPath("F:\\project\\PaneDeck\\build"),
             QStringLiteral("F:\\project\\PaneDeck"));
    CHECK_EQ(PathClassifier::parentPath("F:\\Download"), QStringLiteral("F:"));
    CHECK_EQ(PathClassifier::parentPath("F:\\project\\PaneDeck\\build\\"),
             QStringLiteral("F:\\project\\PaneDeck"));
    CHECK_EQ(PathClassifier::parentPath("\\\\server\\share\\dir"),
             QStringLiteral("\\\\server\\share"));
    CHECK_EQ(PathClassifier::parentPath("\\\\server\\share"), QString());
    CHECK_EQ(PathClassifier::parentPath("\\\\server"), QString());
    CHECK_EQ(PathClassifier::parentPath("G:\\"), QString());
    CHECK_EQ(PathClassifier::folderDisplay(QStringLiteral("F:")), QStringLiteral("F:\\"));

    // Title/path pairing: the row carries the leaf title, so show its parent.
    CHECK_EQ(PathClassifier::displayLabel("F:\\project\\PaneDeck\\build", QStringLiteral("build")),
             QStringLiteral("F:\\project\\PaneDeck\\"));
    CHECK_EQ(PathClassifier::displayLabel("F:\\Download", QStringLiteral("download")),
             QStringLiteral("F:\\"));
    CHECK_EQ(PathClassifier::displayLabel("G:\\", QStringLiteral("可移动磁盘 (G:)")),
             QStringLiteral("G:\\"));
    CHECK_EQ(PathClassifier::displayLabel("F:\\custom\\display", QStringLiteral("Other")),
             QStringLiteral("F:\\custom\\display"));
    CHECK_EQ(PathClassifier::displayLabel("\\\\server\\share\\folder", QStringLiteral("folder")),
             QStringLiteral("\\\\server\\share\\"));

    // Case ids: 6-8 digits, date-looking numbers are ignored unless the folder
    // sits below case/cases.
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\case\\08000000"), QStringLiteral("08000000"));
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\other\\08000000"), QStringLiteral("08000000"));
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\case\\20240101"), QStringLiteral("20240101"));
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\20240101"), QString());
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\202401"), QString());
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\240101"), QString());
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\2024011"), QString());
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\08000000"), QStringLiteral("08000000"));
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\12345"), QString());
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\123456789"), QString());
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\123456"), QStringLiteral("123456"));
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\version1\\x"), QString());
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\cases\\08000000\\deep\\123456789"),
             QStringLiteral("08000000"));
    // Same length, no case/cases parent: the segment closest to the leaf wins.
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\08000000\\a\\08000001"),
             QStringLiteral("08000001"));
    // A longer id still beats a deeper shorter one.
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\data\\123456\\08000000\\sub"),
             QStringLiteral("08000000"));
    // A case/cases parent beats a deeper numeric child.
    CHECK_EQ(PathClassifier::caseIdFromPath("F:\\case\\08000000\\08000001"),
             QStringLiteral("08000000"));

    // Two roots, one case number: paired paths under one case section.
    {
        const auto groupings = classify({
            {QStringLiteral("08000000"), QStringLiteral("F:\\case\\08000000")},
            {QStringLiteral("08000000"), QStringLiteral("F:\\other\\08000000")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("Case 08000000"));
        CHECK_EQ(at(groupings, 0).pathLabel, QStringLiteral("F:\\case\\"));
        CHECK_EQ(at(groupings, 1).pathLabel, QStringLiteral("F:\\other\\"));
    }

    // A single window is not a case section.
    {
        const auto groupings = classify({
            {QStringLiteral("08000001"), QStringLiteral("F:\\case\\08000001")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QString());
        CHECK_EQ(at(groupings, 0).sectionLabel, QString());
        CHECK_EQ(at(groupings, 0).pathLabel, QStringLiteral("F:\\case\\"));
    }

    // Same non-root parent: the header carries the folder once.
    {
        const auto groupings = classify({
            {QStringLiteral("build"), QStringLiteral("F:\\project\\PaneDeck\\build")},
            {QStringLiteral("src"), QStringLiteral("F:\\project\\PaneDeck\\src")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("Pf:\\project\\panedeck"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("Pf:\\project\\panedeck"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("F:\\project\\PaneDeck\\"));
        CHECK_EQ(at(groupings, 0).pathLabel, QString());
        CHECK_EQ(at(groupings, 1).pathLabel, QString());
    }

    // Case sections win over shared-parent sections.
    {
        const auto groupings = classify({
            {QStringLiteral("08000000"), QStringLiteral("F:\\cases\\08000000")},
            {QStringLiteral("08000000"), QStringLiteral("F:\\cases\\08000000")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("Case 08000000"));
    }

    // A shared drive root is too broad to be a "related folder" section.
    {
        const auto groupings = classify({
            {QStringLiteral("dev"), QStringLiteral("F:\\dev")},
            {QStringLiteral("Download"), QStringLiteral("F:\\Download")},
            {QStringLiteral("08000000"), QStringLiteral("G:\\")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QString());
        CHECK_EQ(at(groupings, 1).sectionKey, QString());
        CHECK_EQ(at(groupings, 2).sectionKey, QString());
        CHECK_EQ(at(groupings, 0).pathLabel, QStringLiteral("F:\\"));
        CHECK_EQ(at(groupings, 1).pathLabel, QStringLiteral("F:\\"));
        CHECK_EQ(at(groupings, 2).pathLabel, QStringLiteral("G:\\"));
    }

    // Dates with no case/cases parent stay unrelated.
    {
        const auto groupings = classify({
            {QStringLiteral("20240101"), QStringLiteral("D:\\a\\20240101")},
            {QStringLiteral("20240101"), QStringLiteral("E:\\b\\20240101")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QString());
        CHECK_EQ(at(groupings, 1).sectionKey, QString());
    }

    // Date-shaped folder below case/cases is still a case.
    {
        const auto groupings = classify({
            {QStringLiteral("20240101"), QStringLiteral("F:\\case\\20240101")},
            {QStringLiteral("20240101"), QStringLiteral("F:\\other\\20240101")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C20240101:20240101"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("Case 20240101"));
    }

    // Parent comparison is case-insensitive, as on Windows.
    {
        const auto groupings = classify({
            {QStringLiteral("build"), QStringLiteral("F:\\Proj\\Sub\\build")},
            {QStringLiteral("src"), QStringLiteral("f:\\proj\\sub\\src")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("Pf:\\proj\\sub"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("Pf:\\proj\\sub"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("F:\\Proj\\Sub\\"));
    }

    // UNC: a share root is not a section, a folder below it is.
    {
        const auto groupings = classify({
            {QStringLiteral("a"), QStringLiteral("\\\\srv\\share\\a")},
            {QStringLiteral("b"), QStringLiteral("\\\\srv\\share\\b")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QString());
        CHECK_EQ(at(groupings, 0).pathLabel, QStringLiteral("\\\\srv\\share\\"));
    }
    {
        const auto groupings = classify({
            {QStringLiteral("a"), QStringLiteral("\\\\srv\\share\\proj\\a")},
            {QStringLiteral("b"), QStringLiteral("\\\\srv\\share\\proj\\b")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("P\\\\srv\\share\\proj"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("\\\\srv\\share\\proj\\"));
    }

    // Long-path prefixes and mixed separators are canonicalised before use.
    {
        const auto groupings = classify({
            {QStringLiteral("a"), QStringLiteral("\\\\?\\F:\\Proj\\Sub\\a")},
            {QStringLiteral("b"), QStringLiteral("F:/Proj/Sub/b")},
        });
        CHECK_EQ(at(groupings, 0).normalizedPath, QStringLiteral("F:\\Proj\\Sub\\a"));
        CHECK_EQ(at(groupings, 1).normalizedPath, QStringLiteral("F:\\Proj\\Sub\\b"));
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("Pf:\\proj\\sub"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("Pf:\\proj\\sub"));
    }

    // Unicode folder names still pair by parent.
    {
        const auto groupings = classify({
            {QStringLiteral("甲"), QStringLiteral("F:\\案例\\甲")},
            {QStringLiteral("乙"), QStringLiteral("F:\\案例\\乙")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("Pf:\\案例"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("F:\\案例\\"));
    }

    // Duplicate windows on one folder stay separate rows in one section.
    {
        const auto groupings = classify({
            {QStringLiteral("same"), QStringLiteral("F:\\Work\\same")},
            {QStringLiteral("same"), QStringLiteral("F:\\Work\\same")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("Pf:\\work"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("Pf:\\work"));
        CHECK_EQ(at(groupings, 0).pathLabel, QString());
    }

    // Case ids directly under a root still pair; roots are fine for cases.
    {
        const auto groupings = classify({
            {QStringLiteral("a"), QStringLiteral("F:\\08000000\\a")},
            {QStringLiteral("b"), QStringLiteral("F:\\08000000\\b")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 0).pathLabel, QStringLiteral("F:\\08000000\\"));
    }

    // A date-looking number confirmed below case/cases lets a sibling outside
    // case/cases join the same pair.
    {
        const auto groupings = classify({
            {QStringLiteral("20240101"), QStringLiteral("F:\\case\\20240101")},
            {QStringLiteral("20240101"), QStringLiteral("F:\\other\\20240101")},
        });
        CHECK_EQ(at(groupings, 0).caseId, QStringLiteral("20240101"));
        CHECK_EQ(at(groupings, 1).caseId, QStringLiteral("20240101"));
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C20240101:20240101"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C20240101:20240101"));
    }

    // Empty / unresolved paths never crash or join a section.
    {
        const auto groupings = classify({
            {QStringLiteral("This PC"), QString()},
        });
        CHECK_EQ(at(groupings, 0).normalizedPath, QString());
        CHECK_EQ(at(groupings, 0).sectionKey, QString());
        CHECK_EQ(at(groupings, 0).pathLabel, QString());
    }

    // Longer case ids win inside one path; shorter/date ones are ignored.
    {
        const auto groupings = classify({
            {QStringLiteral("x"), QStringLiteral("F:\\data\\123456\\08000000\\sub")},
        });
        CHECK_EQ(at(groupings, 0).caseId, QStringLiteral("08000000"));
    }

    // case/cases is matched without regard to case.
    {
        const auto groupings = classify({
            {QStringLiteral("08000000"), QStringLiteral("F:\\CASE\\08000000")},
            {QStringLiteral("08000000"), QStringLiteral("F:\\other\\08000000")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C08000000:08000000"));
    }
    {
        const auto groupings = classify({
            {QStringLiteral("20240101"), QStringLiteral("F:\\Cases\\20240101")},
            {QStringLiteral("20240101"), QStringLiteral("F:\\mirror\\20240101")},
        });
        CHECK_EQ(at(groupings, 1).caseId, QStringLiteral("20240101"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C20240101:20240101"));
    }

    // Non-ASCII digits are folder text, not case ids.
    CHECK_EQ(PathClassifier::caseIdFromPath(QStringLiteral("F:\\case\\٠٨٠٠٠٠٠٠")), QString());
    CHECK_EQ(PathClassifier::caseIdFromPath(QStringLiteral("F:\\case\\０８００００００")), QString());

    // A single case window can still share a parent section with a sibling.
    {
        const auto groupings = classify({
            {QStringLiteral("08000000"), QStringLiteral("F:\\case\\08000000")},
            {QStringLiteral("notes"), QStringLiteral("F:\\case\\notes")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("Pf:\\case"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("Pf:\\case"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("F:\\case\\"));
        CHECK_EQ(at(groupings, 0).pathLabel, QString());
        CHECK_EQ(at(groupings, 1).pathLabel, QString());
    }

    // A parent used by one window alone is not a section.
    {
        const auto groupings = classify({
            {QStringLiteral("build"), QStringLiteral("F:\\proj\\PaneDeck\\build")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QString());
        CHECK_EQ(at(groupings, 0).pathLabel, QStringLiteral("F:\\proj\\PaneDeck\\"));
    }

    // A case folder and a subfolder of it share the case section.
    {
        const auto groupings = classify({
            {QStringLiteral("08000000"), QStringLiteral("F:\\08000000")},
            {QStringLiteral("sub"), QStringLiteral("F:\\08000000\\sub")},
        });
        CHECK_EQ(at(groupings, 0).caseId, QStringLiteral("08000000"));
        CHECK_EQ(at(groupings, 1).caseId, QStringLiteral("08000000"));
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C08000000:08000000"));
    }

    // The case number may be the leaf for one window and a middle segment for
    // another; position does not matter, they are the same case.
    {
        const auto groupings = classify({
            {QStringLiteral("08000000"), QStringLiteral("F:\\case\\08000000")},
            {QStringLiteral("logs"), QStringLiteral("F:\\other\\08000000\\logs")},
            {QStringLiteral("b"), QStringLiteral("F:\\third\\08000000\\a\\b")},
        });
        CHECK_EQ(at(groupings, 0).caseId, QStringLiteral("08000000"));
        CHECK_EQ(at(groupings, 1).caseId, QStringLiteral("08000000"));
        CHECK_EQ(at(groupings, 2).caseId, QStringLiteral("08000000"));
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 2).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 1).pathLabel, QStringLiteral("F:\\other\\08000000\\"));
        CHECK_EQ(at(groupings, 2).pathLabel, QStringLiteral("F:\\third\\08000000\\a\\"));
    }

    // A date-like case in the middle joins a case/cases-anchored sibling.
    {
        const auto groupings = classify({
            {QStringLiteral("20240101"), QStringLiteral("F:\\case\\20240101")},
            {QStringLiteral("logs"), QStringLiteral("F:\\other\\20240101\\logs")},
        });
        CHECK_EQ(at(groupings, 0).caseId, QStringLiteral("20240101"));
        CHECK_EQ(at(groupings, 1).caseId, QStringLiteral("20240101"));
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C20240101:20240101"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C20240101:20240101"));
        CHECK_EQ(at(groupings, 1).pathLabel, QStringLiteral("F:\\other\\20240101\\"));
    }

    // A shallower numeric ancestor no longer steals the id from the case that
    // is closer to the window's leaf.
    {
        const auto groupings = classify({
            {QStringLiteral("logs"), QStringLiteral("F:\\11111111\\08000000\\logs")},
            {QStringLiteral("08000000"), QStringLiteral("F:\\case\\08000000")},
        });
        CHECK_EQ(at(groupings, 0).caseId, QStringLiteral("08000000"));
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C08000000:08000000"));
        CHECK_EQ(at(groupings, 1).caseId, QStringLiteral("08000000"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C08000000:08000000"));
    }

    // Repeated non-date numbers in deep folders pair by the number. This is the
    // documented heuristic; date-looking numbers stay excluded.
    {
        const auto groupings = classify({
            {QStringLiteral("x"), QStringLiteral("F:\\a\\123456\\x")},
            {QStringLiteral("y"), QStringLiteral("K:\\b\\123456\\y")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C00123456:123456"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C00123456:123456"));
    }

    // Mixed separators and trailing separators classify like native paths.
    {
        const auto groupings = classify({
            {QStringLiteral("a"), QStringLiteral("F:/Proj/Sub/a/")},
            {QStringLiteral("b"), QStringLiteral("F:\\Proj\\Sub\\b\\")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("Pf:\\proj\\sub"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("Pf:\\proj\\sub"));
        CHECK_EQ(at(groupings, 0).normalizedPath, QStringLiteral("F:\\Proj\\Sub\\a"));
    }

    // Zero-padding matters: 123456 and 00123456 are different case ids.
    {
        const auto groupings = classify({
            {QStringLiteral("x"), QStringLiteral("F:\\a\\123456\\x")},
            {QStringLiteral("y"), QStringLiteral("F:\\b\\123456\\y")},
            {QStringLiteral("u"), QStringLiteral("F:\\c\\00123456\\u")},
            {QStringLiteral("v"), QStringLiteral("F:\\d\\00123456\\v")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C00123456:123456"));
        CHECK_EQ(at(groupings, 1).sectionKey, QStringLiteral("C00123456:123456"));
        CHECK_EQ(at(groupings, 2).sectionKey, QStringLiteral("C00123456:00123456"));
        CHECK_EQ(at(groupings, 3).sectionKey, QStringLiteral("C00123456:00123456"));
        CHECK_EQ(at(groupings, 0).sectionLabel, QStringLiteral("Case 123456"));
        CHECK_EQ(at(groupings, 2).sectionLabel, QStringLiteral("Case 00123456"));
    }

    // UNC server/share names are never case ids.
    {
        const auto groupings = classify({
            {QStringLiteral("a"), QStringLiteral("\\\\srv\\123456\\a")},
            {QStringLiteral("b"), QStringLiteral("\\\\srv\\123456\\b")},
        });
        CHECK_EQ(at(groupings, 0).caseId, QString());
        CHECK_EQ(at(groupings, 0).sectionKey, QString());
    }
    {
        const auto groupings = classify({
            {QStringLiteral("a"), QStringLiteral("\\\\srv\\share\\123456\\a")},
            {QStringLiteral("b"), QStringLiteral("\\\\srv\\share\\123456\\b")},
        });
        CHECK_EQ(at(groupings, 0).caseId, QStringLiteral("123456"));
        CHECK_EQ(at(groupings, 0).sectionKey, QStringLiteral("C00123456:123456"));
    }

    // Drive paths with a \\?\ prefix are normalized and classified; device
    // namespaces are kept verbatim and never form sections.
    CHECK_EQ(PathClassifier::normalize(QStringLiteral("\\\\?\\F:\\case\\08000000")),
             QStringLiteral("F:\\case\\08000000"));
    CHECK_EQ(PathClassifier::caseIdFromPath(QStringLiteral("\\\\?\\F:\\case\\08000000")),
             QStringLiteral("08000000"));
    CHECK_EQ(PathClassifier::normalize(QStringLiteral("\\\\?\\Volume{abc}\\proj\\a")),
             QStringLiteral("\\\\?\\Volume{abc}\\proj\\a"));
    CHECK_EQ(PathClassifier::caseIdFromPath(QStringLiteral("\\\\?\\Volume{abc}\\case\\08000000")),
             QString());
    {
        const auto groupings = classify({
            {QStringLiteral("a"), QStringLiteral("\\\\?\\Volume{abc}\\proj\\a")},
            {QStringLiteral("b"), QStringLiteral("\\\\?\\Volume{abc}\\proj\\b")},
        });
        CHECK_EQ(at(groupings, 0).sectionKey, QString());
        CHECK_EQ(at(groupings, 0).pathLabel, QStringLiteral("\\\\?\\Volume{abc}\\proj\\a"));
    }

    // Empty input is fine.
    {
        const auto groupings = classify({});
        CHECK_EQ(QString::number(groupings.size()), QStringLiteral("0"));
    }

    if (gFailures == 0)
    {
        std::printf("All classification tests passed.\n");
        return 0;
    }

    std::fprintf(stderr, "%d classification test(s) failed.\n", gFailures);
    return 1;
}
