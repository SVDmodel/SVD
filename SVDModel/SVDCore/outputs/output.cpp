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
