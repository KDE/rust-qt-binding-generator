#include "Bindings.h"

#include "ui_main.h"

extern "C" {
   int main_cpp(const char* app);
}

int main_cpp(const char* appPath)
{
    int argc = 1;
    char* argv[1] = { (char*)appPath };
    QApplication app(argc, argv);

    Simple simple; // This is the Rust object

    QMainWindow main;
    Ui_MainWindow ui;
    ui.setupUi(&main);

    // Quit the application by...
    // * Using the mouse to select File->Quit
    // * Pressing <alt>+F, <alt>+Q
    // * Pressing <ctrl>+Q
    QObject::connect(ui.action_Quit, &QAction::triggered, &app, &QApplication::closeAllWindows);

    // Update the status message whenever the rust message changes
    simple.connect(&simple, &Simple::messageChanged, ui.statusbar, [&simple, &ui]() {
          ui.statusbar->showMessage(simple.message());
    });

    // Update the rust message when the button is clicked
    ui.statusButton->connect(ui.statusButton, &QPushButton::clicked, &simple, [&simple, &ui]() {
          simple.setMessage(ui.statusEdit->text());
    });

    // Initialize gui/model state
    ui.statusEdit->setText("Hello World!");
    ui.statusButton->clicked();

    main.show();

    return app.exec();
}
