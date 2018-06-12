#include "states.h"

#include "model.h"
#include "tools.h"
#include "strtools.h"
#include "filereader.h"
#include "randomgen.h"

std::vector<std::string> State::mValueNames;

States::States()
{

}

void States::setup()
{
    std::string file_name = Tools::path(Model::instance()->settings().valueString("states.file"));
    FileReader rdr(file_name);
    rdr.requiredColumns({"stateId", "composition", "structure", "fct"});

    while (rdr.next()) {
        // read line
        state_t id = state_t( rdr.value("stateId") );
        mStates.push_back( State(id,
                rdr.valueString("composition"),
                int(rdr.value("structure")),
                int(state_t(rdr.value("fct"))))
                           );
        mStateSet.insert({id, mStates.size()-1}); // save id and index

    }
    spdlog::get("setup")->debug("Loaded {} states from file '{}'", mStates.size(), file_name);

}

bool States::loadProperties(const std::string &filename)
{
    FileReader rdr(filename);
    rdr.requiredColumns({"stateId"});
    int sidx = static_cast<int>(rdr.columnIndex("stateId"));
    int lineno=0;
    bool has_errors = false;
    while (rdr.next()) {
        ++lineno;
        try {
            const State &state = Model::instance()->states()->stateById(static_cast<int>(rdr.value(sidx)));
            for (int i=0;i<rdr.columnCount();++i)
                if (i != sidx)
                    const_cast<State&>(state).setValue(rdr.columnName(i), rdr.value(i));

        } catch (const std::logic_error &) {
            spdlog::get("setup")->warn("loading of properties: state {} is not valid (line {})!", rdr.value(sidx), lineno);
            has_errors = true;
        }
    }
    spdlog::get("setup")->debug("Loaded {} values from file '{}'. States have now: {}", rdr.columnCount()-1, filename, join(State::valueNames()));
    return !has_errors;

}

const State &States::randomState() const
{
    size_t i = static_cast<size_t>(  irandom(0, static_cast<int>(mStates.size())) );
    return mStates[i];
}

const State &States::stateById(int id)
{

    auto s = mStateSet.find(id);
    if (s!=mStateSet.end())
        return stateByIndex(s->second);
    throw std::logic_error("Invalid state id: " + to_string(id));

}

State::State(state_t id, std::string composition, int structure, int function)
{
    mId = id;
    mComposition = trimmed(composition);
    mStructure = structure;
    mFunction = function;
    auto species = Model::instance()->species();
    mSpeciesShare = std::vector<double>(species.size(), 0.);

    // special cases:
    if (mComposition == "mix" || mComposition=="unforested")
        return;

    int dominant_species_index = -1;
    int admixed_species_index[5];
    for (int i=0;i<5;++i) admixed_species_index[i]=-1;

    auto comp_species = split(mComposition, ' ');
    int adm_index = 0;

    for (const auto &s : comp_species) {
       auto it = std::find(species.begin(), species.end(), lowercase(s));
       if (it == species.end())
           throw std::logic_error("Invalid state: species '" + s + "' not found. Id: " + to_string(id));

       if (s == lowercase(s)) {
           // save the index of the admixed species
           admixed_species_index[adm_index++] = static_cast<int>(it-species.begin());;
           if (adm_index>5)
               throw std::logic_error("Invalid state: too many admixed species. id: " + to_string(id) + ", " + mComposition);
       } else {
           // the dominant species
           if (dominant_species_index!=-1)
               throw std::logic_error("Invalid state: more than 1 dominant species: id: " + to_string(id) + ", " + mComposition);
           dominant_species_index = static_cast<int>(it-species.begin());
       }
    }

    /* apply some rules:
     * (a) only 1 dominant species: 100%
     * (b) 1 dom. and 1 other: 67/33
     * (c) only 1 other: 50
     * (d) two other: 50/50
     * (e) three other: 33/33/33
     * (f) 4 other: 4x 25
     * none: 0
     * */


    if (dominant_species_index>-1) {
        if (admixed_species_index[0]==-1) {
            mSpeciesShare[dominant_species_index]=1.; // (a)
        } else {
            // max 1 other species: >66% + >20% -> at least 86% -> no other species possible
            mSpeciesShare[dominant_species_index]=0.67; // (b)
            mSpeciesShare[admixed_species_index[0]]=0.33;
        }
    } else {
        // no dominant species
        int n_s = 0;
        for (int i=0;i<5;++i)
            if (admixed_species_index[i]>-1) ++n_s;

        double f;
        switch (n_s) {
        case 0: f=0.; // (f)
        case 1: f=0.5; break;
        case 2: f=0.5; break;
        case 3: f=0.33;  break;
        case 4: f=0.25; break;

        }

        // apply cases
        for (int i=0;i<n_s;++i)
            mSpeciesShare[admixed_species_index[i]]=f;

    }


}

std::string State::asString() const
{
    std::stringstream s;
    s << compositionString() << " S" << structure() << " F" << function();
    return s.str();
}

void State::setValue(const std::string &name, double value)
{
    int index = valueIndex(name);
    if (index<0) {
        // add property to the list of properties
        mValueNames.push_back(name);
        index = valueIndex(name);
    }
    setValue(index, value);

}

void State::setValue(const int index, double value)
{
    // check if array is large enough
    if (mValues.size() != mValueNames.size()) {
        mValues.resize(mValueNames.size());
    }
    mValues[index] = value;
}
