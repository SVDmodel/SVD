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
#include "externalseeds.h"

#include "settings.h"
#include "filereader.h"
#include "tools.h"
#include "model.h"

#include <random>

ExternalSeeds::ExternalSeeds()
{

}

void ExternalSeeds::setup()
{
    auto settings = Model::instance()->settings();
    auto lg = spdlog::get("setup");

    settings.requiredKeys("externalSeeds", {"grid", "file", "enabled"});
    bool enabled = settings.valueBool("externalSeeds.enabled");
    if (!enabled) {
        mExternalSeedTypes.clear();
        lg->debug("External seeds not enabled.");
        return;
    }
    // fixed mixtureType 0 -> non-forested area (empty)
    size_t n_species = Model::instance()->species().size();
    mExternalSeedTypes[0] = std::vector<double>(n_species);

    std::string table_file_name = Tools::path(settings.valueString("externalSeeds.file"));
    std::string grid_file_name = Tools::path(settings.valueString("externalSeeds.grid"));

    lg->debug("Setup of external seeds, load grid from '{}', table from '{}'.", grid_file_name, table_file_name);

    Grid<int> eseed;
    eseed.loadGridFromFile(grid_file_name);
    lg->debug("Loaded the grid '{}'. Dimensions: {} x {}, with cell size: {}m. ", grid_file_name, eseed.sizeX(), eseed.sizeY(), eseed.cellsize());

    FileReader rdr(table_file_name);
    rdr.requiredColumns({"id"});

    mTableMode =  ( rdr.columnIndex("state") == -1);

    int n_added;
    if (mTableMode) {
        lg->debug("Loading from a table with species shares.");
        n_added = setupFromSpeciesShares(rdr, eseed);
    } else {
        lg->debug("Loading from a table with discrete states.");
        n_added = setupFromStates(rdr, eseed);

    }


    lg->info("Setup of external seeds finished, loaded {} mixture types.", n_added);


}

const std::vector<double> &ExternalSeeds::speciesShares(int type_id) const
{
    std::map<int, std::vector<double> >::const_iterator vec = mExternalSeedTypes.find(type_id);
    if (vec != mExternalSeedTypes.end())
        return vec->second;
    throw std::logic_error("ExternalSeeds: invalid type " + to_string(type_id));
}

/// the file contains an Id, and pairs of columns for species and fraction.
int ExternalSeeds::setupFromStates(FileReader &rdr, Grid<int> &eseed)
{
    struct SItem {
        std::vector<state_t> states;
        std::vector<int> shares;
        std::discrete_distribution<int> distribution;
        state_t randomState() {
            int i = distribution(generator);
            return states[i];
        }
        std::default_random_engine generator;
    };



    std::map<int, SItem> item_map;
    auto lg = spdlog::get("setup");

    auto i_id = rdr.columnIndex("id");
    if (i_id != 0)
        throw std::logic_error("External seeds: setup from state: 'id' column must be the first column.");

    while (rdr.next()) {
        int id = static_cast<int>(rdr.value(i_id));
        SItem &item = item_map[id];
        for (size_t i=1;i<rdr.columnCount();i+=2) {
            if (rdr.columnName(i) != "state" || i+1>=rdr.columnCount() || rdr.columnName(i+1)!="fraction") {
                lg->error("Setup of external seeds from state file: invalid column at index {}. Expected are pairs of 'state' and 'fraction' columns. ", i);
                throw std::logic_error("External seeds error (check log)");
            }
            item.states.push_back(static_cast<state_t>(rdr.value(i)));
            item.shares.push_back(static_cast<int>(rdr.value(i+1)*100));
        }
        item.distribution = std::discrete_distribution<int>(item.shares.begin(), item.shares.end());
    }

    // assign random states all pixels that are on the grid
    auto &grid = Model::instance()->landscape()->grid();
    for (int i=0;i<grid.count();++i) {
        if (grid[i].isNull()) {
           PointF p = grid.cellCenterPoint(i);
           if (eseed.coordValid(p)) {
               int mix_type = eseed[p];
               if (mix_type==0) {
                   grid[i].setExternalSeedType(0); // species non-forested state
                   continue;
               }
               if (item_map.find(mix_type) == item_map.end()) {
                   lg->error("Error: mixture type '{}' is not a valid mixture type (found at: {}/{}m)", mix_type,p.x(),p.y() );
                   throw std::logic_error("Error in setup of exernal seeds (check the log).");
               }
               state_t s = item_map[mix_type].randomState();
               grid[i].setExternalState(s);
           }

        }
    }
    return static_cast<int>(item_map.size());

}


/// species shares: the table contains two columns per species (l_<species> for local neighborhood,
/// m_<species> for mid-range neighborhood. The value is used as a relative species fraction for the mixture type.
int ExternalSeeds::setupFromSpeciesShares(FileReader &rdr, Grid<int> &eseed)
{
    auto lg = spdlog::get("setup");

    auto i_id = rdr.columnIndex("id");
    auto species_names = Model::instance()->species();
    std::vector<int> col_idx(rdr.columnCount(), -1);

    for (size_t i=0;i<rdr.columnCount();++i) {
        if (i != i_id) {
            if (rdr.columnName(i)[0] != 'l' && rdr.columnName(i)[0] != 'm')
                throw std::logic_error("Setup of external species: invalid column name '" + rdr.columnName(i) + "'. Column format: [l|m]_<speciesId>. ");
            int local_offset = rdr.columnName(i)[0] == 'l' ? 0 : 1; // local/midrange
            std::string species = rdr.columnName(i).substr(2);
            int i_species = indexOf(species_names, species);
            if (i_species<0)
                throw std::logic_error("Setup of external seeds: Species '" + species + "' is not a valid species!");
            col_idx[i] = i_species*2 + local_offset;
        }
    }

    while (rdr.next()) {
        int mixture_id = static_cast<int>(rdr.value(i_id));
        if (mExternalSeedTypes.find(mixture_id) != mExternalSeedTypes.end())
            throw std::logic_error("Setup of external seeds: Invalid mixture type: " + to_string(mixture_id)+" - Key already used.");
        std::vector<double> spec_shares(species_names.size()*2, 0.);

        for (size_t i=0;i<rdr.columnCount();++i) {
            if (i!=i_id) {
                double val = rdr.value(i);
                if (val<0. || val>1.)
                    throw std::logic_error("Setup of external species: Invalid share of mixture type " + to_string(mixture_id) + ", column: " + rdr.columnName(i) + "; range has to be [0..1], value is: " + to_string(val));
                spec_shares[ static_cast<size_t>(col_idx[i]) ] = val;

            }
        }
        mExternalSeedTypes[mixture_id] = spec_shares;
    }

    auto &grid = Model::instance()->landscape()->grid();
    for (int i=0;i<grid.count();++i) {
        if (grid[i].isNull()) {
           PointF p = grid.cellCenterPoint(i);
           if (eseed.coordValid(p)) {
               int mix_type = eseed[p];
               if (mExternalSeedTypes.find(mix_type) == mExternalSeedTypes.end()) {
                   lg->error("Error: mixture type '{}' is not a valid mixture type (found at: {}/{}m)", mix_type,p.x(),p.y() );
                   throw std::logic_error("Error in setup of exernal seeds (check the log).");
               }
               grid[i].setExternalSeedType(mix_type);
           }

        }
    }
    return static_cast<int>(mExternalSeedTypes.size())-1; // without the "0"-type

}
