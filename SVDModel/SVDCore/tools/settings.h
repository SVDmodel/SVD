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
    bool hasKey(const std::string &key) {return mValues.find(key) != mValues.end(); }

    std::string valueString(const std::string &key, std::string default_value="") const {
        auto it = mValues.find(key);
        if (it==mValues.end())
            if (default_value.size()>0)
                return default_value;
            else
                throw std::logic_error( "Error in Settings: Key: '" + key + "' not found." );
        return it->second;
    }
    int valueInt(const std::string &key, int default_value=-999999) const;
    double valueDouble(const std::string &key, double default_value=-99999999.) const;
    bool valueBool(const std::string &key, bool default_value = false) const { std::string v = valueString(key);
                                                                               if (v=="true" || v=="True") return true;
                                                                               if (v=="false" || v=="False") return false;
                                                                             return default_value; }

private:
    std::map< std::string, std::string > mValues;
};

#endif // SETTINGS_H
