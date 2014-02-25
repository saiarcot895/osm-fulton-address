/*
 * File:   Building.cpp
 * Author: saikrishna
 *
 * Created on January 24, 2014, 9:29 PM
 */

#include "Building.h"
#include "QHash"

Building::Building() :
    id(0),
    version(-1),
    year(0)
{
}

Building::Building(const Building& orig) {
    id = orig.id;
    version = orig.version;
    year = orig.year;
    featureID = orig.featureID;
    building = orig.building;
    nodeIndices = orig.nodeIndices;
    tags = orig.tags;
}

uint qHash(const Building& key) {
    return qHash(key.id) ^ qHash(key.featureID) ^ qHash(key.year);
}

bool operator==(const Building& building1, const Building& building2) {
    return building1.id == building2.id
            && building1.featureID == building2.featureID
            && building1.year == building2.year
            && building1.building == building2.building
            && building1.nodeIndices == building2.nodeIndices
            && building1.tags == building2.tags;
}

Building::~Building() {
}

