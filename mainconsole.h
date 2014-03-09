#ifndef MAINCONSOLE_H
#define MAINCONSOLE_H

#include <QObject>
#include "fultoncountyconverter.h"

class MainConsole : public QObject
{
    Q_OBJECT
public:
    explicit MainConsole(QStringList options, QObject *parent = 0);
private:
    QString output;
    FultonCountyConverter* converter;

private slots:
    void onConversionCompleted();
};

#endif // MAINCONSOLE_H
