#ifndef strtoolsH
#define strtoolsH

#include <sstream>
#include <string>
#include <vector>
#include <map>
// convert a value to a string:
template<class T> std::string to_string(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// to-lower/ to-upper function.
// functions modify 'str'!!!
void toLowercase(std::string &str);
void toUppercase(std::string &str);

// return a lowercase/uppercase copy of the string
std::string lowercase(const std::string &str);
std::string uppercase(const std::string &str);


// trimming: modify the string and return referenced
std::string &trimLeft(std::string &str);
std::string &trimRight(std::string &str);

// trim left and right and return new string
std::string trimmed(const std::string &str);

// replace every occurence of 'oldStr' with 'newStr' in str. 'str' will be modified.
void replace_string(std::string& str, const std::string& oldStr, const std::string& newStr);

// split a string (using 'delim' and put into "elems"
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
// split a string and return a new vector
std::vector<std::string> split(const std::string &s, char delim);

// split a string and return a new vector, each element is trimmed
std::vector<std::string> split_and_trim(const std::string &s, char delim);


// split a string into substrings and return as a vector. Other than split(),
// tokenize recognizes and handles quote-characters (")
std::vector<std::string> tokenize(const std::string& str, const char delimiters = ' ');

// join the elements of "vec" together
std::string join(const std::vector<std::string> &vec, const std::string &delim=", ");

template <typename Iter>
std::string join(Iter it, Iter end, const std::string &delim=", ", int print_max=1000 ) {
    std::string result;

    for (int n=0; it!=end; ++it,++n) {
        if (n>print_max) {
            result+=" <truncated>...";
            return result;
        }
        if (n>0) result+=delim;
        result+= std::to_string( *it );
    }
    return result;
}


// return true if fullString ends with 'ending'
bool has_ending(std::string const &fullString, std::string const &ending);

// replace every occurence of "searchString" in "str" with "replaceString"
void find_and_replace(std::string &str, const std::string &searchString, const std::string &replaceString);

// split a string into several name/value pairs.
// each line of the source is one pair (you can specify different delimiters); name and value are separated by "="
std::map<std::string, std::string> split_name_value_pairs(const std::string source, const char delimiter='\n');

// look for 'search' in the vector of type T 'v'.
// return -1 if not in list, or the index of 'search'
template <typename T> int index_of(const std::vector<T> &v, const T& search)
{
    int index = 0;
    for (typename std::vector<T>::const_iterator it=v.begin(); it!=v.end(); ++it, ++index)
       if ( *it == search)
           return index;
    // nothing found
    return -1;
}

// read and write a text file
// read a text file and return a list of strings
std::vector<std::string> readFile(const std::string &file_name);
std::string readFileStr(const std::string &file_name);

// write a string or a string list to a file
bool writeFile(const std::string &file_name, const std::vector<std::string> &lines);
bool writeFile(const std::string &file_name, const std::string &content);


// delete all elements of a vector by calling the destructor, then
// clear the vector
template <typename T> void delete_and_clear(std::vector<T*> &v)
{
    for (typename std::vector<T*>::const_iterator it=v.begin(); it!=v.end(); ++it)
       delete (*it);
    v.clear();
}
 

template <class Container>
bool contains(Container& container, const typename Container::value_type& element)
{
    return std::find(container.begin(), container.end(), element) != container.end();
}

/// returns the index of 'element' in the container 'container'. If 'element' is not in the container
/// the function returns -1.
template <class Container>
int indexOf(Container& container, const typename Container::value_type& element)
{
    auto it = std::find(container.begin(), container.end(), element);
    if (it == container.end())
        return -1;
    return static_cast<int>(std::distance(container.begin(), it));
}


/// splits the full path 'fileName' into a pair with the path as first and the filename as second.
std::pair<std::string, std::string> splitPath (const std::string& fileName);

/// find all keys in a map (mapOfElem) that have the given value; return a vector of keys
// https://thispointer.com/how-to-search-by-value-in-a-map-c/
template<typename K, typename V>
std::vector<K> findByValue( std::map<K, V> mapOfElemen, V value)
{
    std::vector<K> vec;
    auto it = mapOfElemen.begin();
    // Iterate through the map
    while(it != mapOfElemen.end())
    {
        // Check if value of this entry matches with given value
        if(it->second == value)
        {
            // Push the key in given map
            vec.push_back(it->first);
        }
        // Go to next entry in map
        it++;
    }
    return vec;
}


#endif
