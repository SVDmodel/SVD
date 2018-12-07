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
#include "climate.h"

#include <regex>

#include "model.h"
#include "filereader.h"
#include "tools.h"
#include "strtools.h"
#include "expression.h"

Climate::Climate()
{

}

void Climate::setup()
{
    auto settings = Model::instance()->settings();
    settings.requiredKeys("climate", {"file"});
    std::string file_name = Tools::path(settings.valueString("climate.file"));
    FileReader rdr(file_name);

    const auto &targetIds = Model::instance()->landscape()->climateIds();

    rdr.requiredColumns({"climateId", "year"});

    auto i_id = rdr.columnIndex("climateId");
    auto i_year = rdr.columnIndex("year");

    auto lg = spdlog::get("setup");
    lg->debug("reading climate file '{}' with {} columns. climateId: col {}, year: col {}.", file_name, rdr.columnCount(), i_id, i_year);
    mNColumns = rdr.columnCount()-2;

    // set up transformations
    std::vector<Expression> transformations;
    if (settings.hasKey("climate.transformations")) {
        transformations.resize(mNColumns);
        std::string tlist = settings.valueString("climate.transformations");
        // get elements via regex
        std::regex re("\\{([^\\:]*)\\:([^\\}]*)\\}");
        // \{([^\:]*)\:([^\}]*)\}
        std::sregex_iterator next(tlist.begin(), tlist.end(), re);
        std::sregex_iterator end;
        int n_setup=0;
        while (next != end) {
          std::smatch match = *next;
          lg->debug("climate transformation: for indices '{}' apply '{}'.", match.str(1), match.str(2));
          auto indices = split(match.str(1),',');
          auto expr = match.str(2);
          for (auto sidx : indices) {
              auto idx = std::stoull( sidx ); // to unsigned long long
              if (idx>=mNColumns) {
                  lg->error("Error in climate transformation: index '{}' is out of range (indices: {}, expression: {}, #of colums: {}). Note: indices are 0-based.", idx, join(indices, ","), expr, mNColumns);
                  throw std::logic_error("Error in setting up climate transformations.");
              }
              transformations[idx].setExpression(expr);
              ++n_setup;
          }
          next++;
        }
        for (size_t i=0;i<mNColumns;++i)
            if (transformations[i].expression().empty())
                transformations[i].setExpression("x"); // default: just pass-through
        lg->debug("Using '{}' expressions for {} columns.", n_setup, mNColumns);

    } else {
        lg->debug("No climate transformations specified. Using climate data as is.");
    }

    int n=0;
    int id_skipped = 0;
    while (rdr.next()) {
        int id = int( rdr.value(i_id) );
        int year =int (rdr.value(i_year));

        if (targetIds.find(id) == targetIds.end()) {
            ++id_skipped;
            continue;
        }

        mAllIds.insert(id);
        mAllYears.insert(year);

        auto &year_container = mData[year];
        auto &vec = year_container[id];
        vec.resize(rdr.columnCount()-2);
        for (size_t i=2;i<rdr.columnCount();++i)
            vec[i-2] = static_cast<float>(rdr.value(i));

        for (size_t i=0;i<transformations.size();++i) {
            // apply transformations (if present)
            vec[i] = static_cast<float>( transformations[i].calculate(vec[i]) );
        }
        // monthly climate: (fix for NPKA)
//        for (int i=0;i<12;++i)
//            vec[i] =  (vec[i]- 6.3) / 6.7 ; // temp
//        for (int i=12;i<24;++i)
//            vec[i] = (vec[i]- 116) / 63; // precip

        ++n;
    }
    lg->debug("loaded {} records.", n);

    if (lg->should_log(spdlog::level::trace)) {
        lg->trace("************");
        lg->trace("Elements of {}", file_name);
        lg->trace("Years: {}", join(mAllYears.begin(), mAllYears.end(), ", "));
        lg->trace("Ids: {}", join(mAllIds.begin(), mAllIds.end(), ", "));
        lg->trace("Skipped '{}' records (not present on landscape)", id_skipped);

    }

    settings.requiredKeys("climate", {"sequence.enabled"});
    if (settings.valueBool("climate.sequence.enabled")) {
        std::string txt = settings.valueString("climate.sequence");
        auto v = split_and_trim(txt, ','); // now a vector of strings....
        for (const auto &s : v) {
            int key = std::stoi(s);
            if (mAllYears.count(key)==0)
                throw std::logic_error("climate.sequence: the year '" + to_string(key) + "' is not valid!");
            mSequence.push_back( key );
        }
        lg->debug("climate sequence enabled, length={}", mSequence.size());
    } else {
        // create dummy sequence (just all years in the data)
        mSequence.insert(mSequence.begin(), mAllYears.begin(), mAllYears.end());
        lg->debug("climate sequence disabled, using the sequence from the data ({}-{}).", mSequence.front(), mSequence.back());

    }
    if (lg->should_log(spdlog::level::trace)) {
        // print the first and last elements...
        //std::vector<float> &vec = mData[*mAllYears.begin()][*mAllIds.begin()];
        const std::vector<float> &vec = singleSeries(*mAllYears.begin(), *mAllIds.begin());
        lg->trace("First entry: year={}, climateId={}: {}", *mAllYears.begin(), *mAllIds.begin(), join(vec.begin(), vec.end(), ", "));
        //const std::vector<float> &vec2 = series(*(mAllYears.end()--),*(mAllIds.end()--));
        //lg->trace("Last entry: year={}, climateId={}: ", *(mAllYears.end()--), *(mAllIds.end()--), join(vec2.begin(), vec2.end(), ", "));

        lg->trace("************");
    }
}

std::vector<const std::vector<float> *> Climate::series(int start_year, size_t series_length, int climateId) const
{
    size_t istart = static_cast<size_t>(start_year - 1);
    if (istart+series_length >= mSequence.size())
        throw std::logic_error("Climate-series: start year "+ to_string(start_year) +" is out of range (min: 1, max: "+ to_string(mSequence.size()-series_length)+")");
    std::vector<const std::vector<float> *> set(series_length);
    for (size_t i=0;i<series_length;++i)  {
        int year = mSequence[ istart + i ];
        set[i] = &singleSeries(year, climateId);
    }
    return set;
}
