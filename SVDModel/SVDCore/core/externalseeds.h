#ifndef EXTERNALSEEDS_H
#define EXTERNALSEEDS_H

#include <map>
#include <vector>
class FileReader; // forward
template<typename T> class Grid; // forward
class ExternalSeeds
{
public:
    ExternalSeeds();
    void setup();

    const std::vector<double> &speciesShares(int type_id) const;
private:
    bool mTableMode;
    int setupFromSpeciesShares(FileReader &rdr, Grid<int> &eseed);
    int setupFromStates(FileReader &rdr, Grid<int> &eseed);
    std::map<int, std::vector<double> > mExternalSeedTypes;
};

#endif // EXTERNALSEEDS_H
