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

#include "expressionwrapper.h"
#include "expression.h"

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
    auto iexpr = rdr.columnIndex("expression");
    int n=0;
    while (rdr.next()) {
        // read line
        state_t id = state_t( rdr.value(is) );
        int key = int( rdr.value(ik) );
        state_t target = state_t( rdr.value(it) );
        double p =  rdr.value(ip);
        if (p<0. || p>1.)
            throw logic_error_fmt("TransitionMatrix: invalid probability {}. Allowed is the range 0..1", p);

        auto &e = mTM[ {id, key} ];
        e.push_back(STransitionItem(target, p));
        if (iexpr != std::numeric_limits<std::size_t>::max() )
            if (!rdr.valueString(iexpr).empty()) {
                //CellWrapper wrap(nullptr);
                e.back().expr = std::unique_ptr<Expression>(new Expression(rdr.valueString(iexpr)));
                //e.back().expr->parse(&wrap); // parse immediately
            }
        ++n;
    }

    // TODO: check transition matrix: states have to be valid, p should sum up to 1

    spdlog::get("setup")->debug("Loaded transition matrix for {} states from file '{}' (processed {} records).", mTM.size(), filename, n);
    return true;
}

state_t TransitionMatrix::transition(state_t stateId, int key, CellWrapper *cell)
{
    auto it = mTM.find({stateId, key});
    if (it == mTM.end()) {
        throw logic_error_fmt("TransitionMatrix: no valid transitions found for state {}, key {}", stateId, key);
    }

    const auto &prob = it->second;
    if (prob.size() == 1)
        return prob.front().state;
    // choose a state probabilistically
    bool has_expr = false;
    double p_sum = 0.;
    for (const auto &item : prob) {
        p_sum+=item.prob;
        if (item.expr) {
            has_expr=true;
            if (!cell)
                logic_error_fmt("TransitionMatrix: a transition with an expression: {}, state {} key {} is used, but there is no valid cell.", item.expr->expression(), stateId, key);

        }
    }

    // special case:
    if (has_expr) {
        // we need to run the expressions and store their result
        std::vector<double> ps(prob.size());
        double *cp = &ps.front();
        p_sum = 0.;
        for (const auto &item : prob) {
            *cp = item.prob;
            if (item.expr) {
                *cp *= std::max( item.expr->calculate(*cell), 0.); // multiply base probability with result of the expression, do not allow negative probability
            }
            p_sum+= *cp++;
        }
        double p = nrandom(0, p_sum);
        p_sum = 0.;
        for (size_t i=0;i<ps.size();++i) {
            p_sum += ps[i];
            if (p < p_sum)
                return prob[i].state;
        }

    }

    double p = nrandom(0, p_sum);
    p_sum = 0.;

    for (const auto &item : prob) {
        p_sum+=item.prob;
        if (p < p_sum)
            return item.state;
    }
    throw logic_error_fmt("TransitionMatrix: no valid target found for state {}, key {}", stateId, key);
}
