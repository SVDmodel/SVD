/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#include "filereader.h"
#include "strtools.h" // for lowercase functions

#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <algorithm>



// helper: extract tokens from string 'str' and fill a vector with them.
// source: http://stackoverflow.com/questions/236129/c-how-to-split-a-string
void tokenize(const std::string& str,
                      std::vector<std::string>& tokens,
                      const char delimiters = ' ')
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}


FileReader::~FileReader()
{
   clear();
}

void FileReader::clear()
{
    mFields.clear();
    mValues.clear();
    if (mInStream.is_open())
         mInStream.close();
    mInStream.clear();  // clear any flags
    mHasSections = false;
    mCurrentSection = "";
    mCaseSensitive = true;

}

void FileReader::setCaseSensitive(const bool sensitive)
{
    mCaseSensitive = sensitive;
    if (!sensitive)
        for (size_t i=0;i<mFields.size(); i++)
            mFields[i]=lowercase(mFields[i]);

}

void FileReader::loadFile(const std::string &fileName)
{
    clear();
    mFileName = fileName;
    mInStream.open(fileName.c_str(), std::ifstream::in);
    if (!mInStream.is_open())
        throw std::logic_error("FileReader:: cannot open file: " + fileName);
    // read first line...
    while (!mInStream.eof()) {
       mInStream.getline(mBuffer, FRBUFSIZE);
       if (mBuffer[0]!='#' && *mBuffer!='\0')    // skip comment lines
           break;
    }
    if (mInStream.eof())
        throw std::logic_error("FileReader:: file contains no data: " + fileName);

    mHasSections = true; // to trigger testing of the first line
    mHasSections = scanSection();

    readHeader();

}

bool FileReader::scanSection()
{
    if (mHasSections && mBuffer[0]=='[') {
        // section found...
        mCurrentSection = trimmed( std::string(mBuffer) );
        mCurrentSection = mCurrentSection.substr(1, mCurrentSection.size()-2); // remove brackets
        // read next line of data
        mInStream.getline(mBuffer, FRBUFSIZE);
        return true;
    }
    return false;
}

bool FileReader::nextSection()
{
    if (!mHasSections)
        return false;
    // skip empty and comment lines
    while (!mInStream.eof()) {
       mInStream.getline(mBuffer, FRBUFSIZE);
       if (mBuffer[0]!='#' && mBuffer[0]!='\0')    // skip comment and empty lines lines
           break;
    }
    mCurrentSection = "";
    if (mInStream.eof())
        return false;
    if (!scanSection())
        return false;
    readHeader();

    return true;
}

void FileReader::readHeader()
{
    // determine the used delimiter
    size_t ctab = count_occ(mBuffer,'\t');
    size_t csemi = count_occ(mBuffer,';');
    size_t ccol = count_occ(mBuffer,',');
    size_t cspc = count_occ(mBuffer,' ');
    size_t maxc = std::max( std::max(ctab, csemi), std::max(ccol, cspc) );
    if (maxc==0)
        throw std::logic_error("FileReader:: cannot determine delimiter in " + mFileName);
    if (ctab == maxc) mDelimiter = '\t';
    if (csemi == maxc) mDelimiter = ';';
    if (ccol == maxc) mDelimiter = ',';
    if (cspc == maxc) mDelimiter = ' ';

    // read headers
    mFields.clear();
    tokenize(std::string(mBuffer), mFields, mDelimiter );

    mColCount = mFields.size();
    for (size_t i=0;i<mFields.size(); i++) {
       mValues.push_back(0.);
       replace_string(mFields[i], "\"", ""); // drop quotes
    }

    // Note: in gcc calling tellg() seems to invalidate the stream loading process (i.e. the next loaded line is truncated).
    // in bcb this works ok.
    // so for now deactivated (30.05.2011)
    // mDataStart = mInStream.tellg();

}

// count how often 'c' is in the string 's'
size_t FileReader::count_occ(const char* s, char c)
{
  size_t n = 0;
  while(*s) {
    if(*s++ == c)
    n++;
  }
  return n;
}

size_t FileReader::indexOf(const std::string &columnName)
{
    std::vector<std::string>::iterator it;
    if (!mCaseSensitive)
        it = std::find(mFields.begin(), mFields.end(), lowercase(columnName));
    else
        it = std::find(mFields.begin(), mFields.end(),columnName);

    if (it==mFields.end())
        throw std::logic_error("FileReader:: invalid column:" + columnName + "\nin:" + mFileName);
    return (it - mFields.begin());
}

