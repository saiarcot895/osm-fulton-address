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

MainForm::MainForm() {
    widget.setupUi(this);

    widget.doubleSpinBox->setValue(33.7857);
    widget.doubleSpinBox_2->setValue(-84.3982);
    widget.doubleSpinBox_3->setValue(33.7633);
    widget.doubleSpinBox_4->setValue(-84.3574);

    nam = new QNetworkAccessManager(this);

    connect(widget.pushButton, SIGNAL(clicked()), this, SLOT(setAddressFile()));
    connect(widget.pushButton_2, SIGNAL(clicked()), this, SLOT(convert()));
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(readOSM(QNetworkReply*)));
}

void MainForm::setAddressFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
            tr("OSM File (*.osm);;XML File (*.xml)"));
    if (fileName.length() != 0) {
        widget.lineEdit->setText(fileName);
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
        while (!reader.atEnd()) {
            switch (reader.readNext()) {
                case QXmlStreamReader::StartElement:
                    if (reader.name().toString() == "node") {
                        Coordinate coordinate;
                        int nodeId = reader.attributes().value("id").toString().toInt();
                        coordinate.lat = reader.attributes().value("lat").toString().toDouble();
                        coordinate.lon = reader.attributes().value("lon").toString().toDouble();
                        nodes.insert(nodeId, coordinate);
                    } else if (reader.name().toString() == "way") {

                    }

                    break;
                case QXmlStreamReader::EndElement:
                    break;
                default:
                    break;
            }
        }
    } else {
        qWarning() << reply->errorString();
    }
}

MainForm::~MainForm() {
}
