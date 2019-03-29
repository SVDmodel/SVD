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
#include "model.h"
#include "tools.h"
#include "strtools.h"
#include "../Predictor/batchmanager.h"
#include "modules/module.h"
#include "expressionwrapper.h"

#include <QThreadPool>

Model *Model::mInstance = nullptr;

Model::Model(const std::string &fileName)
{
    if (mInstance!=nullptr)
        throw std::logic_error("Creation of model: model instance ptr is not 0.");
    mInstance = this;
    mYear = -1; // not set up

    if (!Tools::fileExists(fileName))
        throw std::logic_error("Error: The configuration file '" + fileName + "' does not exist.");
    mSettings.loadFromFile(fileName);
    auto split_path = splitPath(fileName);
    Tools::setProjectDir( split_path.first );

    // set up logging
    inititeLogging();

    lg_setup->info("Model Setup, config file: '{}', project root folder: '{}'", fileName, split_path.first);

}

Model::~Model()
{
    shutdownLogging();
    mInstance = nullptr;
}

bool Model::setup()
{
    // general setup
    bool mt = settings().valueBool("model.multithreading", "true");
    if (mt) {
        int n_threads = settings().valueInt("model.threads", -1);
        if (n_threads == -1)
            n_threads = QThread::idealThreadCount();
        QThreadPool::globalInstance()->setMaxThreadCount( n_threads );
        lg_setup->info("Enabled multithreading for the model (# threads={}).", n_threads);
    } else {
        QThreadPool::globalInstance()->setMaxThreadCount( 1 );
        lg_setup->info("Disabled multithreading for the model.");
    }

    // set up outputs
    mOutputManager = std::shared_ptr<OutputManager>(new OutputManager());
    mOutputManager->setup();

    // set up model components
    setupSpecies();
    mStates = std::shared_ptr<States>(new States());
    mStates->setup();

    mLandscape = std::shared_ptr<Landscape>(new Landscape());
    mLandscape->setup();

    mClimate = std::shared_ptr<Climate>(new Climate());
    mClimate->setup();

    mExternalSeeds.setup();

    setupModules();

    setupExpressionWrapper();

    lg_setup->info("************************************************************");
    lg_setup->info("************   Setup completed, Ready to run  **************");
    lg_setup->info("************************************************************");

    mYear = 0; // model is set up, ready to run
    return true;

}

void Model::finalizeYear()
{
    // increment residence time for all pixels (updated pixels go from 0 -> 1)
    for (Cell &c : landscape()->grid()) {
        if (!c.isNull()) {
            c.update();

        }
    }

    outputManager()->yearEnd();

    stats.NPackagesTotalSent += stats.NPackagesSent;
    stats.NPackagesTotalDNN += stats.NPackagesDNN;
}

void Model::runModules()
{
    auto lg = spdlog::get("main");
    lg->debug("Run modules (year {})", year());
    for (auto &module : mModules) {
        lg->debug("Run module '{}'", module->name());
        module->run();
    }
}

Module *Model::module(const std::string &name)
{
    for (const auto &m : mModules)
        if (m->name() == name)
            return m.get();
    return nullptr;
}

void Model::newYear()
{
    if (mYear == 0) {
        // this indicates the state before the first year of the simulation is executed

    }

    stats.NPackagesSent = stats.NPackagesDNN = 0;
    // increment the counter
    mYear = mYear + 1;
    // other initialization ....
    BatchManager::instance()->newYear();
}

