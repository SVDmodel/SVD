#ifndef OUTPUT_H
#define OUTPUT_H
#include <string>

class Output
{
public:
    Output();
protected:
    void setName(std::string name) const { mName=name; }
    void setDescription(std::string desc) const { mDescription=desc; }
private:
    std::string mName;
    std::string mDescription;
};

#endif // OUTPUT_H
