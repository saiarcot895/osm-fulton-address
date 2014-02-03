/*
 * File:   mainForm.h
 * Author: saikrishna
 *
 * Created on January 1, 2014, 9:14 AM
 */

#ifndef _MAINFORM_H
#define	_MAINFORM_H

#define DEGREES_TO_METERS 111000

#include "ui_MainForm.h"
#include "Street.h"
#include "Address.h"
#include "Building.h"
#include <geos/geom/GeometryFactory.h>
#include "QtNetwork/QNetworkAccessManager"
#include "QtNetwork/QNetworkReply"
#include "QXmlStreamWriter"
#include "QFile"

class MainForm : public QMainWindow {
    Q_OBJECT
public:
    MainForm();
	MainForm(QStringList options);
    virtual ~MainForm();
public slots:
    void setAddressFile();
    void setZipCodeFile();
    void setBuildingFile();
    void setOutputFile();
    void convert();
    void downloadOSM();
private:
    Ui::mainForm widget;
    QNetworkAccessManager* nam;
    geos::geom::GeometryFactory* factory;
    QHash<uint, geos::geom::Point*> nodes;
    QHash<int, geos::geom::Polygon*> zipCodes;
    QHash<QString, Street*> streets;
	QList<Building> existingBuildings;
    QList<Building> buildings;
    QList<Address> existingAddresses;
    QList<Address> newAddresses;
    QList<Address> excludedAddresses;
    QHash<Address, Building> addressBuildings;
    enum FeatureType {
        None = 0,
        Node,
        Way,
        WayConfirmed,
		BuildingConfirmed,
        Relation
    };

    QString openFile();
    void readZipCodeFile();
    void readBuildingFile();
    void validateBuildings();
    void simplifyBuildings();
    void readAddressFile();
    void validateAddresses();
    void validateBetweenAddresses();
    void checkZipCodes();
    void mergeAddressBuilding();
	void mergeNearbyAddressBuilding();
    void outputChangeFile();
    void writeXMLFile(QFile& file,
            QList<Address>* address,
            QList<Building>* buildings,
            QHash<Address, Building>* addressBuildings);
    void outputStartOfFile(QXmlStreamWriter& writer);
    void outputEndOfFile(QXmlStreamWriter& writer);
    void cleanup();
    QString expandQuadrant(QString street);
    QString toTitleCase(QString street);
private slots:
    void readOSM(QNetworkReply* reply);
};

#endif	/* _MAINFORM_H */
