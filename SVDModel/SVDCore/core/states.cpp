#include "states.h"

#include "model.h"
#include "tools.h"
#include "strtools.h"
#include "filereader.h"

States::States()
{

}

void States::setup()
{
    std::string file_name = Tools::path(Model::instance()->settings().valueString("states.file"));
    FileReader rdr(file_name);
    rdr.requiredColumns({"stateId", "composition", "structure", "fct"});

    while (rdr.next()) {
        // read line
        mStates.push_back( State(rdr.value("stateId"),
                rdr.valueString("composition"),
                int(rdr.value("structure")),
                int(rdr.value("fct")))
                           );

    }
    spdlog::get("setup")->debug("Loaded {} states from file '{}'", mStates.size(), file_name);

}

State::State(int id, std::string composition, int structure, int function)
{
    mId = id;
    mComposition = trimmed(composition);
    mStructure = structure;
    mFunction = function;
    auto species = Model::instance()->species();
    mSpeciesShare = std::vector<double>(species.size(), 0.);

    auto comp_species = split(mComposition, ' ');
    for (const auto &s : comp_species) {
        auto it = std::find(species.begin(), species.end(), lowercase(s));
        if (it!=species.end())
            mSpeciesShare[it-species.begin()] = 1.; // TODO: separate between dominant/other species
    }
}
