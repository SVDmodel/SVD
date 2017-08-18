#-------------------------------------------------
#
# Project created by QtCreator 2017-06-26T14:07:19
#
#-------------------------------------------------

QT       -= gui
QT       += concurrent

TARGET = SVDCore
TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += third_party

CONFIG += c++14
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        svdcore.cpp \
    modelshell.cpp \
    toymodel.cpp \
    tools/gisgrid.cpp \
    tools/grid.cpp \
    tools/strtools.cpp \
    tools/filereader.cpp \
    tools/settings.cpp \
    tools/randomgen.cpp

HEADERS += \
        svdcore.h \
    modelshell.h \
    toymodel.h \
    tools/gisgrid.h \
    tools/grid.h \
    tools/strtools.h \
    tools/filereader.h \
    tools/settings.h \
    tools/randomgen.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
