//---------------------------------------------------------------------------

#ifndef FileReaderH
#define FileReaderH
/*  FileReader reads potentially large files as a stream, line by line
    it automatically parses the first line and detects appropriate column delimiters.
    Possible delimiters are: tab, ';', ',' and (multiple) space
Usage example:
...
FileReader reader(file_name);
int iy = reader.indexOf("year");

while (!reader.next()) {
   reader.value("year"); // access by column name
   reader.value(iy); // double
   reader.valueString(iy); // result as std::string  :: note implemented yet ;)
}

*/
#include <string>
#include <vector>
#include <fstream>
#include <cassert>


#define FRBUFSIZE 10000
class FileReader
{
public:
   // maintenance
   FileReader() { }
   ~FileReader();
    ///< create and open file
   FileReader(const std::string &fileName) { clear(); loadFile(fileName); }
   /// open file and auto-parse
   void loadFile(const std::string &fileName);
   /// enable check for sections. Needs to be done prior calling 'loadFile()'
   /// a section is a block of data headed by a tag in square brackets (like ini-files)
   bool hasSections() const { return mHasSections; }
   void doScanSections() { mHasSections = true; }
   void setCaseSensitive(const bool sensitive);

   bool nextSection(); ///< switch to next section (return false if last section reached)
   const std::string &section() { return mCurrentSection; }

   // access
   /// true, if end of file reached
   bool eof() { return mInStream.eof();}
   bool next(); ///< go to next line, return "false" if end of file reached
   void first(); ///< reset to first line
   /// number of columns
   int columnCount() {return mColCount; }
   const std::string &columnName(const int columnIndex) {assert(columnIndex<mColCount); return mFields[columnIndex];}
   /// retrieve the index of a given column or -1 if the column is not found.
   /// @sa indexOf
   int columnIndex(const char *columnName);
   const char *currentLine() {return mBuffer; }
   double value(const int columnIndex) { assert(columnIndex<mColCount); return mValues[columnIndex]; }
   double value(const char *columnName) { return value(indexOf(columnName)); }
   std::string valueString(const int columnIndex);
   std::string valueString(const char* columnName) { return valueString(indexOf(columnName)); }
   /// retrieve the index of a column name.
   /// the functions throws an error if the column is not present.
   /// @sa colummnIndex
   int indexOf(const char *columnName);

   /// check if *all* columns provided in 'cols' are in the file.
   /// throws an exception if not.
   bool requiredColumns(const std::vector<std::string> &cols);
private:
   void readHeader(); ///< scan the headers
   bool scanSection();
   char mBuffer[FRBUFSIZE];
   std::string mFileName;
   std::streampos mDataStart;
   void clear();
   std::vector<std::string> mFields;
   bool mHasSections;
   std::string mCurrentSection;
   std::vector<double> mValues;
   std::ifstream mInStream;
   size_t count_occ(const char* s, char c);
   char mDelimiter;
   int mColCount; // number of columns
   bool mCaseSensitive;
};

//---------------------------------------------------------------------------
#endif
