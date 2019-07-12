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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QTimer>
#include <QTime>
#include <QMessageBox>
#include <QClipboard>
#include <QFileDialog>
#include <QQuickWidget>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDesktopServices>


#include "testdnn.h"

#include "modelcontroller.h"
#include "model.h"
#include "modelrunstate.h"

#include "integratetest.h"
#include "../Predictor/predtest.h"

#include "spdlog/spdlog.h"
#include "strtools.h"

#include "aboutdialog.h"

// visualization
#include "cameracontrol.h"
#include "colorpalette.h"
#include "expressionwrapper.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // start basic logging
    //initiateLogging();

    // start up the model shell
    mMC = std::unique_ptr<ModelController>(new ModelController());

    // initialize visualization
    mLandscapeVis = new LandscapeVisualization(this);

    // signals & slots
    initiateModelController();

    // some UI tweaks
    ui->mainToolBar->addWidget(ui->sYears);
    ui->mainToolBar->addWidget(ui->progressBarContainer);

    readSettings();

    QString lastconfigfile = QSettings().value("project/lastprojectfile").toString();
    if (!lastconfigfile.isEmpty() && QFile::exists(lastconfigfile))
        ui->lConfigFile->setText(lastconfigfile);

    checkAvailableActions();

    // setup the ruler (QML based)

    mLegend = new Legend(); // global object

    mQmlView = new QQuickWidget();

    mQmlView->engine()->rootContext()->setContextProperty("legend", mLegend);
    mQmlView->engine()->addImageProvider(QLatin1String("colors"), new ColorImageProvider);

    // from resource (proper)
    // for develop/debug from file system
    mQmlView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    //mQmlView->setSource(QUrl::fromLocalFile("E:/dev/SVD/SVDModel/SVDUI/res/qml/legend.qml"));
    mQmlView->setSource(QUrl("qrc:/qml/legend.qml"));
    mQmlView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->legendLayout->replaceWidget(ui->legendContainer, mQmlView);

    ui->tabWidget->setTabEnabled(1,false); // the other tab next to "main" visualization
    ui->pbReloadQml->setVisible(false); // hide the reload qml button


}


MainWindow::~MainWindow()
{

    auto l = spdlog::get("main");
    if (l)
        l->info("Shutdown of the application.");
    spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l)
    {
        l->flush();
    });
    //spdlog::drop_all();
    delete ui;
    delete mLegend;
    delete mLandscapeVis;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    QApplication::closeAllWindows();
    event->accept();
}

void MainWindow::modelStateChanged(QString s)
{
    if (mMC->state()->state() == ModelRunState::ReadyToRun && !mLandscapeVis->isValid()) {
        // setup of the visualization
        mLandscapeVis->setup(ui->main3d, mLegend);
        readVisualizationSettings(); // from ini file
        onModelCreated();
    }

    // stop the update timer...
    if (mMC->state()->isModelFinished() || mMC->state()->isModelPaused()) {
        mUpdateModelTimer.stop();
        modelUpdate();
    }

    checkAvailableActions();

}

void MainWindow::modelUpdate()
{
    int stime = ui->lModelState->property("starttime").toTime().elapsed();
    //QTime().addMSecs(stime).toString(Qt::ISODateWithMs)

    QString statusstring;
    if (RunState::instance()->isModelRunning()) {
        statusstring = QString("%1 - %3/%4 - %2").arg( QTime(0,0).addMSecs(stime).toString(Qt::ISODate) )
                .arg(QString::fromStdString(RunState::instance()->state().stateString()))
                .arg(mMC->model()->year()).arg(mMC->yearsToRun());
    } else {
        statusstring = QString("%1 - %2").arg( QTime(0,0).addMSecs(stime).toString(Qt::ISODate) )
                .arg(QString::fromStdString(RunState::instance()->asString()));
    }
    ui->lModelState->setText(statusstring);

    updateModelStats();
    if (mMC->state()->isModelFinished() || mMC->state()->isModelPaused()) {
        mUpdateModelTimer.stop();
    }
    if (mMC->state()->isModelRunning())
        mUpdateModelTimer.start(100);
}

