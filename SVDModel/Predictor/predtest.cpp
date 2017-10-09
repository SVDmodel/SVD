#include "predtest.h"

#include <iomanip>

#include "tensorhelper.h"
#include "spdlog/spdlog.h"

PredTest::PredTest()
{

}

void PredTest::testTensor()
{
    auto console = spdlog::get("main");

    TensorWrap2d<float> *tw = new TensorWrap2d<float>(6, 4);
    for (int i=0;i<6;++i) {
        for (int j=0;j<4;++j)
            tw->example(i)[j] = i*100 + j;
    }
    std::stringstream ss;
    for (int i=0;i<6;++i) {
        ss << "Example " << i << ": ";
        for (int j=0;j<4;++j)
            ss << std::setw(8) << tw->example(i)[j];
        ss << std::endl;
    }
    console->debug("{}", ss.str());
    delete tw;

    TensorWrapper *tw2 = new TensorWrap2d<float>(6, 4);
    std::vector<TensorWrapper*> twlist;
    twlist.push_back(tw2);

    TensorWrap2d<float> *tw3 = static_cast<TensorWrap2d<float>*>(twlist[0]);

    for (int i=0;i<6;++i) {
        for (int j=0;j<4;++j)
            tw3->example(i)[j] = i*100 + j;
    }

    ss.str(""); // clear
    for (int i=0;i<6;++i) {
        ss << "Example " << i << ": ";
        for (int j=0;j<4;++j)
            ss << std::setw(8) << tw3->example(i)[j];
        ss << std::endl;
    }
    console->debug("{}", ss.str());

    //
    TensorWrapper *w3 = new TensorWrap3d<float>(4,2,2);
    //TensorWrap3d<float> *tw3d = new TensorWrap3d<float>(4,2,2);
    TensorWrap3d<float> *tw3d = static_cast<TensorWrap3d<float>*>(w3);
    // put data in
    float *src = tw3->example(1);
    float *dest = tw3d->example(1);
    for (int i=0;i<4;++i)
        *dest++ = *src++;
    memcpy(tw3d->example(2), tw3->example(2),4*sizeof(float));

    ss.str(""); // clear
    for (int i=0;i<6;++i) {
        ss << "Example " << i << ": ";
        for (int j=0;j<2;++j) {
            for (int k=0;k<2;++k)
                ss << std::setw(8) << tw3d->row(i,j)[k];
            ss << " * ";
        }

        ss << std::endl;
    }
    console->debug("{}", ss.str());

    delete tw3d;
}
