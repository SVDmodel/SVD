#ifndef MODELCONTROLLER_H
#define MODELCONTROLLER_H

#include <QObject>
#include <QThread>

class ModelShell; // forward

class ModelController : public QObject
{
    Q_OBJECT
    QThread *modelThread;
    QThread *dnnThread;
public:
    explicit ModelController(QObject *parent = nullptr);
    ~ModelController();

signals:
    void log(const QString &s);

public slots:
    void run();
    void abort();
private:
    ModelShell *mModel;

};

#endif // MODELCONTROLLER_H
