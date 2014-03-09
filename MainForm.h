/*
 * File:   mainForm.h
 * Author: saikrishna
 *
 * Created on January 1, 2014, 9:14 AM
 */

#ifndef _MAINFORM_H
#define _MAINFORM_H

#include <QMainWindow>
#include "fultoncountyconverter.h"

namespace Ui {
class mainForm;
}

class Street;
class Address;
class Building;
class Node;

class MainForm : public QMainWindow {
    Q_OBJECT
public:
    MainForm(QWidget *parent = 0);
    MainForm(QStringList options, QWidget *parent = 0);
    virtual ~MainForm();
public slots:
    void setAddressFile();
    void setZipCodeFile();
    void setBuildingFile();
    void setOutputFile();
    void convert();
    void onConversionFinished();
private:
    Ui::mainForm* widget;
    FultonCountyConverter* converter;

    QString openFile();
};

#endif /* _MAINFORM_H */
