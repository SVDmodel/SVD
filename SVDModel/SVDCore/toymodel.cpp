#include "toymodel.h"
#include <random>
#include <iostream>

#include <QtConcurrent>
#include <QMutex>
#include <QMutexLocker>


ToyModel::ToyModel()
{
    data = std::vector<int>(10000,0);
    mAbort = false;
    mLivePackages = 0;

}

void ToyModel::run()
{
    // the main steps:
    // 1) find issues to process
    to_process.clear();

    for (int i=0;i<data.size();++i)
        if (data[i] == 0)
            to_process.push_back(i);

    std::cout << to_process.size() << "items in list" << std::endl;

    // 2) now prepare the data
    QFutureWatcher<void> watcher;
    QFuture<void> result = QtConcurrent::map(to_process, [this](int idx){ this->buildDataPackage(idx); });

    watcher.setPendingResultsLimit(5);
    watcher.setFuture(result);

    result.waitForFinished();
    startInference(); // start last batch (even if < than batch size)

    std::cout << "finished prepare..." << std::endl;

    // wait for the results coming in ....
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

QMutex lock_inference_list;
void ToyModel::addDataPackage(InferenceItem *item)
{
    const int batch_size = 128;
    QMutexLocker locker(&lock_inference_list);
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
        std::cout << "created new data list, #=" << for_inference.size() << std::endl;
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
    std::cout << "Zeros:" << zeros << std::endl;
}

void ToyModel::processedPackage(std::list<InferenceItem *> *package)
{
    // DNN delivered processed package....
    std::cout << "Model: DNN package received!" << std::endl;

    // write back to data.....
    for (InferenceItem * &ii : *package) {
        data[ii->index] = ii->next_value;
    }
    std::cout << "Model: Wrote back!" << std::endl;

    // now the data can be freed:
    {
    QMutexLocker locker(&lock_inference_list);
    mLivePackages--;
    delete package;
    }

    if (mLivePackages==0) {
        finalizeState();
        std::cout << "Model: processsed Last Package!" << std::endl;
    } else {
        std::cout << "Model: #packages: " << mLivePackages << std::endl;
    }
    emit finished();


}

void ToyInference::doWork(std::list<InferenceItem *> *package)
{
    // package received!
    std::cout << "Inference: package received!" << std::endl;

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
