#include "climate.h"
#include "model.h"
#include "filereader.h"
#include "tools.h"
#include "strtools.h"

Climate::Climate()
{

}

void Climate::setup()
{
    auto settings = Model::instance()->settings();
    settings.requiredKeys("climate", {"file"});
    std::string file_name = Tools::path(settings.valueString("climate.file"));
    FileReader rdr(file_name);

    rdr.requiredColumns({"climateId", "year"});

    auto i_id = rdr.columnIndex("climateId");
    auto i_year = rdr.columnIndex("year");

    auto lg = spdlog::get("setup");
    lg->debug("reading climate file '{}' with {} columns. climateId: col {}, year: col {}.", file_name, rdr.columnCount(), i_id, i_year);
    mNColumns = rdr.columnCount()-2;
    int n=0;
    while (rdr.next()) {
        int id = int( rdr.value(i_id) );
        int year =int (rdr.value(i_year));

        mAllIds.insert(id);
        mAllYears.insert(year);

        auto &year_container = mData[year];
        auto &vec = year_container[id];
        vec.resize(rdr.columnCount()-2);
        for (size_t i=2;i<rdr.columnCount();++i)
            vec[i-2] = static_cast<float>(rdr.value(i));
        // scaling, TODO!
        // monthly climate:
        for (int i=0;i<12;++i)
            vec[i] =  (vec[i]- 6.3) / 6.7 ; // temp
        for (int i=12;i<24;++i)
            vec[i] = (vec[i]- 116) / 63; // precip

        ++n;
    }
    lg->debug("loaded {} records.", n);

    if (lg->should_log(spdlog::level::trace)) {
        lg->trace("************");
        lg->trace("Elements of {}", file_name);
        lg->trace("Years: {}", join(mAllYears.begin(), mAllYears.end(), ", "));
        lg->trace("Ids: {}", join(mAllIds.begin(), mAllIds.end(), ", "));

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
