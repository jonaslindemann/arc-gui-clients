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

SOURCES += src/arcstorage-ui/main.cpp\
        src/arcstorage-ui/mainwindow.cpp \
    src/arcstorage-ui/fileserver.cpp \
    src/arcstorage-ui/arcfileelement.cpp \
    src/arcstorage-ui/dragdropabletreewidget.cpp \
    src/arcstorage-ui/fileserverfactory.cpp \
    src/arcstorage-ui/filepropertiesdialog.cpp \
    src/arcstorage-ui/srmfileserver.cpp \
    src/arcstorage-ui/srmsettingsdialog.cpp \
    src/arcstorage-ui/settings.cpp \
    src/arcstorage-ui/filetransfer.cpp \
    src/arcstorage-ui/filetransferlist.cpp \
    src/arcstorage-ui/transferlistwindow.cpp \
    src/arcstorage-ui/globalstateinfo.cpp \
    src/common/arctools.cpp \
    src/common/arcproxy-utils.cpp \
    src/common/proxywindow.cpp 

HEADERS  += src/arcstorage-ui/mainwindow.h \
    src/arcstorage-ui/fileserver.h \
    src/arcstorage-ui/arcfileelement.h \
    src/arcstorage-ui/dragdropabletreewidget.h \
    src/arcstorage-ui/fileserverfactory.h \
    src/arcstorage-ui/filepropertiesdialog.h \
    src/arcstorage-ui/srmfileserver.h \
    src/arcstorage-ui/srmsettingsdialog.h \
    src/arcstorage-ui/settings.h \
    src/arcstorage-ui/filetransfer.h \
    src/arcstorage-ui/qdebugstream.h \
    src/arcstorage-ui/arcstorage.h \
    src/arcstorage-ui/filetransferlist.h \
    src/arcstorage-ui/transferlistwindow.h \
    src/arcstorage-ui/globalstateinfo.h \
    src/common/arctools.h \
    src/common/arcproxy-utils.h \
    src/common/proxywindow.h

FORMS    += src/arcstorage-ui/mainwindow.ui \
    src/arcstorage-ui/filepropertiesdialog.ui \
    src/arcstorage-ui/srmsettingsdialog.ui \
    src/arcstorage-ui/transferlistwindow.ui \
    src/common/proxywindow.ui

