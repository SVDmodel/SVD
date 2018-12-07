/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
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

    /// builds a markdown documentation from all outputs
    std::string createDocumentation();

    // access

    /// return the output or nullptr if 'output_name' is not a valid output.
    Output *find(std::string output_name);
private:
    std::vector<Output*> mOutputs;
    bool mIsSetup;
};

#endif // OUTPUTMANAGER_H
