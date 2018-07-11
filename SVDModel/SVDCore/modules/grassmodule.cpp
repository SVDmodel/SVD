#include "model.h"
#include "grassmodule.h"
#include "tools.h"
#include "filereader.h"

GrassModule::GrassModule() :
    Module("grass", State::Grass) // set name and type explcitly
{
    // decide which type of batch to use for the module (Simple: no preprocessing)
    mBatchType = Batch::Simple;

    registerModule();
}

GrassModule::~GrassModule()
{

}

void GrassModule::setup()
{
    lg = spdlog::get("setup");
    lg->info("Setup of GrassModule");
    auto settings = Model::instance()->settings();
    settings.requiredKeys("modules.grass", {"transitionFile"});

    // set up the transition matrix
    std::string filename = settings.valueString("modules.grass.transitionFile");
    mMatrix.load(Tools::path(filename));

    lg->info("Setup of GrassModule complete.");
    lg = spdlog::get("main");


}

void GrassModule::prepareCell(Cell *cell)
{
    // nothing to do here!
    (void)cell;
}

void GrassModule::processBatch(Batch *batch)
{
    // run the transition probabilities for the grass landcover
    for (size_t i=0;i<batch->usedSlots();++i) {
        Cell *cell = batch->cells()[i];

        state_t new_state = mMatrix.transition(cell->stateId() );
        if (new_state != cell->stateId()) {
            cell->setNewState(new_state);
            if ( cell->state()->type() != Model::instance()->states()->stateById(new_state).type() )
                lg->debug("Landcover type change (to forest): from state '{}' to '{}'", cell->stateId(), new_state);

        }
    }

}
