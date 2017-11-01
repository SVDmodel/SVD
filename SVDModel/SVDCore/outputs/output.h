#ifndef OUTPUT_H
#define OUTPUT_H
#include <string>

class Output
{
public:
    Output();
    virtual void setup();
    virtual void execute();

    const std::string &name() const {return mName; }
    bool enabled() const { return mEnabled; }
    void setEnabled(bool enable) { mEnabled = enable; }
protected:
    void setName(std::string name)  { mName=name; }
    void setDescription(std::string desc)  { mDescription=desc; }
    /// return the full setting name, e.g. from 'interval' to 'output.StateGrid.interval'.
    std::string key(std::string key_elem) const;
private:
    std::string mName;
    std::string mDescription;
    bool mEnabled;
};

#endif // OUTPUT_H
