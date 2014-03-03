/*
 * File:   Building.cpp
 * Author: saikrishna
 *
 * Created on January 24, 2014, 9:29 PM
 */

#include <QHash>
#include "BuildingPrivate.h"
#include "Building.h"

Building::Building() :
    d(new BuildingPrivate)
{
}

Building::Building(const Building& other) :
    d(other.d)
{
}

uint Building::id() const {
    return d->id;
}

int Building::version() const {
    return d->version;
}

int Building::year() const {
    return d->year;
}

QString Building::featureID() const {
    return d->featureID;
}

QSharedPointer<geos::geom::Polygon> Building::building() const {
    return d->building;
}

QList<uint> Building::nodeIndices() const {
    return d->nodeIndices;
}

QMap<QString, QString> Building::tags() const {
    return d->tags;
}

void Building::setId(uint id) {
    d->id = id;
}

void Building::setVersion(int version) {
    d->version = version;
}

void Building::setYear(int year) {
    d->year = year;
}

void Building::setFeatureID(QString featureID) {
    d->featureID = featureID;
}

void Building::setBuilding(QSharedPointer<geos::geom::Polygon> building) {
    d->building = building;
}

void Building::setNodeIndices(QList<uint> nodeIndices) {
    d->nodeIndices = nodeIndices;
}

void Building::setTags(QMap<QString, QString> tags) {
    d->tags = tags;
}

Building& Building::operator=(const Building& other) {
    d = other.d;
    return *this;
}

uint qHash(const Building& key) {
    return qHash(key.id()) ^ qHash(key.featureID()) ^ qHash(key.year());
}

bool operator==(const Building& building1, const Building& building2) {
    return building1.id() == building2.id()
            && building1.featureID() == building2.featureID()
            && building1.year() == building2.year()
            && building1.building() == building2.building()
            && building1.nodeIndices() == building2.nodeIndices()
            && building1.tags() == building2.tags();
}

Building::~Building() {
}

