#ifndef BUILDINGPRIVATE_H
#define BUILDINGPRIVATE_H

#include <QSharedData>
#include <QSharedPointer>
#include <QMap>
#include <geos/geom/Polygon.h>

class BuildingPrivate : public QSharedData {
public:
    BuildingPrivate() :
        id(0),
        version(-1),
        year(0)
    {
    }

    BuildingPrivate(const BuildingPrivate& other) :
        QSharedData(other),
        id(other.id),
        version(other.version),
        year(other.year),
        featureID(other.featureID),
        building(other.building),
        nodeIndices(other.nodeIndices),
        tags(other.tags)
    {
    }

    uint id;
    int version;
    int year;
    QString featureID;
    QSharedPointer<geos::geom::Polygon> building;
    QList<uint> nodeIndices;
    QMap<QString, QString> tags;

    ~BuildingPrivate() {}
};

#endif // BUILDINGPRIVATE_H
