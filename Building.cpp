/*
 * File:   Building.cpp
 * Author: saikrishna
 *
 * Created on January 24, 2014, 9:29 PM
 */

#include "Building.h"

Building::Building() {
    year = 0;
}

Building::Building(const Building& orig) {
    year = orig.year;
    building = orig.building;
}

int Building::getYear() const {
    return year;
}

QString Building::getFeatureID() const {
    return featureID;
}

QSharedPointer<geos::geom::Polygon> Building::getBuilding() const {
    return building;
}

void Building::setYear(int year) {
    this->year = year;
}

void Building::setFeatureID(QString featureID) {
    this->featureID = featureID;
}

void Building::setBuilding(QSharedPointer<geos::geom::Polygon> building) {
    this->building = building;
}

Building::~Building() {
}

