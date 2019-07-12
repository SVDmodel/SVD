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
#include "tools.h"
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "settings.h"
#include "strtools.h"

#include <chrono>
#include <sstream>
#include <iomanip>

std::string Tools::mProjectDir;
std::vector< std::pair<std::string, std::string > > Tools::mPathReplace;

// helper function for getting a timestamp:

std::string getTimeStamp() {
  auto now = std::chrono::system_clock::now();
  auto itt = std::chrono::system_clock::to_time_t(now);
  std::ostringstream ss;
  ss << std::put_time(gmtime(&itt), "%Y%m%d_%H%M%S");
  return ss.str();
}

Tools::Tools()
{

}

bool Tools::fileExists(const std::string &fileName)
{
    // https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
    if (FILE *file = fopen(fileName.c_str(), "r")) {
            fclose(file);
            return true;
        } else {
            return false;
        }
}

std::string Tools::path(const std::string &fileName)
{
    if (fileName.find('$') != std::string::npos) {
        std::string str = fileName; // a copy
        // the path contains the magic delimiter - replace all elements of the
        for (auto &p : mPathReplace) {
            find_and_replace(str, p.first, p.second);
        }
        if (mProjectDir.size()>0)
            return mProjectDir + "/" + str;
        else
            return str;
    }
    if (mProjectDir.size()>0)
        return mProjectDir + "/" + fileName;
    else
        return fileName;
}

void Tools::setupPaths(const std::string &path, const Settings *settings)
{
    Tools::mProjectDir = path;
    mPathReplace.clear();

    auto keys = settings->findKeys("filemask");

    for (auto &s : keys) {
        auto key = "$" + s.substr(9) + "$"; // 9: "filemask."
        mPathReplace.push_back(std::pair<std::string, std::string>(key, settings->valueString(s)));

    }

    mPathReplace.push_back(std::pair<std::string, std::string>("$timestamp$",  getTimeStamp()));


}
