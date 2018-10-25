#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <memory>

#include "modelcontroller.h"
#include "landscapevisualization.h"

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

protected:
     void closeEvent(QCloseEvent *event);

private slots:
    void modelStateChanged(QString s);
    void modelUpdate();
    void finishedYear();
    void checkVisualization();

    void on_actionTest_DNN_triggered();

    void on_pbTest_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();


    void on_pushButton_5_clicked();


    void on_pbTestTF_clicked();

    void on_pbTestExpre_clicked();

    void on_actioncreate_output_docs_triggered();

    void on_pbTestVis_clicked();

    void on_pbRenderExpression_clicked();

    void on_visState_clicked() { checkVisualization(); }
    void on_visExpression_clicked() {checkVisualization(); }
    void on_visNone_clicked() {checkVisualization(); }

    void on_actionRender_to_file_triggered();

    void on_actionSetupProject_triggered();

    void on_actionRunSim_triggered();

    void on_actionStopSim_triggered();

    void on_actiondelete_model_triggered();


    void on_openProject_clicked();


    void menuRecent_files();

    void on_actionOpenProject_triggered();

private:
    QList<QString> mRecentFileList;

    void readSettings(); ///< read UI settings from ini file
    void writeSettings(); ///< save UI settings
    void recentFileMenu(); ///< update the list of recently used project files
    void checkAvailableActions(); ///< check status of the actions (run, cancel, ...)
    void updateModelStats(); ///< refresh model stats

    Ui::MainWindow *ui;
    std::unique_ptr<ModelController> mMC;
    LandscapeVisualization *mLandscapeVis;
    QTimer mUpdateModelTimer;
};

#endif // MAINWINDOW_H
