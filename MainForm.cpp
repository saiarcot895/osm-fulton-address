/*
 * File:   mainForm.cpp
 * Author: saikrishna
 *
 * Created on January 1, 2014, 9:14 AM
 */

#include "MainForm.h"
#include "QFileDialog"
#include "QUrl"
#include "QXmlStreamReader"
#include "QDebug"
#include "Address.h"
#include <geos/geom/PrecisionModel.h>
#include <geos/geom/CoordinateSequenceFactory.h>

MainForm::MainForm() {
    widget.setupUi(this);

    widget.doubleSpinBox->setValue(33.7857);
    widget.doubleSpinBox_2->setValue(-84.3982);
    widget.doubleSpinBox_3->setValue(33.7633);
    widget.doubleSpinBox_4->setValue(-84.3574);

    nam = new QNetworkAccessManager(this);
    factory = new geos::geom::GeometryFactory(new geos::geom::PrecisionModel(), 4326);

    connect(widget.pushButton, SIGNAL(clicked()), this, SLOT(setAddressFile()));
    connect(widget.pushButton_2, SIGNAL(clicked()), this, SLOT(convert()));
    connect(widget.pushButton_3, SIGNAL(clicked()), this, SLOT(setOutputFile()));
    connect(widget.pushButton_4, SIGNAL(clicked()), this, SLOT(setZipCodeFile()));
    connect(widget.pushButton_5, SIGNAL(clicked()), this, SLOT(setBuildingFile()));
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(readOSM(QNetworkReply*)));
}

QString MainForm::openFile() {
    return QFileDialog::getOpenFileName(this, tr("Open File"), "",
            tr("OSM File (*.osm);;XML File (*.xml)"));
}

void MainForm::setAddressFile() {
    QString fileName = openFile();
    if (!openFile().isEmpty()) {
        widget.lineEdit->setText(fileName);
    }
}

void MainForm::setZipCodeFile() {
    QString fileName = openFile();
    if (!openFile().isEmpty()) {
        widget.lineEdit_3->setText(fileName);
    }
}

void MainForm::setBuildingFile() {
    QString fileName = openFile();
    if (!openFile().isEmpty()) {
        widget.lineEdit_4->setText(fileName);
    }
}

void MainForm::setOutputFile() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "",
            tr("OsmChange File (*.osc)"));
    if (fileName.length() != 0) {
        if (fileName.endsWith(".osc")) {
            widget.lineEdit_2->setText(fileName);
        } else {
            widget.lineEdit_2->setText(fileName + ".osc");
        }

    }
}

void MainForm::convert() {
    widget.pushButton_2->setEnabled(false);
    qApp->processEvents();
    downloadOSM();
}

void MainForm::downloadOSM() {
    QString query = tr("[out:xml];"
            "(node[\"addr:housenumber\"](%1,%2,%3,%4);"
            "way[\"addr:housenumber\"](%1,%2,%3,%4);"
            "way[\"name\"](%1,%2,%3,%4);"
            "relation[\"addr:housenumber\"](%1,%2,%3,%4););"
            "out meta;>;out meta;")
            .arg(widget.doubleSpinBox_3->value())
            .arg(widget.doubleSpinBox_2->value())
            .arg(widget.doubleSpinBox->value())
            .arg(widget.doubleSpinBox_4->value());
    QUrl url("http://overpass-api.de/api/interpreter/");
    url.addQueryItem("data", query);
    nam->get(QNetworkRequest(url));
}

