#ifndef ADDRESSPRIVATE_H
#define ADDRESSPRIVATE_H

#include "Address.h"
#include "Street.h"
#include "Building.h"
#include <geos/geom/Point.h>
#include "QString"
#include "QSharedPointer"
#include "QSharedData"

class AddressPrivate : public QSharedData {
public:
    AddressPrivate() :
        zipCode(0),
        allowStructural(true)
    {
    }

    AddressPrivate(const AddressPrivate& other) :
        QSharedData(other),
        houseNumber(other.houseNumber),
        street(other.street),
        city(other.city),
        zipCode(other.zipCode),
        coordinate(other.coordinate),
        addressType(other.addressType),
        allowStructural(other.allowStructural)
    {
    }

    QString houseNumber;
    Street street;
    QString city;
    int zipCode;
    QSharedPointer<geos::geom::Point> coordinate;
    Address::AddressType addressType;
    bool allowStructural;

    ~AddressPrivate() {}
};

#endif // ADDRESSPRIVATE_H
