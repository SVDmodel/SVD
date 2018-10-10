#ifndef MODULE_H
#define MODULE_H

#include <memory>

#include "states.h"
#include "../Predictor/batch.h"

class Cell; // forward
class Batch; // forward

class Module
{

public:
    Module(std::string module_name, State::StateType type) : mName(module_name), mType(type) {}
    virtual ~Module();
    static std::vector<std::string> &allModuleNames() {return mModuleNames; }
    static std::shared_ptr<Module> moduleFactory(std::string module_name);

    // properties
    const std::string &name() const { return mName; }
    State::StateType type() const { return mType; }
    Batch::BatchType batchType() const { return mBatchType; }

    /// register the module using the ID and name
    bool registerModule();

    // actions to overload
    virtual void prepareCell(Cell *) {}
    virtual void processBatch(Batch *) {}

    // startup
    virtual void setup() {}
    virtual void run() {}
protected:
    std::string mName;
    State::StateType mType; ///< states of this type are automatically handled by the module
    Batch::BatchType mBatchType; ///< type of the batch used by the module (e.g. DNN or Simple)
    static std::vector<std::string> mModuleNames;

};

#endif // MODULE_H
