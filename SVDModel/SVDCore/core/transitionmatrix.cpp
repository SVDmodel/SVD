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
    const auto &prob = mTM.at({stateId, key});
    if (prob.size() == 1)
        return prob.front().first;
    // choose a state probabilistically
    double p_sum = 0;
    for (const auto &item : prob) p_sum+=item.second;

    double p = nrandom(0, p);
    p_sum = 0.;

    for (const auto &item : prob) {
        p_sum+=item.second;
        if (p < p_sum)
            return item.first;
    }
    spdlog::get("main")->error("TransitionMatrix: no valid target found for state {}, key {}", stateId, key);
    return stateId;
}
