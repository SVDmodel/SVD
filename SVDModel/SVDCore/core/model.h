#ifndef MODEL_H
#define MODEL_H
// system
#include <memory>
#include "spdlog/spdlog.h"

#include "settings.h"
#include "states.h"
#include "climate.h"
#include "landscape.h"


class Model
{
public:
    Model(const std::string &fileName);
    ~Model();
    /// sets up the model components (such as states, climate, landscape) from the config file 'fileName'
    /// returns true on success
    bool setup();

    void newYear();
    void finalizeYear();

    // access
    /// access to the currently avaialable global model
    /// this allows accessing the model with Model::instance()->....
    static Model *instance() {
        assert(mInstance!=nullptr);
        return mInstance; }

    /// the current time step of the simulation
    /// year=0 after setup, and incremented whenever a new step starts, i.e. first sim. year=1, 2nd year=2, ...
    int year() const { return mYear; }

    /// return the available states
    std::shared_ptr<States> states() const { return mStates; }
    const std::vector<std::string> &species() { return mSpeciesList; }
    std::shared_ptr<Landscape> &landscape() { return mLandscape; }
    std::shared_ptr<Climate> &climate() { return mClimate; }


    /// access to the model configuration
    const Settings &settings() const { return mSettings; }
    struct SystemStats {
        SystemStats(): NPackagesDNN(0), NPackagesSent(0), NPackagesTotalDNN(0), NPackagesTotalSent(0) {}
        size_t NPackagesSent;
        size_t NPackagesDNN;
        size_t NPackagesTotalSent;
        size_t NPackagesTotalDNN;
    } stats;
private:
    // actions
    void inititeLogging();
    void shutdownLogging();

    // setup functions
    void setupSpecies();

    // helpers
    Settings mSettings;

    // model state
    int mYear;
    // model components
    std::vector<std::string> mSpeciesList;
    std::shared_ptr<States> mStates;
    std::shared_ptr<Climate> mClimate;
    std::shared_ptr<Landscape> mLandscape;
    // loggers
    std::shared_ptr<spdlog::logger> lg_main;
    std::shared_ptr<spdlog::logger> lg_setup;
    // model instance
    static Model *mInstance;
};

#endif // MODEL_H