void MainForm::readOSM(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QXmlStreamReader reader(reply->readAll());
        Street* street;
        Address* address;
        FeatureType current = None;
        while (!reader.atEnd()) {
            // Tags are assumed to be in alphabetical order
            switch (reader.readNext()) {
                case QXmlStreamReader::StartElement:
                    if (reader.name().toString() == "node") {
                        // Store all nodes
                        geos::geom::Coordinate coordinate;
                        uint nodeId = reader.attributes().value("id").toString().toUInt();
                        coordinate.y = reader.attributes().value("lat").toString().toDouble();
                        coordinate.x = reader.attributes().value("lon").toString().toDouble();
                        nodes.insert(nodeId, factory->createPoint(coordinate));
                    } else if (reader.name().toString() == "way") {
                        // A way is typically either be a road or a building. For
                        // now, assume it is a road, since we need the nodes and
                        // those come before the way tags
                        current = Way;
                        street = new Street();
                    } else if (reader.name().toString() == "tag") {
                        if (reader.attributes().value("k") == "addr:housenumber") {
                            address = new Address();
                            address->houseNumber = reader.attributes().value("v").toString();
                        } else if (reader.attributes().value("k") == "addr:street") {
                            // We don't care what street instance the address is
                            // linked to. If there is any address with the same number
                            // and street name, skip it.
                            address->street.name = reader.attributes().value("v").toString();
                            existingAddresses.append(*address);
                        } else if (current == Way && reader.attributes().value("k") == "building") {
                            // No buildings...yet.
                            current = None;
                            delete street;
                        } else if (reader.attributes().value("k") == "highway"
                                && current == Way) {
                            // We now know this is a way.
                            current = WayConfirmed;
                        } else if (reader.attributes().value("k") == "name"
                                && current == WayConfirmed) {
                            street->name = reader.attributes().value("v").toString();
                        }
                    } else if (reader.name().toString() == "nd" && current == Way) {
                        street->nodeIndices.append(reader.attributes().value("ref").toString().toUInt());
                    }
                    break;
                case QXmlStreamReader::EndElement:
                    if (current == WayConfirmed && reader.name().toString() == "way") {
                        streets.insertMulti(street->name.toUpper(), street);
                        current = None;
                    }
                    break;
                default:
                    break;
            }
        }
        QList<Street*> streetValues = streets.values();
        for (int i = 0; i < streetValues.size(); i++) {
            Street* street = streetValues.at(i);
            geos::geom::CoordinateSequence* nodePoints = factory
                    ->getCoordinateSequenceFactory()->create(street->nodeIndices.size(), 0);
            for (int j = 0; j < street->nodeIndices.size(); j++) {
                nodePoints->add(*(nodes.value(street->nodeIndices.at(j))->getCoordinate()));
            }
            street->path = QSharedPointer<geos::geom::LineString>(factory->createLineString(nodePoints));
        }
        if (widget.checkBox->isChecked()) {
            widget.textBrowser->append("Streets:");
            QList<Street*> streetValues = streets.values();
            for (int i = 0; i < streetValues.size(); i++) {
                Street* street = streetValues.at(i);
                widget.textBrowser->append(street->name);
            }
        }
        if (widget.checkBox_2->isChecked()) {
            widget.textBrowser->append("");
            widget.textBrowser->append("Existing Addresses:");
            for (int i = 0; i < existingAddresses.size(); i++) {
                Address address = existingAddresses.at(i);
                widget.textBrowser->append(address.houseNumber + " " + address.street.name);
            }
        }
        readAddressFile();
    } else {
        widget.textBrowser->insertPlainText(reply->errorString());
        qCritical() << reply->errorString();
        cleanup();
    }
}

