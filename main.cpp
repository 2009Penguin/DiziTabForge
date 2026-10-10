#include <QApplication>
#include <QIcon>

#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("DiziTabForge"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setApplicationDisplayName(QStringLiteral("DiziTabForge"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icon.png")));
    MainWindow window;
    window.show();
    return QApplication::exec();
}
