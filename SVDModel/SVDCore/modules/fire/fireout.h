#ifndef FIREOUT_H
#define FIREOUT_H

#include "../../outputs/output.h"
#include "expression.h"

class FireOut : public Output
{
public:
    FireOut();
    void setup();
    void execute();
private:
    Expression mLastFire;
    std::string mLastFirePath;

};

#endif // FIREOUT_H
