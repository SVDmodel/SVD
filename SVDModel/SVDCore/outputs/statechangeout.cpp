#include "statechangeout.h"
#include "model.h"
#include "tools.h"
#include "expressionwrapper.h"

StateChangeOut::StateChangeOut()
{
    setName("StateChange");
    setDescription("Details for individual state changes from DNN (potentially a lot of output data!)");
    mInterval=0; // every year
    mCellId=-1; // all cells
}

void StateChangeOut::setup()
{
    auto lg = spdlog::get("setup");
    mInterval = Model::instance()->settings().valueInt(key("interval"));
    mFilter.setExpression(Model::instance()->settings().valueString(key("filter")));
    int n_time = Model::instance()->settings().valueInt("dnn.restime.N");
    int n_prob = Model::instance()->settings().valueInt("dnn.topKNClasses");

    openOutputFile("file", false); // false: do not write header
    std::string cap = "year,cellId,state,restime,nextState,nextTime";
    for (int i=0;i<n_prob;++i) cap += ",s" + to_string(i+1) + ",p" + to_string(i+1);
    for (int i=0;i<n_time;++i) cap += ",t" + to_string(i+1);

    file() << cap << std::endl;


}

void StateChangeOut::execute()
{
    int year =  Model::instance()->year();
    if (mInterval>0)
        if (year % mInterval != 1)
            return;


}

static std::mutex filter_mtx;
bool StateChangeOut::shouldWriteOutput(const InferenceData &id)
{
    int year =  Model::instance()->year();
    if (mInterval>0)
        if (year % mInterval != 1)
            return false;

    if (mFilter.isEmpty())
        return true;

    std::lock_guard<std::mutex> guard(filter_mtx);
    InferenceDataWrapper wrap(&id);
    if (mFilter.calculateBool(wrap))
        return true;
    return false;
}

static std::mutex output_mutex;
void StateChangeOut::writeLine(std::string content)
{
    std::lock_guard<std::mutex> guard(output_mutex);
    out() << content;
    out().write();
}
