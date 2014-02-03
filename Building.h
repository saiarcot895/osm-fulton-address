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

	uint getId() const;
    int getYear() const;
    QString getFeatureID() const;
    QSharedPointer<geos::geom::Polygon> getBuilding() const;
	QList<uint> getNodeIndices() const;

	void setId(uint id);
    void setYear(int year);
    void setFeatureID(QString featureID);
    void setBuilding(QSharedPointer<geos::geom::Polygon> building);
	void setNodeIndices(QList<uint> nodeIndices);

    virtual ~Building();
private:
	uint id;
    int year;
    QString featureID;
    QSharedPointer<geos::geom::Polygon> building;
	QList<uint> nodeIndices;
};

uint qHash(const Building& key);

bool operator==(const Building& building1, const Building& building2);

#endif	/* BUILDING_H */

