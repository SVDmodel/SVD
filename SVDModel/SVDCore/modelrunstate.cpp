#include "modelrunstate.h"

#include <mutex>
#include <sstream>

#include "spdlog/spdlog.h"

RunState *RunState::mInstance = nullptr;

std::string ModelRunState::stateString(const ModelRunState &s) const
{
    // Invalid=0, Creating, ReadyToRun, Stopping, Running, Paused, Finsihed, Canceled, ErrorDuringSetup, Error };
    switch (s.state()) {
    case Invalid: return "Invalid";
    case Creating: return "Creating...";
    case ReadyToRun: return "Ready";
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

void ModelRunState::update()
{
    // notify parent state....
    RunState::instance()->update(this);
}

RunState::RunState()
{
    mInstance = this;
    mInUpdate = false;
    clear();
}

void RunState::clear()
{
    mCancel=false;
    mTotal = ModelRunState::Invalid;
    mModel = ModelRunState::Invalid;
    mDNN   = ModelRunState::Invalid;
}

void RunState::set(ModelRunState state)
{
    //
}

std::mutex runstate_mutex;
void RunState::update(ModelRunState *source)
{
    if (mInUpdate)
        return;

    std::lock_guard<std::mutex> guard(runstate_mutex);
    mInUpdate=true;
    if (source!=&mTotal) {
        // update the "total" state based on the changed sub states
        mTotal.set(mModel, mDNN);

        if (spdlog::get("main"))
            spdlog::get("main")->trace("Model Status: Total: {}, Model: {}, DNN: {}.", mTotal.stateString(), mModel.stateString(), mDNN.stateString());
    }
    mInUpdate=false;
}

std::string RunState::asString() const
{
    std::stringstream s;
    s << mTotal.stateString()
      << " [Model: " << mModel.stateString()
      << ", DNN: " << mDNN.stateString() << "]";
    return s.str();
}

void RunState::setError(std::string error_message, ModelRunState &state)
{
    mErrorMessage = error_message;
    state=ModelRunState::Error;
    setCancel(true);
    if (spdlog::get("main"))
        spdlog::get("main")->error("An error occured in {}: {}", (&state == &mDNN ? "DNN":"Model"), error_message);
}
