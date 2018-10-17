#include "statechangeout.h"
#include "model.h"
#include "tools.h"
#include "expressionwrapper.h"

StateChangeOut::StateChangeOut()
{
    setName("StateChange");
    setDescription("Details for individual state changes from DNN (potentially a lot of output data!)\n\n" \
                   "The output contains for each cell the predicted states/probabilities (for `dnn.topKNClasses` classes), " \
                   "and the probabilities for the year of state change.\n\n" \
                   "### Parameters\n" \
                   "* `filter`: a filter expression; output is written if the expression is true; available variables are: `state`, `restime`, `x`, `y`, `year`\n" \
                   "* `interval`: output is written only every `interval` years (or every year if `interval=0`). For example, a value of 10 limits output to the simulation years 1, 11, 21, ...\n");
    columns() = {
    {"year", "simulation year of the state change", DataType::Int},
    {"cellIndex", "index of the affected cell (0-based)", DataType::Int},
    {"state", "original stateId (before change)", DataType::Int},
    {"restime", "residence time (yrs) (before change)", DataType::Int},
    {"nextState", "new stateId (selected from s[i])", DataType::Int},
    {"nextTime", "year the state change (selected from t[i])", DataType::Int},
    {"s[i]", "candidate state Id *i* (with i 1..number of top-K classes)", DataType::Int},
    {"p[i]", "probability for state *i* (0..1)", DataType::Double},
    {"t[i]", "probability for a change in year *i* (with i 1..number of residence time classes)", DataType::Double},
    };

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
    std::string cap = "year,cellIndex,state,restime,nextState,nextTime";
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
