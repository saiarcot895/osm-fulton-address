/*
 * File:   Street.h
 * Author: saikrishna
 *
 * Created on January 1, 2014, 12:14 PM
 */

#ifndef STREET_H
#define	STREET_H

#include "Coordinate.h"

struct Street {
    QString name;
    QList<int> nodeIndices;
};

#endif	/* STREET_H */