void MainWindow::finishedYear(int)
{
    if (mLandscapeVis->isValid()) {

        if (mMC->model()->settings().hasKey("visualization.render") && mMC->model()->settings().valueBool("visualization.render")) {
            QString filename=QString("render_%1.png").arg(mMC->model()->year());
            mLandscapeVis->update(); // force update
            mLandscapeVis->renderToFile(filename);
        }
    }
}

void MainWindow::checkVisualization()
{
    // for any radiobuttion
    if (ui->visExpression->isChecked())
        mLandscapeVis->renderExpression(ui->lExpression->text());

    if (ui->visState->isChecked())
        mLandscapeVis->setRenderType(LandscapeVisualization::RenderState);

    if (ui->visNone->isChecked())
        mLandscapeVis->setRenderType(LandscapeVisualization::RenderNone);

    if (ui->visVariable->isChecked())
        on_visVariables_currentItemChanged(ui->visVariables->currentItem(), nullptr);
}

void MainWindow::pointClickedOnVisualization(QVector3D world_pos)
{
    //spdlog::get("main")->info("x/y: {}/{}", world_pos.x(), world_pos.y());
    QString label=QString("%1m/%2m").arg(static_cast<int>(world_pos.x())).arg(static_cast<int>(world_pos.y()));
    ui->visCoords->setText(label);
    ui->visCoordsInspector->setText(label);
    populateInspector(world_pos);

}


void MainWindow::on_actionTest_DNN_triggered()
{
    // open test DNN form and show model
    TestDNN *testdnn = new TestDNN();
    testdnn->show();

}


void MainWindow::on_pbTest_clicked()
{
    IntegrateTest it;
    it.testGrid();
    qDebug() << "test end";
}

void MainWindow::on_pushButton_2_clicked()
{
    IntegrateTest it;
    it.testSettings();
}


class my_threaded_sink : public spdlog::sinks::base_sink < std::mutex >
{
public:
    my_threaded_sink(QPlainTextEdit *output) { mOut = output; }
protected:
    void _sink_it(const spdlog::details::log_msg& msg) override
    {
        //Your code here
        char *buf = const_cast<char *>(msg.formatted.c_str());
        // search for \r
        for (char *p=buf; *p!=0; p++)
            if (*p == '\r') {
                *p = '\0';
                break;
            }

        // QMetaObject::invokeMethod(mOut, "appendPlainText", Qt::QueuedConnection, Q_ARG(QString, QString(buf)));
        //mOut->appendPlainText( QString(buf));
    }
    void _flush() override
    {
        // do nothing
        //mOut->appendPlainText(".... flushing .....");
    }

private:
    QPlainTextEdit *mOut;
};

void MainWindow::on_pushButton_3_clicked()
{




    IntegrateTest it;
    //auto console = spdlog::basic_logger_mt("console", "log.txt");

    it.testLogging();
}


void MainWindow::initiateLogging()
{
    if (spdlog::get("main"))
        return;
    // asynchronous logging, 2 seconds auto-flush
    spdlog::set_async_mode(8192, spdlog::async_overflow_policy::block_retry,
                           nullptr,
                           std::chrono::seconds(2));

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>("log.txt"));
    sinks.push_back(std::make_shared<my_threaded_sink>(ui->lLog));
    auto combined_logger = spdlog::create("main", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    combined_logger->flush_on(spdlog::level::err);

    combined_logger=spdlog::create("dnn", sinks.begin(), sinks.end());
    combined_logger->set_level(spdlog::level::debug);
    combined_logger->flush_on(spdlog::level::err);

    //auto combined_logger = std::make_shared<spdlog::logger>("console", begin(sinks), end(sinks));

    //register it if you need to access it globally
    //spdlog::register_logger(combined_logger);

    combined_logger->info("Started logging");
}

