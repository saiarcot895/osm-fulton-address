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

class Street {
public:
    QString name;
    QList<uint> nodeIndices;
    geos::geom::LineString* path;
    bool operator==(Street street1) const;
};

#endif	/* STREET_H */

