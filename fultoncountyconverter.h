#ifndef FULTONCOUNTYCONVERTER_H
#define FULTONCOUNTYCONVERTER_H

#define DEGREES_TO_METERS 111000

#include "Building.h"
#include "Address.h"
#include "node.h"
#include "rtree.h"
#include <QFile>
#include <QNetworkAccessManager>
#include <QXmlStreamWriter>
#include <geos/geom/GeometryFactory.h>

class FultonCountyConverter : public QObject
{
    Q_OBJECT
public:
    FultonCountyConverter(QObject* parent = 0);
    virtual ~FultonCountyConverter();

    enum Logging {
        NoLogging = 0x000,
        ExistingStreets = 0x001,
        ExistingAddresses = 0x002,
        ZipCodesLoaded = 0x004,
        OverlappingBuildings = 0x008,
        AddressesButNoStreet = 0x010,
        NewAddressesBeforeValidation = 0x020,
        AddressesCloseToStreet = 0x040,
        AddressesFarFromStreet = 0x080,
        CloseAddresses = 0x100,
        NewAddressesAfterValidation = 0x200,
        AddressesNotInZipCodeArea = 0x400,
        MergedAddressesAndBuilding = 0x800,
        All = 0xfff
    };

    void setBoundingBox(double top, double left, double bottom, double right);
    void setAddresses(QString addressesFile);
    void setBuildings(QString buildingsFile);
    void setTaxParcels(QString taxParcelsFile);
    void setZipCodes(QString zipCodesFile);
    void setLogOptions(Logging logOptions);

    void convert();
    void writeChangeFile(QString fileName);

    QString getLog();

signals:
    void conversionFinished();
private:
    QNetworkAccessManager* nam;
    geos::geom::GeometryFactory* factory;
    QHash<uint, Node> nodes;
    QHash<int, QSharedPointer<geos::geom::Polygon> > zipCodes;
    QHash<QString, Street> streets;
    QList<Building> existingBuildings;
    QList<Building> buildings;
    QList< QSharedPointer<geos::geom::Polygon> > taxParcels;
    QList<Address> existingAddresses;
    QList<Address> newAddresses;
    QList<Address> excludedAddresses;
    QHash<Address, Building> addressBuildings;

    RTree<Building, double, 2> buildingsTree;
    RTree< QSharedPointer<geos::geom::Polygon>, double, 2> taxParcelsTree;
    RTree<Address, double, 2> addressesTree;

    QString newline;

    enum FeatureType {
        None = 0,
        NodePoint,
        Way,
        WayConfirmed,
        BuildingConfirmed,
        Relation
    };

    double top;
    double left;
    double bottom;
    double right;

    QFile addressesFile;
    QFile buildingsFile;
    QFile taxParcelsFile;
    QFile zipCodesFile;
    Logging logOptions;

    QString output;

    void downloadOSM();
    void readZipCodeFile();
    void readBuildingFile();
    void validateBuildings();
    void removeExistingIntersectingBuildings();
    void simplifyBuildings();
    void readAddressFile();
    void validateAddresses();
    void validateBetweenAddresses();
    void checkZipCodes();
    void readTaxParcels();
    void mergeAddressBuildingTaxParcels();
    void mergeAddressBuildingTree();
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

FultonCountyConverter::Logging operator| (FultonCountyConverter::Logging a, FultonCountyConverter::Logging b);

#endif // FULTONCOUNTYCONVERTER_H
