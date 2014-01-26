/*
 * File:   Building.h
 * Author: saikrishna
 *
 * Created on January 24, 2014, 9:29 PM
 */

#ifndef BUILDING_H
#define	BUILDING_H

#include "QSharedPointer"
#include <geos/geom/Polygon.h>

class Building {
public:
    Building();
    Building(const Building& orig);

    int getYear() const;
    QSharedPointer<geos::geom::Polygon> getBuilding() const;

    void setYear(int year);
    void setBuilding(QSharedPointer<geos::geom::Polygon> building);

    virtual ~Building();
private:
    int year;
    QSharedPointer<geos::geom::Polygon> building;
};

#endif	/* BUILDING_H */

