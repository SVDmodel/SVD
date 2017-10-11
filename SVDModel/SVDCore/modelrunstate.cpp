#include "modelrunstate.h"

std::string ModelRunState::stateString(ModelRunState &s)
{
    // Invalid=0, Creating, ReadyToRun, Stopping, Running, Paused, Finsihed, Canceled, ErrorDuringSetup, Error };
    switch (s.state()) {
    case Invalid: return "Invalid";
    case Creating: return "Creating...";
    case ReadyToRun: return "Ready.";
    case Stopping: return "Stopping...";
    case Running: return "Running...";
    case Finished: return "Finished!";
    case Canceled: return "Canceled!";
    case ErrorDuringSetup: return "Error during setup!!";
    case Error: return "Error!!";
    default: return "invalid state";
    }

}

ModelRunState::State ModelRunState::combine(const ModelRunState &model, const ModelRunState &dnn)
{
    State ns = Invalid;
    // if one is error during startup -> error during startup
    if (model==ErrorDuringSetup || dnn==ErrorDuringSetup) ns = Error;
    // if one is error -> error
    if (model==Error || dnn==Error) ns = Error;

    if (ns != Invalid)
        return ns;



    // if one is canceled...
    if (model==Canceled || dnn==Canceled) return Canceled;
    if (model==ReadyToRun && dnn==ReadyToRun) return ReadyToRun;
    if (model==Finished && dnn==ReadyToRun) return Finished;

    if (model==Running || dnn==Running) return Running;

    // if one is still creating
    if (model.in({Creating, ReadyToRun}) || dnn.in({Creating, ReadyToRun})) return Creating;

    return model.state();
}
