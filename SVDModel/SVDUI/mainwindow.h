/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <memory>

#include "modelcontroller.h"
#include "landscapevisualization.h"
#include "colorpalette.h"

class QQuickWidget; // forward
class QTreeWidgetItem;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
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


    void on_pbRenderExpression_clicked();

    void on_visState_clicked() { checkVisualization(); }
    void on_visExpression_clicked() {checkVisualization(); }
    void on_visNone_clicked() {checkVisualization(); }
    void on_visVariable_clicked() {checkVisualization(); }

    void on_actionRender_to_file_triggered();

    void on_actionSetupProject_triggered();

    void on_actionRunSim_triggered();

    void on_actionStopSim_triggered();

    void on_actiondelete_model_triggered();


    void on_openProject_clicked();


    void menuRecent_files();

    void on_actionOpenProject_triggered();

    void on_pbReloadQml_clicked();

    void on_action3D_Camera_settings_triggered();

    void on_actionAbout_SVD_triggered();

    void on_actionOnline_resources_triggered();

    void on_lConfigFile_textChanged(const QString &arg1);

    void on_visVariables_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    QList<QString> mRecentFileList;

    void readSettings(); ///< read UI settings from ini file
    void writeSettings(); ///< save UI settings
    void recentFileMenu(); ///< update the list of recently used project files
    void checkAvailableActions(); ///< check status of the actions (run, cancel, ...)
    void updateModelStats(); ///< refresh model stats
    void onModelCreated(); ///< called after the model is created (and ready to run)

    Ui::MainWindow *ui;
    std::unique_ptr<ModelController> mMC;
    LandscapeVisualization *mLandscapeVis;
    QQuickWidget *mQmlView;
    Legend *mLegend;
    QTimer mUpdateModelTimer;
};

#endif // MAINWINDOW_H
