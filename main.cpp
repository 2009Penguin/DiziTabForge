#include <QApplication>
#include <QStyleFactory>

#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication::setPalette(QApplication::style()->standardPalette());
    QApplication::setApplicationName(QStringLiteral("DiziTabForge"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    MainWindow window;
    window.show();
    return QApplication::exec();
}
