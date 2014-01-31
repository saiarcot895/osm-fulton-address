# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = app
DESTDIR = dist
TARGET = Converter
VERSION = 1.0.0
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += release 
PKGCONFIG +=
QT = core gui network
SOURCES += Address.cpp MainForm.cpp Street.cpp main.cpp Building.cpp
HEADERS += Address.h MainForm.h Street.h Building.h
FORMS += MainForm.ui
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build
MOC_DIR = 
RCC_DIR = 
UI_DIR = 
QMAKE_CC = gcc
QMAKE_CXX = g++
DEFINES += 
INCLUDEPATH += 
unix{
  LIBS += `geos-config --libs`
  QMAKE_CXXFLAGS=`geos-config --cflags`
}
macx{
  LIBS += -framework geos
}
