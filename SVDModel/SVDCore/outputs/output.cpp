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

std::string Output::createDocumentation()
{
    std::string result;
    result = fmt::format("<a name=\"{}\"></a>\n## {}\n", mName, mName);
    result += fmt::format("{}\n", mDescription);
    if (mColumns.size() > 0) {
        result += "\n### Columns\n";
        result += "Column|Description|Data type\n";
        result += "------|-----------|---------\n";
        for (auto &col : mColumns) {
            result += fmt::format("{} | {} | {}\n", col.columnName, col.description, dataTypeString(col.type));
        }
        result += "\n";
    }
    result += "\n";
    return result;
}

std::string Output::dataTypeString(Output::DataType type)
{
    switch (type) {
    case String: return "String";
    case Int: return "Int";
    case Double: return "Double";
    }
    return "Invalid datatype!";
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
