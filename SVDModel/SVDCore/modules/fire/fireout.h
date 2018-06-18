#ifndef FIREOUT_H
#define FIREOUT_H

#include "../../outputs/output.h"

class FireOut : public Output
{
public:
    FireOut();
    void setup();
    void execute();
private:

};

#endif // FIREOUT_H
