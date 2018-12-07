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
#ifndef MODEL_H
#define MODEL_H
// system
#include <memory>
#include <functional>

#include "spdlog/spdlog.h"

#include "modelrunstate.h"
#include "settings.h"
#include "states.h"
#include "climate.h"
#include "landscape.h"
#include "externalseeds.h"
#include "outputs/outputmanager.h"

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

    void runModules();

    // callbacks
    void setProcessEventsCallback( std::function<void()> event) { mProcessEvents = event; }
    void processEvents() { if (mProcessEvents) mProcessEvents(); }
    const ModelRunState &state() const { return mState; }

    // access
    /// access to the currently avaialable global model
    /// this allows accessing the model with Model::instance()->....
    static Model *instance() {
        assert(mInstance!=nullptr);
        return mInstance; }
    /// check if a Model object is available
    static bool hasInstance() { return mInstance!=nullptr; }

    /// the current time step of the simulation
    /// year=0 after setup, and incremented whenever a new step starts, i.e. first sim. year=1, 2nd year=2, ...
    int year() const { return mYear; }

    /// return the model components
    std::shared_ptr<States> states() const { return mStates; }
    const std::vector<std::string> &species() { return mSpeciesList; }
    std::shared_ptr<Landscape> &landscape() { return mLandscape; }
    std::shared_ptr<Climate> &climate() { return mClimate; }
    const ExternalSeeds &externalSeeds() {return mExternalSeeds; }

    /// return ptr to a module with the given name, or nullptr if not available
    Module *module(const std::string &name);

    /// access to the output machinery
    std::shared_ptr<OutputManager> &outputManager() { return mOutputManager; }


    /// access to the model configuration
    const Settings &settings() const { return mSettings; }
    struct SystemStats {
        SystemStats(): NPackagesSent(0),  NPackagesDNN(0), NPackagesTotalSent(0), NPackagesTotalDNN(0)  {}
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
    void setupModules();

    void setupExpressionWrapper();

    // helpers
    Settings mSettings;

    // callbacks
    std::function<void()> mProcessEvents;
    ModelRunState mState;

    // model state
    int mYear;
    // model components
    std::vector<std::string> mSpeciesList;
    std::shared_ptr<States> mStates;
    std::shared_ptr<Climate> mClimate;
    std::shared_ptr<Landscape> mLandscape;
    ExternalSeeds mExternalSeeds;
    std::shared_ptr<OutputManager> mOutputManager;
    // modules
    std::vector< std::shared_ptr<Module> > mModules;
    // loggers
    std::shared_ptr<spdlog::logger> lg_main;
    std::shared_ptr<spdlog::logger> lg_setup;
    // model instance
    static Model *mInstance;
};

#endif // MODEL_H
