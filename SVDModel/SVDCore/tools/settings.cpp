#include "settings.h"
#include "strtools.h"
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
