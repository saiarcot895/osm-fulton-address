/*
 * File:   Building.h
 * Author: saikrishna
 *
 * Created on January 24, 2014, 9:29 PM
 */

#ifndef BUILDING_H
#define BUILDING_H

#include <QSharedPointer>
#include <QMap>
#include <QDateTime>
#include <geos/geom/Polygon.h>

class BuildingPrivate;

class Building {
public:
    Building();
    Building(const Building& orig);

    uint id() const;
    int version() const;
    int year() const;
    QString featureID() const;
    QSharedPointer<geos::geom::Polygon> building() const;
    QList<uint> nodeIndices() const;
    QMap<QString, QString> tags() const;
    QString user() const;
    uint uid() const;
    uint changesetID() const;
    QDateTime timestamp() const;

    void setId(uint id);
    void setVersion(int version);
    void setYear (int year);
    void setFeatureID(QString featureID);
    void setBuilding(QSharedPointer<geos::geom::Polygon> building);
    void setNodeIndices(QList<uint> nodeIndices);
    void setTags(QMap<QString, QString> tags);
    void setUser(QString user);
    void setUid(uint uid);
    void setChangesetID(uint changesetID);
    void setTimestamp(QDateTime timestamp);

    Building& operator=(const Building& other);
    virtual ~Building();

private:
    QExplicitlySharedDataPointer<BuildingPrivate> d;
};

uint qHash(const Building& key);

bool operator==(const Building& building1, const Building& building2);

Q_DECLARE_TYPEINFO(Building, Q_MOVABLE_TYPE);

#endif /* BUILDING_H */