void MainWindow::initiateModelController()
{
    // bookkeeping, signal - slot connections
    connect(mMC.get(), &ModelController::stateChanged, [this](QString s) {ui->statusBar->showMessage(s);});
    connect(mMC.get(), &ModelController::stateChanged, this, &MainWindow::modelStateChanged);
    connect(mMC.get(), &ModelController::finishedYear, ui->progressBar, &QProgressBar::setValue);
    connect(mMC.get(), &ModelController::finishedYear, this, &MainWindow::finishedYear);
    connect(mMC.get(), &ModelController::finished, [this]() { ui->progressBar->setValue(ui->progressBar->maximum());});

    connect(mMC.get(), &ModelController::finishedYear, mLandscapeVis, &LandscapeVisualization::update);
    connect(mMC.get(), &ModelController::finished, mLandscapeVis, &LandscapeVisualization::update);
    connect(mLandscapeVis, &LandscapeVisualization::pointSelected, this, &MainWindow::pointClickedOnVisualization);

    connect(&mUpdateModelTimer, &QTimer::timeout, this, &MainWindow::modelUpdate);

    //connect(mMC.get(), &ModelController::stateChanged, ui->statusBar, &QStatusBar::showMessage);
    //QObject::connect(mMC.get(), SIGNAL(modelState(QString)), ui->statusBar, SLOT(showMessage(QString,int)));
}

void MainWindow::on_pushButton_4_clicked()
{
    IntegrateTest it;
    it.testRandom();
}

void MainWindow::on_pushButton_5_clicked()
{
    initiateLogging();
    PredTest it;
    it.testTensor();
}



void MainWindow::on_pbTestTF_clicked()
{
    initiateLogging();
    PredTest it;
    it.testDevicePlacement();
}

void MainWindow::on_pbTestExpre_clicked()
{
    initiateLogging();
    IntegrateTest it;
    it.testExpression();
}

void MainWindow::on_actioncreate_output_docs_triggered()
{
    std::string output_doc = Model::instance()->outputManager()->createDocumentation();

    QApplication::clipboard()->setText(QString::fromStdString(output_doc));

    spdlog::get("main")->info("Output documentation copied to the clipboard!");

}


void MainWindow::on_pbRenderExpression_clicked()
{
    if (mLandscapeVis->isValid())
        mLandscapeVis->renderExpression(ui->lExpression->text());
}


void MainWindow::on_actionRender_to_file_triggered()
{
    mLandscapeVis->renderToFile();
}

void MainWindow::on_actionSetupProject_triggered()
{
    // save currently selected config file
    recentFileMenu();

    // create & load model
    if (mMC && mMC->model()) {
        if (QMessageBox::question(this, "Confirm reload", "The model is already created. Create a new model?")==QMessageBox::No)
            return;
    }
    ui->lModelState->setProperty("starttime", QTime::currentTime());
    mMC.reset();
    mMC.reset(new ModelController()); // this frees the current model
    mLandscapeVis->invalidate();
    initiateModelController();

    mMC->setup(ui->lConfigFile->text());
    mUpdateModelTimer.start(100);

}

void MainWindow::on_actionRunSim_triggered()
{
    ui->progressBar->reset();
    ui->progressBar->setMaximum( ui->sYears->value() );
    mMC->run( ui->sYears->value() );
    ui->lModelState->setProperty("starttime", QTime::currentTime());
    mUpdateModelTimer.start(100);
}

void MainWindow::on_actionStopSim_triggered()
{
    if (mMC && mMC->model())
        mMC->shutdown();
}

void MainWindow::on_actiondelete_model_triggered()
{
    mMC->shutdown();
    mLandscapeVis->invalidate();
    checkAvailableActions();
}


void MainWindow::on_openProject_clicked()
{
    QString the_filter = "*.conf;;All files (*.*)";

    QString fileName = QFileDialog::getOpenFileName(this,
     "Select project config file", "", the_filter);


    if (fileName.isEmpty())
        return;

    ui->lConfigFile->setText(fileName);

}

void MainWindow::on_actionOpenProject_triggered()
{
    on_openProject_clicked();
}


