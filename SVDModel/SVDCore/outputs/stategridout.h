#ifndef STATEGRIDOUT_H
#define STATEGRIDOUT_H
#include "output.h"

class StateGridOut: public Output
{
public:
    StateGridOut();
    void setup();
    void execute();
private:
    int mInterval;
    std::string mPath;
};

#endif // STATEGRIDOUT_H
