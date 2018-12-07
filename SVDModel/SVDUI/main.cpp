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
