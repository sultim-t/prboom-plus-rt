// ===================================================================
// AdvXMLParser
//
// See AdvXMLParser.h
// ===================================================================

#include "AdvXMLParser.h"

#if defined(_DEBUG) & defined(DEBUG_NEW)
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// I prefer to use a namespace instead of "static" because the C++ standard
// says that "static" is deprecated.
namespace AdvXMLParser { namespace Private
{

// ===================================================================

// White space: space, tabulation, carriage return, line feed.
static const Char s_spaces[] = TEXT(" \t\r\n");

void TrimSpaces(String& strText)
// Remove leading and trailing spaces 
{
    // Seach spaces at the beginning
    String::size_type nFirst = strText.find_first_not_of(s_spaces);
    if(nFirst == String::npos)
    {
        // Only spaces. Empty the string
        strText.erase();
        return;
    }

    if(nFirst != 0) // Some spaces at the beginning. Remove them
        strText.erase(0, nFirst);

    // Search spaces at the end
    String::size_type nLast = strText.find_last_not_of(s_spaces);
    if(nLast != String::npos) // Some spaces at the end. Remove them
        strText.erase(nLast + 1);
}


void NormalizeWhiteSpaces(String& strText) // static
// Replace white spaces by only one space
{
    String::size_type nPosition = 0;
    while(nPosition < strText.size())
    {
        String::size_type nFirst = strText.find_first_of(s_spaces, nPosition);
        if(nFirst == String::npos)
            break; // No space, so nothing to normalize

        String::size_type nEnd = strText.find_first_not_of(s_spaces, nFirst);
        if(nEnd == String::npos)
        {
            // Only spaces. Replace by only one space
            strText.replace(nFirst, strText.size() - nFirst, 1, TEXT(' '));
            break;
        }

        // Replace by only one space
        strText.replace(nFirst, nEnd - nFirst, 1, TEXT(' '));
        // New position: just after the space
        nPosition = nFirst + 1;
    }
}

void Normalize(String& strText) // static
{
    NormalizeWhiteSpaces(strText);
    TrimSpaces(strText);
}

// ===================================================================

} }


