#include "outputmanager.h"

#include "model.h"
#include "tools.h"
#include "strtools.h"
#include "filereader.h"

OutputManager::OutputManager()
{
    mOutputNames = {"state_grid", "test"};
}

OutputManager::~OutputManager()
{

}

void OutputManager::setup()
{
    auto lg = spdlog::get("setup");
    lg->info("Setup of outputs");
    auto keys = Model::instance()->settings().findKeys("output.");
    std::sort(keys.begin(), keys.end());
    for (auto s : keys) {

        auto toks = split(s, '.');
        if (toks.size() != 3)
            throw std::logic_error("Output Manager: invalid key: " + s);
        lg->debug("Output: {}, key: {} = {}", toks[1], toks[2], Model::instance()->settings().valueString(s));

        if (indexOf(mOutputNames, toks[2])==-1)
            throw std::logic_error("Output Manager: the output '" + toks[2] + "' does not exist. Available outputs are: " + join(mOutputNames));
    }

//    std::string file_name = Tools::path(Model::instance()->settings().valueString("states.file"));
//    FileReader rdr(file_name);
//    rdr.requiredColumns({"stateId", "composition", "structure", "fct"});

//    while (rdr.next()) {
//        // read line
//        state_t id = state_t( rdr.value("stateId") );
//        mStates.push_back( State(id,
//                rdr.valueString("composition"),
//                int(rdr.value("structure")),
//                int(state_t(rdr.value("fct"))))
//                           );
//        mStateSet.insert({id, mStates.size()-1}); // save id and index

//    }
//    spdlog::get("setup")->debug("Loaded {} states from file '{}'", mStates.size(), file_name);

}

void OutputManager::run()
{

}
