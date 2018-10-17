#ifndef OUTPUT_H
#define OUTPUT_H
#include <string>
#include <fstream>
#include <vector>
struct OutputColumn; // forward

struct outstream
{
    outstream() : os(nullptr), _first_pos(true), _lc(0), _sep(",") {}
    outstream (std::ostream& os) : os(&os), _first_pos(true), _lc(0), _sep(",") {}
    void write() { if (os) (*os) << '\n'; _first_pos=true; _lc++; if (_lc % 1000 == 0) os->flush();  } // no automatic flushing per line, do it every 1000 lines
     //outstream( outstream&& o): os(std::move(o.os)) { }
     std::ostream*   os;

     bool _first_pos; // first element?
     int _lc; // line counter
     std::string _sep; // the column separator
};

template <class T>
outstream& operator<< (outstream& con, const T& x)
{
    if (con._first_pos)  {
        (*con.os) << x;
        con._first_pos = false;
    } else {
        (*con.os)  << con._sep << x; // add extra delimeter
    }
    return con;
}

class Output
{
public:
    Output();
    virtual ~Output();
    virtual void setup();
    virtual void execute();

    const std::string &name() const {return mName; }
    const std::string &fileName() const { return mOutputFileName; }
    bool enabled() const { return mEnabled; }
    void setEnabled(bool enable) { mEnabled = enable; }
    void flush();
    /// builds a markdown compatible documentation from the output description
    std::string createDocumentation();

    // data types
    enum DataType { String, Int, Double };
    std::string dataTypeString(DataType type);



protected:
    void setName(std::string name)  { mName=name; }
    void setDescription(std::string desc)  { mDescription=desc; }
    std::vector< OutputColumn > &columns() {return mColumns; }
    /// open the output file for writing
    void openOutputFile(std::string default_key = "file", bool write_header=true);
    /// return the full setting name, e.g. from 'interval' to 'output.StateGrid.interval'.
    std::string key(std::string key_elem) const;
    /// get the internal filestream object
    std::fstream &file() { return mFile; }
    outstream &out() { return mOutStream; }
private:
    outstream mOutStream;
    std::fstream mFile;
    std::string mOutputFileName;
    std::string mName;
    std::string mDescription;
    bool mEnabled;
    std::vector< OutputColumn > mColumns;
    char mSeparator;
};

struct OutputColumn
{
    std::string columnName;
    std::string description;
    Output::DataType type;
};

#endif // OUTPUT_H
