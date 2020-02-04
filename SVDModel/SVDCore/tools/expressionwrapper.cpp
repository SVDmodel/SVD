/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

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
#include "modules/module.h"

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
                                                       // "id", ??
                                                       "x",
                                                       "y",
                                                       "year",
                                                       "environmentId"
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
        PointF coord =  Model::instance()->landscape()->grid().cellCenterPoint( cell.cellIndex() );
        return coord.x();
    }
    case 3:  { // y
        PointF coord =  Model::instance()->landscape()->grid().cellCenterPoint( cell.cellIndex() );
        return coord.y();
    }
    case 4:
        return static_cast<double>(Model::instance()->year());
    case 5:
        return static_cast<double>(cell.environment()->id());

    default: throw std::logic_error("invalid variable in CellWrapper: " + to_string(variableIndex));
    }

}


/* ***********************************
 * Wrapper for cells
*/

std::vector<std::string> CellWrapper::mVariableList = { "index", "environmentId" "climateId", "stateId", "residenceTime", "function", "structure" };
std::vector<std::pair<std::string, std::string> > CellWrapper::mVariablesMetaData;
size_t CellWrapper::mMaxStateVar = 0;
size_t CellWrapper::mMaxEnvVar = 0;
size_t CellWrapper::mMaxClimVar = 0;
std::vector<std::pair<const Module*, size_t> > CellWrapper::mModules;

void CellWrapper::setupVariables(EnvironmentCell *ecell, const State *astate)
{
    mVariableList = {  "index", "environmentId", "climateId", "elevation" ,"stateId", "residenceTime", "function", "structure" }; // reset
    mVariablesMetaData = {
        { "General", "0-based cell index" }, // index
        { "General", "Id of the environment zone" }, // environmentId
        { "General", "Id of the climate zone" }, // climateId
        { "General", "elevation (m) of the cell" }, // elevation
        { "General", "Id of the cell state" }, // stateId
        { "General", "residence time of the cell (years)" }, // residenceTime
        { "General", "ecosystem functioning class of the cell" }, // function
        { "General", "ecosystem structure class of the cell" }, // structure
    };
    mModules.clear();

    // add variables from states
    const auto &names = astate->valueNames();
    for (auto &v : names) {
        if (indexOf(mVariableList, v)>=0) {
            spdlog::get("main")->error("Setup of variable names for CellWrapper: state variable '{}' already exists! (list of variables so far: {})", v, join(mVariableList));
            throw std::logic_error("Error in setting up variable names (check log).");
        }
        mVariableList.push_back(v);
        mVariablesMetaData.push_back({"State", "State variable (user-defined)"});
    }
    mMaxStateVar = mVariableList.size();

    // add environment variables
    const auto &enames = ecell->variables();
    for (auto &v : enames) {
        if (indexOf(mVariableList, v)>=0) {
            spdlog::get("main")->error("Setup of variable names for CellWrapper: environment variable '{}' already exists! (list of variables so far: {})", v, join(mVariableList));
            throw std::logic_error("Error in setting up variable names (check log).");
        }

        mVariableList.push_back(v);
        mVariablesMetaData.push_back({"Environment", "Environment variable (user-defined)"});
    }
    mMaxEnvVar = mVariableList.size();

    for (auto &v : Model::instance()->climate()->climateVariables()) {
        if (indexOf(mVariableList, v)>=0) {
            spdlog::get("main")->error("Setup of variable names for CellWrapper: climate variable '{}' already exists! (list of variables so far: {})", v, join(mVariableList));
            throw std::logic_error("Error in setting up variable names (check log).");
        }
        mVariableList.push_back(v);
        mVariablesMetaData.push_back({"Climate", "Climate variable (user-defined)"});
    }
    mMaxClimVar = mVariableList.size();
}

void CellWrapper::setupVariables(const Module *module)
{
    auto vars = module->moduleVariableNames();
    if (vars.size()==0)
        return;
    for (size_t i=0;i<vars.size();++i) {
        mVariableList.push_back(vars[i].first);
        mModules.push_back(std::pair<const Module*, size_t>(module, i));
        mVariablesMetaData.push_back({module->name(), vars[i].second});
    }
}



double CellWrapper::value(const size_t variableIndex)
{
    const size_t NFixedVariables = 8;

    if (variableIndex < NFixedVariables) {
        // fixed variables: id, climateId, ...

        switch (variableIndex) {
        case 0: return mData->cellIndex();
        case 1: return static_cast<double>(mData->environment()->id());
        case 2: return static_cast<double>(mData->environment()->climateId());
        case 3: return static_cast<double>(mData->elevation());
        case 4: return static_cast<double>(mData->stateId());
        case 5: return static_cast<double>(mData->residenceTime());
        case 6: return static_cast<double>( mData->state() ? mData->state()->function() : 0); // function
        case 7: return static_cast<double>( mData->state() ? mData->state()->structure() : 0); // structure

        }


    } else if (variableIndex < mMaxStateVar) {
        // state variable
        const State *s = mData->state();
        return s->value(variableIndex - NFixedVariables);
    } else if (variableIndex < mMaxEnvVar){
        // environment variable
        const EnvironmentCell *ec = mData->environment();
        return ec->value(variableIndex - mMaxStateVar);
    } else if (variableIndex < mMaxClimVar) {
        const EnvironmentCell *ec = mData->environment();
        return Model::instance()->climate()->value( variableIndex - mMaxEnvVar, ec->climateId() );
    } else {
        // module variable
        size_t mod_idx = variableIndex - mMaxClimVar;
        if (mod_idx < mModules.size()) {
            return mModules[mod_idx].first->moduleVariable( mData, mModules[mod_idx].second );
        }

    }
    return 0.;
}

double CellWrapper::localStateAverage(size_t stateId)
{
    return mData->stateFrequencyLocal(static_cast<state_t>(stateId));
}

double CellWrapper::intermediateStateAverage(size_t stateId)
{
    return mData->stateFrequencyIntermediate(static_cast<state_t>(stateId));
}

double CellWrapper::globalStateAverage(size_t stateId)
{
    auto shist = Model::instance()->states()->stateHistogram();

    return shist[stateId] / static_cast<double>( Model::instance()->landscape()->NCells() );
}
