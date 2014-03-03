/*
 * File:   Address.h
 * Author: saikrishna
 *
 * Created on January 4, 2014, 5:58 PM
 */

#ifndef ADDRESS_H
#define ADDRESS_H

#include "Street.h"
#include "Building.h"
#include <geos/geom/Point.h>
#include <QString>
#include <QExplicitlySharedDataPointer>

class AddressPrivate;

class Address {
public:
    Address();
    Address(const Address& other);

    enum AddressType {
        Primary,
        Structural,
        Other = 99
    };

    QString houseNumber() const;
    Street street() const;
    QString city() const;
    int zipCode() const;
    QSharedPointer<geos::geom::Point> coordinate() const;
    AddressType addressType() const;
    bool allowStructural() const;

    void setHouseNumber(QString houseNumber);
    void setStreet(Street street);
    void setCity(QString city);
    void setZipCode(int zipCode);
    void setCoordinate(QSharedPointer<geos::geom::Point> coordinate);
    void setAddressType(AddressType addressType);
    void setAllowStructural(bool allowStructural);

    Address& operator=(const Address& other);
    virtual ~Address();
private:
    QExplicitlySharedDataPointer<AddressPrivate> d;
};

uint qHash(const Address& key);

bool operator==(const Address& address1, const Address& address2);

Q_DECLARE_TYPEINFO(Address, Q_MOVABLE_TYPE);

#endif /* ADDRESS_H */

