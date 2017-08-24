#ifndef MODEL_H
#define MODEL_H
// system
#include <memory>
#include "spdlog/spdlog.h"

#include "settings.h"
#include "states.h"
#include "climate.h"


class Model
{
public:
    Model();
    ~Model();
    /// sets up the model components (such as states, climate, landscape) from the config file 'fileName'
    /// returns true on success
    bool setup(const std::string &fileName);
    // access
    /// access to the currently avaialable global model
    /// this allows accessing the model with Model::instance()->....
    static Model *instance() {
        assert(mInstance!=nullptr);
        return mInstance; }

    /// return the available states
    std::shared_ptr<States> states() const { return mStates; }
    const std::vector<std::string> &species() { return mSpeciesList; }

    /// access to the model configuration
    const Settings &settings() const { return mSettings; }
private:
    // actions
    void inititeLogging();
    void shutdownLogging();

    // setup functions
    void setupSpecies();

    // helpers
    Settings mSettings;

    // model components
    std::vector<std::string> mSpeciesList;
    std::shared_ptr<States> mStates;
    std::shared_ptr<Climate> mClimate;
    // loggers
    std::shared_ptr<spdlog::logger> lg_main;
    std::shared_ptr<spdlog::logger> lg_setup;
    // model instance
    static Model *mInstance;
};

#endif // MODEL_H
