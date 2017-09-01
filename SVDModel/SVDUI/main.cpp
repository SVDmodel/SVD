#include "mainwindow.h"
#include <QApplication>

#include "spdlog/spdlog.h"
#include "signal.h"
// crash and exception handling....
void shutdownLogging() {
    // release and drop all handlers (and flush)
    spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l)
            {
                l->flush();
            });
    spdlog::drop_all();

}
void terminateHandler() {
    auto l = spdlog::get("main");
    if (l)
        l->critical("Program terminated!");
    std::abort();
}

void signalHandler(int sig) {
    auto l = spdlog::get("main");
    if (l)
        l->critical("Signal {} catched.", sig);
    shutdownLogging();
}
void exitHandler() {
    //auto l = spdlog::get("main");
    //l->info("Program exiting.");
    //shutdownLogging();
}

int main(int argc, char *argv[])
{
    // set crash handling
    std::set_terminate( terminateHandler );
    signal(SIGSEGV, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGTERM, signalHandler);
    atexit (exitHandler);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    try {
    int retval =  a.exec();
    return retval;
    } catch(...) {
        auto l = spdlog::get("main");
        if (l)
            l->critical("Unhandled exception in main!");
        shutdownLogging();
    }
}
