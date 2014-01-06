/*
 * File:   Street.h
 * Author: saikrishna
 *
 * Created on January 1, 2014, 12:14 PM
 */

#ifndef STREET_H
#define	STREET_H

#include "Coordinate.h"
#include "QString"
#include "QList"

class Street {
public:
    QString name;
    QList<int> nodeIndices;
    bool operator==(Street street1) const;
};

#endif	/* STREET_H */

