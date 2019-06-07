#include "statehistout.h"

#include "model.h"
#include "landscape.h"


StateHistOut::StateHistOut()
{
    setName("StateHist");
    setDescription("Outputs a frequency distribution of states over the landscape.\n\n" \
                   "### Parameters\n" \
                   " * `lastFireGrid.filter`: a grid is written only if the expression evaluates to `true` (with `year` as variable). A value of 0 deactivates the grid output.");
    // define the columns
    columns() = {
    {"year", "simulation year", DataType::Int},
    {"state", "stateId", DataType::Int},
    {"n", "number of cells that are currently in the state `state`", DataType::Int}   };

}

void StateHistOut::setup()
{
    openOutputFile();
}

void StateHistOut::execute()
{
    auto &grid = Model::instance()->landscape()->grid();
    const auto &states = Model::instance()->states()->states();
    int year = Model::instance()->year();
    // create a (sparse) vector of stateIds
    state_t max_state=0;
    for (const auto &s : states)
        max_state = std::max(max_state, s.id());
    std::vector<int> state_count;
    state_count.resize(static_cast<size_t>(max_state));

    // count every state on the landscape
    for (Cell *c=grid.begin(); c!=grid.end(); ++c)
        if (!c->isNull())
            state_count[static_cast<size_t>(c->stateId())]++;

    // write output table
    for (size_t i=0;i<state_count.size();++i) {
        if (state_count[i]>0) {
            out() << year << i << state_count[i];
            out().write();
        }
    }
}
