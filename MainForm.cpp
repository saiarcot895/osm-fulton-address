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
#include "QXmlStreamWriter"
#include "QDebug"
#include "Address.h"

MainForm::MainForm() {
    widget.setupUi(this);

    widget.doubleSpinBox->setValue(33.7857);
    widget.doubleSpinBox_2->setValue(-84.3982);
    widget.doubleSpinBox_3->setValue(33.7633);
    widget.doubleSpinBox_4->setValue(-84.3574);

    nam = new QNetworkAccessManager(this);

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
            tr("OSM File (*.osm);;XML File (*.xml)"));
    if (fileName.length() != 0) {
        widget.lineEdit_2->setText(fileName);
    }
}

void MainForm::convert() {
    downloadOSM();
    // Read in streets and existing addresses
    // Read and filter the address XML to include only those within the BBox
    // and matching requirements
    // For each filtered address, check for a matching street, and correct case
    // Verify the address point is close to the street
    // Output list of points to an OSM change file
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
                        Coordinate coordinate;
                        int nodeId = reader.attributes().value("id").toString().toInt();
                        coordinate.lat = reader.attributes().value("lat").toString().toDouble();
                        coordinate.lon = reader.attributes().value("lon").toString().toDouble();
                        nodes.insert(nodeId, coordinate);
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
                            address->street = reader.attributes().value("v").toString();
                            existingAddresses.append(*address);
                        } else if (current == Way && reader.attributes().value("k") == "building") {
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
                        int i = streets.indexOf(*street, 0);
                        if (i != -1) {
                            Street mainStreet = streets.at(i);
                            mainStreet.nodeIndices.append(street->nodeIndices);
                            delete street;
                        } else {
                            streets.append(*street);
                        }
                        current = None;
                    }
                    break;
                default:
                    break;
            }
        }
        widget.textBrowser->insertPlainText("Streets:\n");
        for (int i = 0; i < streets.size(); i++) {
            Street street = streets.at(i);
            widget.textBrowser->insertPlainText(street.name + "\n");
        }
        widget.textBrowser->insertPlainText("\n");
        widget.textBrowser->insertPlainText("Existing Addresses:\n");
        for (int i = 0; i < existingAddresses.size(); i++) {
            Address address = existingAddresses.at(i);
            widget.textBrowser->insertPlainText(address.houseNumber + " " + address.street + "\n");
        }
        readAddressFile();
    } else {
        widget.textBrowser->insertPlainText(reply->errorString());
        qCritical() << reply->errorString();
    }
}

void MainForm::readAddressFile() {
    QFile file(widget.lineEdit->text());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QXmlStreamReader reader(&file);
    Address* address;
    bool valid = true;
    bool skip = false;
    int numLo, numHi;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name().toString() == "node") {
                    address = new Address();
                    address->coordinate.lat = reader.attributes().value("lat").toString().toDouble();
                    address->coordinate.lon = reader.attributes().value("lon").toString().toDouble();
                    skip = !(address->coordinate.lat <= widget.doubleSpinBox->value() &&
                            address->coordinate.lat >= widget.doubleSpinBox_3->value() &&
                            address->coordinate.lon >= widget.doubleSpinBox_2->value() &&
                            address->coordinate.lon <= widget.doubleSpinBox_4->value());
                } else if (reader.name().toString() == "tag" && !skip) {
                    if (reader.attributes().value("k") == "STR_NUM_LO") {
                        numLo = reader.attributes().value("v").toString().toInt();
                    } else if (reader.attributes().value("k") == "STR_NUM_HI") {
                        numHi = reader.attributes().value("v").toString().toInt();
                    } else if (reader.attributes().value("k") == "NAME") {
                        Street tempStreet;
                        tempStreet.name = expandQuadrant(reader.attributes().value("v").toString());
                        int i;
                        if ((i = streets.indexOf(tempStreet)) != -1) {
                            address->street = streets.at(i).name;
                        }
                    } else if (reader.attributes().value("k") == "FEAT_TYPE") {
                        if (reader.attributes().value("v") == "driv") {
                            address->addressType = Address::Primary;
                        } else if (reader.attributes().value("v") == "stru") {
                            address->addressType = Address::Structural;
                        }
                    } else if (reader.attributes().value("k") == "STATUS") {
                        valid &= reader.attributes().value("v") == "A";
                    } else if (reader.attributes().value("k") == "SUNSET") {
//                        valid &= reader.attributes().value("v") == "999999" ||
//                                reader.attributes().value("v") == "-1";
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.name().toString() == "node") {
                    if (!skip) {
                        if (numLo != -1 && numHi != -1) {
                            if (numLo == numHi) {
                                address->houseNumber = QString::number(numLo);
                            }
                            // define behavior for different low and high?
                        }
                        if (!address->houseNumber.isEmpty()
                                && !address->street.isEmpty()
                                && !existingAddresses.contains(*address)
                                && address->addressType != Address::Other
                                && valid) {
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
                    numLo = -1;
                    numHi = -1;
                    valid = true;
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
        widget.textBrowser->insertPlainText(address.houseNumber + " " + address.street + "\n");
    }
    outputChangeFile();
}

void MainForm::outputChangeFile() {
    QFile file(widget.lineEdit_2->text());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("osm");
    writer.writeAttribute("version", "0.6");
    for (int i = 0; i < newAddresses.size(); i++) {
        Address address = newAddresses.at(i);

        writer.writeStartElement("node");
        writer.writeAttribute("id", tr("-%1").arg(i + 1));
        writer.writeAttribute("lat", tr("%1").arg(address.coordinate.lat));
        writer.writeAttribute("lon", tr("%1").arg(address.coordinate.lon));

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:houseNumber");
        writer.writeAttribute("v", address.houseNumber);
        writer.writeEndElement();

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:street");
        writer.writeAttribute("v", address.street);
        writer.writeEndElement();

        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndDocument();
}

QString MainForm::expandQuadrant(QString street) {
    return street.replace("NE", "Northeast").replace("NW", "Northwest")
            .replace("SE", "Southeast").replace("SW", "Southwest");
}

MainForm::~MainForm() {
    delete nam;
}
