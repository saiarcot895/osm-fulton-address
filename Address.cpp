#include "Address.h"
#include "Street.h"

Address::Address() {
    addressType = Other;
    allowStructural = true;
    coordinate = NULL;
}

Address::~Address() {
    //delete coordinate;
}

bool Address::operator==(Address address) const {
    return houseNumber == address.houseNumber && street.name == address.street.name;
}
