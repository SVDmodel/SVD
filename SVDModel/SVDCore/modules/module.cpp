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
#include "module.h"
#include "model.h"

// include all module headers for the factory function
#include "matrix/matrixmodule.h"
#include "fire/firemodule.h"

/// list of all available special modules
/// this has to be edited in case new modules are added
std::vector<std::string> Module::mModuleTypes = {
    "matrix", "fire"
};

std::vector<std::string> Module::mModuleNames;

Module::~Module()
{

}

std::shared_ptr<Module> Module::moduleFactory(std::string module_name, std::string module_type)
{

    int idx = indexOf(allModuleTypes(), module_type);

    if (idx==-1)
        throw std::logic_error(fmt::format("The module with name '{}' and type '{}' cannot be created. Specify the type (modules.xxx.type) as one of: {}", module_name, module_type, join(allModuleTypes(), ",")));

    if (indexOf(moduleNames(), module_name) != -1)
        throw std::logic_error(fmt::format("The module with name '{}' cannot be created because the name is already used.", module_name));
    Module *m = nullptr;
    switch (idx) {
    case 0: m = new MatrixModule(module_name); break;
    case 1: m = new FireModule(module_name); break;
    default: throw std::logic_error("Error: " + module_type + " is not a valid module type (Module::moduleFactory)!");
    }
    mModuleNames.push_back(module_name);

    return std::shared_ptr<Module>(m);
}

bool Module::registerModule()
{
    if (!Model::instance()->states()->registerHandler(this, name())) {
        RunState::instance()->setError("Error while registering modules.", RunState::instance()->modelState());
        return false;
    }
    return true;
}

std::vector<std::pair<std::string, std::string> > Module::moduleVariableNames() const
{
    return {}; // an empty variable list
}

double Module::moduleVariable(const Cell *, size_t ) const
{
    return 0.;
}
