#include <QApplication>

#include "gui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    scapms::gui::MainWindow window;
    window.show();
    return app.exec();
}
