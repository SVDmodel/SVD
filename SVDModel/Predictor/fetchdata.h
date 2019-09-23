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
#ifndef FETCHDATA_H
#define FETCHDATA_H

#include "inputtensoritem.h"
#include "expression.h"
#include "strtools.h"

class Cell; // forward
class Batch; // forward
class BatchDNN; // forward
class Settings; // forward
class FetchData
{
public:
    FetchData(InputTensorItem *item) { mItem = item; }
    virtual ~FetchData() { }
    virtual void setup(const Settings *settings, const std::string &key, const InputTensorItem &item);

    virtual void fetch(Cell *cell, BatchDNN* batch, size_t slot);

    // factory function
    static FetchData *createFetchObject(InputTensorItem *def);
protected:
    InputTensorItem *mItem;

};

class FetchDataStandard : public FetchData
{
public:
    ~FetchDataStandard() {}
    FetchDataStandard(InputTensorItem *item) : FetchData(item) {}
    virtual void setup(const Settings *settings, const std::string &key, const InputTensorItem &item);
    virtual void fetch(Cell *cell, BatchDNN *batch, size_t slot);
private:
    void fetchClimate(Cell *cell, BatchDNN* batch, size_t slot);
    void fetchState(Cell *cell, BatchDNN *batch, size_t slot);
    void fetchResidenceTime(Cell *cell, BatchDNN* batch, size_t slot);
    void fetchNeighbors(Cell *cell, BatchDNN* batch, size_t slot);
    void fetchSite(Cell *cell, BatchDNN* batch, size_t slot);
    void fetchDistanceOutside(Cell *cell, BatchDNN* batch, size_t slot);
    // columns
    int i_distance;
    int i_nitrogen;
    int i_soildepth;
};


/** FetchDataVars retrieves values from the model based on Expressions that
 *  can access state/environment variables.
 * */
class FetchDataVars : public FetchData
{
public:
    ~FetchDataVars() { delete_and_clear(mExpressions); }
    FetchDataVars(InputTensorItem *item) : FetchData(item) {}
    virtual void setup(const Settings *settings, const std::string &key, const InputTensorItem &item);
    virtual void fetch(Cell *cell, BatchDNN *batch, size_t slot);
private:
    std::vector<Expression*> mExpressions; ///< list of expressions
};

/** FetchDataFunction is the gateway to more complex data items
 *
 * */
class SimpleManagementModule; // forward
class FetchDataFunction : public FetchData
{
public:
    ~FetchDataFunction() {  }
    FetchDataFunction(InputTensorItem *item) : FetchData(item) { mFn = Invalid; }
    virtual void setup(const Settings *settings, const std::string &key, const InputTensorItem &item);
    virtual void fetch(Cell *cell, BatchDNN *batch, size_t slot);

    // functions
    enum EFunctions { Invalid=0,
                      DistToSeedSource = 1,
                      SimpleManagement = 2};
private:
    EFunctions mFn;

    // entrypoints for the individual variables
    void setupDisttoSeedSource();
    float calculateDistToSeedSource(Cell *cell);
    size_t mD2S_target; // index of target
    size_t mD2S_seed_source; // index of source

    // simple management
    void setupSimpleManagement();
    void calculateSimpleManagement(Cell *cell, float &rActivity, float &rTime);
    SimpleManagementModule *mMgmtModule;

};

#endif // FETCHDATA_H
