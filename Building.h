/*
 * File:   Building.h
 * Author: saikrishna
 *
 * Created on January 24, 2014, 9:29 PM
 */

#ifndef BUILDING_H
#define	BUILDING_H

#include <QSharedPointer>
#include <QMap>
#include <geos/geom/Polygon.h>

class Building {
public:
    Building();
    Building(const Building& orig);

    uint id;
    int version;
    int year;
    QString featureID;
    QSharedPointer<geos::geom::Polygon> building;
    QList<uint> nodeIndices;
    QMap<QString, QString> tags;

    virtual ~Building();
};

uint qHash(const Building& key);

bool operator==(const Building& building1, const Building& building2);

#endif	/* BUILDING_H */

