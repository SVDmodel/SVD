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

static std::mutex runstate_mutex;
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
        spdlog::get("main")->error("An error occurred in {}: {}", (&state == &mDNN ? "DNN":"Model"), error_message);
}
