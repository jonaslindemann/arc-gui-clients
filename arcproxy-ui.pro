#-------------------------------------------------
#
# Project created by QtCreator 2012-03-03T18:11:29
#
#-------------------------------------------------

QT       += core gui

TARGET = arcproxy-ui
TEMPLATE = app


SOURCES += src/arcproxy-ui/main.cpp\
        src/common/proxywindow.cpp \
    src/common/arcproxy-utils.cpp \
    src/common/arcproxy-utils-functions.cpp \
    src/common/helpwindow.cpp

HEADERS  += src/common/proxywindow.h \
    src/common/arcproxy-utils.h \
    src/common/arcproxy-utils-functions.h \
    src/common/helpwindow.h

FORMS    += src/common/proxywindow.ui \
    src/common/helpwindow.ui
