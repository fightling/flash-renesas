#-------------------------------------------------
#
# Project created by QtCreator 2014-07-18T15:23:34
#
#-------------------------------------------------

QT       += core serialport

include( common.pri )

TARGET = flash-renesas
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += $$files(*.h) 
SOURCES += $$files(*.cpp) 

