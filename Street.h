/*
 * File:   Street.h
 * Author: saikrishna
 *
 * Created on January 1, 2014, 12:14 PM
 */

#ifndef STREET_H
#define STREET_H

#include <QString>
#include <QList>
#include <QSharedPointer>
#include <QExplicitlySharedDataPointer>
#include <geos/geom/LineString.h>

class StreetPrivate;

class Street {
public:
    Street();
    Street(const Street& other);

    QString name() const;
    QList<uint> nodeIndices() const;
    QSharedPointer<geos::geom::LineString> path() const;

    void setName(QString name);
    void setNodeIndices(QList<uint> nodeIndices);
    void setPath(QSharedPointer<geos::geom::LineString> path);

    Street& operator=(const Street& other);

    virtual ~Street();
private:
    QExplicitlySharedDataPointer<StreetPrivate> d;
};

bool operator==(const Street& street1, const Street& street2);

Q_DECLARE_TYPEINFO(Street, Q_MOVABLE_TYPE);

#endif /* STREET_H */

