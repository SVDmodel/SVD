/** @class ExpressionWrapper
  The base class for objects that can be used within Expressions.
  Derived from ExpressionWrapper are wrappers for e.g. Trees or ResourceUnits.
  They must provide a getVariablesList() and a value() function.
  Note: the must also provide "virtual double value(const QString &variableName) { return value(variableName); }"
      because it seems to be not possible in C++ to use functions from derived and base class simultaneously that only differ in the
      argument signature.
  @sa Expression

  */
#include "expressionwrapper.h"
#include "strtools.h"
#include "../Predictor/inferencedata.h"
#include "model.h"

#define VARCOUNT 52
static const char *VarList[VARCOUNT]={"bhd", "alter", "hoehe", "art", "id", "vorrat",
                               "npp", "gpp", "leafarea", "leafweight", "mstem",
                               "mfoliage", "mtwigs", "x","y","patchindex",
                               "highlight", "ali","kronenansatz",
                               "areamod", "stress",
                               "lightindex", "lightresponse",
                               "flight", "fsite", "fenv",
                               "biomassbu", "biomasstd",
                               "bhdclass", "hoeheclass", "ageclass",
                               "ncontent", "cngrowth", "yearsdead",
                               "deltahpot", "gruppe", "vage", "sage",
                               "agemax", "birthyear", "basalarea", "mortreason", "tag",
                               "addtag", "chestnutblight", "deathyear", "dbh", "age", "height", "species", "volume",
                               "crownradius"};


static std::vector<std::string> baseVarList;
static size_t baseVarListCount=0;

static std::vector<std::string> treeVarList;


void set_strings()
{
    if (baseVarList.size()!=0)
        return;
    // ********* base **********
    baseVarList.push_back("year");
    baseVarListCount = baseVarList.size();

    // ********* tree **********

    // also add base vars to varlist
    for (size_t i=0;i<baseVarListCount;i++)
        treeVarList.push_back(baseVarList[i]);

    for (int i=0;i<VARCOUNT;i++)
        treeVarList.push_back(VarList[i]);
}


ExpressionWrapper::ExpressionWrapper()
{
    set_strings();
}


// must be overloaded!


const std::vector<std::string> &ExpressionWrapper::getVariablesList()
{
    throw std::logic_error("expression wrapper reached base getVariableList");
}
// must be overloaded!
double ExpressionWrapper::value(const size_t variableIndex)
{
    switch (variableIndex) {
    case 0: // year
        return  0.;
    }
    throw std::logic_error("expression wrapper reached base with invalid index index " + to_string(variableIndex));
}

int ExpressionWrapper::variableIndex(const std::string &variableName)
{
    return index_of(getVariablesList(), variableName);
}

double ExpressionWrapper::valueByName(const std::string &variableName)
{
    size_t idx = static_cast<size_t>(variableIndex(variableName));
    return value(idx);
}

/***********************
*** Tree Wrapper     ***
***********************/
/*
const std::vector<std::string> &TreeWrapper::getVariablesList()
{
    return treeVarList;
}


double TreeWrapper::value(const int variableIndex)
{
    ASSERT(mTree!=0);


    if (variableIndex >= baseVarListCount)
       return mTree->GetVar(variableIndex-baseVarListCount);

    return ExpressionWrapper::value(variableIndex);
}
*/
std::vector<std::string> InferenceDataWrapper::mVariableList = {"state",
                                                       "restime",
                                                       "id",
                                                       "x",
                                                       "y",
                                                       "year"
                                                      };



double InferenceDataWrapper::value(const size_t variableIndex)
{
    const Cell &cell=mData->cell();
    switch (variableIndex) {
    case 0: // state
        return static_cast<double>(cell.stateId());
    case 1: // restime
        return static_cast<double>(cell.residenceTime());
    case 2:  { // x
        PointF coord =  Model::instance()->landscape()->grid().cellCenterPoint( Model::instance()->landscape()->grid().indexOf(&cell) );
        return coord.x();
    }
    case 3:  { // y
        PointF coord =  Model::instance()->landscape()->grid().cellCenterPoint( Model::instance()->landscape()->grid().indexOf(&cell) );
        return coord.y();
    }
    case 4: return static_cast<double>(Model::instance()->year());
    default: throw std::logic_error("invalid variable in CellWrapper: " + to_string(variableIndex));
    }

}


/* ***********************************
 * Wrapper for cells
*/

std::vector<std::string> CellWrapper::mVariableList = { "id", "climateId", "stateId", "residenceTime" };
size_t CellWrapper::mMaxStateVar = 0;

void CellWrapper::setupVariables(EnvironmentCell *ecell, const State *astate)
{
    mVariableList = { "id", "climateId", "stateId", "residenceTime" }; // reset

    // add variables from states
    for (auto &v : astate->valueNames()) {
        if (indexOf(mVariableList, v)>=0) {
            spdlog::get("main")->error("Setup of variable names for CellWrapper: state variable '{}' already exists! (list of variables so far: {})", v, join(mVariableList));
            throw std::logic_error("Error in setting up variable names (check log).");
        }
        mVariableList.push_back(v);
    }
    mMaxStateVar = mVariableList.size();

    // add environment variables
    for (auto &v : ecell->variables()) {
        if (indexOf(mVariableList, v)>=0) {
            spdlog::get("main")->error("Setup of variable names for CellWrapper: environment variable '{}' already exists! (list of variables so far: {})", v, join(mVariableList));
            throw std::logic_error("Error in setting up variable names (check log).");
        }

        mVariableList.push_back(v);
    }


}



double CellWrapper::value(const size_t variableIndex)
{

    if (variableIndex < 4) {
        // fixed variables: id, climateId

        switch (variableIndex) {
        case 0: return static_cast<double>(mData->environment()->id());
        case 1: return static_cast<double>(mData->environment()->climateId());
        case 2: return static_cast<double>(mData->stateId());
        case 3: return static_cast<double>(mData->residenceTime());
        }


    } else if (variableIndex < mMaxStateVar) {
        // state variable
        const State *s = mData->state();
        return s->value(variableIndex - 4);
    } else {
        const EnvironmentCell *ec = mData->environment();
        ec->value(variableIndex - mMaxStateVar);
    }
    return 0.;
}
