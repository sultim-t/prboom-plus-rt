// ===================================================================
// AdvXMLParser
// ===================================================================

// The AdvXMLParser can be customized by some parameters (macros).
// For example, you can choose if you want to include the code that
// generates XML.
// Mofify this file to change the behaviour of the Parser.

// ===================================================================
// This version accepts the following parameters (macros):
//
// ADVXMLPARSER_STD_NAMESPACE
//  Name of the STL namespace (by def. std)
//
// ADVXMLPARSER_NO_CONVERSION
//  Define it to remove the operators "<<" with int, ...
//  Usefull if you want to define your own implementation
//
// ADVXMLPARSER_NO_WRITE
//  Remove all the functions that generate XML or create / alter objects.
//
// ADVXMLPARSER_DTD
//  Level of DTD (Document Type Definition) support.
//  - 0 : (default) Don't parse DTD at all. Error if there is one.
//  - 1 : Skip DTD without trying to parse it
//  - 2 : Parse DTD but don't validate the document
//  - 3 : Parse DTD and validate the document (NOT YET IMPLEMENTED)
// ===================================================================

#define ADVXMLPARSER_NO_WRITE
#define ADVXMLPARSER_STD_NAMESPACE std

// ===================================================================
// Compiler stuff
// ===================================================================
// Visual C++ is not able to use symbols longer than 255 characters and
// generates a lot of warnings. Since templates generate very long symbols,
// and we can't do anything against this, disable these warnings. 
// See Visual C++ documentation about this limitation (search C4786)
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

// ===================================================================
// Default value for parameters (macros)
// ===================================================================

#ifndef ADVXMLPARSER_DTD
// Parse DTD but don't validate the document
#define ADVXMLPARSER_DTD 2
#endif

// ===================================================================
// Which namespace to use for STL
// (reduce conflict when there are several STL in the path)
// For example, use stlport for STLport

// If ADVXMLPARSER_STD_NAMESPACE is defined use it
#ifndef ADVXMLPARSER_STD_NAMESPACE
// STLport
#ifdef __STLPORT_STD
#define ADVXMLPARSER_STD_NAMESPACE __STLPORT_STD
#else
// If nothing is defined, use "std".
#define ADVXMLPARSER_STD_NAMESPACE std
#endif // __STLPORT_STD
#endif // ADVXMLPARSER_STD_NAMESPACE

// An alias shorter than ADVXMLPARSER_STD_NAMESPACE
#define advstd ADVXMLPARSER_STD_NAMESPACE

// ===================================================================
// String and character types
// (to compile either as ANSI or UNICODE)
// ===================================================================

#if defined(UNICODE) || defined(_UNICODE)

// Character type used
typedef wchar Char;

// L for Unicode literals
#ifndef TEXT
#define TEXT(str) L##str
#endif

#else // non Unicode (ANSI)

// Character type used
typedef char Char;

// Permit to use Unicode (L"..") later
#ifndef TEXT
#define TEXT(str) str
#endif

#endif // UNICODE || _UNICODE

// ===================================================================
