#include "module.h"
#include "model.h"

// include all module headers for the factory function
#include "matrixmodule.h"
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
    int idx = indexOf(allModuleTypes(), module_name);
    if (idx == -1)
        idx = indexOf(allModuleTypes(), module_type);

    if (idx==-1)
        throw std::logic_error(fmt::format("The module with name '{}' and type '{}' cannot be created. Either use as name (modules.<name>) or specify the type (modules.xxx.type) as one of: {}", module_name, module_type, join(allModuleTypes(), ",")));

    if (indexOf(moduleNames(), module_name) != -1)
        throw std::logic_error(fmt::format("The module with name '{}' cannot be created because it already exists.", module_name));
    Module *m = nullptr;
    switch (idx) {
    case 0: m = new MatrixModule(module_name); break;
    case 1: m = new FireModule(); break;
    default: throw std::logic_error("Module " + module_name + " is not a valid Module (Module::moduleFactory)!");
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
