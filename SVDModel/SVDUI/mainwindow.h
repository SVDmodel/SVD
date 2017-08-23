#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

#include "modelcontroller.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void initiateLogging();
    void initiateModelController();
    ~MainWindow();
private slots:

    void on_actionTest_DNN_triggered();

    void on_pbStart_clicked();

    void on_pbStop_clicked();

    void on_run_clicked();

    void on_pushButton_clicked();

    void on_pbTest_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pbLoad_clicked();

    void on_pbDeleteModel_clicked();

    void on_pbRunModel_clicked();

private:
    Ui::MainWindow *ui;
    std::unique_ptr<ModelController> mMC;
};

#endif // MAINWINDOW_H
