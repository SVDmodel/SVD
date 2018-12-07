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
