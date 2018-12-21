#-------------------------------------------------
#
# Project created by QtCreator 2013-05-01T17:02:04
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = corrected-version
TEMPLATE = app


SOURCES += main.cpp\
        epuckuserinterface.cpp \
    epuckinterface.cpp \
    serialport/serialport.cpp

HEADERS  += epuckuserinterface.h \
    epuckinterface.h \
    serialport/serialport.h

FORMS    += epuckuserinterface.ui