bool FileReader::requiredColumns(const std::vector<std::string> &cols)
{
    std::string msg;
    for (auto s : cols)
        if (!contains(mFields, s))
            msg += s + ", ";
    if (msg.size()==0)
        return true;
    throw std::logic_error("Required column(s) not in File '" + mFileName + "': " + msg + " (required are: " +  join(cols) + ")");
}

/// the same as indexOf but does not throw an error
size_t FileReader::columnIndex(const char *columnName)
{
    std::vector<std::string>::iterator it;
    if (!mCaseSensitive)
        it = std::find(mFields.begin(), mFields.end(),lowercase(columnName));
    else
        it = std::find(mFields.begin(), mFields.end(),std::string(columnName));
    if (it==mFields.end())
        return std::string::npos; // =-1
    return (it - mFields.begin());
}
void FileReader::first()
{
    if (mInStream.is_open()) {
        mInStream.clear();
        mInStream.seekg(mDataStart);
  }
}

// read the next line
bool FileReader::next()
{
    if (eof())
        return false;

    size_t line_len;
    while (!eof()) {
        mInStream.getline(mBuffer, FRBUFSIZE);
        // skip empty lines (unless we are in section mode - then a empty line signals end of section)
        if (((line_len=strlen(mBuffer)) > 0) || mHasSections)   // skip empty lines
           break;
    }
    if (line_len==0)
       return false; // last line was empty line

    char dsp[2]="\0";
    dsp[0]=mDelimiter;

    // parse....
    char *p = mBuffer;
    while (*p && (*p==mDelimiter || *p==' ') ) p++; // skip delimeters
    for (int i=0;i<mColCount;i++) {
        if (*p==mDelimiter)
            mValues[i] = 0.;
        else
            mValues[i] = atof(p);
        while (!strchr(dsp,*p)) p++; // skip non-delimiter (=data)
        if (*p==mDelimiter) p++; // skip delimiter
        //while (*p && (*p==mDelimiter || *p==' ')) p++;
        while (*p && *p==' ') p++; // skip also spaces
        if (!p) {
            throw std::logic_error("FileReader:: not enough values.\nError at line:" + std::string(mBuffer) + "\nin:" + mFileName);
        }
    }
    return true;
}


/// read the column at 'columnIndex' and return the content as a string....
std::string FileReader::valueString(const size_t columnIndex)
{
    // we assume, the current line is in "mBuffer"
    // so seek for the n-th delimiter
    int dfound = 0;
    char *p = mBuffer;
    char *ps;
    while(*p) {
        if (*p==mDelimiter) {
            dfound++;
        }
        if (dfound == columnIndex) {
            if (*p==mDelimiter)
                ++p; // skip delimiter
            ps = p;  // start, then search for the end-delimiter of the field:

            while (*p && *p!=mDelimiter) p++; // search again for delimiter
            if (!*p){
                // reached the end of the string
                if (columnIndex<mColCount-1)
                    throw std::logic_error("FileReader::valueString not enough values.\nError at line:" + std::string(mBuffer) + "\nin:" + mFileName);
                std::string s(ps);
                return s;
            }
            std::string s(ps, p-ps);  // copy only content, subtract delimiter
            return s;
        }
        ++p;
    }
    throw std::logic_error("FileReader::valueString not enough values.\nError at line:" + std::string(mBuffer) + "\nin:" + mFileName);
//    char dsp[2]="\0";
//    dsp[0]=mDelimiter;
//    while (dfound<=columnIndex) {
//      ps = p; // start, then search for the end-delimiter of the field:
//      while (!strchr(dsp,*p)) p++;
//      if (!*p) {
//          // reached the end of the string
//          if (columnIndex<mColCount-1)
//              throw std::logic_error("FileReader::valueString not enough values.\nError at line:" + std::string(mBuffer) + "\nin:" + mFileName);
//          std::string s(ps);
//          return s;
//      }
//      p++; // skip delimiter
//      dfound++;
//    }
//    std::string s(ps, p-ps-1);  // copy only content, subtract delimiter
    //    return s;
}


