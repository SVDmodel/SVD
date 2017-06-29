#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void on_runInception_clicked();

    void on_setupModel_clicked();

    void on_selectFile_clicked();

    void on_doPredict_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
