#ifndef MODEL_H
#define MODEL_H
// system
#include <memory>
#include "spdlog/spdlog.h"

#include "settings.h"
#include "states.h"

class States; // forward

class Model
{
public:
    Model();
    ~Model();
    /// sets up the model components (such as states, climate, landscape) from the config file 'fileName'
    /// returns true on success
    bool setup(const std::string &fileName);
    // access

    /// return the available states
    std::shared_ptr<States> states() const { return mStates; }

    /// access to the model configuration
    const Settings &settings() const { return mSettings; }
private:
    // actions
    void inititeLogging();
    void shutdownLogging();

    // helpers
    Settings mSettings;

    // model components
    std::shared_ptr<States> mStates;
    // loggers
    std::shared_ptr<spdlog::logger> lg_main;
    std::shared_ptr<spdlog::logger> lg_setup;
};

#endif // MODEL_H
