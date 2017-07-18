#ifndef TOYMODEL_H
#define TOYMODEL_H

#include <vector>
#include <list>
#include <memory>

#include <QObject>

struct InferenceItem {
    int index;
    int next_value;
    std::vector<double> data;
};

class ToyInference: public QObject
{
    Q_OBJECT
public:

private:
public slots:
    void doWork(std::list<InferenceItem*>*package);
signals:
    void workDone(std::list<InferenceItem*>*);
};

class ToyModel : public QObject
{
    Q_OBJECT
public:
    ToyModel();
    void run();
    void abort() { mAbort = true; }
    bool isCancel() const { return mAbort; }

private:
    bool mAbort;
    int mLivePackages;
    void buildDataPackage(int point_index);
    void addDataPackage(InferenceItem *item);
    void startInference();

    void finalizeState();

    std::vector<int> data;
    std::vector<int> to_process;
    std::list< std::list<InferenceItem*>* > for_inference;
public slots:
    void processedPackage(std::list<InferenceItem*>*package);
signals:
    void newPackage(std::list<InferenceItem*>*);
    void finished();

};



#endif // TOYMODEL_H
