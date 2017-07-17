#ifndef MODELCONTROLLER_H
#define MODELCONTROLLER_H

#include <QObject>
#include <QThread>

class ModelController : public QObject
{
    Q_OBJECT
    QThread modelThread;
public:
    explicit ModelController(QObject *parent = nullptr);
    ~ModelController();

signals:
    void log(const QString &s);

public slots:

};

#endif // MODELCONTROLLER_H
