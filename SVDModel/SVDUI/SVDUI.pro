#-------------------------------------------------
#
# Project created by QtCreator 2017-06-29T17:46:43
#
#-------------------------------------------------

QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SVDUI
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += ../SVDCore ../SVDCore/third_party

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    modelcontroller.cpp \
    testdnn.cpp \
    integratetest.cpp

HEADERS += \
        mainwindow.h \
    modelcontroller.h \
    testdnn.h \
    integratetest.h

FORMS += \
        mainwindow.ui \
    testdnn.ui

win32:CONFIG (release, debug|release): LIBS += -L../Predictor/release -lPredictor
else:win32:CONFIG (debug, debug|release): LIBS += -L../Predictor/debug -lPredictor

# https://forum.qt.io/topic/22298/solved-change-of-library-but-creator-does-not-build-completely
win32:CONFIG (release, debug|release): PRE_TARGETDEPS += ../Predictor/release/Predictor.lib
else:win32:CONFIG (debug, debug|release): PRE_TARGETDEPS += ../Predictor/debug/Predictor.lib


win32:CONFIG (release, debug|release): LIBS += -L../SVDCore/release -lSVDCore
else:win32:CONFIG (debug, debug|release): LIBS += -L../SVDCore/debug -lSVDCore

win32:CONFIG (release, debug|release): PRE_TARGETDEPS += ../SVDCore/release/SVDCore.lib
else:win32:CONFIG (debug, debug|release): PRE_TARGETDEPS += ../SVDCore/debug/SVDCore.lib


LIBS += -LE:/dev/tensorflow/tensorflow/contrib/cmake/build/Release -ltensorflow

