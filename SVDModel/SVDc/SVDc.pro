QT -= gui
QT += core concurrent

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../SVDCore ../SVDCore/third_party ../SVDCore/tools ../SVDCore/core ../SVDCore/outputs


SOURCES += \
    consoleshell.cpp \
        main.cpp \
    ../SVDUI/modelcontroller.cpp \
    ../SVDUI/version.cpp


HEADERS += \
    ../SVDUI/modelcontroller.h \
    ../SVDUI/version.h \
    consoleshell.h


win32:CONFIG (release, debug|release): LIBS += -L../Predictor/release -lPredictor
else:win32:CONFIG (debug, debug|release): LIBS += -L../Predictor/debug -lPredictor

# https://forum.qt.io/topic/22298/solved-change-of-library-but-creator-does-not-build-completely
win32:CONFIG (release, debug|release): PRE_TARGETDEPS += ../Predictor/release/Predictor.lib
else:win32:CONFIG (debug, debug|release): PRE_TARGETDEPS += ../Predictor/debug/Predictor.lib


win32:CONFIG (release, debug|release): LIBS += -L../SVDCore/release -lSVDCore
else:win32:CONFIG (debug, debug|release): LIBS += -L../SVDCore/debug -lSVDCore

win32:CONFIG (release, debug|release): PRE_TARGETDEPS += ../SVDCore/release/SVDCore.lib
else:win32:CONFIG (debug, debug|release): PRE_TARGETDEPS += ../SVDCore/debug/SVDCore.lib

win32 {
LIBS += -LE:/dev/tensorflow/tensorflow/contrib/cmake/build/RelWithDebInfo -ltensorflow
LIBS += -LE:\dev\tensorflow\tensorflow\contrib\cmake\build\protobuf\src\protobuf\RelWithDebInfo -llibprotobuf
# for profiling only:
LIBS += -L"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v8.0/lib/x64" -lcudart
}
linux-g++ {
PRE_TARGETDEPS += ../SVDCore/libSVDCore.a
PRE_TARGETDEPS += ../Predictor/libPredictor.a
PRE_TARGETDEPS += /usr/lib/tensorflow-cpp/libtensorflow_cc.so
LIBS += -L../SVDCore -lSVDCore
LIBS += -L../Predictor -lPredictor
#LIBS += -L/usr/lib/tensorflow-cpp/ -libtensorflow_cc.so
}

unix:!macx: LIBS += -L/usr/lib/tensorflow-cpp/ -ltensorflow_cc

INCLUDEPATH += $$PWD/../../../../../../usr/lib/tensorflow-cpp
DEPENDPATH += $$PWD/../../../../../../usr/lib/tensorflow-cpp


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
