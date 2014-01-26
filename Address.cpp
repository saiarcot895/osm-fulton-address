#include "Address.h"
#include "Street.h"
#include "QHash"

Address::Address() {
    addressType = Other;
    allowStructural = true;
}

uint qHash(const Address& key) {
    return qHash(key.houseNumber) ^ qHash(key.street.name);
}


bool operator==(const Address& address1, const Address& address2) {
    return address1.houseNumber == address2.houseNumber
            && address1.street.name == address2.street.name;
}
