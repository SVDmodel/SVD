#ifndef MODELSHELL_H
#define MODELSHELL_H

#include <QObject>
#include "toymodel.h"

class ModelShell : public QObject
{
    Q_OBJECT
public:
    explicit ModelShell(QObject *parent = nullptr);
    ToyModel &toyModel() { return toy; }
    ~ModelShell();
signals:
    void log(const QString &s);
public slots:
    void run();
    void runTest();
    void process(int n);
    void abort();
private:
    bool mAbort;
    ToyModel toy;

};


#endif // MODELSHELL_H
