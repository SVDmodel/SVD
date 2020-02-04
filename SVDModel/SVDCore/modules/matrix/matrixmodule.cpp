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
#include "model.h"
#include "matrixmodule.h"
#include "tools.h"
#include "filereader.h"
#include "expressionwrapper.h"

MatrixModule::MatrixModule(std::string module_name) :
    Module("matrix", State::Matrix) // set name and type explcitly
{
    mName = module_name;
    // decide which type of batch to use for the module (Simple: no preprocessing)
    mBatchType = Batch::Simple;

    registerModule();
}

MatrixModule::~MatrixModule()
{

}

void MatrixModule::setup()
{
    lg = spdlog::get("setup");
    lg->info("Setup of module '{}'", name());
    auto settings = Model::instance()->settings();
    settings.requiredKeys("modules." + name(), {"transitionFile"});

    // set up the transition matrix
    std::string filename = settings.valueString("modules."+name()+".transitionFile");
    mMatrix.load(Tools::path(filename));

    // set up key formula
    std::string expr = settings.valueString("modules." + name() + ".keyFormula");
    mHasKeyFormula = false;
    if (!expr.empty()) {
        mKeyFormula.setExpression(expr);
        mHasKeyFormula = true;
        lg->debug("Module has a keyFormula: '{}'", expr);
    }

    lg->info("Setup of module '{}' complete.", name());
    lg = spdlog::get("main");


}

void MatrixModule::prepareCell(Cell *cell)
{
    // nothing to do here!
    (void)cell;
}

void MatrixModule::processBatch(Batch *batch)
{
    // run the transition probabilities for the cells in the batch
    CellWrapper cw(nullptr);
    int key=0;
    state_t new_state;
    for (size_t i=0;i<batch->usedSlots();++i) {
        Cell *cell = batch->cells()[i];
        if (mHasKeyFormula) {
            cw.setData(cell);
            key = static_cast<int>(mKeyFormula.calculate(cw));
            if (mMatrix.isValid(cell->stateId(), key))
                new_state = mMatrix.transition(cell->stateId(), key, &cw); // we have a specific transition for the given key
            else
                new_state = mMatrix.transition(cell->stateId(), 0, &cw); // use the default transition for the state
        } else {
            // no key, just execute based on current state
            new_state = mMatrix.transition(cell->stateId(), 0, &cw );
        }

        if (new_state != cell->stateId()) {
            cell->setNewState(new_state);
            if ( cell->state()->type() != Model::instance()->states()->stateById(new_state).type() )
                lg->debug("Landcover type change (to forest): from state '{}' to '{}'", cell->stateId(), new_state);

        } else {
            // re-evaluate the cell in one year
            cell->setNextStateId(new_state);
            cell->setNextUpdateTime(Model::instance()->year() + 1);
        }
    }

}
