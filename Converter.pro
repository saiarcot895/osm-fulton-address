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

CONFIG(debug, debug|release) {
    CONFIG += address_sanitizer
    QMAKE_CXXFLAGS += "-fsanitize=address -fno-omit-frame-pointer"
    QMAKE_CFLAGS += "-fsanitize=address -fno-omit-frame-pointer"
    QMAKE_LFLAGS += "-fsanitize=address"
}


SOURCES += main.cpp\
    Address.cpp \
    Building.cpp \
    MainForm.cpp \
    Street.cpp \
    node.cpp \
    fultoncountyconverter.cpp \
    mainconsole.cpp

HEADERS  += \
    Address.h \
    Building.h \
    MainForm.h \
    Street.h \
    node.h \
    AddressPrivate.h \
    StreetPrivate.h \
    BuildingPrivate.h \
    fultoncountyconverter.h \
    mainconsole.h \
    rtree.h

FORMS += \
    MainForm.ui


unix{
  LIBS += `geos-config --libs`
  QMAKE_CXXFLAGS += `geos-config --cflags`
}
macx{
  LIBS += -framework geos
}
