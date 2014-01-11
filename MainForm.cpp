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
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(readOSM(QNetworkReply*)));
}

void MainForm::setAddressFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
            tr("OSM File (*.osm);;XML File (*.xml)"));
    if (fileName.length() != 0) {
        widget.lineEdit->setText(fileName);
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
                        int nodeId = reader.attributes().value("id").toString().toInt();
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
                        street->nodeIndices.append(reader.attributes().value("ref").toString().toInt());
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
        for (int i = 0; i < streets.size(); i++) {
            Street* street = streets.values().at(i);
            geos::geom::CoordinateSequence* nodePoints = factory
                    ->getCoordinateSequenceFactory()->create(street->nodeIndices.size(), 0);
            for (int j = 0; j < street->nodeIndices.size(); j++) {
                nodePoints->add(*(nodes.value(street->nodeIndices.at(j))->getCoordinate()));
            }
            street->path = factory->createLineString(nodePoints);
        }
        // Debug output
        widget.textBrowser->insertPlainText("Streets:\n");
        for (int i = 0; i < streets.size(); i++) {
            Street* street = streets.values().at(i);
            widget.textBrowser->insertPlainText(street->name + "\n");
        }
        widget.textBrowser->insertPlainText("\n");
        widget.textBrowser->insertPlainText("Existing Addresses:\n");
        for (int i = 0; i < existingAddresses.size(); i++) {
            Address address = existingAddresses.at(i);
            widget.textBrowser->insertPlainText(address.houseNumber + " " + address.street.name + "\n");
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
                    address->coordinate = factory->createPoint(coordinate);
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
                            double minDistance = address->coordinate->distance(closestStreet->path);
                            for (int i = 1; i < matches.size(); i++) {
                                double distance = address->coordinate->distance(matches.at(i)->path);
                                if (distance < minDistance) {
                                    closestStreet = matches.at(i);
                                    minDistance = distance;
                                }
                            }
                            address->street = *closestStreet;
                        }
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
    widget.textBrowser->insertPlainText("\n");
    widget.textBrowser->insertPlainText("New Addresses:\n");
    for (int i = 0; i < newAddresses.size(); i++) {
        Address address = newAddresses.at(i);
        widget.textBrowser->insertPlainText(address.houseNumber + " " + address.street.name + "\n");
        qDebug() << "Distance: " << address.coordinate->distance(address.street.path) * 111000;
    }
    outputChangeFile();
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
        qDebug() << fullFileName;
        QFile file(fullFileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            continue;
        }

        QXmlStreamWriter writer(&file);
        writer.setAutoFormatting(true);
        outputStartOfFile(writer);
        for (int j = i * 5000; j < newAddresses.size() && j < (i + 1) * 5000; j++) {
            Address address = newAddresses.at(j);

            writer.writeStartElement("node");
            writer.writeAttribute("id", tr("-%1").arg(j + 1));
            writer.writeAttribute("lat", QString::number(address.coordinate->getY(), 'g', 12));
            writer.writeAttribute("lon", QString::number(address.coordinate->getX(), 'g', 12));

            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:houseNumber");
            writer.writeAttribute("v", address.houseNumber);
            writer.writeEndElement();

            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:street");
            writer.writeAttribute("v", address.street.name);
            writer.writeEndElement();

            writer.writeEndElement();
        }
        outputEndOfFile(writer);
    }
    cleanup();
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
    nodes.clear();
    streets.clear();
    existingAddresses.clear();
    newAddresses.clear();

    widget.pushButton_2->setEnabled(true);
}

QString MainForm::expandQuadrant(QString street) {
    return street.replace("NE", "Northeast").replace("NW", "Northwest")
            .replace("SE", "Southeast").replace("SW", "Southwest");
}

MainForm::~MainForm() {
    delete nam;
}
