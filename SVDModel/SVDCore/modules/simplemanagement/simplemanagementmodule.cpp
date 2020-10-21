#include "simplemanagementmodule.h"

#include "model.h"
#include "environmentcell.h"
#include "tools.h"
#include "filereader.h"

SimpleManagementModule::SimpleManagementModule(std::string module_name): Module(module_name, State::None)
{

}

void SimpleManagementModule::setup()
{
    lg = spdlog::get("setup");
    lg->info("Setup of SimpleManagementModule '{}'", name());
    auto settings = Model::instance()->settings();
    settings.requiredKeys("modules." + name(), {"activityFile"});


    // setup of the fire grid (values per cell)
    auto grid = Model::instance()->landscape()->grid();
    mGrid.setup(grid.metricRect(), grid.cellsize());

    lg->debug("Created mgmt grid {} x {} cells.", mGrid.sizeX(), mGrid.sizeY());

    int iRegime = EnvironmentCell::indexOf("regime");
    int iInitAge = EnvironmentCell::indexOf("initialStandAge");
    if (iRegime<0 || iInitAge<0)
        throw std::logic_error("SimpleManagementModule: values 'regime' and 'initialStandAge' are required environment variables");

    auto env_grid = Model::instance()->landscape()->environment().begin();
    for (auto *p = mGrid.begin(); p!=mGrid.end(); ++p, ++env_grid) {
        if (*env_grid) {
            p->age = static_cast<short int>((*env_grid)->value(static_cast<size_t>(iInitAge)));
            p->regime = static_cast<short int>((*env_grid)->value(static_cast<size_t>(iRegime)));
        }
    }


    std::string filename = Tools::path(settings.valueString("modules." + name() + ".activityFile"));
    FileReader rdr(filename);
    rdr.requiredColumns({"regime", "activityId", "age"});
    size_t iregime=rdr.columnIndex("regime"), iact=rdr.columnIndex("activityId"), iAge=rdr.columnIndex("age");
    // pass 1: find max values for age and regime
    size_t max_regime=0, max_age=0;
    while (rdr.next()) {
        if (rdr.value(iregime)<0.)
            throw logic_error_fmt("{} is an invalid management regime! (Valid are only values >=0)", rdr.value(iregime));
        if (rdr.value(iAge)<0.)
            throw logic_error_fmt("{} is an invalid management age! (Valid are only values >=0)", rdr.value(iAge));
        max_regime = std::max(max_regime, static_cast<size_t>(rdr.value(iregime)));
        max_age = std::max(max_age, static_cast<size_t>(rdr.value(iAge)));
    }
    mActivities.resize(max_regime+1);
    mMaxAge.resize(max_regime+1);
    for (auto &l : mActivities)
        l.resize(max_age+1, std::pair<float,float>(0.f, -1.f/10.f)); // default: state: 0: no activity, -1: nothing in the next 10yrs

    rdr.reset();
    while (rdr.next()) {
        float act = static_cast<float>(rdr.value(iact))/10.f;
        size_t regime = static_cast<size_t>(rdr.value(iregime));
        int age = static_cast<int>(rdr.value(iAge));

        auto &l = mActivities[regime];
        mMaxAge[ regime ] = std::max( mMaxAge[regime], age);
        for (int i=std::max(0, age-10); i<=age; ++i) {
            size_t il=static_cast<size_t>(i);
            if (l[il].first == 0) {
                // do not overwrite already loaded activities
                l[il].first = act;
                l[il].second = static_cast<float>(age - i) / 10.f; // count down: 10, 9, 8, .. to 0 for the year of execution
            }
        }
    }
    if (lg->should_log(spdlog::level::trace)) {
        lg->trace("** SimpleManagement Activities ***");
        for (size_t i=0;i<mActivities.size();++i) {
            const auto &l = mActivities[i];
            lg->trace("* Regime: {}, maximum age: {} *", i, mMaxAge[i]);
            std::string act, tm;
            for (const auto &m : l) {
                act += " " + to_string(m.first);
                tm += " " + to_string(m.second);
            }
            lg->trace("Act:  {}", act);
            lg->trace("Time: {}", tm);
        }
    }

    //lg->debug("Loaded {} management regime items from '{}'", mIgnitions.size(), filename);


    lg->info("Setup of SimpleManagementModule '{}' complete.", name());

    lg = spdlog::get("modules");


}

std::vector<std::pair<std::string, std::string> > SimpleManagementModule::moduleVariableNames() const
{
    return { {"age", "current stand age"},
        {"regime", "curent management regime (0: no mgmt)"},
        {"activity", "current activity (scheduled for the next 10 years)"},
        {"activityIn", "number of years in the future an activity should happen"}};

}

double SimpleManagementModule::moduleVariable(const Cell *cell, size_t variableIndex) const
{
    auto &gr = mGrid[cell->cellIndex()];
    switch (variableIndex) {
    case 0: return static_cast<double>(gr.age);
    case 1: return static_cast<double>(gr.regime);
    case 2: { float a, t;
              managementActivity(cell, a, t);
              return a*10.f;
    }
    case 3: { float a, t;
              managementActivity(cell, a, t);
              return t*10.f;
    }

    default: return 0.;
    }

}

void SimpleManagementModule::run()
{
    // increment age info by 1
    for (auto *p = mGrid.begin(); p!=mGrid.end(); ++p) {
        //p->age++;
        p->age = p->age < mMaxAge[static_cast<size_t>(p->regime)] ? p->age+1 : 0;
    }
}

void SimpleManagementModule::managementActivity(const Cell *cell, float &rActivity, float &rTime) const
{
    const auto &mgmt = mGrid[cell->cellIndex()];
    if (mgmt.regime<0 || mgmt.age<0) {
        rActivity = 0.f;
        rTime = -0.1f; // -1/10
        return;
    }

    const auto &res = mActivities[ static_cast<size_t>(mgmt.regime) ][static_cast<size_t>(mgmt.age)];

    rActivity = res.first;
    rTime = res.second;
}


