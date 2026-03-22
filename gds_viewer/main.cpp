#include "MainWindow.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    if (argc > 1) {
        window.loadFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
