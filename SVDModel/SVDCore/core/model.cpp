#include "model.h"
#include "tools.h"
#include "strtools.h"


Model::Model()
{

}

Model::~Model()
{
    shutdownLogging();
}

bool Model::setup(const std::string &fileName)
{
    if (!Tools::fileExists(fileName))
        throw std::logic_error("Error: The configuration file '" + fileName + "' does not exist.");
    mSettings.loadFromFile(fileName);
    auto split_path = splitPath(fileName);
    Tools::setProjectDir( split_path.first );
    // set up logging
    inititeLogging();

    return true;

}

void Model::inititeLogging()
{
    std::string log_file = Tools::path(settings().valueString("logging.file"));


    // asynchronous logging, 2 seconds auto-flush
    spdlog::set_async_mode(8192, spdlog::async_overflow_policy::block_retry,
                           nullptr,
                           std::chrono::seconds(2));

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>(log_file));

    //sinks.push_back(std::make_shared<my_threaded_sink>(ui->lLog));

    auto combined_logger = spdlog::create("main", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    combined_logger->flush_on(spdlog::level::err);

    combined_logger=spdlog::create("setup", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    combined_logger->flush_on(spdlog::level::err);


    combined_logger=spdlog::create("dnn", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    combined_logger->flush_on(spdlog::level::err);

    //auto combined_logger = std::make_shared<spdlog::logger>("console", begin(sinks), end(sinks));

    //register it if you need to access it globally
    //spdlog::register_logger(combined_logger);
    lg_main = spdlog::get("main");
    lg_setup = spdlog::get("setup");

    lg_main->info("Started logging");

}

void Model::shutdownLogging()
{
    if (lg_main)
        lg_main->info("Shutdown logging");

    spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l)
    {
        l->flush();
    });
    spdlog::drop_all();


}
