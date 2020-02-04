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
#include "settings.h"
#include "strtools.h"

#include "spdlog/spdlog.h"

#include <iostream>

Settings::Settings()
{

}

bool Settings::loadFromFile(const std::string &fileName)
{
    mValues.clear();
    std::vector<std::string> content = readFile(fileName);
    int line = 0;
    for (std::string &s: content) {
        // remove comments starting with # characters
        line ++;
        if (s.size()>0 && s[0]!='#') {
            size_t p = s.find('=');
            if (p!=std::string::npos) {
                std::string key = trimmed(s.substr(0,p));
                std::string value = trimmed(s.substr(p+1, s.size()));
                if (mValues.find(key)!=mValues.end())
                    throw std::logic_error("Error in Settings: The key '" + key +"' is not unique! In line: " + std::to_string(line));
                mValues[ key ] = value;
            }

        }
    }
    return true;
}

void Settings::loadFromSettings(const Settings &copy_from)
{
    mValues = copy_from.mValues;
}

void Settings::dump()
{
    std::cout << "There are " << mValues.size() << "entries in the configuration.";
    for (const auto &s : mValues)
        std::cout << s.first << ": " << s.second;
}

int Settings::valueInt(const std::string &key, int default_value) const
{
    std::string s = valueString(key);
    if (s.size()==0) {
        if (default_value != -999999)
            return default_value;
        else
            throw std::logic_error("Error in Settings: Key '"+ key + "' (int) is empty and no default value is specified.");
    }

    int result = default_value;
    try {
        result = std::stoi( s );
    } catch (std::invalid_argument) {
        throw std::logic_error("Error in Settings: The value of '" + key + "': '" + s + "' is not a valid (int) number.");
    }
    return result;
}

double Settings::valueDouble(const std::string &key, double default_value) const
{
    std::string s = valueString(key);
    if (s.size()==0) {
        if (default_value != -99999999.)
            return default_value;
        else
            throw std::logic_error("Error in Settings: Key '"+ key + "' (double) is empty and no default value is specified.");
    }

    double result = default_value;
    try {
        result = std::stod( s );
    } catch (std::invalid_argument) {
        throw std::logic_error("Error in Settings: The value of '" + key + "': '" + s + "' is not a valid (double) number.");
    }
    return result;
}

bool Settings::setValue(const std::string &key, std::string value)
{
    if (!hasKey(key))
        throw std::logic_error("Error in updating the settings: " + key + " is not a valid setting in the configuration file.");
    mValues[key] = value;
    return true;
}

void Settings::checkLogDefault(const std::string &key, std::string default_value)
{
    auto lg = spdlog::get("setup");
    if (lg->should_log(spdlog::level::trace)) {
        lg->trace("Settings: query key '{}', used default: '{}'.", key, default_value);
    }
}


bool Settings::requiredKeys(std::string praefix, const std::vector<std::string> &keys) const
{
    std::string msg;
    if (praefix.size()>0)
        praefix = praefix + ".";
    for (auto &s : keys)
        if (!hasKey(praefix + s))
            msg += praefix + s + ", ";
    if (msg.size()==0)
        return true;
    throw std::logic_error("Error: Required column(s) not in settings: " + msg);
}

std::vector<std::string> Settings::findKeys(std::string start_with, bool one_level) const
{
    std::vector<std::string> keys;
    for (auto it=mValues.begin(); it!=mValues.end(); ++it) {
        if(it->first.substr(0, start_with.size()) == start_with) {
            if (one_level) {
                std::string rest = it->first.substr(start_with.size());
                rest = rest.substr(0, rest.find('.') );
                if (indexOf(keys, rest)==-1)
                    keys.push_back(rest);

            } else {
                keys.push_back(it->first);
            }
        }
    }

    return keys;
}

