#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H
#include <string>
#include <vector>
#include "output.h"

class OutputManager
{
public:
    OutputManager();
    ~OutputManager();
    /// set up the outputs
    void setup();
    bool isSetup() const { return mIsSetup; }

    /// executes the output 'output_name'.
    /// returns true if the output was actually executed.
    bool run(const std::string &output_name);

    void yearEnd();

    // access

    /// return the output or nullptr if 'output_name' is not a valid output.
    Output *find(std::string output_name);
private:
    std::vector<Output*> mOutputs;
    bool mIsSetup;
};

#endif // OUTPUTMANAGER_H
