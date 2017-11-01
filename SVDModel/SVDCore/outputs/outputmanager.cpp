#include "outputmanager.h"

#include "model.h"
#include "tools.h"
#include "strtools.h"
#include "filereader.h"

// the individual outputs
#include "stategridout.h"

OutputManager::OutputManager()
{
    mOutputs.push_back(new StateGridOut());
}

OutputManager::~OutputManager()
{
    delete_and_clear(mOutputs);
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

        Output *o = find(toks[1]);
        if (o == nullptr)
            throw std::logic_error("Output Manager: the output '" + toks[1] + "' does not exist. Key: " + s);

    }
    for (auto o : mOutputs) {
        // check if enabled:
        if (Model::instance()->settings().valueBool("output." + o->name() + ".enabled")) {
            o->setEnabled(true);
            o->setup();
        } else {
            o->setEnabled(false);
        }
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

bool OutputManager::run(const std::string &output_name)
{
    Output *o = find(output_name);
    if (o==nullptr)
        throw std::logic_error("Output Manager: invalid output '"+output_name+"' in run().");
    if (o->enabled())
        o->execute();
    return o->enabled();
}

void OutputManager::run()
{

}

Output *OutputManager::find(std::string output_name)
{
    for (auto o : mOutputs)
        if ( o->name() == output_name)
            return o;
    return nullptr;
}
