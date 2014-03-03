#include "Street.h"
#include "StreetPrivate.h"

Street::Street() :
    d(new StreetPrivate)
{
}

Street::Street(const Street& other) :
    d(other.d)
{
}

QString Street::name() const {
    return d->name;
}

QList<uint> Street::nodeIndices() const {
    return d->nodeIndices;
}

QSharedPointer<geos::geom::LineString> Street::path() const {
    return d->path;
}

void Street::setName(QString name) {
    d->name = name;
}

void Street::setNodeIndices(QList<uint> nodeIndices) {
    d->nodeIndices = nodeIndices;
}

void Street::setPath(QSharedPointer<geos::geom::LineString> path) {
    d->path = path;
}

Street& Street::operator=(const Street& other) {
    d = other.d;
    return *this;
}

Street::~Street() {
}

bool operator==(const Street& street1, const Street& street2) {
    return street1.name().toUpper() == street2.name().toUpper();
}
