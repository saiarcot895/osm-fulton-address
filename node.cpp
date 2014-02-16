#include "node.h"

Node::Node() :
    id(0),
    version(-1)
{
}

Node::Node(const Node& orig) {
    id = orig.id;
    version = orig.version;
    point = orig.point;
}
