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
            int p = s.find('=');
            if (p!=-1) {
                std::string key = trimmed(s.substr(0,p));
                std::string value = trimmed(s.substr(p+1, s.size()));
                if (mValues.find(key)!=mValues.end())
                    throw std::logic_error("Error in Settings: The key '" + key +"' is not unique! In line: " + std::to_string(line));
                mValues[ key ] = value;
            }

        }
    }
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
