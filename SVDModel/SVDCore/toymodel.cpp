#include "toymodel.h"
#include <random>
#include <iostream>

#include <QtConcurrent>
#include <QMutex>
#include <QMutexLocker>

#include "spdlog/spdlog.h"

ToyModel::ToyModel()
{
    data = std::vector<int>(1000,0);
    mAbort = false;
    mLivePackages = 0;

    connect(&packageWatcher, SIGNAL(finished()), this, SLOT(allPackagesBuilt()));
    packageWatcher.setFuture(packageFuture);

    // logging
    lg = spdlog::get("main");
}

void ToyModel::run()
{
    // the main steps:
    // 1) find issues to process
    lg->info("Model run started");
    lg->info("*****************");
    to_process.clear();

    for (int i=0;i<data.size();++i)
        if (data[i] == 0)
            to_process.push_back(i);

    lg->debug("{} items in list", to_process.size());

    // 2) now prepare the data
    packageFuture = QtConcurrent::map(to_process, [this](int idx){ this->buildDataPackage(idx); });
    packageWatcher.setFuture(packageFuture);
    lg->debug(".... prepara data packages running ....");

}

void ToyModel::buildDataPackage(int point_index)
{
    if (isCancel())
        return;

    // wait for some time
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> rndtime(1,5);

    QThread::msleep(rndtime(gen));
    InferenceItem *item = new InferenceItem();
    item->data = std::vector<double> (100,0.);
    item->index = point_index;

    // add to processing chain
    addDataPackage(item);
}

QMutex toy_lock_inference_list;
void ToyModel::addDataPackage(InferenceItem *item)
{
    const int batch_size = 128;
    QMutexLocker locker(&toy_lock_inference_list);
    if (for_inference.size()==0) {
        for_inference.push_back( new std::list<InferenceItem*>() );
        mLivePackages++;
    }

    std::list<InferenceItem*> *list = for_inference.back();
    if (list->size()>=batch_size) {
        startInference();

        for_inference.push_back( new std::list<InferenceItem*>() );
        mLivePackages++;
        list = for_inference.back();
        // std::cout << "created new data list, #=" << for_inference.size() << std::endl;
    }
    list->push_back(item);

}

void ToyModel::startInference()
{
    if (for_inference.size()==0)
        return;

    // take the first element of the list:
    std::list<InferenceItem*> *inf_list = for_inference.front();
    for_inference.pop_front();

    // Call inference machine: route to DNN thread
    while (mLivePackages>3 && !isCancel()) {
        // wait some time...
        lg->debug("... wait until <=3 .... #packages= {}", mLivePackages);
        QThread::msleep(1000);
        QCoreApplication::processEvents(); // allow receiving of packages...

    }
    lg->debug("sending package to Inference... (now {})", mLivePackages);
    emit newPackage(inf_list);

}

void ToyModel::finalizeState()
{
    // just a toy.... advance to next year
    int zeros=0;
    for (int &item : data) {
        item = std::max(item-1, 0);
        if (item==0) zeros++;
    }
    lg->debug("Zeros: {}", zeros);
    emit finished();
}

QMutex toy_lock_processed_package;
void ToyModel::processedPackage(std::list<InferenceItem *> *package)
{
    // DNN delivered processed package....
    lg->debug("Model: DNN package received!");

    // write back to data.....
    for (InferenceItem * &ii : *package) {
        data[ii->index] = ii->next_value;
    }
    lg->debug( "Model: Wrote back!" );

    // now the data can be freed:
    {
    QMutexLocker locker(&toy_lock_processed_package);
    mLivePackages--;
    delete package;
    }

    if (mLivePackages==0) {
        finalizeState();
        lg->info( "Model: processsed Last Package!" );
        lg->info("********************************");
    } else {
        lg->debug( "Model: #packages: {}", mLivePackages);
    }



}

void ToyModel::allPackagesBuilt()
{
    lg->debug( "all data preparations are finished... building last batch");
    startInference(); // start last batch (even if < than batch size)

}

void ToyInference::doWork(std::list<InferenceItem *> *package)
{
    // package received!
    spdlog::get("dnn")->debug( "Inference: package received!" );

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> rndtime(0,10);
    std::uniform_int_distribution<> rndtimelong(100,1000);

    // update
    for (InferenceItem *&ii : *package) {
        ii->next_value = rndtime(gen);
    }

    // now wait
    QThread::msleep(rndtimelong(gen));

    // and send back
    emit workDone(package);

}
