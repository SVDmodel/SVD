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
#ifndef TOOLS_H
#define TOOLS_H
#include <string>
#include <vector>

class Settings; // forward

class Tools
{
public:
    Tools();
    /// returns 'true' if the file exists, false otherwise
    static bool fileExists(const std::string &fileName);
    /// returns the full path to 'fileName'. If a relative path if provided,
    /// it is resolved relative to the project directory
    static std::string path(const std::string &fileName);


    // maintenance
    static void setupPaths(const std::string &path, const Settings *settings);
private:
    static std::string mProjectDir;
    static std::vector< std::pair<std::string, std::string > > mPathReplace;
};

#endif // TOOLS_H
