#include "modelshell.h"

ModelShell::ModelShell(QObject *parent) : QObject(parent)
{
    emit log("Model shell created.");
}

ModelShell::~ModelShell()
{
    emit log("Model shell destroyed");
}

