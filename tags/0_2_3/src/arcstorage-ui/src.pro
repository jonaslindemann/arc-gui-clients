#-------------------------------------------------
#
# Project created by QtCreator 2011-01-23T23:16:09
#
#-------------------------------------------------

macx {
    !include( ../macx.pri ) {
        error( Couldn't find the macx.pri file! )
    }
} else {
    unix {
        !include( ../unix.pri ) {
            error( Couldn't find the glib.pri file! Run configure. )
        }
        !include( ../glib.pri ) {
            error( Couldn't find the glib.pri file! Run configure. )
        }
        !include( ../glibmm.pri ) {
            error( Couldn't find the glibmm.pri file! Run configure )
        }
        !include( ../libxml.pri ) {
            error( Couldn't find the libxml.pri file! Run configure )
        }
    } else {
        !include( ../win32.pri ) {
            error( Couldn't find the win32.pri file! )
        }
    }
}

!include( ../common.pri ) {
    error( "Couldn not find the common.pri file!" )
}

QT       += core gui
QT       += network

CONFIG   += no_keywords

TARGET = arcstorage-ui
TEMPLATE = app

PREFIX = /usr

BINDIR = $$PREFIX/bin
DATADIR = $$PREFIX/share

INSTALLS += target desktop icon

target.path = $$BINDIR

desktop.path = $$DATADIR/applications
desktop.files += arcstorage-ui.desktop

#icon.path = $$DATADIR/icons/hicolor/48x48/apps
#icon.files += arcstorage-ui.png

target.path = $$BINDIR
INSTALLS += target

INCLUDEPATH += $$ARC_INCLUDE

macx {
	INCLUDEPATH += $$GLIBMM_INCLUDE
	INCLUDEPATH += $$SIGCXX_INCLUDE
	INCLUDEPATH += $$GLIB_INCLUDE
	INCLUDEPATH += $$XML2_INCLUDE
}

LIBS += $$ARC_LIBS
LIBS += -larcclient
LIBS += -larccommon
LIBS += -larcdata2
LIBS += -larccredential

macx {
	LIBS += $$GLIBMM_LIBS
	LIBS += $$GLIB_LIBS
	LIBS += $$SIGCXX_LIBS
}

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

