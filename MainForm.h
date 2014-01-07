/*
 * File:   mainForm.h
 * Author: saikrishna
 *
 * Created on January 1, 2014, 9:14 AM
 */

#ifndef _MAINFORM_H
#define	_MAINFORM_H

#include "ui_MainForm.h"
#include "Street.h"
#include "Address.h"
#include "QtNetwork/QNetworkAccessManager"
#include "QtNetwork/QNetworkReply"

class MainForm : public QMainWindow {
    Q_OBJECT
public:
    MainForm();
    virtual ~MainForm();
public slots:
    void setAddressFile();
    void setOutputFile();
    void convert();
    void downloadOSM();
private:
    Ui::mainForm widget;
    QNetworkAccessManager* nam;
    QHash<int, Coordinate> nodes;
    QList<Street> streets;
    QList<Address> existingAddresses;
    QList<Address> newAddresses;
    enum FeatureType {
        None = 0,
        Node,
        Way,
        WayConfirmed,
        Relation
    };

    void readAddressFile();
    void outputChangeFile();
    QString expandQuadrant(QString street);
private slots:
    void readOSM(QNetworkReply* reply);
};

#endif	/* _MAINFORM_H */
