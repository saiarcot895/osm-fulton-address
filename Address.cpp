#include "Address.h"
#include "AddressPrivate.h"
#include <QHash>

Address::Address() :
    d(new AddressPrivate)
{
}

Address::Address(const Address& other) :
    d(other.d)
{
}

QString Address::houseNumber() const {
    return d->houseNumber;
}

Street Address::street() const {
    return d->street;
}

QString Address::city() const {
    return d->city;
}

int Address::zipCode() const {
    return d->zipCode;
}

QSharedPointer<geos::geom::Point> Address::coordinate() const {
    return d->coordinate;
}

Address::AddressType Address::addressType() const {
    return d->addressType;
}

bool Address::allowStructural() const {
    return d->allowStructural;
}

void Address::setHouseNumber(QString houseNumber) {
    d->houseNumber = houseNumber;
}

void Address::setStreet(Street street) {
    d->street = street;
}

void Address::setCity(QString city) {
    d->city = city;
}

void Address::setZipCode(int zipCode) {
    d->zipCode = zipCode;
}

void Address::setCoordinate(QSharedPointer<geos::geom::Point> coordinate) {
    d->coordinate = coordinate;
}

void Address::setAddressType(AddressType addressType) {
    d->addressType = addressType;
}

void Address::setAllowStructural(bool allowStructural) {
    d->allowStructural = allowStructural;
}

Address& Address::operator=(const Address& other) {
    d = other.d;
    return *this;
}

Address::~Address() {
}

uint qHash(const Address& key) {
    return qHash(key.houseNumber()) ^ qHash(key.street().name());
}

bool operator==(const Address& address1, const Address& address2) {
    return address1.houseNumber() == address2.houseNumber()
            && address1.street().name() == address2.street().name();
}
