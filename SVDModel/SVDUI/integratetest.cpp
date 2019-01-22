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
#include "integratetest.h"
#include "../SVDCore/tools/filereader.h"
#include "../SVDCore/tools/settings.h"
#include "../SVDCore/tools/randomgen.h"
#include "../Predictor/predictortest.h"
#include "tools/expression.h"

#include "spdlog/spdlog.h"

#include <QDebug>
QDebug operator<<(QDebug debug, const std::string &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << QString(c.data());

    return debug;
}

IntegrateTest::IntegrateTest()
{

}

void IntegrateTest::testGrid()
{
    DoubleGrid test(100.f,20,10); // 100m grid, 20x10 elements
    test.initialize(10.);
    test[PointF(52., 10.)] = 5.;

    Point p(10,10);
    qDebug() << test.isIndexValid(p);
    qDebug() << test.avg();

    p=Point(10,8);
    qDebug() << test[p];
    qDebug() << test[52., 10.];
    qDebug() << test[0];



    Grid<int> itest(10., 4,3);
    itest.setValues( {1,2,3,4,5,6,7,8,9,10,11,12} );
    qDebug() << itest(0,0) << itest(3,2) <<  itest(9.999, 9.999) << itest(10.,10.);

    qDebug() << gridToString(itest);

    qDebug() << "---- grid ----";
    Grid<double> gg;
    qDebug() << gg.cellsize() << ", " << gg.sizeX() << "," << gg.sizeY();
    std::string fname="E:\\Daten\\Projekte\\staff\\masterarbeit_lea\\height_10.asc";
    gg.loadGridFromFile(fname);
    //qDebug() << gridToString(gg).substr(0,1000);


    Vector3D a,b;
    modelToWorld(a,b);

    try {
    FileReader fr("E:\\Daten\\iLand\\projects\\RESIN\\tatra\\tatra_v2\\abe\\abe.csv");
    qDebug() << fr.columnCount();
    int n=10;
    while (fr.next() && --n>0)
        qDebug() << fr.value(0);

    fr.requiredColumns({"id", "agent", "pcs_ha", "fish", "fresh", "frash"});

    } catch(const std::exception &e) {
        qDebug() << e.what();
    }
}

bool tst(const Settings &s, const std::string key)
{
    try {
        qDebug() << key << s.valueDouble(key);
        return true;
    } catch (const std::logic_error &e) {
        qDebug() << key << e.what();
        return false;
    }
}

void IntegrateTest::testSettings()
{
    try{

    Settings s;
    s.loadFromFile("e:\\Daten\\SVD\\projects\\npka\\config.txt");

    s.dump();
    qDebug() << s.valueString("model.stateFile");
    qDebug() << s.valueString("model.someOtherThing");
    qDebug() << s.valueInt("test.intval");
    qDebug() << s.valueBool("test.boolval");
    qDebug() << s.valueDouble("test.dbl");

    std::vector<std::string> test_keys = {"test.test1", "test.test2", "test.test3", "test.test4"};
    std::for_each(test_keys.begin(), test_keys.end(), [&](auto k) {tst(s,k); } );

    } catch (const std::exception &e) {
            qDebug() << e.what();
    }
}

void IntegrateTest::testLogging()
{
    auto console = spdlog::get("main");
    console->info("Welcome to spdlog!");
    console->error("Some error message with arg{}..", 1);

    // Conditional logging example
    auto i = 2;
    console->warn_if(i != 0, "an important message");

    // Formatting examples
    console->warn("Easy padding in numbers like {:08d}", 12);
    console->critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    console->info("Support for floats {:03.2f}", 1.23456);
    console->info("Positional args are {1} {0}..", "too", "supported");
    console->info("{:<30}", "left aligned");


    spdlog::get("main")->info("loggers can be retrieved from a global registry using the spdlog::get(logger_name) function");

}

void IntegrateTest::testRandom()
{
    auto console = spdlog::get("main");

    console->debug("Rnd-number 0-1: {}, rnd-number: 0-10: {}, rnd-int: 3-5: {}", drandom(), nrandom(0.,10.), irandom(3,5));
    std::vector<int> buckets(6,0);
    for (int i=0;i<1000000;++i)
        buckets[irandom(3,5)]++;
    std::for_each(buckets.begin(), buckets.end(), [=](int n) { console->debug("N={}", n);});

    //char *p=0;
    //*p=1;
    console->warn("check the flush ;) ");

    buckets = {0,0};
    for (int i=0;i<10000000000;++i)
        buckets[drandom()]++;
    console->debug("N<0: {}, N=1: {}", buckets[0], buckets[1]);
    console->warn("check the flush 2 ;) ");

    buckets = {0,0,0,0,0,0};
    for (int i=0;i<1000000;++i)
        buckets[nrandom(3.0,5.0)]++;
    std::for_each(buckets.begin(), buckets.end(), [=](int n) { console->debug("N={}", n);});

    // test random generator for small numbers
    buckets = {0,0,0,0,0};
    for (int i=0;i<1000000000;++i) {
        double x = drandom();
        if (x<0.0001) buckets[0]++;
        if (x<0.00001) buckets[1]++;
        if (x<0.000001) buckets[2]++;
        if (x<0.0000001) buckets[3]++;
        if (x<0.00000001) buckets[4]++;
    }
    console->debug("Test for small probabilities: factors of 10:");
    std::for_each(buckets.begin(), buckets.end(), [=](int n) { console->debug("N={}", n);});

    double x2=0.;
    for (int i=0;i<1000000000;++i)
        x2 += drandom();
    console->debug("Drawn 1'000'000'000 random numbers");

}

void IntegrateTest::testTensor()
{

//    PredictorTest pt;
//    pt.tensorTest();

}

void IntegrateTest::testExpression()
{
    auto console = spdlog::get("main");
    Expression e("sin(x*2.434)");
    for (double x=0.; x<3; x+=0.1) {
        double res = e.calculate(x);
        console->debug("x: {} y: {}", x, res);
    }

}
