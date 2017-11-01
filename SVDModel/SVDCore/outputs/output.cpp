#include "output.h"

Output::Output()
{
    mEnabled=false;
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