void MainForm::readAddressFile() {
    QFile file(widget.lineEdit->text());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Error: Couldn't open " << widget.lineEdit->text();
        cleanup();
        return;
    }

    QXmlStreamReader reader(&file);
    Address* address;
    bool skip = false;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name().toString() == "node") {
                    address = new Address();
                    geos::geom::Coordinate coordinate;
                    coordinate.y = reader.attributes().value("lat").toString().toDouble();
                    coordinate.x = reader.attributes().value("lon").toString().toDouble();
                    address->coordinate = QSharedPointer<geos::geom::Point>(factory->createPoint(coordinate));
                    // Check to see if the address is inside the BBox
                    skip = !(coordinate.y <= widget.doubleSpinBox->value() &&
                            coordinate.y >= widget.doubleSpinBox_3->value() &&
                            coordinate.x >= widget.doubleSpinBox_2->value() &&
                            coordinate.x <= widget.doubleSpinBox_4->value());
                } else if (reader.name().toString() == "tag" && !skip) {
                    if (reader.attributes().value("k") == "addr:housenumber") {
                        address->houseNumber = reader.attributes().value("v").toString();
                    } else if (reader.attributes().value("k") == "addr:street") {
                        QString streetName = reader.attributes().value("v").toString();
                        QList<Street*> matches = streets.values(streetName.toUpper());
                        if (!matches.isEmpty()) {
                            Street* closestStreet = matches.at(0);
                            double minDistance = address->coordinate.data()->distance(closestStreet->path.data());
                            for (int i = 1; i < matches.size(); i++) {
                                double distance = address->coordinate.data()->distance(matches.at(i)->path.data());
                                if (distance < minDistance) {
                                    closestStreet = matches.at(i);
                                    minDistance = distance;
                                }
                            }
                            address->street = *closestStreet;
                        }
                    } else if (reader.attributes().value("k") == "addr:city") {
                        address->city = toTitleCase(reader.attributes().value("v").toString());
                    } else if (reader.attributes().value("k") == "addr:postcode") {
                        address->zipCode = reader.attributes().value("v").toString().toInt();
                    } else if (reader.attributes().value("k") == "import:FEAT_TYPE") {
                        if (reader.attributes().value("v") == "driv") {
                            address->addressType = Address::Primary;
                        } else if (reader.attributes().value("v") == "stru") {
                            address->addressType = Address::Structural;
                        }
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.name().toString() == "node") {
                    if (!skip) {
                        if (!address->houseNumber.isEmpty()
                                && !address->street.name.isEmpty()
                                && !existingAddresses.contains(*address)
                                && address->addressType != Address::Other) {
                            int i = newAddresses.indexOf(*address);
                            if (i != -1) {
                                Address existingAddress = newAddresses.at(i);
                                if (existingAddress.addressType == Address::Structural
                                        && address->addressType == Address::Structural) {
                                    existingAddress.allowStructural = false;
                                } else if (existingAddress.addressType == Address::Primary
                                        && address->addressType == Address::Structural
                                        && existingAddress.allowStructural) {
                                    existingAddress.coordinate = address->coordinate;
                                    existingAddress.addressType = Address::Structural;
                                } else if (existingAddress.addressType == Address::Structural
                                        && address->addressType == Address::Primary
                                        && !existingAddress.allowStructural) {
                                    existingAddress.coordinate = address->coordinate;
                                    existingAddress.addressType = Address::Primary;
                                }
                                delete address;
                            } else {
                                newAddresses.append(*address);
                            }
                        } else {
                            delete address;
                        }
                    } else {
                        delete address;
                    }
                }
                break;
            default:
                break;
        }
    }
    if (widget.checkBox_3->isChecked()) {
        widget.textBrowser->append("");
        widget.textBrowser->append("New Addresses (before vvalidation):");
        for (int i = 0; i < newAddresses.size(); i++) {
            Address address = newAddresses.at(i);
            widget.textBrowser->append(address.houseNumber + " " + address.street.name);
        }
    }
    validateAddresses();
}

void MainForm::validateAddresses() {
    if (widget.checkBox_4->isChecked() || widget.checkBox_5->isChecked() || widget.checkBox_6->isChecked()) {
        widget.textBrowser->append("");
        widget.textBrowser->append("Address Validations");
    }
    for (int i = 0; i < newAddresses.size(); i++) {
        Address address = newAddresses.at(i);

        double distance = address.coordinate.data()->distance(address.street.path.data()) * 111000;

        if (distance > 100) {
            if (widget.checkBox_5->isChecked()) {
                widget.textBrowser->append(tr("Too far from the street: %1 %2").arg(address.houseNumber)
                        .arg(address.street.name));
            }
            newAddresses.removeOne(address);
            excludedAddresses.append(address);
            i--;
        } else if (distance < 5) {
            if (widget.checkBox_6->isChecked()) {
                widget.textBrowser->append(tr("Too close to the street: %1 %2").arg(address.houseNumber)
                        .arg(address.street.name));
            }
            newAddresses.removeOne(address);
            excludedAddresses.append(address);
            i--;
        }
    }
    validateBetweenAddresses();

    if (widget.checkBox_7->isChecked()) {
        widget.textBrowser->append("");
        widget.textBrowser->append("New Addresses (after validation):");
        for (int i = 0; i < newAddresses.size(); i++) {
            Address address = newAddresses.at(i);
            widget.textBrowser->append(address.houseNumber + " " + address.street.name);
        }
    }

    outputChangeFile();
}

