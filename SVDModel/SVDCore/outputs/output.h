#ifndef OUTPUT_H
#define OUTPUT_H
#include <string>
#include <fstream>

class Output
{
public:
    Output();
    ~Output();
    virtual void setup();
    virtual void execute();

    const std::string &name() const {return mName; }
    bool enabled() const { return mEnabled; }
    void setEnabled(bool enable) { mEnabled = enable; }
    void flush();
protected:
    void setName(std::string name)  { mName=name; }
    void setDescription(std::string desc)  { mDescription=desc; }
    /// return the full setting name, e.g. from 'interval' to 'output.StateGrid.interval'.
    std::string key(std::string key_elem) const;
    std::fstream mFile;
private:
    std::string mName;
    std::string mDescription;
    bool mEnabled;
};

#endif // OUTPUT_H