void MainWindow::menuRecent_files()
{
    QAction* action = dynamic_cast<QAction*>(sender());
    if (action)
        ui->lConfigFile->setText(action->text());

}

void MainWindow::readSettings()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QCoreApplication::setOrganizationName("SVD");
    QCoreApplication::setOrganizationDomain("svd.boku.ac.at");
    QCoreApplication::setApplicationName("SVD");
    QSettings settings;
    qDebug() << "reading settings from" << settings.fileName();

    // window state and
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());

    // read javascript commands
//    int size = settings.beginReadArray("javascriptCommands");
//    for (int i=0;i<size; ++i) {
//        settings.setArrayIndex(i);
//        ui->scriptCommandHistory->addItem(settings.value("item").toString());
//    }
//    settings.endArray();
    //recent files menu qsettings registry load
    settings.beginGroup("recent_files");
    for(int i = 0;i < settings.childKeys().size();i++){
        mRecentFileList.append(settings.value(QString("file-%1").arg(i)).toString());
    }
    recentFileMenu();

    settings.endGroup();

}

void MainWindow::readVisualizationSettings()
{
    QSettings settings;
    QString view_name = QString("view-%1").arg(ui->lConfigFile->text()
                                               .replace(QChar('/'),"_")
                                               .replace(QChar('\\'), "_"));

    settings.beginGroup(view_name);

    for(int i = 0;i < settings.childKeys().size();i++){
        QString camsettings = settings.value(QString("camera-%1").arg(i)).toString();
        mLandscapeVis->setViewString(i, camsettings);
    }
    ui->actionCustom_view_1->setEnabled( mLandscapeVis->isViewValid(1) );
    ui->actionCustom_View_2->setEnabled( mLandscapeVis->isViewValid(2) );
    ui->actionCustom_View_3->setEnabled( mLandscapeVis->isViewValid(3) );


    settings.endGroup();
}

void MainWindow::writeVisualizationSettings()
{
    QSettings settings;
    if (mLandscapeVis->isValid()) {
        QString view_name = QString("view-%1").arg(ui->lConfigFile->text()
                                                   .replace(QChar('/'),"_")
                                                   .replace(QChar('\\'), "_"));
        settings.beginGroup(view_name);
        for (int i=0; i< mLandscapeVis->viewCount(); ++i)
            settings.setValue(QString("camera-%1").arg(i), mLandscapeVis->viewString(i));
        settings.endGroup();
    }

}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.endGroup();
    // javascript commands
    //    settings.beginWriteArray("javascriptCommands");
    //    int size = qMin(ui->scriptCommandHistory->count(), 15); // max 15 entries in the history
    //    for (int i=0;i<size; ++i) {
    //        settings.setArrayIndex(i);
    //        settings.setValue("item", ui->scriptCommandHistory->itemText(i));
    //    }
    //    settings.endArray();
    settings.beginGroup("project");
    settings.setValue("lastprojectfile", ui->lConfigFile->text());
    settings.endGroup();
    //recent files menu qsettings registry save
    settings.beginGroup("recent_files");
    for(int i = 0;i < mRecentFileList.size();i++){
        settings.setValue(QString("file-%1").arg(i),mRecentFileList[i]);
    }
    settings.endGroup();

    writeVisualizationSettings();

    //settings.setValue("javascript", ui->scriptCode->toPlainText());

}

void MainWindow::recentFileMenu()
{
    if(mRecentFileList.size() > 9){
        mRecentFileList.removeAt(9);
    }
    if(mRecentFileList.contains(ui->lConfigFile->text())){
        mRecentFileList.removeAt(mRecentFileList.indexOf(ui->lConfigFile->text()));
     }

    if (!ui->lConfigFile->text().isEmpty())
        mRecentFileList.prepend(ui->lConfigFile->text());

    for(int i = 0;i < ui->menuRecent_files->actions().size();i++){
        if(i < mRecentFileList.size()){
            ui->menuRecent_files->actions()[i]->setText(mRecentFileList[i]);
            connect(ui->menuRecent_files->actions()[i],SIGNAL(triggered()),this,SLOT(menuRecent_files()));
            ui->menuRecent_files->actions()[i]->setVisible(true);
        }else{
            ui->menuRecent_files->actions()[i]->setVisible(false);
        }
     }

}

