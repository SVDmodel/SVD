#ifndef CONSOLESHELL_H
#define CONSOLESHELL_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include "../SVDUI/modelcontroller.h"

class ConsoleShell : public QObject
{
    Q_OBJECT
public:
    explicit ConsoleShell(QObject *parent = nullptr);

signals:

public slots:
    void run(); // execute the SVD model
    void runModel();
    void modelUpdate();
    void finishedYear(int year);
    void finished();
private:
    ModelController *mController;
    QTimer mUpdateModelTimer;
    QStringList mParams;
    int mYears;
    bool mCreated;
    bool mRunning;
    QTime mStopwatch;
};

#endif // CONSOLESHELL_H
