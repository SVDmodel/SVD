
#include "strtools.h"
#include <fstream>
#include <algorithm>

// to-lower/ to-upper function
void toLowercase(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) ::tolower);
}

void toUppercase(std::string &str)
{
   std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) ::toupper);
}

std::string lowercase(const std::string &str)
{
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), (int(*)(int)) ::tolower);
    return s;
}

std::string uppercase(const std::string &str)
{
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), (int(*)(int)) ::toupper);
    return s;
}

std::string &trimLeft(std::string &str)
{
  std::string::iterator i;
  for (i = str.begin(); i != str.end(); i++) {
      if (!::isspace(*i)) {
          break;
      }
  }
  if (i == str.end()) {
      str.clear();
  } else {
      str.erase(str.begin(), i);
  }
  return str;
}

std::string &trimRight(std::string &str)
{
  std::string::iterator i;
  if (str.empty()) return str;
  for (i = str.end() - 1; ;i--) {
      if (!::isspace(*i)) {
          str.erase(i + 1, str.end());
          break;
      }
      if (i == str.begin()) {
          str.clear();
          break;
      }
  }
  return str;
}

std::string trimmed(const std::string &str)
{
   std::string result = str;
   trimLeft(result);
   trimRight(result);
   return result;
}

void replace_string(std::string& str, const std::string& oldStr, const std::string& newStr)
{
  size_t pos = 0;
  while((pos = str.find(oldStr, pos)) != std::string::npos)
  {
     str.replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
}

// http://stackoverflow.com/questions/236129/how-to-split-a-string

// split a string (using 'delim' and put into "elems"
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

// split a string and return a new vector
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

// split a string and return a new vector
std::vector<std::string> split_and_trim(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems); // split in elements
    for (auto &s : elems)
        s = trimmed(s);
    return elems;
}


std::vector<std::string> tokenize(const std::string& str, const char delimiters)
{
    std::string copy_str = str; // make a copy
    std::string token;

    bool in_quote=false;
    for (std::string::iterator i=copy_str.begin(); i!=copy_str.end(); ++i) {
         if (*i=='\"' && !(i>copy_str.begin() && *(i-1)=='\\')  ) { in_quote=!in_quote; *i=' '; }
         if (in_quote && *i==delimiters)
            *i=char(7);
    }
    std::vector<std::string> tokens;
    // Skip delimiters at beginning.
    std::string::size_type lastPos = copy_str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = copy_str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        token = copy_str.substr(lastPos, pos - lastPos);
        token = trimmed(token);
        // recover the "hidden" delimiters
        std::replace( token.begin(), token.end(), char(7), delimiters );
        replace_string(token, "\\\"", "\"" );

        tokens.push_back(token);
        // Skip delimiters.  Note the "not_of"
        lastPos = copy_str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = copy_str.find_first_of(delimiters, lastPos);
    }
    return tokens;
}
bool has_ending (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() > ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::string join(const std::vector<std::string> &vec, const std::string &delim)
{
    std::string result;
    for (size_t i=0;i<vec.size(); ++i) {
        if (i>0) result+=delim;
        result+=vec[i];
    }
    return result;
}

void find_and_replace(std::string &str, const std::string &searchString, const std::string &replaceString)
{
    std::string::size_type pos = 0;
    while ( (pos = str.find(searchString, pos)) != std::string::npos ) {
        str.replace( pos, searchString.size(), replaceString );
        pos++;
    }
}

std::map<std::string, std::string> split_name_value_pairs(const std::string source, const char delimiter)
{
    std::map<std::string, std::string> result;
    std::vector<std::string> lines = split(source, delimiter);
    for (size_t i=0;i<lines.size(); ++i) {
       size_t p = lines[i].find('=');
       if (p!=std::string::npos)
           result[lines[i].substr(0,p)] = lines[i].substr(p+1, 100);
    }
    return result;
}

/////

std::vector<std::string> readFile(const std::string &file_name)
{
    std::ifstream infile(file_name.c_str());
    if (!infile.is_open())
        throw std::logic_error(std::string("cannot open file ") + file_name);
    std::string s;
    std::vector<std::string> result;
    while (!infile.eof()) {
        std::getline(infile, s);
        result.push_back(s);
    }
    infile.close();
    return result;
}

std::string readFileStr(const std::string &file_name)
{
    std::ifstream infile(file_name.c_str());
    std::string s;

    if (!infile.is_open())
        throw std::logic_error(std::string("Failed to open text file: ") + file_name );
        
    std::vector<std::string> result;
    while (!infile.eof()) {
        std::getline(infile, s);
        result.push_back(s);
    }
    infile.close();
    return join(result, "\n");
}

// write a string or a string list to a file
bool writeFile(const std::string &file_name, const std::vector<std::string> &lines)
{
    std::ofstream outfile(file_name.c_str());
    if (!outfile.is_open())
        return false;
    for (std::vector<std::string>::const_iterator i=lines.begin(); i!=lines.end(); ++i)
        outfile << *i << std::endl;
    outfile.close();
    return true;
}

bool writeFile(const std::string &file_name, const std::string &content)
{
    std::ofstream outfile(file_name.c_str());
    if (!outfile.is_open())
        return false;
    outfile << content << std::endl;
    outfile.close();
    return true;
}

std::pair<std::string, std::string> splitPath (const std::string& fileName)
{
  std::pair<std::string, std::string> parts;
  std::size_t found = fileName.find_last_of("/\\");

  parts.first =  fileName.substr(0,found);
  parts.second = fileName.substr(found+1);
  return parts;
}
