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
    void dump();
    // access
    bool hasKey(const std::string &key) const {return mValues.find(key) != mValues.end(); }
    /// throw an exception when any of the elements of 'keys' is not part of the settings
    /// the praefix is added, e.g.: praefix='climate', keys={a,b} -> test for 'climate.a' and 'climate.b'
    bool requiredKeys(std::string praefix, const std::vector<std::string> &keys) const;

    /// get a list of all keys that start with a certain string
    std::vector<std::string> findKeys(std::string start_with, bool one_level=false) const;

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

private:
    std::map< std::string, std::string > mValues;
    void checkLogDefault(const std::string &key, std::string default_value);
};

#endif // SETTINGS_H
