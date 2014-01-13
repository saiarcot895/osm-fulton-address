/*
 * File:   Address.h
 * Author: saikrishna
 *
 * Created on January 4, 2014, 5:58 PM
 */

#ifndef ADDRESS_H
#define	ADDRESS_H

#include "Street.h"
#include <geos/geom/Point.h>
#include "QString"
#include "QSharedPointer"

class Address {
public:
    enum AddressType {
        Primary,
        Structural,
        Other = 99
    };

    QString houseNumber;
    Street street;
    QString city;
    int zipCode;
    QSharedPointer<geos::geom::Point> coordinate;
    AddressType addressType;
    bool allowStructural;

    Address();
    bool operator==(Address address) const;
};

#endif	/* ADDRESS_H */

