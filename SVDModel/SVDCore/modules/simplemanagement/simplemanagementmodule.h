#ifndef SIMPLEMANAGEMENTMODULE_H
#define SIMPLEMANAGEMENTMODULE_H

#include "modules/module.h"
#include "states.h"
#include "grid.h"
#include "spdlog/spdlog.h"

/// Cell level info stored for the management module
struct SimpleMgmtItem {
    SimpleMgmtItem(): age(0), regime(0) {}
    short int age;
    short int regime;
};


struct SimpleMgmtDataItem {

};

/// The SimpleManagementModule implements a basic management regime
///
class SimpleManagementModule: public Module
{
public:
    SimpleManagementModule(std::string module_name);

    void setup();

    std::vector<std::pair<std::string, std::string> > moduleVariableNames() const;
    virtual double moduleVariable(const Cell *cell, size_t variableIndex) const;

    void run();

    // access
    void managementActivity(const Cell *cell, float &rActivity, float &rTime) const;
private:
    // logging
    std::shared_ptr<spdlog::logger> lg;
    // grid with module data
    Grid<SimpleMgmtItem> mGrid;
    // look-up-table for activities, 1st dimension: regime, 2nd: mgmt details (activity + time)
    std::vector< std::vector<std::pair<float,float> > > mActivities;
    std::vector< int > mMaxAge;


};

#endif // SIMPLEMANAGEMENTMODULE_H
