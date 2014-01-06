/*
 * File:   Address.h
 * Author: saikrishna
 *
 * Created on January 4, 2014, 5:58 PM
 */

#ifndef ADDRESS_H
#define	ADDRESS_H

#include "Coordinate.h"
#include "QString"

class Address {
public:
    enum AddressType {
        Primary,
        Structural,
        Other = 99
    };

    QString houseNumber;
    QString street;
    Coordinate coordinate;
    AddressType addressType;
    bool allowStructural;

    Address();
    bool operator==(Address address) const;
};

#endif	/* ADDRESS_H */

