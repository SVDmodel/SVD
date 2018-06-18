#include "output.h"
#include "model.h"
#include "tools.h"
#include "settings.h"
#include "spdlog/spdlog.h"

Output::Output()
{
    mEnabled=false;
    mSeparator = ',';
}

Output::~Output()
{
    if (mFile.is_open())
        mFile.close();
}

void Output::setup()
{

}

void Output::execute()
{

}

std::string Output::key(std::string key_elem) const
{
    return "output." + name() + "." + key_elem;
}

void Output::flush()
{
    if (mFile.is_open())
        mFile.flush();
}

void Output::openOutputFile(std::string default_key, bool write_header)
{
    mOutputFileName = Tools::path(Model::instance()->settings().valueString(key(default_key)));
    auto lg = spdlog::get("setup");
    file().open(mOutputFileName, std::fstream::out);
    if (file().fail()) {
      lg->error("Cannot create output file: '{}' (output: {}): {}", mOutputFileName, name(), strerror(errno));
      throw std::logic_error("Error in setup of output '" + name() + "'.");
    }

    mOutStream = outstream(mFile);
    if (write_header) {
        for (auto &c : mColumns)
            out() << c.columnName;
        out().write();

    }

    // write header based on columns
//    if (write_header) {
//        for (auto &c : mColumns)
//            file() << c.columnName << mSeparator;
//        file() << std::endl;
//    }


}
