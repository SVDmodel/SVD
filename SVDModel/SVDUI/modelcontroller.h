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
#ifndef MODELCONTROLLER_H
#define MODELCONTROLLER_H

#include <QObject>
#include <QThread>
#include "modelshell.h"
#include "modelrunstate.h"


class DNNShell; // forward


class ModelController : public QObject
{
    Q_OBJECT
    QThread *modelThread;
    QThread *dnnThread;
public:
    explicit ModelController(QObject *parent = nullptr);
    ~ModelController();
    ModelShell *shell() const { return mModelShell; }
    const Model *model() const { return mModelShell->model(); }
    DNNShell *dnnShell() const { return mDNNShell; }
    RunState *state() { return mState.get(); }
    int yearsToRun() { return mYearsToRun; }

    void setInteractiveMode(bool interactive) { mInteractiveMode = interactive; }
    void setDelay(int msecs) {mDelayMSecs = msecs; }

    /// retrieve system statistics as key-value pairs
    std::unordered_map<std::string, std::string> systemStatus();

signals:
    void log(const QString &s);
    void stateChanged(QString s);

    void finishedYear(int n);
    void finished();

public slots:
    void setup(QString fileName, Settings *settings=nullptr);
    void shutdown();

    void run(int n_years);
    void runStep();
    //void abort();
    void finishedStep(int n);
private:
    ModelShell *mModelShell; // lives in main thread
    DNNShell *mDNNShell; // lives in DNN thread
    std::unique_ptr<RunState> mState; // the state of the system (lives in main thread)

    int mYearsToRun;
    int mCurrentStep;
    int mDelayMSecs; // pause each year for a time
    QTime mStopWatch;
    bool mIsCurrentlyRunning;
    bool mInteractiveMode;

};

#endif // MODELCONTROLLER_H
