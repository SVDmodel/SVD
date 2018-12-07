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
    static std::vector<std::string> &allModuleTypes() {return mModuleTypes; }
    static std::vector<std::string> &moduleNames() {return mModuleNames; }
    static void clearModuleNames() { mModuleNames.clear(); }
    static std::shared_ptr<Module> moduleFactory(std::string module_name, std::string module_type);

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
    static std::vector<std::string> mModuleNames; ///< names of all created and active modules
    static std::vector<std::string> mModuleTypes; ///< available module types

};

#endif // MODULE_H
