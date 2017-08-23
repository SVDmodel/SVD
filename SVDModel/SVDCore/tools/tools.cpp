#include "tools.h"

std::string Tools::mProjectDir;

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
    if (mProjectDir.size()>0)
        return mProjectDir + "/" + fileName;
    else
        return fileName;
}

void Tools::setProjectDir(const std::string &path)
{
    Tools::mProjectDir = path;
}
