#include "Bindings.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Simple simple; // This is the Rust object
    QMessageBox msgBox;
    msgBox.setText(simple.message());
    msgBox.connect(&msgBox, &QMessageBox::finished, &msgBox, []() {
        QCoreApplication::quit();
    });
    msgBox.show();

    return app.exec();
}
