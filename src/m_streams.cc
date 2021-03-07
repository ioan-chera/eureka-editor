#include "m_streams.h"
#include "m_strings.h"

//
// this automatically strips CR/LF from the line.
// returns true if ok, false on EOF or error.
//
bool M_ReadTextLine(SString &string, std::istream &is)
{
    string.clear();
    std::getline(is, string.get());
    if(is.bad() || is.fail())
        return false;
    if(is.eof() && string.empty())
        return false;
    if(string.substr(0, 3) == "\xef\xbb\xbf")
        string.erase(0, 3);
    string.trimTrailingSpaces();
    return true;
}
