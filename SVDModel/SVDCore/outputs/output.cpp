#include "output.h"

Output::Output()
{
    mEnabled=false;
}

Output::~Output()
{
    if (mFile.is_open())
        mFile.close();
}

void Output::setup()
{

}

void Output::execute()
{

}

std::string Output::key(std::string key_elem) const
{
    return "output." + name() + "." + key_elem;
}

void Output::flush()
{
    if (mFile.is_open())
        mFile.flush();
}