void MainWindow::checkAvailableActions()
{
    if (!mMC)
        return;
    ui->actionRunSim->setEnabled( mMC->state()->isModelPaused() || mMC->state()->isModelReadyToRun());
    ui->actionStopSim->setEnabled( mMC->state()->isModelRunning() );
    ui->actiondelete_model->setEnabled( mMC->state()->isModelValid() );
    ui->actionSetupProject->setEnabled( !mMC->state()->isModelRunning() && !ui->lConfigFile->text().isEmpty());
    ui->actionOpenProject->setEnabled( !mMC->state()->isModelRunning() );
    ui->openProject->setEnabled(!mMC->state()->isModelRunning());
    ui->actionReset_view->setEnabled( mMC->state()->isModelValid() );

}

void MainWindow::updateModelStats()
{
    auto stats = mMC->systemStatus();
    ui->listStatus->clear();
    ui->listStatus->setHorizontalHeaderLabels({"Statistic", "Value"});
    ui->listStatus->setRowCount(static_cast<int>(stats.size()));
    ui->listStatus->setColumnCount(2);
    int r=0;
    for (std::pair<std::string, std::string> s : stats) {
        ui->listStatus->setItem(r,0, new QTableWidgetItem(QString::fromStdString(s.first)));
        ui->listStatus->setItem(r,1, new QTableWidgetItem(QString::fromStdString(s.second)));
        ++r;
    }

}

void MainWindow::onModelCreated()
{
    ui->visVariables->clear();
    ui->visCellData->clear();
    CellWrapper cw(nullptr);
    auto & vars = cw.getVariablesList();
    auto & metadata = cw.getVariablesMetaData();

    QList<QTreeWidgetItem *> items;
    QStack<QTreeWidgetItem*> stack;
    QList<QTreeWidgetItem *> items_insp;
    QStack<QTreeWidgetItem*> stack_insp;
    items_insp.append(new QTreeWidgetItem(QStringList() << "State")); // add group
    items_insp.back()->setData(0, Qt::UserRole+0, -2);

    stack.push(nullptr);
    stack_insp.push(nullptr);
    std::string group="";
    for (size_t i=0;i<vars.size();++i) {
        if (metadata[i].first != group) {
            if (stack.size()>1) {
                stack.pop();
                stack_insp.pop();
            }
            items.append(new QTreeWidgetItem(stack.last(), QStringList() << QString::fromStdString(metadata[i].first))); // add group
            items.back()->setData(0, Qt::UserRole+0, -1);
            stack.push(items.back());

            items_insp.append(new QTreeWidgetItem(stack_insp.last(), QStringList() << QString::fromStdString(metadata[i].first))); // add group
            items_insp.back()->setData(0, Qt::UserRole+0, -1);
            stack_insp.push(items_insp.back());

            group = metadata[i].first;
        }
        items.append( new QTreeWidgetItem(stack.last(),  QStringList() << QString::fromStdString(vars[i]) ) ); // add variable
        items.back()->setToolTip(0, QString::fromStdString(metadata[i].second));
        items.back()->setData(0, Qt::UserRole+0, static_cast<int>(i));
        items_insp.append( new QTreeWidgetItem(stack_insp.last(),  QStringList() << QString::fromStdString(vars[i]) ) ); // add variable
        items_insp.back()->setToolTip(0, QString::fromStdString(metadata[i].second));
        items_insp.back()->setData(0, Qt::UserRole+0, static_cast<int>(i));

    }

    ui->visVariables->addTopLevelItems(items);
    ui->visCellData->addTopLevelItems(items_insp);

}

