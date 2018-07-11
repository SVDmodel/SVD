#include "module.h"
#include "model.h"

// include all module headers for the factory function
#include "grassmodule.h"
#include "fire/firemodule.h"

/// list of all available modules
/// this has to be edited in case new modules are added
std::vector<std::string> Module::mModuleNames = {
    "grass", "fire"
};


Module::~Module()
{

}

std::shared_ptr<Module> Module::moduleFactory(std::string module_name)
{
    int idx = indexOf(allModuleNames(), module_name);

    Module *m = nullptr;
    switch (idx) {
    case 0: m = new GrassModule(); break;
    case 1: m = new FireModule(); break;
    default: throw std::logic_error("Module " + module_name + " is not a valid Module (Module::moduleFactory)!");
    }
    return std::shared_ptr<Module>(m);
}

bool Module::registerModule()
{
    if (!Model::instance()->states()->registerHandler(this, type())) {
        RunState::instance()->setError("Error while registering modules.", RunState::instance()->modelState());
        return false;
    }
    return true;
}