void MainForm::validateBetweenAddresses() {
    int sections = 2;
    double lonSection = (widget.doubleSpinBox_4->value() - widget.doubleSpinBox_2->value()) / sections;
    double latSection = (widget.doubleSpinBox->value() - widget.doubleSpinBox_3->value()) / sections;

    for (int region = 0; region < sections * sections; region++) {
        double minLon = widget.doubleSpinBox_2->value() + (region % sections) * lonSection;
        double minLat = widget.doubleSpinBox->value() - (region / sections) * latSection;
        double maxLon = minLon + lonSection;
        double maxLat = minLat - latSection;

        for (int i = 0; i < newAddresses.size(); i++) {
            Address address1 = newAddresses.at(i);
            const geos::geom::Coordinate* coordinate1 = address1.coordinate.data()->getCoordinate();
            if (!(coordinate1->x <= maxLon && coordinate1->x >= minLon
                    && coordinate1->y >= maxLat && coordinate1->y <= minLat)) {
                continue;
            }

            for (int j = i + 1; j < newAddresses.size(); j++) {
                Address address2 = newAddresses.at(j);
                const geos::geom::Coordinate* coordinate2 = address2.coordinate.data()->getCoordinate();
                if (!(coordinate2->x <= maxLon && coordinate2->x >= minLon
                        && coordinate2->y >= maxLat && coordinate2->y <= minLat)) {
                    continue;
                }

                double distance = address1.coordinate.data()->distance(address2.coordinate.data()) * 111000;

                if (distance < 4) {
                    if (widget.checkBox_4->isChecked()) {
                        widget.textBrowser->append(tr("Too close to another address: %1 %2")
                                .arg(address2.houseNumber).arg(address2.street.name));
                    }
                    newAddresses.removeOne(address2);
                    excludedAddresses.append(address2);
                    j--;
                }
            }
        }
    }
}


void MainForm::outputChangeFile() {
    if (widget.lineEdit_2->text().isEmpty()) {
        return;
    }
    for (int i = 0; i <= newAddresses.size() / 5000; i++) {
        QString fullFileName = widget.lineEdit_2->text();
        if (i != 0) {
            if (widget.lineEdit_2->text().lastIndexOf(".") == -1) {
                fullFileName = tr("%1_%2").arg(widget.lineEdit_2->text()).arg(i);
            }
            else {
                QString baseName = widget.lineEdit_2->text().left(widget.lineEdit_2
                        ->text().lastIndexOf("."));
                QString extension = widget.lineEdit_2->text().right(widget.lineEdit_2->text().length()
                        - widget.lineEdit_2->text().lastIndexOf(".") - 1);
                fullFileName = tr("%1_%2.%3").arg(baseName).arg(i).arg(extension);
            }
        }
        QFile file(fullFileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            continue;
        }

        writeXMLFile(file, newAddresses, i);
    }

    for (int i = 0; i <= excludedAddresses.size() / 5000; i++) {
        QString fullFileName = widget.lineEdit_2->text();
        if (i == 0) {
            if (widget.lineEdit_2->text().lastIndexOf(".") == -1) {
                fullFileName = tr("%1_errors").arg(widget.lineEdit_2->text());
            }
            else {
                QString baseName = widget.lineEdit_2->text().left(widget.lineEdit_2
                        ->text().lastIndexOf("."));
                QString extension = widget.lineEdit_2->text().right(widget.lineEdit_2->text().length()
                        - widget.lineEdit_2->text().lastIndexOf(".") - 1);
                fullFileName = tr("%1_errors.%2").arg(baseName).arg(extension);
            }
        } else {
            if (widget.lineEdit_2->text().lastIndexOf(".") == -1) {
                fullFileName = tr("%1_errors_%2").arg(widget.lineEdit_2->text()).arg(i);
            }
            else {
                QString baseName = widget.lineEdit_2->text().left(widget.lineEdit_2
                        ->text().lastIndexOf("."));
                QString extension = widget.lineEdit_2->text().right(widget.lineEdit_2->text().length()
                        - widget.lineEdit_2->text().lastIndexOf(".") - 1);
                fullFileName = tr("%1_errors_%2.%3").arg(baseName).arg(i).arg(extension);
            }
        }
        QFile file(fullFileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            continue;
        }

        writeXMLFile(file, excludedAddresses, i);
    }

    // Output the log to a file
    QString baseName = widget.lineEdit_2->text().left(widget.lineEdit_2->text()
            .lastIndexOf("."));
    QFile logFile(baseName + ".log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream writer(&logFile);
        writer << widget.textBrowser->document()->toPlainText();
        writer.flush();
    }

    cleanup();
}

