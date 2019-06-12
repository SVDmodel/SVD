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
#ifndef MODELSHELL_H
#define MODELSHELL_H

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

#include "modelrunstate.h"
#include "spdlog/spdlog.h"
#include "settings.h"


class Model; // forward
class Cell; // forward
class InferenceData; // forward
class Batch; // forward
class Module; // forward


class ModelShell: public QObject
{
    Q_OBJECT
    Q_ENUMS(ModelRunState)
public:
    explicit ModelShell(QObject *parent = nullptr);
    ~ModelShell();
    void destroyModel();

    Model *model() { return mModel; }

    // test function
    std::string run_test_op(std::string what);
    int packagesBuilt() const { return mPackagesBuilt; }
    int packagesProcessed() const { return mPackagesProcessed; }
    size_t cellsProcessed() const { return mCellsProcesssed; }

signals:
    void stateChanged(QString s);
    void processedStep(int n);
    void log(QString s);


    void newPackage(Batch *batch);


public slots:
    void createModel(QString fileName, Settings *settings=nullptr);
    void setup();
    void runOneStep(int current_step);
    void run(int n_steps);
    void abort();
    //ModelRunState state() { return *mState; }

    void processedPackage(Batch *batch);
    void allPackagesBuilt();

private:
    void internalRun();
    void evaluateCell(Cell *cell);
    void buildInferenceDataDNN(Cell *cell);
    std::pair<Batch *, size_t> getSlot(Cell *cell, Module *module);
    bool checkBatch(Batch *batch);
    void sendBatch(Batch *batch);
    void sendPendingBatches();
    void finalizeCycle();
    void cancel();

    void processEvents();


    void setState(ModelRunState::State new_state, QString msg=QString());
    bool mAbort;
    Model *mModel;

    int mPackagesBuilt;
    int mPackagesProcessed;
    bool mAllPackagesBuilt;
    int mPackageId;
    size_t mCellsProcesssed; // cells that are processed in the model (not via DNN)



    QFutureWatcher<void> packageWatcher;
    QFuture<void> packageFuture;

    // loggers
    std::shared_ptr<spdlog::logger> lg;



};

#endif // MODELSHELL_H
