#include "tag.h"

Tag::Tag(QString key, QString value) :
    key(key),
    value(value)
{
}

bool operator==(const Tag& tag1, const Tag& tag2) {
    return tag1.key == tag2.key && tag1.value == tag2.value;
}
