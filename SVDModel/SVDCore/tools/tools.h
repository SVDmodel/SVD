#ifndef TOOLS_H
#define TOOLS_H
#include <string>

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
    static void setProjectDir(const std::string &path);
private:
    static std::string mProjectDir;
};

#endif // TOOLS_H