void MainWindow::populateInspector(QVector3D point)
{
    if (!mMC->state()->isModelValid())
        return;
    auto &grid = mMC->model()->instance()->landscape()->grid();
    if (!grid.coordValid(static_cast<double>(point.x()), static_cast<double>(point.y())))
        return;
    const auto &cell = grid(static_cast<double>(point.x()), static_cast<double>(point.y()));
    if (cell.isNull())
        return;
    CellWrapper cw(&cell);

    // loop over all elements of the inspector and
    ui->visCellData->topLevelItem(0);
    QTreeWidgetItemIterator it(ui->visCellData);
    while (*it) {
        int idx = (*it)->data(0, Qt::UserRole+0).toInt();
        if (idx >= 0) {
            double val = cw.value(static_cast<size_t>(idx));
            (*it)->setText(1, QString::number(val));
        }
        if (idx<-1) {
            switch (-idx) {
            case 2: (*it)->setText(1, QString::fromStdString(cell.state()->asString()));  break;

            }
        }
        ++it;
    }
}



void MainWindow::on_pbReloadQml_clicked()
{
    if (!mQmlView)
        return;
    mQmlView->engine()->clearComponentCache();
    qDebug() << mQmlView->source();
    mQmlView->setSource(mQmlView->source());
}

void MainWindow::on_action3D_Camera_settings_triggered()
{
    // open form for camera control
    CameraControl *cntrl = new CameraControl(this);
    cntrl->setSurfaceGraph( ui->main3d);
    cntrl->setLandscapeVisualization(mLandscapeVis);
    QObject::connect(ui->main3d, &SurfaceGraph::cameraChanged, cntrl, &CameraControl::cameraChanged);
    cntrl->cameraChanged(); // update initital values
    cntrl->show();
}

void MainWindow::on_actionAbout_SVD_triggered()
{
    AboutDialog adlg(nullptr);
    adlg.exec();
}

void MainWindow::on_actionOnline_resources_triggered()
{
    //https://github.com/SVDmodel/SVD/blob/master/README.md
    QDesktopServices::openUrl(QUrl("https://github.com/SVDmodel/SVD/blob/master/README.md"));

}

void MainWindow::on_lConfigFile_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    checkAvailableActions();
}

void MainWindow::on_visVariables_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!mLandscapeVis->isValid())
        return;

    if (!current || current->data(0, Qt::UserRole+0).isNull())
        return;

    int key = current->data(0, Qt::UserRole+0).toInt();
    if (key<0)
        return;
    size_t ukey = static_cast<size_t>(key);
    spdlog::get("main")->debug("Clicked on {}", key);
    CellWrapper cw(nullptr);
    ui->visVariable->setChecked(true);
    mLandscapeVis->renderVariable(QString::fromStdString( cw.getVariablesList()[ukey] ),
                                  QString::fromStdString(cw.getVariablesMetaData()[ukey].second) );


}

void MainWindow::on_actionReset_view_triggered()
{
    if (mLandscapeVis->isValid()) {
        mLandscapeVis->resetView();
    }
}

void MainWindow::on_actionSaveView_1_triggered()
{
    if (mLandscapeVis->isValid()) {
        mLandscapeVis->saveView(1);
        ui->actionCustom_view_1->setEnabled(true);
        writeVisualizationSettings();
    }
}

void MainWindow::on_actionSaveView_2_triggered()
{
    if (mLandscapeVis->isValid()) {
        mLandscapeVis->saveView(2);
        ui->actionCustom_View_2->setEnabled(true);
        writeVisualizationSettings();
    }

}

void MainWindow::on_actionSaveView_3_triggered()
{
    if (mLandscapeVis->isValid()) {
        mLandscapeVis->saveView(3);
        ui->actionCustom_View_3->setEnabled(true);
        writeVisualizationSettings();
    }

}

void MainWindow::on_actionCustom_view_1_triggered()
{
    if (mLandscapeVis->isValid()) {
        mLandscapeVis->resetView(1);
    }
}

void MainWindow::on_actionCustom_View_2_triggered()
{
    if (mLandscapeVis->isValid()) {
        mLandscapeVis->resetView(2);
    }

}

void MainWindow::on_actionCustom_View_3_triggered()
{
    if (mLandscapeVis->isValid()) {
        mLandscapeVis->resetView(3);
    }

}
