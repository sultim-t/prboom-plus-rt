// ===================================================================
// AdvXMLParser
//
// See AdvXMLParser.h
// ===================================================================

#include "AdvXMLParser.h"

// Some debugging stuf (only for Win32)
#if defined(_DEBUG) & defined(DEBUG_NEW)
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// Our namespace
namespace AdvXMLParser
{

// ===================================================================

const Char* s_szMessages[] =
{
	/*ERROR_NONE*/                          "No error",
	/*ERROR_END_OF_DOC*/                    "Unexpected end of document",
	/*ERROR_INVALID_CHAR*/                  "Unlegal character",
	/*ERROR_NO_EQUAL*/                      "There is no equal sign",
	/*ERROR_INVALID_ATTRIBUTE_VALUE*/       "Invalid value of the attribute",
	/*ERROR_UNEXPECTED_TAG*/                "Unexpected tag",
	/*ERROR_INVALID_REFERENCE*/             "Invalid reference",
	/*ERROR_INVALID_HEX_NUMBER*/            "Invalid hexadecimal number",
	/*ERROR_INVALID_NUMBER*/                "Invalid decimal number",
	/*ERROR_UNKNOWN_REFERENCE*/             "Unknown entity reference",
	/*ERROR_INVALID_TAG_NAME*/              "Invalid name of tag",
	/*ERROR_INVALID_EMPTY_TAG*/             "Invalid empty tag",
	/*ERROR_INVALID_TAG_END*/               "Invalid End tag", 
	/*ERROR_TAG_NAME_MISMATCH*/             "The end tag has not the same name as the start tag",
	/*ERROR_INVALID_MARKUP*/                "Invalid or unknown markup",
	/*ERROR_INVALID_CDATA_CHARS*/           "']]>' is not allowed here",
	/*ERROR_NO_ROOT*/                       "The document has no root (element)",
	/*ERROR_INVALID_VERSION_INFO*/          "Invalid XML declaration (version)",   
	/*ERROR_INVALID_DECLARATION_END*/       "Invalid end of XML declaration",
	/*ERROR_INVALID_ENCODING_DECLARATION*/  "Invalid encoding declaration",
	/*ERROR_INVALID_ENCODING_NAME*/         "Invalid name of encoding",
	/*ERROR_INVALID_STANDALONE_DECLARATION*/"Invalid standalone declaration",
	/*ERROR_INVALID_STANDALONE_VALUE*/      "Invalid standalone value. Must be 'yes' or 'no'",
	/*ERROR_INVALID_COMMENT_CHAR*/          "'--' is not allowed in the content of a comment",
	/*ERROR_INVALID_PI_TARGET*/             "Invalid Processing instruction target name",
	/*ERROR_RESERVED_PI_TARGET*/            "Processing instruction target names may not be 'XML' in any combination of cases",
	/*ERROR_INVALID_DOCTYPE*/               "Error in DOCTYPE declaration",
	/*ERROR_INVALID_PE_REFERENCE*/          "Invalid parameter entity",
	/*ERROR_INVALID_PUBID_LITERAL*/         "Invalid character in PUBLIC literal",
	/*ERROR_INVALID_ELEMENT_DECL*/          "Invalid ELEMENT declaration",
	/*ERROR_CHOICE_EXPECTED*/               "'|' is expected here",
    /*ERROR_SEQ_EXPECTED*/                  "',' is expected here",
	/*ERROR_INVALID_CP*/                    "Invalid content particle",
	/*ERROR_INVALID_ATTLIST_DECL*/          "Invalid ATTLIST declaration",
	/*ERROR_INVALID_ATTDEF*/                "Invalid definition of attribute",
	/*ERROR_INVALID_NOTATION_TYPE*/         "Invalid NOTATION type",
	/*ERROR_INVALID_ENUMERATION*/           "Invalid enumeration",
	/*ERROR_INVALID_DEFAULT_DECL*/          "Invalid #FIXED declaration",
	/*ERROR_INVALID_ENTITY_DECL*/           "Invalid ENTITY declatation",
	/*ERROR_INVALID_NDATA_DECL*/            "Invalid NDATA declaration",
	/*ERROR_INVALID_NOTATION_DECL*/         "Invalid NOTATION declaration",
	/*ERROR_INVALID_EXTERNAL_ID*/           "Invalid SYSTEM literal",
	/*ERROR_INVALID_PUBLIC_ID*/             "Invalid PUBLIC literal",
	/*ERROR_DATA_AT_END*/                   "Unexpected data after the root element"
};

// ===================================================================

const Char* ParsingException::GetErrorMessage() const
{
    ASSERT(elemof(s_szMessages) == ERROR_LAST);
    ASSERT(m_nError > 0 && m_nError < elemof(s_szMessages));
    return(s_szMessages[m_nError]);
}

// ===================================================================

}

