#ifndef STATEHISTOUT_H
#define STATEHISTOUT_H
#include "output.h"


class StateHistOut: public Output
{
public:
    StateHistOut();
    void setup();
    void execute();

};

#endif // STATEHISTOUT_H
