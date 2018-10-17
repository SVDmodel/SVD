#ifndef ENVIRONMENTCELL_H
#define ENVIRONMENTCELL_H

#include <vector>
#include <strtools.h>

class EnvironmentCell
{
public:
    EnvironmentCell(int id, int climate_id): mId(id), mClimateId(climate_id) {}
    int climateId() const {return mClimateId; }
    int id() const {return mId; }
    double value(const std::string &s) const { return value(static_cast<size_t>(indexOf(s))); }
    double value(const size_t var_idx) const { if (var_idx<mValues.size()) return mValues[var_idx]; throw std::logic_error("Invalid index for environment cell!");}
    /// return the index of variable 's' or -1 if invalid
    int indexOf(const std::string &s) const { return ::indexOf(mVariables, s); }

    /// set the value of a variable given by the index ('var_idx'). Throws an error of var_idx is invalid.
    void setValue(const int var_idx, double new_value);
    /// set a value of variable 'var_name' to value 'new_value'. throws an error if var_name is invalid
    void setValue(const std::string &var_name, double new_value) { setValue(indexOf(var_name), new_value);}
    /// access the list of variables change
    static std::vector<std::string> &variables()  { return mVariables; }
private:
    int mId; ///< the cell/region ID
    int mClimateId; ///< the unique ID of the climate series that represents this region
    std::vector<double> mValues; ///< further values (constants)
    static std::vector<std::string> mVariables; ///< (static) list of variable names (linked to the mValues vector)
};

#endif // ENVIRONMENTCELL_H
