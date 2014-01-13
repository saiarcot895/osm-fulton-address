#include "Street.h"

bool Street::operator==(Street street) const {
    return name.toUpper() == street.name.toUpper();
}
