#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H
#include <string>
#include <vector>

class OutputManager
{
public:
    OutputManager();
    void setup();

    void run();
private:
    std::vector<std::string> mOutputNames;
    std::vector<Output*> mOutputs;
};

#endif // OUTPUTMANAGER_H
