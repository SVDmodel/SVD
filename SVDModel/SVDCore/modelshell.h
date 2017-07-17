#ifndef MODELSHELL_H
#define MODELSHELL_H

#include <QObject>

class ModelShell : public QObject
{
    Q_OBJECT
public:
    explicit ModelShell(QObject *parent = nullptr);
    ~ModelShell();
signals:
    void log(const QString &s);
};


#endif // MODELSHELL_H
