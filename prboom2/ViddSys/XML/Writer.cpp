// ===================================================================
// AdvXMLParser
//
// See AdvXMLParser.h
// ===================================================================

#include "AdvXMLParser.h"
#include "AdvXMLParserUtils.h"

#if defined(_DEBUG) & defined(DEBUG_NEW)
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef ADVXMLPARSER_NO_WRITE

// Our namespace
namespace AdvXMLParser
{

// ===================================================================
// GenerateContext - Context of a generation of XML (see GenerateXML)
// ===================================================================

GenerateContext::GenerateContext(String& strXml, const Char* szIndentation, GENERATE nOptions)
:   m_nOptions(nOptions),
    m_szIndentation(szIndentation != NULL ? szIndentation : TEXT("  ")),
    m_strXml(strXml),
    m_nLevel(0),
    m_bEndTag(false),
    m_bNewLine(false)
{
}

GenerateContext::GenerateContext(const GenerateContext& context)
:   m_nOptions(context.m_nOptions),
    m_szIndentation(context.m_szIndentation),
    m_strXml(context.m_strXml),
    m_nLevel(context.m_nLevel),
    m_bEndTag(context.m_bEndTag),
    m_bNewLine(context.m_bNewLine)
{
}

void GenerateContext::operator=(const GenerateContext& context)
{
    m_nOptions      = context.m_nOptions;
    m_szIndentation = context.m_szIndentation;
    m_strXml        = context.m_strXml;
    m_nLevel        = context.m_nLevel;
    m_bEndTag       = context.m_bEndTag;
    m_bNewLine      = context.m_bNewLine;
}

void GenerateContext::GenerateIndentation()
{
    ASSERT((m_nOptions & GENERATE_PRESERVE) == 0);
    m_bNewLine = false;

    for(int nIndex = 0; nIndex < m_nLevel - 1; ++nIndex)
        m_strXml += m_szIndentation;
}

void GenerateContext::GenerateStartTagIndentation()
{
    ++m_nLevel;

    if(m_nOptions & GENERATE_PRESERVE)
        return;

    GenerateNewLine();
    GenerateIndentation();

    // It's not an end tag
    m_bEndTag = false;
}

void GenerateContext::GenerateEndTagIndentation()
// Generate new line and identation only if the previous tag
// was also an end tag. If the previous tag was a start tag,
// stay on the same line.
{
    ASSERT(m_nLevel > 0);

    if(m_nOptions & GENERATE_PRESERVE)
        return;

    if(m_bEndTag)
    {
        GenerateNewLine();
        GenerateIndentation();
    }

    --m_nLevel;
    m_bEndTag = true;
}

void GenerateContext::EndTag()
{
    ASSERT(m_nLevel > 0);
    --m_nLevel;
    m_bEndTag = true;
}

void GenerateContext::GenerateNewLine()
{
    if(m_nOptions & GENERATE_PRESERVE)
        return;

    // Generate a new line only if the previous generation
    // was not already a new line.

    if(!m_bNewLine)
    {
        m_strXml += TEXT('\x0A');
        m_bNewLine = true;
    }
}

// ===================================================================
// Text - Textual content
// ===================================================================

// I prefer to use a namespace instead of "static" because the C++ standard
// says that "static" is deprecated.
namespace Private
{
	// Map some characters to their "safe" equivalent
	typedef advstd::map<Char, String> MapEscapes;
	MapEscapes s_mapEscapes;

	void FillMap()
	// Initialized by MapReferenceName (first call)
	{
		s_mapEscapes[TEXT('<')]  = TEXT("lt");
		s_mapEscapes[TEXT('>')]  = TEXT("gt");
		s_mapEscapes[TEXT('&')]  = TEXT("amp");
		s_mapEscapes[TEXT('\'')] = TEXT("apos");
		s_mapEscapes[TEXT('\"')] = TEXT("quot");
	};

