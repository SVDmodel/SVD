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
#ifndef DNNSHELL_H
#define DNNSHELL_H

#include <QObject>
#include <QFutureWatcher>
#include <QThreadPool>
#include "spdlog/spdlog.h"

#include "modelrunstate.h"

class InferenceData; // forward
class Batch; // forward
class DNN; // forward
class BatchManager; // forward

class DNNShell: public QObject
{
    Q_OBJECT
public:
    DNNShell();
    ~DNNShell();

    bool isRunnig();
    size_t batchesProcessed() const { return mBatchesProcessed; }
    size_t cellsProcessed() const { return mCellsProcessed; }

private:
public slots:
    void setup(QString fileName);
    void doWork(Batch *batch);

    void dnnFinished(void *vbatch);
signals:
    void workDone(Batch *batch);

private:
    /// thread pool for running the DNN
    QThreadPool *mThreads;

    // loggers
    std::shared_ptr<spdlog::logger> lg;

    std::unique_ptr<BatchManager> mBatchManager;
    std::unique_ptr<DNN> mDNN;
    std::atomic<int> mProcessing;

    // store a watcher and a flag if the watcher is used (=true) or free (false)
    //std::vector<std::pair<QFutureWatcher<Batch*>*, Batch*> > mWatchers;
    size_t mBatchesProcessed;
    size_t mCellsProcessed;

};

#endif // DNNSHELL_H
