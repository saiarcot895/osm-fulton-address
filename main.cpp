/*
 * File:   main.cpp
 * Author: saikrishna
 *
 * Created on January 1, 2014, 9:13 AM
 */

#include <QApplication>
#include <QWidget>
#include "MainForm.h"

int main(int argc, char *argv[]) {
    // initialize resources, if needed
    // Q_INIT_RESOURCE(resfile);

    QApplication app(argc, argv);

    MainForm mainForm;
    mainForm.show();

    return app.exec();
}