void MainForm::writeXMLFile(QFile& file, QList<Address>& addresses, int i) {
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    outputStartOfFile(writer);
    for (int j = i * 5000; j < addresses.size() && j < (i + 1) * 5000; j++) {
        Address address = addresses.at(j);

        writer.writeStartElement("node");
        writer.writeAttribute("id", tr("-%1").arg(j + 1));
        writer.writeAttribute("lat", QString::number(address.coordinate.data()->getY(), 'g', 12));
        writer.writeAttribute("lon", QString::number(address.coordinate.data()->getX(), 'g', 12));

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:housenumber");
        writer.writeAttribute("v", address.houseNumber);
        writer.writeEndElement();

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:street");
        writer.writeAttribute("v", address.street.name);
        writer.writeEndElement();

        if (!address.city.isEmpty()) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:city");
            writer.writeAttribute("v", address.city);
            writer.writeEndElement();
        }

        if (address.zipCode != 0) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:postcode");
            writer.writeAttribute("v", QString::number(address.zipCode));
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }
    outputEndOfFile(writer);
}

void MainForm::outputStartOfFile(QXmlStreamWriter& writer) {
    writer.writeStartDocument();
    writer.writeStartElement("osmChange");
    writer.writeAttribute("version", "0.6");
    writer.writeStartElement("create");
}

void MainForm::outputEndOfFile(QXmlStreamWriter& writer) {
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
}

void MainForm::cleanup() {
    QList<geos::geom::Point*> pointValues = nodes.values();
    for (int i = 0; i < pointValues.size(); i++) {
        geos::geom::Point* point = pointValues.at(i);
        delete point;
    }
    nodes.clear();
    QList<Street*> streetValues = streets.values();
    for (int i = 0; i < streetValues.size(); i++) {
        Street* street = streetValues.at(i);
        delete street;
    }
    streets.clear();
    existingAddresses.clear();
    newAddresses.clear();

    widget.pushButton_2->setEnabled(true);
}

QString MainForm::expandQuadrant(QString street) {
    return street.replace("NE", "Northeast").replace("NW", "Northwest")
            .replace("SE", "Southeast").replace("SW", "Southwest");
}

QString MainForm::toTitleCase(QString str) {
    QRegExp re("\\W\\w");
    int pos = -1;
    str = str.toLower();
    QChar *base = str.data();
    QChar *ch;
    do {
        pos++;
        ch = base + pos;
        pos = str.indexOf(re, pos);
        *ch = ch->toUpper();
    } while (pos >= 0);
    return str;
}

MainForm::~MainForm() {
    delete nam;
    delete factory;
}
