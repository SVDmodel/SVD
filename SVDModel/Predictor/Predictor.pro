#-------------------------------------------------
#
# Project created by QtCreator 2017-06-26T15:53:37
#
#-------------------------------------------------

QT       -= gui

TARGET = Predictor
TEMPLATE = lib
CONFIG += staticlib

### tensorflow compiled locally
INCLUDEPATH += e:/dev/tensorflow e:/dev/tensorflow/tensorflow/contrib/cmake/build  e:/dev/tensorflow/tensorflow/contrib/cmake/build/external/eigen_archive
INCLUDEPATH += e:/dev/tensorflow/tensorflow/contrib/cmake/build/external/nsync/public
INCLUDEPATH += e:/dev/tensorflow/third_party/eigen3 e:/dev/tensorflow/tensorflow/contrib/cmake/build/protobuf/src/protobuf/src
INCLUDEPATH += ../SVDCore ../SVDCore/core ../SVDCore/tools ../SVDCore/third_party

# https://joe-antognini.github.io/machine-learning/windows-tf-project
DEFINES +=  COMPILER_MSVC
DEFINES += NOMINMAX
#https://gist.github.com/Garoe/a6a82b75ea8277d12829eee81d6d2203
DEFINES +=  WIN32
DEFINES +=  _WINDOWS
#DEFINES +=  NDEBUG
DEFINES +=  EIGEN_AVOID_STL_ARRAY
DEFINES +=  _WIN32_WINNT=0x0A00
DEFINES +=  LANG_CXX11
DEFINES +=  COMPILER_MSVC
DEFINES +=  OS_WIN
DEFINES +=  _MBCS
DEFINES +=  WIN64
DEFINES +=  WIN32_LEAN_AND_MEAN
DEFINES +=  NOGDI
DEFINES +=  TENSORFLOW_USE_EIGEN_THREADPOOL
DEFINES +=  EIGEN_HAS_C99_MATH

win32:CONFIG(release, debug|release): DEFINES +=  _ITERATOR_DEBUG_LEVEL=0


LIBS += -LE:/dev/tensorflow/tensorflow/contrib/cmake/build/Release -ltensorflow

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
    modelinterface.cpp \
    predictortest.cpp \
    batchmanager.cpp \
    batch.cpp \
    inferencedata.cpp \
    predtest.cpp \
    dnn.cpp

HEADERS += \
    modelinterface.h \
    predictortest.h \
    batchmanager.h \
    batch.h \
    tensorhelper.h \
    inferencedata.h \
    predtest.h \
    dnn.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
