#include "Street.h"

Street::Street() {
    path = NULL;
}

Street::~Street() {
    //delete path;
}

bool Street::operator==(Street street) const {
    return name.toUpper() == street.name.toUpper();
}
