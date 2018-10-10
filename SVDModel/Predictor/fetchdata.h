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
    std::vector<Expression*> mExpressions;

};

/** FetchDataFunction is the gateway to more complex data items
 *
 * */
class FetchDataFunction : public FetchData
{
public:
    ~FetchDataFunction() {  }
    FetchDataFunction(InputTensorItem *item) : FetchData(item) { mFn = Invalid; }
    virtual void setup(const Settings *settings, const std::string &key, const InputTensorItem &item);
    virtual void fetch(Cell *cell, BatchDNN *batch, size_t slot);

    // functions
    enum EFunctions { Invalid=0,
                      DistToSeedSource = 1};
private:
    EFunctions mFn;

};

#endif // FETCHDATA_H
