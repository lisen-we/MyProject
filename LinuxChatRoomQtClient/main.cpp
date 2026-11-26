#include "LoginPage.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("LinuxChatRoomQtClient"));
    LoginPage window;
    window.show();

    return app.exec();
}
