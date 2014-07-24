#-------------------------------------------------
#
# Project created by QtCreator 2014-07-18T15:23:34
#
#-------------------------------------------------

QT       += core

QT       -= gui

include( common.pri )

TARGET = flash-renesas
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../qextserialport/src
LIBS += -L../qextserialport
CONFIG += extserialport
LIBS += -lQt5ExtSerialPort

HEADERS += $$files(*.h) 
SOURCES += $$files(*.cpp) 

