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
#include <geos/geom/GeometryFactory.h>
#include "QtNetwork/QNetworkAccessManager"
#include "QtNetwork/QNetworkReply"
#include "QXmlStreamWriter"

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
    geos::geom::GeometryFactory* factory;
    QHash<int, geos::geom::Point*> nodes;
    QHash<QString, Street*> streets;
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
    void outputStartOfFile(QXmlStreamWriter& writer);
    void outputEndOfFile(QXmlStreamWriter& writer);
    void cleanup();
    QString expandQuadrant(QString street);
private slots:
    void readOSM(QNetworkReply* reply);
};

#endif	/* _MAINFORM_H */
