#include "Address.h"
#include "Street.h"

Address::Address() {
    addressType = Other;
    allowStructural = true;
}

bool Address::operator==(const Address& address) const {
    return houseNumber == address.houseNumber && street.name == address.street.name;
}
