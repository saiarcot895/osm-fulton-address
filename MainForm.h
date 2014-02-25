/*
 * File:   mainForm.h
 * Author: saikrishna
 *
 * Created on January 1, 2014, 9:14 AM
 */

#ifndef _MAINFORM_H
#define _MAINFORM_H

#define DEGREES_TO_METERS 111000

#include "Street.h"
#include "Address.h"
#include "Building.h"
#include "node.h"
#include <geos/geom/GeometryFactory.h>
#include <QMainWindow>
#include "QtNetwork/QNetworkAccessManager"
#include "QtNetwork/QNetworkReply"
#include "QXmlStreamWriter"
#include "QFile"

namespace Ui {
class mainForm;
}

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
    void downloadOSM();
private:
    Ui::mainForm* widget;
    QNetworkAccessManager* nam;
    geos::geom::GeometryFactory* factory;
    QHash<uint, Node> nodes;
    QHash<int, QSharedPointer<geos::geom::Polygon> > zipCodes;
    QHash<QString, QSharedPointer<Street> > streets;
    QList<Building> existingBuildings;
    QList<Building> buildings;
    QList<Address> existingAddresses;
    QList<Address> newAddresses;
    QList<Address> excludedAddresses;
    QHash<Address, Building> addressBuildings;
    enum FeatureType {
        None = 0,
        NodePoint,
        Way,
        WayConfirmed,
        BuildingConfirmed,
        Relation
    };

    QString openFile();
    void readZipCodeFile();
    void readBuildingFile();
    void validateBuildings();
    void removeIntersectingBuildings();
    void simplifyBuildings();
    void readAddressFile();
    void validateAddresses();
    void validateBetweenAddresses();
    void checkZipCodes();
    void mergeAddressBuilding();
    void mergeNearbyAddressBuilding();
    void outputChangeFile();
    void writeXMLFile(QFile& file,
            const QList<Address> address,
            const QList<Building> buildings,
            const QHash<Address, Building> addressBuildings);
    void outputStartOfFile(QXmlStreamWriter& writer);
    void outputEndOfFile(QXmlStreamWriter& writer);
    void cleanup();
    QString expandQuadrant(QString street);
    QString toTitleCase(QString street);
private slots:
    void readOSM(QNetworkReply* reply);
};

#endif /* _MAINFORM_H */
