#include "Street.h"

bool Street::operator==(const Street& street) const {
    return name.toUpper() == street.name.toUpper();
}
