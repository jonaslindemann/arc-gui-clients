#-------------------------------------------------
#
# Project created by QtCreator 2011-01-23T23:16:09
#
#-------------------------------------------------

QT       += core gui
QT       += network

CONFIG   += no_keywords

TARGET = arcstorage-ui
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    fileserver.cpp \
    localfileserver.cpp \
    arcfileelement.cpp \
    ftpfileserver.cpp \
    dragdropabletreewidget.cpp \
    fileserverfactory.cpp \
    filepropertiesdialog.cpp \
    srmfileserver.cpp \
    srmsettingsdialog.cpp \
    settings.cpp \
    filetransfer.cpp

HEADERS  += mainwindow.h \
    fileserver.h \
    localfileserver.h \
    arcfileelement.h \
    ftpfileserver.h \
    dragdropabletreewidget.h \
    fileserverfactory.h \
    filepropertiesdialog.h \
    srmfileserver.h \
    srmsettingsdialog.h \
    settings.h \
    filetransfer.h \
    qdebugstream.h \
    arcstorage.h

FORMS    += mainwindow.ui \
    filepropertiesdialog.ui \
    srmsettingsdialog.ui

