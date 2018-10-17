#include "outputmanager.h"

#include "model.h"
#include "tools.h"
#include "strtools.h"
#include "filereader.h"

// the individual outputs
#include "stategridout.h"
#include "restimegridout.h"
#include "statechangeout.h"
#include "modules/fire/fireout.h"

OutputManager::OutputManager()
{
    mIsSetup = false;
    mOutputs.push_back(new StateGridOut());
    mOutputs.push_back(new ResTimeGridOut());
    mOutputs.push_back(new StateChangeOut());
    mOutputs.push_back(new FireOut());
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
        if (toks.size() < 3)
            throw std::logic_error("Output Manager: invalid key: " + s);
        lg->debug("Output: {}, key: {} {}= {}",
                  toks[1],
                toks[2],
                (toks.size()>=3 ? toks[3] : ""),
                Model::instance()->settings().valueString(s));

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

    mIsSetup = true;

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

void OutputManager::yearEnd()
{
    for (auto o : mOutputs)
        o->flush();
}

std::string OutputManager::createDocumentation()
{
    std::string result;

    result = "# SVD outputs\n";
    result += "## List of outputs\n";
    for (auto o: mOutputs) {
        result += fmt::format("* [{}](#{})\n", o->name(), o->name());
    }
    result += "\n";

    for (auto o : mOutputs) {
        result += o->createDocumentation();
    }
    return result;
}

Output *OutputManager::find(std::string output_name)
{
    for (auto o : mOutputs)
        if ( o->name() == output_name)
            return o;
    return nullptr;
}
