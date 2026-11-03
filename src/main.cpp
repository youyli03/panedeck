#include "MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QPalette>

#include <windows.h>
#include <objbase.h>

#include <cstdio>

int main(int argc, char *argv[])
{
    setvbuf(stderr, nullptr, _IONBF, 0);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PaneDeck"));
    QApplication::setOrganizationName(QStringLiteral("PaneDeck"));
    QApplication::setStyle(QStringLiteral("Fusion"));

    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(13);
    application.setFont(font);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#F3F3F3")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#1A1A1A")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#F3F3F3")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#1A1A1A")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#1A1A1A")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#B26B1F")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    application.setPalette(palette);

    MainWindow window;
    window.show();

    const int result = application.exec();
    CoUninitialize();
    return result;
}




