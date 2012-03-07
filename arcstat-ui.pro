#-------------------------------------------------
#
# Project created by QtCreator 2011-12-28T12:50:06
#
#-------------------------------------------------

QT       += core gui

TARGET = arcstat-ui
TEMPLATE = app


SOURCES += src/arcstat-ui/main.cpp\
        src/artstat-ui/mainwindow.cpp \
    src/arcstat-ui/arcjobcontroller.cpp \
    src/arcstat-ui/jobinfo.cpp

HEADERS  += src/arcstat-ui/mainwindow.h \
    src/arcstat-ui/arcjobcontroller.h \
    src/arcstat-ui/jobinfo.h

FORMS    += src/arcstat-ui/mainwindow.ui

RESOURCES += \
    src/arcstat-ui/arcstat-ui.qrc
