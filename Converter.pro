#-------------------------------------------------
#
# Project created by QtCreator 2014-02-05T12:21:51
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Converter
TEMPLATE = app
CONFIG += c++11


SOURCES += main.cpp\
    Address.cpp \
    Building.cpp \
    MainForm.cpp \
    Street.cpp \
    node.cpp

HEADERS  += \
    Address.h \
    Building.h \
    MainForm.h \
    Street.h \
    node.h

FORMS += \
    MainForm.ui


unix{
  LIBS += `geos-config --libs`
  QMAKE_CXXFLAGS=`geos-config --cflags`
}
macx{
  LIBS += -framework geos
}
