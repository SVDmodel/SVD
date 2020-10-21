/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#ifndef MATRIXMODULE_H
#define MATRIXMODULE_H

#include "spdlog/spdlog.h"

#include "../module.h"
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
