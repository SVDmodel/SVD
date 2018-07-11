#ifndef GRASSMODULE_H
#define GRASSMODULE_H

#include "spdlog/spdlog.h"

#include "module.h"
#include "transitionmatrix.h"

class GrassModule : public Module
{
public:
    GrassModule();
    ~GrassModule();

    void setup();

    void prepareCell(Cell *cell);
    void processBatch(Batch *batch);

private:
    // logging
    std::shared_ptr<spdlog::logger> lg;


    // store for transition probabilites for burned cells
    TransitionMatrix mMatrix;

};

#endif // GRASSMODULE_H
