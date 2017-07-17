#ifndef TESTDNN_H
#define TESTDNN_H

#include <QWidget>

namespace Ui {
class TestDNN;
}

class TestDNN : public QWidget
{
    Q_OBJECT

public:
    explicit TestDNN(QWidget *parent = 0);
    ~TestDNN();
private slots:
    void on_runInception_clicked();

    void on_setupModel_clicked();

    void on_selectFile_clicked();

    void on_doPredict_clicked();

private:
    Ui::TestDNN *ui;
};

#endif // TESTDNN_H
