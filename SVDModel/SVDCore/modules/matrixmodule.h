#ifndef MATRIXMODULE_H
#define MATRIXMODULE_H

#include "spdlog/spdlog.h"

#include "module.h"
#include "transitionmatrix.h"
#include "expression.h"

class MatrixModule : public Module
{
public:
    MatrixModule(std::string module_name);
    ~MatrixModule();

    void setup();

    void prepareCell(Cell *cell);
    void processBatch(Batch *batch);

private:
    // logging
    std::shared_ptr<spdlog::logger> lg;


    // store for transition probabilites for burned cells
    TransitionMatrix mMatrix;
    Expression mKeyFormula;
    bool mHasKeyFormula;

};

#endif // MATRIXMODULE_H
