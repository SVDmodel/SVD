#ifndef MATRIXMODULE_H
#define MATRIXMODULE_H

#include "spdlog/spdlog.h"

#include "module.h"
#include "transitionmatrix.h"

class MatrixModule : public Module
{
public:
    MatrixModule();
    ~MatrixModule();

    void setup();

    void prepareCell(Cell *cell);
    void processBatch(Batch *batch);

private:
    // logging
    std::shared_ptr<spdlog::logger> lg;


    // store for transition probabilites for burned cells
    TransitionMatrix mMatrix;

};

#endif // MATRIXMODULE_H
