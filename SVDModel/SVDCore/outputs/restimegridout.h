#ifndef RESTIMEOUT_H
#define RESTIMEOUT_H
#include "output.h"

class ResTimeGridOut: public Output
{
public:
    ResTimeGridOut();
    void setup();
    void execute();
private:
    int mInterval;
    std::string mPath;

};

#endif // RESTIMEOUT_H
