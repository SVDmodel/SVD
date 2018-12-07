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
#include "transitionmatrix.h"

#include "spdlog/spdlog.h"

#include "strtools.h"
#include "filereader.h"
#include "randomgen.h"

TransitionMatrix::TransitionMatrix()
{

}

bool TransitionMatrix::load(const std::string &filename)
{
    FileReader rdr(filename);
    rdr.requiredColumns({"stateId", "key", "targetId", "p"});
    auto is = rdr.columnIndex("stateId");
    auto ik = rdr.columnIndex("key");
    auto it = rdr.columnIndex("targetId");
    auto ip = rdr.columnIndex("p");
    int n=0;
    while (rdr.next()) {
        // read line
        state_t id = state_t( rdr.value(is) );
        int key = int( rdr.value(ik) );
        state_t target = state_t( rdr.value(it) );
        double p =  rdr.value(ip);

        auto &e = mTM[ {id, key} ];
        e.push_back({target, p});
        ++n;
    }

    // TODO: check transition matrix: states have to be valid, p should sum up to 1

    spdlog::get("setup")->debug("Loaded transition matrix for {} states from file '{}' (processed {} records).", mTM.size(), filename, n);
    return true;
}

state_t TransitionMatrix::transition(state_t stateId, int key)
{
    auto it = mTM.find({stateId, key});
    if (it == mTM.end()) {
        throw logic_error_fmt("TransitionMatrix: no valid transitions found for state {}, key {}", stateId, key);
    }

    const auto &prob = it->second;
    if (prob.size() == 1)
        return prob.front().first;
    // choose a state probabilistically
    double p_sum = 0;
    for (const auto &item : prob) p_sum+=item.second;

    double p = nrandom(0, p_sum);
    p_sum = 0.;

    for (const auto &item : prob) {
        p_sum+=item.second;
        if (p < p_sum)
            return item.first;
    }
    throw logic_error_fmt("TransitionMatrix: no valid target found for state {}, key {}", stateId, key);
}
