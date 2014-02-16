#ifndef NODE_H
#define NODE_H

#include <QSharedPointer>
#include <geos/geom/Point.h>

class Node
{
public:
    Node();
    Node(const Node& orig);

    uint id;
    int version;
    QSharedPointer<geos::geom::Point> point;
};

#endif // NODE_H
