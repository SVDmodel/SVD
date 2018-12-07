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
#ifndef PREDICTORTEST_H
#define PREDICTORTEST_H
#include <QString>

namespace tensorflow {
class Session;
class Tensor;
class Status;
}

class PredictorTest
{

public:
    PredictorTest();
    ~PredictorTest();
    bool setup(QString model_path);
    // inception
    QString classifyImage(QString image_path);
    //
    QString insight();
    QString runModel();
    // tensortest:
    void tensorTest();
private:
    tensorflow::Status getTopClasses(const tensorflow::Tensor &classes, const int n_top, tensorflow::Tensor *indices, tensorflow::Tensor *scores);
    tensorflow::Session *session;
    std::string label_file;


};

#endif // PREDICTORTEST_H
