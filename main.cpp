/*
 * File:   main.cpp
 * Author: saikrishna
 *
 * Created on January 1, 2014, 9:13 AM
 */

#include <QApplication>
#include "MainForm.h"

int main(int argc, char *argv[]) {
    // initialize resources, if needed
    // Q_INIT_RESOURCE(resfile);

    QApplication app(argc, argv);

    QStringList arguments = app.arguments();
    QStringList options;

    for (int i = 1; i < arguments.size(); i++) {
        QString argument = arguments.at(i);
        if (argument.startsWith("--")) {
            options.append(argument);
        }
    }

    MainForm mainForm(options);
    mainForm.show();

    return app.exec();
}
