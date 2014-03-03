#ifndef STREETPRIVATE_H
#define STREETPRIVATE_H

#include <QString>
#include <QList>
#include <QSharedPointer>
#include <QSharedData>
#include <geos/geom/LineString.h>

class StreetPrivate : public QSharedData {
public:
    StreetPrivate() {}

    StreetPrivate(const StreetPrivate& other) :
        QSharedData(other),
        name(other.name),
        nodeIndices(other.nodeIndices),
        path(other.path)
    {
    }

    QString name;
    QList<uint> nodeIndices;
    QSharedPointer<geos::geom::LineString> path;

    ~StreetPrivate() {}
};

#endif // STREETPRIVATE_H
