#include "mainconsole.h"
#include <QFile>
#include <QCoreApplication>

MainConsole::MainConsole(QStringList options, QObject *parent) :
    QObject(parent),
    converter(new FultonCountyConverter(parent))
{
    while (!options.isEmpty()) {
        QString option = options.takeFirst();
        QStringList params = option.remove("--").split('=');
        if (params.size() == 2) {
            QString variable = params.at(0);
            if (variable == "address") {
                converter->setAddresses(params.at(1));
            } else if (variable == "building") {
                converter->setBuildings(params.at(1));
            } else if (variable == "output") {
                output = params.at(1);
            } else if (variable == "zip-code") {
                converter->setZipCodes(params.at(1));
            } else if (variable == "tax-parcel") {
                converter->setTaxParcels(params.at(1));
            } else if (variable == "bbox") {
                QStringList coords = params.at(1).split(",");
                double top = coords.at(0).toDouble();
                double left = coords.at(1).toDouble();
                double bottom = coords.at(2).toDouble();
                double right = coords.at(3).toDouble();
                converter->setBoundingBox(top, left, bottom, right);
            }
        }
    }

    converter->setLogOptions(FultonCountyConverter::All);
    converter->convert();

    connect(converter, SIGNAL(conversionFinished()), this, SLOT(onConversionCompleted()));
}

void MainConsole::onConversionCompleted() {
    converter->writeChangeFile(output);

    QCoreApplication::quit();
}
