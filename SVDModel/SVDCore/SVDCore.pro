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

INCLUDEPATH += third_party tools core ../core ../tools ../outputs
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
    modelshell.cpp \
    outputs/statehistout.cpp \
    tools/grid.cpp \
    tools/strtools.cpp \
    tools/filereader.cpp \
    tools/settings.cpp \
    tools/randomgen.cpp \
    core/model.cpp \
    core/landscape.cpp \
    core/cell.cpp \
    core/states.cpp \
    core/climate.cpp \
    tools/tools.cpp \
    core/environmentcell.cpp \
    modelrunstate.cpp \
    outputs/output.cpp \
    outputs/outputmanager.cpp \
    outputs/stategridout.cpp \
    outputs/restimegridout.cpp \
    core/externalseeds.cpp \
    outputs/statechangeout.cpp \
    tools/expression.cpp \
    tools/expressionwrapper.cpp \
    core/transitionmatrix.cpp \
    modules/fire/firemodule.cpp \
    modules/fire/fireout.cpp \
    modules/module.cpp \
    modules/matrix/matrixmodule.cpp

HEADERS += \
    modelshell.h \
    outputs/statehistout.h \
    tools/grid.h \
    tools/strtools.h \
    tools/filereader.h \
    tools/settings.h \
    tools/randomgen.h \
    core/model.h \
    core/landscape.h \
    core/cell.h \
    core/states.h \
    core/climate.h \
    tools/tools.h \
    core/environmentcell.h \
    modelrunstate.h \
    outputs/output.h \
    outputs/outputmanager.h \
    outputs/stategridout.h \
    outputs/restimegridout.h \
    core/externalseeds.h \
    outputs/statechangeout.h \
    tools/expression.h \
    tools/expressionwrapper.h \
    core/transitionmatrix.h \
    modules/fire/firemodule.h \
    modules/fire/fireout.h \
    modules/module.h \
    modules/matrix/matrixmodule.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
