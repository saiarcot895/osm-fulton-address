#ifndef BUILDINGPRIVATE_H
#define BUILDINGPRIVATE_H

#include <QSharedData>
#include <QSharedPointer>
#include <QMap>
#include <QDateTime>
#include <geos/geom/Polygon.h>

class BuildingPrivate : public QSharedData {
public:
    BuildingPrivate() :
        id(0),
        version(-1),
        year(0),
        uid(0),
        changesetID(0)
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
        tags(other.tags),
        user(other.user),
        uid(other.uid),
        changesetID(other.changesetID),
        timestamp(other.timestamp)
    {
    }

    uint id;
    int version;
    int year;
    QString featureID;
    QSharedPointer<geos::geom::Polygon> building;
    QList<uint> nodeIndices;
    QMap<QString, QString> tags;
    QString user;
    uint uid;
    uint changesetID;
    QDateTime timestamp;

    ~BuildingPrivate() {}
};

#endif // BUILDINGPRIVATE_H