void Model::inititeLogging()
{
    settings().requiredKeys("logging", {"file", "setup.level", "model.level", "dnn.level"});

    std::string log_file = Tools::path(settings().valueString("logging.file"));


    // asynchronous logging, 2 seconds auto-flush
    spdlog::set_async_mode(8192, spdlog::async_overflow_policy::block_retry,
                           nullptr,
                           std::chrono::seconds(2));

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>(log_file,true));

    //sinks.push_back(std::make_shared<my_threaded_sink>(ui->lLog));
    std::vector<std::string> levels = SPDLOG_LEVEL_NAMES; // trace, debug, info, ...

    auto combined_logger = spdlog::create("main", sinks.begin(), sinks.end());
    combined_logger->flush_on(spdlog::level::err);

    int idx = indexOf(levels, settings().valueString("logging.model.level"));
    if (idx==-1)
        throw std::logic_error("Setup logging: the value '" + settings().valueString("logging.model.level") + "' is not a valid logging level for logging.model.level. Valid are: " + join(levels) );
    combined_logger->set_level(spdlog::level::level_enum(idx) );

    combined_logger=spdlog::create("setup", sinks.begin(), sinks.end());
    combined_logger->flush_on(spdlog::level::err);
    idx = indexOf(levels, settings().valueString("logging.setup.level"));
    if (idx==-1)
        throw std::logic_error("Setup logging: the value '" + settings().valueString("logging.setup.level") + "' is not a valid logging level for logging.setup.level. Valid are: " + join(levels) );
    combined_logger->set_level(spdlog::level::level_enum(idx) );


    combined_logger=spdlog::create("dnn", sinks.begin(), sinks.end());
    combined_logger->flush_on(spdlog::level::err);
    idx = indexOf(levels, settings().valueString("logging.dnn.level"));
    if (idx==-1)
        throw std::logic_error("Setup logging: the value '" + settings().valueString("logging.dnn.level") + "' is not a valid logging level for logging.dnn.level. Valid are: " + join(levels) );
    combined_logger->set_level(spdlog::level::level_enum(idx) );

    combined_logger=spdlog::create("modules", sinks.begin(), sinks.end());
    combined_logger->flush_on(spdlog::level::err);
    idx = indexOf(levels, settings().valueString("logging.modules.level"));
    if (idx==-1)
        throw std::logic_error("Setup logging: the value '" + settings().valueString("logging.modules.level") + "' is not a valid logging level for logging.modules.level. Valid are: " + join(levels) );
    combined_logger->set_level(spdlog::level::level_enum(idx) );

    //auto combined_logger = std::make_shared<spdlog::logger>("console", begin(sinks), end(sinks));

    //register it if you need to access it globally
    //spdlog::register_logger(combined_logger);
    lg_main = spdlog::get("main");
    lg_setup = spdlog::get("setup");

    lg_main->info("Started logging. Log levels: main: {}, setup: {}, dnn: {}, modules: {}", levels[lg_main->level()], levels[lg_setup->level()], levels[spdlog::get("dnn")->level()], levels[spdlog::get("modules")->level()]);

}

void Model::shutdownLogging()
{
    if (lg_main)
        lg_main->info("Shutdown logging");
    else
        return; // no logging is set up, no need to destroy loggers

    spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l)
    {
        l->flush();
    });
    spdlog::drop_all();


}

void Model::setupSpecies()
{
    std::string species_list = settings().valueString("model.species");
    mSpeciesList = split(species_list, ',');
    for (std::string &s : mSpeciesList)
        s = trimmed(s);

    lg_setup->debug("Setup of species: N={}.", mSpeciesList.size());

    if (lg_setup->should_log(spdlog::level::trace)) {
        lg_setup->trace("************");
        lg_setup->trace("Species: {}", join(mSpeciesList));
        lg_setup->trace("************");
    }

}

void Model::setupModules()
{
    // reset module list
    Module::clearModuleNames();

    auto module_list = settings().findKeys("modules.", true);
    for (const auto &s : module_list ) {
        if (settings().valueBool("modules." + s + ".enabled", "false")) {
            auto module_type = settings().valueString("modules." + s + ".type", "unknown");
            lg_setup->info("Attempting to create enabled module '{}':", s);
            std::shared_ptr<Module> module = Module::moduleFactory(s, module_type);
            mModules.push_back(module);
            module->setup();

        }
    }

    // update the states to incorporate new modules
    Model::instance()->states()->updateStateHandlers();

    lg_setup->info("Setup of modules completed, {} active modules: {}", Module::moduleNames().size(),  join(Module::moduleNames(), ","));

}

void Model::setupExpressionWrapper()
{
    //EnvironmentCell &ec = mLandscape->environmentCell(0);
    EnvironmentCell ec(-1, -1); // need just something to extract variable names from (which are static)
    const State &s = mStates->stateByIndex(0);
    CellWrapper::setupVariables(&ec, &s);
    // set up module variables
    for (const auto &module : mModules) {
        CellWrapper::setupVariables(module.get());
    }
    CellWrapper cw(nullptr);
    lg_setup->debug("Setup of variables for expressions completed. List of variables: {}", join(cw.getVariablesList()) );
}
