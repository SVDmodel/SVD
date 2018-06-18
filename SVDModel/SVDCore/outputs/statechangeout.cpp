#include "statechangeout.h"
#include "model.h"
#include "tools.h"
#include "expressionwrapper.h"

StateChangeOut::StateChangeOut()
{
    setName("StateChange");
    setDescription("Details for individual state changes (potentially many output data!)");
    mInterval=0; // every year
    mCellId=-1; // all cells
}

void StateChangeOut::setup()
{
    auto lg = spdlog::get("setup");
    mInterval = Model::instance()->settings().valueInt(key("interval"));
    mFilter.setExpression(Model::instance()->settings().valueString(key("filter")));

    openOutputFile("file", false); // false: do not write header

    file() << "year,cellId,state,restime,nextState,nextTime,s1,p1,s2,p2,s3,p3,s4,p4,s5,p5,s6,p6,s7,p7,s8,p8,s9,p9,s10,p10,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10" << std::endl;


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
    CellWrapper wrap(&id);
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
