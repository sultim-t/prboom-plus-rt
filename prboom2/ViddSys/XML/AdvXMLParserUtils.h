// ===================================================================
// AdvXMLParser
//
// See AdvXMLParser.h
// ===================================================================

// I prefer to use a namespace instead of "static" because the C++ standard
// says that "static" is deprecated.
namespace AdvXMLParser { namespace Private
{    
	typedef advstd::basic_string<Char> String; // String of characters

	// Normalize the given text (remove unneccessary spaces)
	void TrimSpaces(String& strText);
	void Normalize(String& strText);
	void NormalizeWhiteSpaces(String& strText);
} }


