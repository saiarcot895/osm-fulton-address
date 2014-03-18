/*
 * File:   mainForm.cpp
 * Author: saikrishna
 *
 * Created on January 1, 2014, 9:14 AM
 */

#include "MainForm.h"
#include "ui_MainForm.h"
#include "QFileDialog"
#include "QDebug"

MainForm::MainForm(QWidget *parent) :
    QMainWindow(parent),
    widget(new Ui::mainForm),
    converter(NULL)
{
    widget->setupUi(this);

    widget->doubleSpinBox->setValue(33.7857);
    widget->doubleSpinBox_2->setValue(-84.3982);
    widget->doubleSpinBox_3->setValue(33.7633);
    widget->doubleSpinBox_4->setValue(-84.3574);

    connect(widget->pushButton, SIGNAL(clicked()), this, SLOT(setAddressFile()));
    connect(widget->pushButton_2, SIGNAL(clicked()), this, SLOT(convert()));
    connect(widget->pushButton_3, SIGNAL(clicked()), this, SLOT(setOutputFile()));
    connect(widget->pushButton_4, SIGNAL(clicked()), this, SLOT(setZipCodeFile()));
    connect(widget->pushButton_5, SIGNAL(clicked()), this, SLOT(setBuildingFile()));
    connect(widget->pushButton_6, SIGNAL(clicked()), this, SLOT(setTaxParcelsFile()));
}

MainForm::MainForm(QStringList options, QWidget *parent) :
    MainForm(parent) {
    while (!options.isEmpty()) {
        QString option = options.takeFirst();
        QStringList params = option.remove("--").split('=');
        if (params.size() == 2) {
            QString variable = params.at(0);
            if (variable == "address") {
                widget->lineEdit->setText(params.at(1));
            } else if (variable == "building") {
                widget->lineEdit_4->setText(params.at(1));
            } else if (variable == "output") {
                widget->lineEdit_2->setText(params.at(1));
            } else if (variable == "zip-code") {
                widget->lineEdit_3->setText(params.at(1));
            } else if (variable == "tax-parcel") {
                widget->lineEdit_5->setText(params.at(1));
            } else if (variable == "bbox") {
                QStringList coords = params.at(1).split(",");
                widget->doubleSpinBox->setValue(coords.at(0).toDouble());
                widget->doubleSpinBox_2->setValue(coords.at(1).toDouble());
                widget->doubleSpinBox_3->setValue(coords.at(2).toDouble());
                widget->doubleSpinBox_4->setValue(coords.at(3).toDouble());
            }
        }
    }
}

QString MainForm::openFile() {
    return QFileDialog::getOpenFileName(this, tr("Open File"), "",
            tr("OSM File (*.osm);;XML File (*.xml)"));
}

void MainForm::setAddressFile() {
    QString fileName = openFile();
    if (!fileName.isEmpty()) {
        widget->lineEdit->setText(fileName);
    }
}

void MainForm::setZipCodeFile() {
    QString fileName = openFile();
    if (!fileName.isEmpty()) {
        widget->lineEdit_3->setText(fileName);
    }
}

void MainForm::setBuildingFile() {
    QString fileName = openFile();
    if (!fileName.isEmpty()) {
        widget->lineEdit_4->setText(fileName);
    }
}

void MainForm::setTaxParcelsFile() {
    QString fileName = openFile();
    if (!fileName.isEmpty()) {
        widget->lineEdit_5->setText(fileName);
    }
}

void MainForm::setOutputFile() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "",
            tr("OsmChange File (*.osc)"));
    if (fileName.length() != 0) {
        if (fileName.endsWith(".osc")) {
            widget->lineEdit_2->setText(fileName);
        } else {
            widget->lineEdit_2->setText(fileName + ".osc");
        }

    }
}

void MainForm::convert() {
    delete converter;
    converter = new FultonCountyConverter(this);

    widget->pushButton_2->setEnabled(false);
    widget->tab->setEnabled(false);
    widget->tabWidget->setCurrentIndex(1);
    qApp->processEvents();

    FultonCountyConverter::Logging logOptions(FultonCountyConverter::NoLogging);

    if (widget->checkBox->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::ExistingStreets;
    }
    if (widget->checkBox_2->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::ExistingAddresses;
    }
    if (widget->checkBox_3->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::NewAddressesBeforeValidation;
    }
    if (widget->checkBox_4->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::CloseAddresses;
    }
    if (widget->checkBox_5->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::AddressesCloseToStreet;
    }
    if (widget->checkBox_6->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::AddressesFarFromStreet;
    }
    if (widget->checkBox_7->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::NewAddressesAfterValidation;
    }
    if (widget->checkBox_8->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::ZipCodesLoaded;
    }
    if (widget->checkBox_9->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::OverlappingBuildings;
    }
    if (widget->checkBox_10->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::MergedAddressesAndBuilding;
    }
    if (widget->checkBox_11->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::AddressesNotInZipCodeArea;
    }
    if (widget->checkBox_12->isChecked()) {
        logOptions = logOptions | FultonCountyConverter::AddressesButNoStreet;
    }

    converter->setBoundingBox(widget->doubleSpinBox->value(), widget->doubleSpinBox_2->value(),
                             widget->doubleSpinBox_3->value(), widget->doubleSpinBox_4->value());
    converter->setAddresses(widget->lineEdit->text());
    converter->setBuildings(widget->lineEdit_4->text());
    converter->setTaxParcels(widget->lineEdit_5->text());
    converter->setZipCodes(widget->lineEdit_3->text());
    converter->setLogOptions(logOptions);
    converter->convert();

    connect(converter, SIGNAL(conversionFinished()), this, SLOT(onConversionFinished()));
}

void MainForm::onConversionFinished() {
    disconnect(converter, SIGNAL(conversionFinished()), this, SLOT(onConversionFinished()));

    widget->textBrowser->setPlainText(converter->getLog());

    converter->writeChangeFile(widget->lineEdit_2->text());

    widget->pushButton_2->setEnabled(true);
    widget->tab->setEnabled(true);
}

MainForm::~MainForm() {
    delete widget;
}
