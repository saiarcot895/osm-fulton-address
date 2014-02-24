#include "Address.h"
#include "Street.h"
#include "QHash"

Address::Address() :
    zipCode(0),
    addressType(Other),
    allowStructural(true) {
}

uint qHash(const Address& key) {
    return qHash(key.houseNumber) ^ qHash(key.street.data()->name);
}


bool operator==(const Address& address1, const Address& address2) {
    return address1.houseNumber == address2.houseNumber
            && (address1.street.data() == NULL ?
                address2.street.data() == NULL :
                address1.street.data()->name == address2.street.data()->name);
}