	void EscapeCharacters(GenerateContext& xml, const String& strText)
	// Replace some characters by their equivalent. For example: quotes, etc...
	{
		// Fill the map the first time
		if(Private::s_mapEscapes.empty())
			Private::FillMap();

		// Characters to be replaced by their equivalent
		const Char szToBeReplaced[] = TEXT("<>&\'\"");

		String::size_type nPreviousPosition = 0;
		// Search the first occurence of one of the "special" characters
		String::size_type nPosition = strText.find_first_of(szToBeReplaced);
		// Find one...
		while(nPosition != String::npos)
		{
			// Put the previous characters
			xml += strText.substr(nPreviousPosition, nPosition - nPreviousPosition);

			// Search the equivalent in the map
			Private::MapEscapes::const_iterator it = Private::s_mapEscapes.find(strText[nPosition]);
			// If this happend, szToBeReplaces and s_mapEscapes or not synchronized
			ASSERT(it != Private::s_mapEscapes.end());

			// Replace the character by it equivalent
			xml += (*it).second; // I'm not sure that it->second works with every compiler

			// Update nPreviousPosition with the new position
			nPreviousPosition = nPosition + 1;
			// Continue the search
			nPosition = strText.find_first_of(szToBeReplaced, nPreviousPosition);
		}

		// Append the remaining characters
		if(nPreviousPosition < strText.length())
			xml += strText.substr(nPreviousPosition);
	}
}

void Text::GenerateXML(GenerateContext& xml) const // virtual
// Output text after replacing some characters (quotes, ...) by the equivalent
//
// If nice XML must be generated (with indentation, etc...), normalize
// the text before generation.
{
    // Do all this stuff to avoid unneccessary copies
    String strNormalizedText;
    if(!xml.MustPreserve())
    {
        strNormalizedText = m_strText;
        Private::NormalizeWhiteSpaces(strNormalizedText);
    }
    const String& strText = xml.MustPreserve() ? m_strText : strNormalizedText;

	Private::EscapeCharacters(xml, strText);
}


// ===================================================================
// NodeContainer - Contains other nodes (Element and Attribute)
// ===================================================================

void NodeContainer::GenerateXML(GenerateContext& xml) const
{
    for(Nodes::const_iterator itChild = m_children.begin(); itChild < m_children.end(); ++itChild)
        (*itChild)->GenerateXML(xml);
}

// ===================================================================
// Attribute of Elements
// ===================================================================

void Attribute::GenerateXML(GenerateContext& xml) const // virtual
// Generate: name="value"
{
    xml += GetName();
    xml += TEXT("=\"");
    Super::GenerateXML(xml);
    xml += TEXT("\"");
}

// ===================================================================
// Character Reference
// ===================================================================

Char* Int2Chars(int nValue, Char* szBuffer, int nBufferSize)
{
    static const Char s_cDigits[] =
    {
        TEXT('0'), TEXT('1'), TEXT('2'), TEXT('3'), TEXT('4'),
        TEXT('5'), TEXT('6'), TEXT('7'), TEXT('8'), TEXT('9'),
        TEXT('A'), TEXT('B'), TEXT('C'), TEXT('D'), TEXT('E'), TEXT('F')
    };

    Char* pDigit = szBuffer + nBufferSize / sizeof(Char) - 2;
    pDigit[1] = 0;

    while(nValue > 0)
    {
        *(pDigit--) = s_cDigits[nValue % 16];
        nValue /= 16;

        ASSERT(pDigit >= szBuffer);
    }

    return(pDigit);
}

void CharRef::GenerateXML(GenerateContext& xml) const // virtual
// Generate: &#x00;
{
    xml += TEXT("&#x");

    // 10 characters is enough for 32-bits numbers (in hexadecimal)
    Char szBuffer[10];
    xml += Int2Chars(m_c, szBuffer, sizeof(szBuffer));

    xml += TEXT(';');
}

// ===================================================================
// Entity Reference
// ===================================================================

void EntityRef::GenerateXML(GenerateContext& xml) const // virtual
// Generate: &name;
{
    xml += TEXT('&');
    xml += GetName();
    xml += TEXT(';');
}

// ===================================================================
// Elements (Markups) - Abstract class
// ===================================================================

void Element::GenerateXML(GenerateContext& xml) const // virtual
// Generate: <name attributes/>
//       or: <name attributes>content</name>
//
// Most complicated generation.
// 1) Generate the start tag with its attributes.
// 2) If the element is empty, generate an empty tag.
// 3) Otherwise, generate the sub-nodes and then the end tag.
// Produce some indentation if neccessary
{
    xml.GenerateStartTagIndentation();

    xml += TEXT('<');
    xml += GetName();

    for(Attributes::const_iterator itAttr = m_attributes.begin(); itAttr < m_attributes.end(); ++itAttr)
    {
        xml += TEXT(' ');
        (*itAttr)->GenerateXML(xml);
    }

    // Two cases: "normal" element or empty element
    if(m_children.empty())
    {
        // Empty element
        xml += TEXT("/>");
        xml.EndTag();
    }
    else
    {
        // "Normal" element
        xml += TEXT('>');
        Super::GenerateXML(xml);

        xml.GenerateEndTagIndentation();

        // End tag
        xml += TEXT("</");
        xml += GetName();
        xml += TEXT('>');
    }
}

// ===================================================================
// Comment
// ===================================================================

void Comment::GenerateXML(GenerateContext& xml) const // virtual
// Generate: <!--comment-->
{
    xml.GenerateStartTagIndentation();
    xml += TEXT("<!--");
    xml += m_strComment;
    xml += TEXT("-->");
    xml.GenerateNewLine();
    xml.EndTag();
}

// ===================================================================
// Processing Instruction (PI)
// ===================================================================

void Pi::GenerateXML(GenerateContext& xml) const // virtual
// Generate: <?target instruction?>
//       or: <?target?>
{
    xml.GenerateStartTagIndentation();

    xml += TEXT("<?");
    xml += GetName();

    if(!m_strInstruction.empty())
    {
        xml += TEXT(' ');
        xml += m_strInstruction;
    }

    xml += TEXT("?>");
    xml.GenerateNewLine();
    xml.EndTag();
}

// ===================================================================
// CDATA Section
// ===================================================================

void CData::GenerateXML(GenerateContext& xml) const // virtual
// Generate: <![CDATA[section]]>
{
    xml.GenerateStartTagIndentation();
    xml += TEXT("<![CDATA[");
    xml += m_strSection;
    xml += TEXT("]]>");
    xml.GenerateNewLine();
    xml.EndTag();
}

// ===================================================================
// Document
// ===================================================================

void Document::GenerateXML(GenerateContext& xml) const // virtual
// Generate:
//  <?xml version="1.0" encoding="UTF-8"?>
//  document
{
    // For the moment, ignore standalone and the DTD
    xml += TEXT("<?xml version=\"");
    xml += m_strXmlVersion;
    xml += TEXT("\" encoding=\"");
    xml += m_strEncoding;
    xml += TEXT("\"?>");
    xml.GenerateNewLine();

    Super::GenerateXML(xml);
}

// ===================================================================
// Set Values
// ===================================================================
// I don't like to use sprintf, but it's a simple way to achive my goal

// You can define these functions ourself if you prefer
#ifndef ADVXMLPARSER_NO_CONVERSION

void operator<<(Value value, int nValue)
{
	using namespace std;
    // 15 characters is enough for 32 bit integer
    Char szBuffer[15];
	sprintf(szBuffer, TEXT("%i"), nValue);
    value.Add(szBuffer);
}

void operator<<(Value value, unsigned int nValue)
{
	using namespace std;
    // 15 characters is enough for 32 bit integer
    Char szBuffer[15];
	sprintf(szBuffer, TEXT("%ui"), nValue);
    value.Add(szBuffer);
}

void operator<<(Value value, double dValue)
{
	using namespace std;
    Char szBuffer[25];
    sprintf(szBuffer, TEXT("%f"), dValue);
    value.Add(szBuffer);
}

#endif

// ===================================================================

}

#endif // ADVXMLPARSER_NO_WRITE
