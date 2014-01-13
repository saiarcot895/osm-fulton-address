/*
 * File:   Street.h
 * Author: saikrishna
 *
 * Created on January 1, 2014, 12:14 PM
 */

#ifndef STREET_H
#define	STREET_H

#include "QString"
#include "QList"
#include <geos/geom/LineString.h>
#include "QSharedPointer"

class Street {
public:
    QString name;
    QList<uint> nodeIndices;
    QSharedPointer<geos::geom::LineString> path;

    bool operator==(Street street) const;
};

#endif	/* STREET_H */

