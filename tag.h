#ifndef TAG_H
#define TAG_H

#include <QString>

class Tag
{
public:
    Tag(QString key, QString value);

    QString key;
    QString value;
};

bool operator==(const Tag& tag1, const Tag& tag2);

#endif // TAG_H
