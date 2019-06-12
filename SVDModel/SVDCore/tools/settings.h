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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <vector>
#include <map>
#include <string>


class Settings
{
public:
    Settings();
    bool loadFromFile(const std::string &fileName);
    void loadFromSettings(const Settings &copy_from);
    void dump();
    // access
    bool hasKey(const std::string &key) const {return mValues.find(key) != mValues.end(); }
    /// throw an exception when any of the elements of 'keys' is not part of the settings
    /// the praefix is added, e.g.: praefix='climate', keys={a,b} -> test for 'climate.a' and 'climate.b'
    bool requiredKeys(std::string praefix, const std::vector<std::string> &keys) const;

    /// get a list of all keys that start with a certain string
    std::vector<std::string> findKeys(std::string start_with, bool one_level=false) const;

    /// get value for 'key', if 'default_value' is not provided, a exception is thrown if the setting is not present
    std::string valueString(const std::string &key, std::string default_value="") const {
        auto it = mValues.find(key);
        if (it==mValues.end()) {
            if (default_value.size()>0) {
                return default_value;
            } else {
                throw std::logic_error( "Error in Settings: Key: '" + key + "' not found." );
            }
        }
        return it->second;
    }
    int valueInt(const std::string &key, int default_value=-999999) const;
    size_t valueUInt(const std::string &key, int default_value=0) const {return static_cast<size_t>(valueInt(key, default_value)); }
    double valueDouble(const std::string &key, double default_value=-99999999.) const;
    bool valueBool(const std::string &key, const std::string default_value="") const { std::string v = valueString(key, default_value);
                                                                               if (v=="true" || v=="True") return true;
                                                                               if (v=="false" || v=="False") return false;
                                                                             throw std::logic_error("Error in Settings: Value for key '"+ key + "' (bool) is empty or invalid (allowed values: 'true','True','false','False') and no default value is specified. Value: '" + v + "', default: '" + default_value + "'"); }

    /// set the 'key' to 'value'. Throws an exception if key is not valid.
    bool setValue(const std::string &key, std::string value);

private:
    std::map< std::string, std::string > mValues;
    void checkLogDefault(const std::string &key, std::string default_value);
};

#endif // SETTINGS_H
