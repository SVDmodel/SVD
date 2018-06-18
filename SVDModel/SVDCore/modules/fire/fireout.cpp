#include "fireout.h"
#include "firemodule.h"

#include "model.h"

FireOut::FireOut()
{
    setName("Fire");
    setDescription("Output on fire events (one event per line)");
    // define the columns
    columns() = {
    {"year", "simulation year of the fire event", DataType::Int},
    {"x", "x coordinate (m) of the ignition point", DataType::Double},
    {"y", "y coordinate (m) of the ignition point", DataType::Double},
    {"planned_size", "planned fire size (ha)", DataType::Double},
    {"realized_size", "realized fire size (ha)", DataType::Double},
    {"share_high_severity", "share of pixels burning with high severity (0..1)", DataType::Double}   };
}

void FireOut::setup()
{
    openOutputFile();

}

void FireOut::execute()
{
    FireModule *fire = Model::instance()->fireModule().get();
    if (!fire)
        return;
    for (auto &s : fire->mStats) {
        if (s.year == Model::instance()->year()) {
            out() << s.year << s.x << s.y << s.max_size << s.ha_burned << (s.ha_burned>0? s.ha_high_severity/static_cast<double>(s.ha_burned) : 0. );
            out().write();
        }
    }
}
