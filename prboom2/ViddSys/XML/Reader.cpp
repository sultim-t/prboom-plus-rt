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

using advstd::auto_ptr;

// ===================================================================
// XML Parser
// ===================================================================

Parser::Parser()
:   m_szSource(NULL),
    m_nLine(1),
    m_nColumn(0),
    m_nOldColumn(0)
{
}

Parser::~Parser()
{
}

void Parser::SyntaxError(PARSER_ERROR nError)
// Syntax error (throw exception)
{
    throw ParsingException(nError, m_nLine, m_nColumn);
}


// ===================================================================
// Low-level parsing
// ===================================================================

Char Parser::NextChar()
// Next char (next position)
{
    // End of document ?
    if(m_szSourceCurrent >= m_szSourceEnd)
    {
        m_szSourceCurrent = m_szSourceEnd + 1;
        return(0);
    }

    // Next char
    Char c = *m_szSourceCurrent++;

    // End-of-line handling (see 2.11)
    if(c == TEXT('\xD'))
    {
        // Is a "#xD#xA" sequence ?
        if(m_szSourceCurrent < m_szSourceEnd && *m_szSourceCurrent == TEXT('\xA'))
            ++m_szSourceCurrent;

        c = TEXT('\xA');
    }

    // Record old m_nColumn (for PreviousChar)
    m_nOldColumn = m_nColumn;

    // If new line, increment the line number
    if(c == TEXT('\xA'))
        ++m_nLine, m_nColumn = 0;
    else // increment the column number
        ++m_nColumn;

    // Return the char
    return(c);
}

void Parser::PreviousChar()
// Previous position
{
    // End of document ?
    if(m_szSourceCurrent > m_szSourceEnd)
        return;

    // Decrement the position if we are not already
    // at the begining of the document
    if(m_szSourceCurrent - 1 < m_szSource)
    {
        // Beginning of the document
        m_szSourceCurrent = m_szSource;
        // Reset line and column #
        m_nLine = m_nColumn = m_nOldColumn = 1;
    }
    else
    {
        // Previous char
        m_szSourceCurrent -= 1;
        // Was a new line ?
        if(*m_szSourceCurrent == TEXT('\xA'))
        {
            // End-of-line handling (see 2.11)
            if(m_szSourceCurrent > m_szSource && m_szSourceCurrent[-1] == TEXT('\xD'))
                --m_szSourceCurrent;

            // Previous line #
            --m_nLine;
            ASSERT(m_nLine >= 1);
        }
        // Previous column #
        m_nColumn = m_nOldColumn;
    }
}

// ===================================================================
// Parse strings, numbers, etc.
// ===================================================================

bool Parser::ParseString(const Char* pString)
// Read the given string
{
    // Record the current position
    Bookmark bookmark(advself);

    // not end of string ?
    while(*pString != 0)
    {
        // Next char
        Char c = NextChar();
        // Same ?
        if(c != *pString)
        {
            // Not the same so revert back to
            // the previous position
            bookmark.Restore();
            return(false);
        }

        // Next char of the string
        ++pString;
    }

    return(true);
}

bool Parser::ParseStringNoCase(const Char* pString)
// Read the given string (not case sensitive)
{
    // Record the current position
    Bookmark bookmark(advself);

    // not end of string ?
    while(*pString != 0)
    {
        // Next char
        Char c = NextChar();
        // Same (not case sensitive) ?
        if(LowCase(c) != LowCase(*pString))
        {
            // Not the same so revert back to
            // the previous position
            bookmark.Restore();
            return(false);
        }

        // Next char of the string
        ++pString;
    }

    return(true);
}

bool Parser::ParseNumber(int& nNum)
// Read a (decimal) number
{
    Char c = NextChar();
    // Start with a digit ?
    if(!IsDigit(c))
        return(false); // not a number
    
    nNum = 0;
    // Read all digits possible
    while(IsDigit(c))
    {
        // Compute new number
        nNum = nNum * 10 + c - TEXT('0');
        // Next char
        c = NextChar();
    }

    // The current char if not part of the number
    PreviousChar();
    return(true);
}

bool Parser::ParseHexNumber(int& nNum)
// Read an hexadecimal number
{
    Char c = NextChar();
    // Start with an hexadecimal digit ?
    if(!IsHexDigit(c))
        return(false);

    nNum = 0;
    // Read all digits possible
    while(IsHexDigit(c))
    {
        // Compute new number
        nNum = nNum * 16 + HexDigitValue(c);
        // Next char
        c = NextChar();
    }

    // The current char if not part of the number
    PreviousChar();
    return(true);
}

bool Parser::ParseChar(Char c)
// Read a given char
{
    if(NextChar() != c)
    {
        PreviousChar();
        return(false);
    }
    return(true);
}

// ===================================================================
// Utilities
// ===================================================================

bool Parser::ParseDeclBegining(const Char* szString)
// Parse a declaration (like in: version = )
// S <szString> Eq.
// Marker: <szString>
{
    // Record the current position
    Bookmark bookmark(advself);

    // Parse: S
    if(!ParseSpaces())
        return(false);

    // Parse: <szString>
    if(!ParseString(szString))
    {
        bookmark.Restore();
        return(false);
    }

    // Parse: Eq
    if(!ParseEq())
        SyntaxError(ERROR_NO_EQUAL);

    return(true);
}

// ===================================================================
// Basic Rules of the Grammar
// ===================================================================

bool Parser::ParseSpaces()
// Read One or more spaces
// [3] S ::= (#x20 | #x9 | #xD | #xA)+
{
    // N+ is equivalent to N N*

    Char c = NextChar();
    if(!IsSpace(c))
    {
        PreviousChar();
        return(false);
    }

    do c = NextChar(); while(IsSpace(c));
    PreviousChar();
    return(true);
}

bool Parser::ParseEq()
// Parse equal sign
// [25] Eq ::= S? '=' S?
// Marker: '='
{
    // Record the current position
    Bookmark bookmark(advself);

    // Parse spaces (optional)
    ParseSpaces();
    // Is an equal sign ?
    if(!ParseChar(TEXT('=')))
    {
        // No, so revert to the previous position
        bookmark.Restore();
        return(false);
    }
    // Skip spaces if any
    ParseSpaces();

    return(true);
}

bool Parser::ParseName(String& strName)
// Read a name (letters, digits and special chars)
// [5] Name ::=  (Letter | '_' | ':') (NameChar)*
// Note: It's strange: [6] Names is defined but never used...
{
    // Record the current position to extract later the name
    Bookmark bookmark(advself);

    Char c = NextChar();
    // Is allowed ?
    if(!IsAlpha(c) && c != TEXT('_') && c != TEXT(':'))
    {
        PreviousChar();
        return(false);
    }

    // Get as more (allowed) char as possible
    while(IsNameChar(c))
        c = NextChar();

    // Current character is not part of the version num.
    PreviousChar();

    // Extract the name
    bookmark.GetSubString(strName);
    return(true);
}

bool Parser::ParseNmtoken(String& strName)
// Read a Name Token
// [7] Nmtoken ::= (NameChar)+
// Note: It's strange: [8] Names is defined but never used...
{
    // Record the current position to extract later the name
    Bookmark bookmark(advself);

    Char c = NextChar();
    // Is allowed ?
    if(!IsNameChar(c))
    {
        PreviousChar();
        return(false);
    }

    // Get as more (allowed) char as possible
    while(IsNameChar(c))
        c = NextChar();

    // Current character is not part of the version num.
    PreviousChar();

    // Extract the name
    bookmark.GetSubString(strName);
    return(true);
}

Document* Parser::Parse(const Char* szSource, int nSourceSize)
// Parse the document
{
    m_szSource          = szSource;
    m_szSourceCurrent   = m_szSource;
    m_szSourceEnd       = m_szSource + nSourceSize;
    m_nLine             = 1;
    m_nColumn           = 1;

    auto_ptr<Document> pDocument(Document::Parse(advself));
    return(pDocument.release());
}

// ===================================================================
// Document
// ===================================================================

Document* Document::Parse(Parser& parser)
// Parse document (whole xml document)
// [1] document ::= prolog element Misc*
{
    auto_ptr<Document> pDocument(new Document);
    pDocument->ParseProlog(parser);

    // Get the root element
    pDocument->ParseRootElement(parser);
    pDocument->ParseMiscs(parser);

    // If there is some data remaining, it's an error
    if(parser.NextChar() != 0)
        parser.SyntaxError(ERROR_DATA_AT_END);

    return(pDocument.release());
}

void Document::ParseRootElement(Parser& parser)
{
    // Get the root element
    auto_ptr<Element> pRoot(Element::Parse(parser, advself));
    if(NULL == pRoot.get())
        parser.SyntaxError(ERROR_NO_ROOT);

    m_pRoot = pRoot.get();
    Add(pRoot.release());
}

void Document::ParseProlog(Parser& parser)
// [22] prolog ::= XMLDecl? Misc* (doctypedecl Misc*)? 
{
    ParseXMLDecl(parser);
    ParseMiscs(parser);

#if ADVXMLPARSER_DTD != 0
    if(m_dtd.ParseDoctypedecl(parser))
        ParseMiscs(parser);
#endif // ADVXMLPARSER_DTD
}

bool Document::ParseXMLDecl(Parser& parser)
// Parse XML declaration
// [23] XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'
// Marker: '<?xml'
{
    // Parse: '<?xml'.
    // I decide to respect the specification and refuse now "XML", ...
    if(!parser.ParseString(TEXT("<?xml")))
        return(false);

    // Parse: VersionInfo EncodingDecl? SDDecl? S? '?>'
    if(!ParseVersionInfo(parser, m_strXmlVersion))
        parser.SyntaxError(ERROR_INVALID_VERSION_INFO);

    // Parse EncodingDecl (optional)
    ParseEncodingDecl(parser, m_strEncoding);
    // Parse SDDecl (optional)
    ParseSDDecl(parser, m_bStandalone);
    // Parse S (spaces) (optional)
    parser.ParseSpaces();

    // Parse end of declaration '?>'
    if(!parser.ParseString(TEXT("?>")))
        parser.SyntaxError(ERROR_INVALID_DECLARATION_END);

    return(true);
}

bool Document::ParseVersionInfo(Parser& parser, String& strVersion)
// Parse XML version
// [24] VersionInfo ::= S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"')
// Marker: 'version'
{
    // Parse: S 'version' Eq
    if(!parser.ParseDeclBegining(TEXT("version")))
        return(false);

    // Parse: ("'" VersionNum "'" | '"' VersionNum '"')
    Char cQuote = parser.NextChar();
    if(cQuote != TEXT('\'') && cQuote != TEXT('\"'))
        parser.SyntaxError(ERROR_INVALID_VERSION_INFO);

    // Parse version number and check the delimiter
    if(!ParseVersionNum(parser, strVersion) || parser.NextChar() != cQuote)
        parser.SyntaxError(ERROR_INVALID_VERSION_INFO);

    return(true);
}

bool Document::ParseVersionNum(Parser& parser, String& strVersion)
// Parse XML version number
// [26] VersionNum ::= ([a-zA-Z0-9_.:] | '-')+
{
    // Record the current position
    Bookmark bookmark(parser);

    Char c = parser.NextChar();
    // Is an allowed character ?
    if(!IsAlphaDigit(c) && c != TEXT('_') && c != TEXT('.') && c != TEXT(':') && c != TEXT('-'))
    {
        parser.PreviousChar();
        return(false);
    }

    c = parser.NextChar();
    // Get as much chars as possible
    while(IsAlphaDigit(c) || c == TEXT('_') || c == TEXT('.') || c == TEXT(':') || c == TEXT('-'))
        c = parser.NextChar();

    // Current character is not part of the version num.
    parser.PreviousChar();

    // Get the version number
    bookmark.GetSubString(strVersion);
    return(true);
}

bool Document::ParseEncodingDecl(Parser& parser, String& strEncoding)
// Parse XML encoding declaration
// [80] EncodingDecl ::= S 'encoding' Eq ('"' EncName '"' | "'" EncName "'")
// Marker: 'encoding'
{
    // Parse: S 'encoding' Eq
    if(!parser.ParseDeclBegining(TEXT("encoding")))
        return(false);

    // Parse: ('"' EncName '"' |  "'" EncName "'")
    Char cQuote = parser.NextChar();
    if(cQuote != TEXT('\'') && cQuote != TEXT('\"'))
        parser.SyntaxError(ERROR_INVALID_ENCODING_DECLARATION);

    // Parse encoding name and check delimiter
    // Bug#0002: Was ParseEncName() instead of !ParseEncName()
    if(!ParseEncName(parser, strEncoding) || parser.NextChar() != cQuote)
        parser.SyntaxError(ERROR_INVALID_ENCODING_NAME);

    return(true);
}

bool Document::ParseEncName(Parser& parser, String& strEncoding)
// Parse encoding name
// [81] EncName ::= [A-Za-z] ([A-Za-z0-9._] | '-')*
{
    // Record the current position
    Bookmark bookmark(parser);

    Char c = parser.NextChar();
    // Is an allowed character ?
    if(!IsAlpha(c))
    {
        parser.PreviousChar();
        return(false);
    }

    c = parser.NextChar();
    // Get as more char as possible
    while(IsAlphaDigit(c) || c == TEXT('_') || c == TEXT('.') || c == TEXT('-'))
        c = parser.NextChar();

    // Current character is not part of the version num.
    parser.PreviousChar();

    // Get the version number
    bookmark.GetSubString(strEncoding);
    return(true);
}

bool Document::ParseSDDecl(Parser& parser, bool& bStandalone)
// Parse Standalone Document Declaration
// [32] SDDecl ::= S 'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"'))
// Marker: 'standalone'
{
    // Parse: S 'standalone' Eq
    if(!parser.ParseDeclBegining(TEXT("standalone")))
        return(false);

    // Parse: "'" | '"'
    Char cQuote = parser.NextChar();
    if(cQuote != TEXT('\'') && cQuote != TEXT('\"'))
        parser.SyntaxError(ERROR_INVALID_STANDALONE_DECLARATION);

    // Parse: 'yes' | 'no'
    if(parser.ParseString(TEXT("yes")))
        bStandalone = true;
    else if(parser.ParseString(TEXT("no")))
        bStandalone = false;
    else
        parser.SyntaxError(ERROR_INVALID_STANDALONE_VALUE);

    // Parse: "'" | '"' (same as first)
    if(parser.NextChar() != cQuote)
        parser.SyntaxError(ERROR_INVALID_STANDALONE_VALUE);

    return(true);
}

bool Document::ParseMisc(Parser& parser)
// Parse Comments, spaces, processing instructions, etc.
// Add Comments and Processing Instructions to the list of nodes
// [27] Misc ::= Comment | PI | S
{
    // Parse comment if any
    auto_ptr<Comment> pComment(Comment::Parse(parser, advself));
    if(NULL != pComment.get())
    {
        Add(pComment.release());
        return(true);
    }

    // Parse Processing Instruction if any
    auto_ptr<Pi> pPI(Pi::Parse(parser, advself));
    if(NULL != pPI.get())
    {
        Add(pPI.release());
        return(true);
    }

    // Parse spaces if any
    return(parser.ParseSpaces());
}

void Document::ParseMiscs(Parser& parser)
// Parse Comments, spaces, processing instructions, etc.
// Add the Comments to the list of nodes
// Misc*
// [27] Misc ::= Comment | PI | S
{
    while(ParseMisc(parser))
        ;
}

// ===================================================================
// Document Type Definition (DTD)
// ===================================================================

#if ADVXMLPARSER_DTD != 0

Dtd::Dtd(Document& document)
:   m_document(document)
{
}

bool Dtd::ParseDoctypedecl(Parser& parser)
// Parse Document Type Definition (DTD)
// [28] doctypedecl ::= '<!DOCTYPE' S Name (S ExternalID)? S? ('[' (markupdecl | PEReference | S)* ']' S?)? '>' 
// Marker: '<!DOCTYPE'
{
    // Parse: '<!DOCTYPE'
    if(!parser.ParseString(TEXT("<!DOCTYPE")))
        return(false);

    // Parse: S
    if(!parser.ParseSpaces())
        parser.SyntaxError(ERROR_INVALID_DOCTYPE);

    // Parse: Name
    String strName;
    if(!parser.ParseName(strName))
        parser.SyntaxError(ERROR_INVALID_DOCTYPE);

    // Parse: (S ExternalID)? S?
    parser.ParseSpaces();
    if(ParseExternalID(parser))
        parser.ParseSpaces();

    // Parse: ('[' (markupdecl | PEReference | S)* ']' S?)?
    if(parser.ParseChar(TEXT('[')))
    {
        // Parse: (markupdecl | PEReference | S)* ']'
#if ADVXMLPARSER_DTD == 1
        Char c = parser.NextChar();
        while(c != TEXT(']'))
        {
            if(c == 0)
                parser.SyntaxError(ERROR_END_OF_DOC);
            c = parser.NextChar();
        }
#else // ADVXMLPARSER_DTD
        while(!parser.ParseChar(TEXT(']')))
        {
            // Parse: markupdecl | PEReference | S
            if(parser.ParseSpaces() || ParseMarkupdecl(parser) || ParsePEReference(parser))
                continue;

            parser.SyntaxError(ERROR_INVALID_DOCTYPE);
        }
#endif // ADVXMLPARSER_DTD

        // Parse: S?
        parser.ParseSpaces();
    }

    // Parse: '>'
    if(!parser.ParseChar(TEXT('>')))
        parser.SyntaxError(ERROR_INVALID_DOCTYPE);

    return(true);
}

bool Dtd::ParsePEReference(Parser& parser)
// [69] PEReference ::= '%' Name ';'
// Marker: '%'
{
    if(!parser.ParseChar(TEXT('%')))
        return(false);

    String strName;
    if(!parser.ParseName(strName) || !parser.ParseChar(TEXT(';')))
        parser.SyntaxError(ERROR_INVALID_PE_REFERENCE);

    return(true);
}

bool Dtd::ParseSystemLiteral(Parser& parser)
// [11] SystemLiteral ::= ('"' [^"]* '"') | ("'" [^']* "'")
// Marker: '"' or "'" 
{
    Char cQuote = parser.NextChar();
    if(cQuote != TEXT('\'') && cQuote != TEXT('\"'))
        return(false);

    Char c = parser.NextChar();
    while(c != cQuote)
    {
        switch(c)
        {
        case 0:
            parser.SyntaxError(ERROR_END_OF_DOC);
            break;

        default:
            if(!IsXmlChar(c))
                parser.SyntaxError(ERROR_INVALID_CHAR);
            break;
        }

        c = parser.NextChar();
    }

    return(true);
}

static bool IsPibidChar(Char c)
// [13] PubidChar ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] 
{
    if(IsAlphaDigit(c))
        return(true);

    static const Char s_szChars[] = TEXT("\x20\xD\xA-'()+,./:=?;!*#@$_%");

    const Char* pEnd = s_szChars + elemof(s_szChars);
    for(const Char* pChar = s_szChars; pChar < pEnd; ++pChar)
    {
        if(c == *pChar)
            return(true);
    }

    return(false);
}

bool Dtd::ParsePubidLiteral(Parser& parser)
// [12] PubidLiteral ::= '"' PubidChar* '"' | "'" (PubidChar - "'")* "'" 
// Marker: " or '
{
    Char cQuote = parser.NextChar();
    if(cQuote != TEXT('\'') && cQuote != TEXT('\"'))
        return(false);

    Char c = parser.NextChar();
    while(c != cQuote)
    {
        if(c == 0)
            parser.SyntaxError(ERROR_END_OF_DOC);

        if(!IsPibidChar(c))
            parser.SyntaxError(ERROR_INVALID_PUBID_LITERAL);

        c = parser.NextChar();
    }

    return(true);
}

bool Dtd::ParseMarkupdecl(Parser& parser)
// [29] markupdecl ::= elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment  
{
    return(ParseElementDecl(parser) || ParseAttlistDecl(parser) || ParseEntityDecl(parser) || ParseNotationDecl(parser));
}

bool Dtd::ParseElementDecl(Parser& parser)
// [45] elementdecl ::= '<!ELEMENT' S Name S contentspec S? '>'
// Marker: '<!ELEMENT'
{
    // Parse: '<!ELEMENT'
    if(!parser.ParseString(TEXT("<!ELEMENT")))
        return(false);

    // Parse: S Name S contentspec
    String strName;
    if(!parser.ParseSpaces() || !parser.ParseName(strName) || !parser.ParseSpaces() || !ParseContentspec(parser))
        parser.SyntaxError(ERROR_INVALID_ELEMENT_DECL);

    // Parse: S?
    parser.ParseSpaces();

    // Parse: '>'
    if(parser.NextChar() != TEXT('>'))
        parser.SyntaxError(ERROR_INVALID_ELEMENT_DECL);

    return(true);
}

bool Dtd::ParseContentspec(Parser& parser)
// [46] contentspec ::= 'EMPTY' | 'ANY' | Mixed | children  
{
    if(parser.ParseString(TEXT("EMPTY")))
        return(true);

    if(parser.ParseString(TEXT("ANY")))
        return(true);

    if(!ParseMixed(parser) && !ParseChildren(parser))
        return(false);

    return(true);
}

bool Dtd::ParseMixed(Parser& parser)
// [51] Mixed ::= '(' S? '#PCDATA' (S? '|' S? Name)* S? ')*' 
//              | '(' S? '#PCDATA' S? ')'  
{
    Bookmark bookmark(parser);

    if(!parser.ParseChar(TEXT('(')))
        return(false);

    parser.ParseSpaces();

    if(!parser.ParseString(TEXT("#PCDATA")))
    {
        bookmark.Restore();
        return(false);
    }

    // (S? '|' S? Name)* S? ')*' is equivalent to:
    // S? ('|' S? Name S?)* ')*'

    // Case 1: S? ')*'
    // Case 2: S? ')' 
    // Case 3: S? ('|' S? Name S?)+ ')*'

    parser.ParseSpaces();

    // Case 1
    if(parser.ParseString(TEXT(")*")))
        return(true);

    // Case 2
    if(parser.ParseChar(TEXT(')')))
        return(true);

    // Case 3
    for(;;)
    {
        if(!parser.ParseChar(TEXT('|')))
            parser.SyntaxError(ERROR_CHOICE_EXPECTED);

        // Parse: S? Name S?
        String strName;
        parser.ParseSpaces();
        parser.ParseName(strName);
        parser.ParseSpaces();

        if(parser.ParseString(TEXT(")*")))
            break;
    }

    return(true);
}

bool Dtd::ParseChildren(Parser& parser)
// [47] children ::= (choice | seq) ('?' | '*' | '+')?
{
    if(!ParseChoiceSeq(parser))
        return(false);

    switch(parser.NextChar())
    {
    case TEXT('?'):
    case TEXT('*'):
    case TEXT('+'):
        break;

    default:
        parser.PreviousChar();
        break;
    }

    return(true);
}

bool Dtd::ParseChoiceSeq(Parser& parser)
// [49] choice ::= '(' S? cp ( S? '|' S? cp )* S? ')' 
// [50] seq    ::= '(' S? cp ( S? ',' S? cp )* S? ')'
// Marker: ')'
//
// Note that the grammar is ambiguous here. For example:
// '(' cp ')' is both a choice and a seq.
{
    // Parse: '('
    if(!parser.ParseChar(TEXT('(')))
        return(false);

    // Parse: S?
    parser.ParseSpaces();

    // Parse: cp
    if(!ParseCp(parser))
        parser.SyntaxError(ERROR_INVALID_CP);

    // ( S? '|' S? cp )* S? ')' is equivalent to:
    // S? ( '|' S? cp S? )* ')'

    // Parse: S?
    parser.ParseSpaces();

    Char c = parser.NextChar();
    Char cDelimiter = c;

    // Parse: (...)* ')'
    while(c != TEXT(')'))
    {
        switch(c)
        {
        case 0:
            parser.SyntaxError(ERROR_END_OF_DOC);
            break;

        default:
            if(c != cDelimiter)
                parser.SyntaxError((cDelimiter == TEXT('|')) ? ERROR_CHOICE_EXPECTED : ERROR_SEQ_EXPECTED);
            break;
        }

        // Parse: S? cp S?
        parser.ParseSpaces();
        if(!ParseCp(parser))
            parser.SyntaxError(ERROR_INVALID_CP);
        parser.ParseSpaces();

        c = parser.NextChar();
    }

    return(true);
}

bool Dtd::ParseCp(Parser& parser)
// [48] cp ::= (Name | choice | seq) ('?' | '*' | '+')? 
{
    String strName;

    if(!parser.ParseName(strName) && !ParseChoiceSeq(parser))
        return(false);

    switch(parser.NextChar())
    {
    case TEXT('?'):
    case TEXT('*'):
    case TEXT('+'):
        break;

    default:
        parser.PreviousChar();
        break;
    }

    return(true);
}

bool Dtd::ParseAttlistDecl(Parser& parser)
// [52] AttlistDecl ::= '<!ATTLIST' S Name AttDef* S? '>'
// Marker: '<!ATTLIST'
{
    // Parse: '<!ATTLIST'
    if(!parser.ParseString(TEXT("<!ATTLIST")))
        return(false);

    // Parse: S Name
    String strName;
    if(!parser.ParseSpaces() || !parser.ParseName(strName))
        parser.SyntaxError(ERROR_INVALID_ATTLIST_DECL);

    // Parse: AttDef*
    while(ParseAttDef(parser))
        ;

    // Parse: S?
    parser.ParseSpaces();

    // Parse: '>'
    if(parser.NextChar() != TEXT('>'))
        parser.SyntaxError(ERROR_INVALID_ATTLIST_DECL);

    return(true);
}

bool Dtd::ParseAttDef(Parser& parser)
// [53] AttDef ::= S Name S AttType S DefaultDecl 
// Marker: Name
{
    Bookmark bookmark(parser);

    // Parse: S Name
    String strName;
    if(!parser.ParseSpaces() || !parser.ParseName(strName))
    {
        bookmark.Restore();
        return(false);
    }

    // Parse: S AttType S DefaultDecl 
    if(!parser.ParseSpaces() || !ParseAttType(parser) || !parser.ParseSpaces() || !ParseDefaultDecl(parser))
        parser.SyntaxError(ERROR_INVALID_ATTDEF);

    return(true);
}

bool Dtd::ParseAttType(Parser& parser)
// [54] AttType        ::= StringType | TokenizedType | EnumeratedType  
// [55] StringType     ::= 'CDATA' 
// [56] TokenizedType  ::= 'ID' | 'IDREF' | 'IDREFS' | 'ENTITY' | 'ENTITIES' | 'NMTOKEN' | 'NMTOKENS' 
// [57] EnumeratedType ::= NotationType | Enumeration  
{
    // Parse: StringType
    if(parser.ParseString(TEXT("CDATA")))
        return(true);

    // Parse: TokenizedType
    // Note: the order of testing is very important because, for example, "ID" is a sub-string of "IDREF"
    if
    (
        parser.ParseString(TEXT("IDREFS"))      ||
        parser.ParseString(TEXT("IDREF"))       ||
        parser.ParseString(TEXT("ID"))          ||
        parser.ParseString(TEXT("ENTITIES"))    ||
        parser.ParseString(TEXT("ENTITY"))      ||
        parser.ParseString(TEXT("NMTOKENS"))    ||
        parser.ParseString(TEXT("NMTOKEN"))
    )
        return(true);

    if(!ParseNotationType(parser) && !ParseEnumeration(parser))
        return(false);

    return(true);
}

bool Dtd::ParseNotationType(Parser& parser)
// [58] NotationType ::= 'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')'  
// Marker: 'NOTATION'
{
    // Parse: 'NOTATION'
    if(!parser.ParseString(TEXT("NOTATION")))
        return(false);

    // Parse: S '('
    if(!parser.ParseSpaces() || !parser.ParseChar(TEXT('(')))
        parser.SyntaxError(ERROR_INVALID_NOTATION_TYPE);

    // Parse: S?
    parser.ParseSpaces();

    // Parse: Name
    String strName;
    if(!parser.ParseName(strName))
        parser.SyntaxError(ERROR_INVALID_NOTATION_TYPE);

    // (S? '|' S? Name)* S? ')' is equivalent to:
    // S? ('|' S? Name S?)* ')'

    // Parse: S?
    parser.ParseSpaces();

    // Parse: ('|' S? Name S?)*
    while(parser.ParseChar(TEXT('|')))
    {
        // Parse: S?
        parser.ParseSpaces();

        // Parse: Name
        String strName;
        if(!parser.ParseName(strName))
            parser.SyntaxError(ERROR_INVALID_NOTATION_TYPE);

        // Parse: S?
        parser.ParseSpaces();
    }

    // Parse: ')' 
    if(!parser.ParseChar(TEXT(')')))
        parser.SyntaxError(ERROR_INVALID_NOTATION_TYPE);

    return(true);
}

bool Dtd::ParseEnumeration(Parser& parser)
// [59] Enumeration ::= '(' S? Nmtoken (S? '|' S? Nmtoken)* S? ')' 
// Marker: '('
{
    if(!parser.ParseChar(TEXT('(')))
        return(false);

    // Parse: S?
    parser.ParseSpaces();

    // Parse: Nmtoken
    String strName;
    if(!parser.ParseNmtoken(strName))
        parser.SyntaxError(ERROR_INVALID_ENUMERATION);

    // (S? '|' S? Nmtoken)* S? ')' is equivalent to:
    // S? ('|' S? Nmtoken S?)* ')'

    // Parse: S?
    parser.ParseSpaces();

    // Parse: ('|' S? Nmtoken S?)*
    while(parser.ParseChar(TEXT('|')))
    {
        // Parse: S?
        parser.ParseSpaces();

        // Parse: Nmtoken
        String strName;
        if(!parser.ParseNmtoken(strName))
            parser.SyntaxError(ERROR_INVALID_ENUMERATION);

        // Parse: S?
        parser.ParseSpaces();
    }

    // Parse: ')' 
    if(!parser.ParseChar(TEXT(')')))
        parser.SyntaxError(ERROR_INVALID_ENUMERATION);

    return(true);
}

bool Dtd::ParseDefaultDecl(Parser& parser)
// [60] DefaultDecl ::= '#REQUIRED' | '#IMPLIED' | (('#FIXED' S)? AttValue) 
{
    // Parse: '#REQUIRED' | '#IMPLIED'
    if(parser.ParseString(TEXT("#REQUIRED")) || parser.ParseString(TEXT("#IMPLIED")))
        return(true);

    // Parse: ('#FIXED' S)?
    if(parser.ParseString(TEXT("#FIXED")) && !parser.ParseSpaces())
        parser.SyntaxError(ERROR_INVALID_DEFAULT_DECL);

    Attribute attr(Node::null, TEXT(""));
    if(!attr.ParseAttValue(parser))
        return(false);

    return(true);
}

bool Dtd::ParseEntityDecl(Parser& parser)
// [70] EntityDecl ::= GEDecl | PEDecl 
// [71] GEDecl     ::= '<!ENTITY' S Name S EntityDef S? '>' 
// [72] PEDecl     ::= '<!ENTITY' S '%' S Name S PEDef S? '>'
// Marker: '<!ENTITY'
{
    // Parse: '<!ENTITY'
    if(!parser.ParseString(TEXT("<!ENTITY")))
        return(false);

    // Parse: S
    if(!parser.ParseSpaces())
        parser.SyntaxError(ERROR_INVALID_ENTITY_DECL);

    // Parse: ('%' S Name S PEDef) | (Name S EntityDef)
    if(parser.ParseChar(TEXT('%')))
    {
        // Parse: S Name S PEDef
        String strName;
        if(!parser.ParseSpaces() || !parser.ParseName(strName) || !parser.ParseSpaces() || !ParsePEDef(parser))
            parser.SyntaxError(ERROR_INVALID_ENTITY_DECL);
    }
    else
    {
        // Parse: Name S EntityDef
        String strName;
        if(!parser.ParseName(strName) || !parser.ParseSpaces() || !ParseEntityDef(parser))
            parser.SyntaxError(ERROR_INVALID_ENTITY_DECL);
    }

    // Parse: S?
    parser.ParseSpaces();

    // Parse: '>'
    if(!parser.ParseChar(TEXT('>')))
        parser.SyntaxError(ERROR_INVALID_ENTITY_DECL);

    return(true);
}

bool Dtd::ParseEntityDef(Parser& parser)
// [73] EntityDef ::= EntityValue | (ExternalID NDataDecl?) 
{
    if(ParseEntityValue(parser))
        return(true);

    if(!ParseExternalID(parser))
        return(false);

    ParseNDataDecl(parser);
    return(true);
}

bool Dtd::ParseNDataDecl(Parser& parser)
// [76] NDataDecl ::= S 'NDATA' S Name 
{
    Bookmark bookmark(parser);

    if(!parser.ParseSpaces())
        return(false);

    if(!parser.ParseString(TEXT("NDATA")))
    {
        bookmark.Restore();
        return(false);
    }

    String strName;
    if(!parser.ParseSpaces() || !parser.ParseName(strName))
        parser.SyntaxError(ERROR_INVALID_NDATA_DECL);

    return(true);
}

bool Dtd::ParsePEDef(Parser& parser)
// [74] PEDef ::= EntityValue | ExternalID 
{
    return(ParseEntityValue(parser) || ParseExternalID(parser));
}

bool Dtd::ParseEntityValue(Parser& parser)
// [9] EntityValue ::= '"' ([^%&"] | PEReference | Reference)* '"' |
//                     "'" ([^%&'] | PEReference | Reference)* "'"
// Marker: '"' or "'"
{
    // Get the value delimiter (quote or apostroph)
    Char cQuote = parser.NextChar();
    if(cQuote != TEXT('\'') && cQuote != TEXT('\"'))
    {
        parser.PreviousChar();
        return(false);
    }

    Char c = parser.NextChar();
    // Search the end of the value
    while(c != cQuote)
    {
        switch(c)
        {
        case 0: // End of document
            parser.SyntaxError(ERROR_END_OF_DOC);
            break;

        case TEXT('&'): // Reference
            {
                parser.PreviousChar();

                auto_ptr<Reference> pReference(Reference::Parse(parser, Node::null));
                if(pReference.get() == NULL) // Failed ?
                    parser.SyntaxError(ERROR_INVALID_REFERENCE);
            }
            break;

        case TEXT('%'): // Parameter-entity Reference
            {
                parser.PreviousChar();

                if(!ParsePEReference(parser)) // Failed ?
                    parser.SyntaxError(ERROR_INVALID_REFERENCE);
            }
            break;

        default:
            // Be sure it's an allowed Char
            if(!IsXmlChar(parser.NextChar()))
                parser.SyntaxError(ERROR_INVALID_CHAR);
            break;
        }
    }

    return(true);
}

bool Dtd::ParseNotationDecl(Parser& parser)
// [82] NotationDecl ::= '<!NOTATION' S Name S (ExternalID | PublicID) S? '>'
// Marker: '<!NOTATION'
{
    // Parse: '<!NOTATION'
    if(!parser.ParseString(TEXT("<!NOTATION")))
        return(false);

    // Parse: S Name S
    String strName;
    if(!parser.ParseSpaces() || !parser.ParseName(strName) || !parser.ParseSpaces())
        parser.SyntaxError(ERROR_INVALID_NOTATION_DECL);

    // Parse: (ExternalID | PublicID)
    if(!ParseExternalID(parser) && !ParsePublicID(parser))
        parser.SyntaxError(ERROR_INVALID_NOTATION_DECL);

    // Parse: S?
    parser.ParseSpaces();

    // Parse: '>'
    if(!parser.ParseChar(TEXT('>')))
        parser.SyntaxError(ERROR_INVALID_NOTATION_DECL);

    return(true);
}

bool Dtd::ParseExternalID(Parser& parser)
// [75] ExternalID ::= 'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral
// Markers: 'SYSTEM' or 'PUBLIC'
{
    // Parse: 'SYSTEM' S SystemLiteral
    if(parser.ParseString(TEXT("SYSTEM")))
    {
        if(!parser.ParseSpaces() || !ParseSystemLiteral(parser))
            parser.SyntaxError(ERROR_INVALID_EXTERNAL_ID);
    }
    // Parse: 'PUBLIC' S PubidLiteral S SystemLiteral
    else if(parser.ParseString(TEXT("PUBLIC")))
    {
        if(!parser.ParseSpaces() || !ParsePubidLiteral(parser) || !parser.ParseSpaces() || !ParseSystemLiteral(parser))
            parser.SyntaxError(ERROR_INVALID_EXTERNAL_ID);
    }
    else
        return(false);

    return(true);
}

bool Dtd::ParsePublicID(Parser& parser)
// [83] PublicID ::= 'PUBLIC' S PubidLiteral  
// Marker: 'PUBLIC'
{
    // Parse: 'PUBLIC'
    if(!parser.ParseString(TEXT("PUBLIC")))
        return(false);

    // Parse: S PubidLiteral
    if(!parser.ParseSpaces() || !ParsePubidLiteral(parser))
        parser.SyntaxError(ERROR_INVALID_PUBLIC_ID);

    return(true);
}

#endif // ADVXMLPARSER_DTD

// ===================================================================
// Comment
// ===================================================================

Comment* Comment::Parse(Parser& parser, NodeContainer& parent)
// Parse comment and construct an element
// [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
{
    // Check the start of the comment
    if(!parser.ParseString(TEXT("<!--")))
        return(NULL);

    // Record the current position to extract later the
    // content of the comment
    Bookmark bookmark(parser);
    for(;;)
    {
        // Look for the end of the comment
        if(parser.ParseString(TEXT("--")))
        {
            // Really the end ?
            if(!parser.ParseChar(TEXT('>')))
                parser.SyntaxError(ERROR_INVALID_COMMENT_CHAR);
            break;
        }

        // Only Char is allowed here
        if(!IsXmlChar(parser.NextChar()))
            parser.SyntaxError(ERROR_INVALID_CHAR);
    }

    // Extract the content of the comment
    String strComment;
    bookmark.GetSubString(strComment, 3);

    // Construct an element
    return(new Comment(parent, strComment));
}

// ===================================================================
// Processing Instruction (PI)
// ===================================================================

Pi* Pi::Parse(Parser& parser, NodeContainer& parent)
// Parse Processing Instruction
// [16] PI ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>' 
// Marker: '<?'
{
    // Check the start of the PI
    if(!parser.ParseString(TEXT("<?")))
        return(NULL);

    // Get PITarget
    String strTarget;
    if(!ParsePITarget(parser, strTarget))
        parser.SyntaxError(ERROR_INVALID_PI_TARGET);

    // Skip spaces
    parser.ParseSpaces();

    // Record the current position to extract later the PI
    Bookmark bookmark(parser);

    while(!parser.ParseString(TEXT("?>")))
    {
        // Only Char is allowed here
        if(!IsXmlChar(parser.NextChar()))
            parser.SyntaxError(ERROR_INVALID_CHAR);
    }

    // Extract the content of the PI
    String strPI;
    bookmark.GetSubString(strPI, 2);

    // Construct an element
    return(new Pi(parent, strTarget, strPI));
}

bool Pi::ParsePITarget(Parser& parser, String& strTarget)
// Parse PITarget
// [17] PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))
{
    // Get the name
    if(!parser.ParseName(strTarget))
        return(false);

    // 'xml' or similar is not allowed (reserved)
    if
    (
        strTarget.length() == 3 &&
        LowCase(strTarget[0]) == TEXT('x') &&
        LowCase(strTarget[1]) == TEXT('m') &&
        LowCase(strTarget[2]) == TEXT('l')
    )
        parser.SyntaxError(ERROR_RESERVED_PI_TARGET);

    return(true);
}

// ===================================================================
// CDATA Section
// ===================================================================

CData* CData::Parse(Parser& parser, NodeContainer& parent)
// Parse CDATA
// [18] CDSect  ::= CDStart CData CDEnd
// [19] CDStart ::= '<![CDATA['
// [20] CData   ::=  (Char* - (Char* ']]>' Char*))  
// [21] CDEnd   ::= ']]>' 
// Marker: '<![CDATA[' 
{
    // Parse: <![CDATA[
    if(!parser.ParseString(TEXT("<![CDATA[")))
        return(NULL);

    Bookmark bookmark(parser);
    // Parse: CData. Search CDEnd
    while(!parser.ParseString(TEXT("]]>")))
    {
        // Is character allowed ?
        if(!IsXmlChar(parser.NextChar()))
            parser.SyntaxError(ERROR_INVALID_CHAR);
    }

    // Get CDATA content and add it as-is to the value
    String strData;
    bookmark.GetSubString(strData, 3);

    return(new CData(parent, strData));
}

// ===================================================================
// Element
// ===================================================================

Element* Element::Parse(Parser& parser, NodeContainer& parent)
// Parse element and construct an object
// [39] element := EmptyElemTag | STag content ETag
// Marker: '<'
{
    // Begining of element (start tag and empty tag)
    bool bEmptyTag = false;
    auto_ptr<Element> pElem(ParseTagBegining(parser, bEmptyTag, parent));
    if(pElem.get() == NULL)
        return(NULL);

    if(bEmptyTag)
        return(pElem.release());

    // Parse the remaining of the element
    pElem->ParseContentETag(parser);
    return(pElem.release());
}

Element* Element::ParseTagBegining(Parser& parser, bool& bEmptyTag, NodeContainer& parent)
// Parse start tag or empty tag and construct an object
// [40] STag         ::=  '<' Name (S Attribute)* S? '>'  
// [44] EmptyElemTag ::=  '<' Name (S Attribute)* S? '/>'
// Marker: '<'
{
    // Assume not an empty tag
    bEmptyTag = false;

    // Parse: '<'
    if(!parser.ParseChar(TEXT('<')))
        return(NULL);

    // Get the name of the tag
    String strName;
    if(!parser.ParseName(strName))
        parser.SyntaxError(ERROR_INVALID_TAG_NAME);

    // Construct an element object
    auto_ptr<Element> pElem(new Element(parent, strName));

    // Parse: (S Attribute)* S?
    for(;;)
    {
        if(!parser.ParseSpaces())
            break;

        auto_ptr<Attribute> pAttribute(Attribute::Parse(parser, *pElem));
        if(NULL == pAttribute.get())
            break;

        pElem->HandleSpecialAttributes(pAttribute.get());
        pElem->m_attributes.push_back(pAttribute.release());
    }

    Char c = parser.NextChar();
    if(c == TEXT('/')) // Empty tag ?
    {
        if(parser.NextChar() != TEXT('>'))
            parser.SyntaxError(ERROR_INVALID_EMPTY_TAG);

        bEmptyTag = true;
        return(pElem.release());
    }

    // End of the tag ?
    if(c != TEXT('>'))
        parser.SyntaxError(ERROR_INVALID_TAG_END);

    // return the element to the caller
    return(pElem.release());
}

bool Element::ParseETag(Parser& parser)
// Parse end tad
// [42] Etag ::= '</' Name S? '>'
// Marker: '</'
{
    // Is an End tag ?
    if(!parser.ParseString(TEXT("</")))
        return(false);

    // Get the tag name
    String strEndTagName;
    if(!parser.ParseName(strEndTagName))
        parser.SyntaxError(ERROR_INVALID_TAG_NAME);

    // Start and end tag names must match
    if(strEndTagName != GetName())
        parser.SyntaxError(ERROR_TAG_NAME_MISMATCH);

    // Skip spaces
    parser.ParseSpaces();

    // End of the tag
    if(!parser.ParseChar(TEXT('>')))
        parser.SyntaxError(ERROR_INVALID_TAG_END);

    return(true);
}

void Element::ParseContentETag(Parser& parser)
// Parse element content and end tag
// [43] content  ::=  (element | CharData | Reference | CDSect | PI | Comment)*
// [14] CharData ::=  [^<&]* - ([^<&]* ']]>' [^<&]*) 
// element begins with   '<'
// CDSect begins with    '<' ('<![CDATA[')
// PI begins with        '<' ('<?')
// Comment begins with   '<' ('<!--')
// ETag begins with      '<' ('</')
// Reference begins with '&'
// everthing else is CharData (Text)
{
    // Record the current position to extract later the content
    Bookmark bookmark(parser);

    Char c = parser.NextChar();
    for(;;)
    {
        switch(c)
        {
        case 0: // End of document
            parser.SyntaxError(ERROR_END_OF_DOC);
            break;

        case TEXT('&'):
            // Put back the char in the buffer so that ParseXxxx
            // functions can do their work
            parser.PreviousChar();

            // Put what we already have in the Element (as Text)
            AddText(bookmark);
            ParseReference(parser);
            // Record the new position (after the reference)
            bookmark.Reset();
            break;

        case TEXT('<'):
            // Put back the char in the buffer so that ParseXxxx
            // functions can do their work
            parser.PreviousChar();

            // Put what we already have in the Element (as Text)
            AddText(bookmark);
            if(ParseMarkup(parser))
                return;
            // Record the new position (after the reference)
            bookmark.Reset();
            break;

        case TEXT(']'):
            // "]]>" not allowed in content
            if(parser.ParseString(TEXT("]>")))
                parser.SyntaxError(ERROR_INVALID_CDATA_CHARS);
            break;

        default:
            if(!IsXmlChar(c))
                parser.SyntaxError(ERROR_INVALID_CHAR);
            break;
        }

        // Next char
        c = parser.NextChar();
    }
}

void Element::HandleSpecialAttributes(const Attribute* pAttribute)
// Some attributes have a special meaning.
{
    if(0 == pAttribute->GetName().compare(TEXT("xml:space")))
        m_bPreserveWS = (0 == pAttribute->GetValue().compare(TEXT("preserve")));
}

void Element::ParseReference(Parser& parser)
{
    // Parse the Reference and add it to the Element
    auto_ptr<Reference> pReference(Reference::Parse(parser, advself));
    if(pReference.get() == NULL) // Failed ?
        parser.SyntaxError(ERROR_INVALID_REFERENCE);

    // Add it to the element
    Add(pReference.release());
}

bool Element::ParseMarkup(Parser& parser)
// Parse markups like ETag, Comment, CDATA, element, or PI
// ETag begins with      '</'
// element begins with   '<'
// CDSect begins with    '<![CDATA['
// PI begins with        '<?'
// Comment begins with   '<!--'
{
    // Try to read an ETag
    if(ParseETag(parser))
        return(true);

    // Try to read a comment
    auto_ptr<Comment> pComment(Comment::Parse(parser, advself));
    if(NULL != pComment.get())
    {
        // Add it to the element
        Add(pComment.release());
        return(false);
    }

    // Try to read a CDATA
    auto_ptr<CData> pCData(CData::Parse(parser, advself));
    if(NULL != pCData.get())
    {
        // Add it to the element
        Add(pCData.release());
        return(false);
    }

    // Try to read a PI
    auto_ptr<Pi> pPi(Pi::Parse(parser, advself));
    if(NULL != pPi.get())
    {
        // Add it to the element
        Add(pPi.release());
        return(false);
    }
    
    // Last case: try to read an element
    auto_ptr<Element> pElement(Element::Parse(parser, advself));
    if(NULL != pElement.get())
    {
        // Add the child element to the element
        Add(pElement.release());
        return(false);
    }

    parser.SyntaxError(ERROR_INVALID_MARKUP);
    return(false);
}

// ===================================================================
// Attribute of Elements
// ===================================================================

Attribute* Attribute::Parse(Parser& parser, Element& parent)
// Parse Attribute and create an Attribute object
// [41] Attribute ::= Name Eq AttValue
// Marker: Name
{
    // Get attribute name
    String strName;
    if(!parser.ParseName(strName))
        return(NULL);

    if(!parser.ParseEq())
        parser.SyntaxError(ERROR_NO_EQUAL);

    // create an Attribute object
    auto_ptr<Attribute> pAttrib(new Attribute(parent, strName));
    // Get attribute value after the equal sign
    if(!pAttrib->ParseAttValue(parser))
        parser.SyntaxError(ERROR_INVALID_ATTRIBUTE_VALUE);

    return(pAttrib.release());
}

bool Attribute::ParseAttValue(Parser& parser)
// Parse attribute value
// [10] AttValue ::= '"' ([^<&"] | Reference)* '"' | "'" ([^<&'] | Reference)* "'" 
// Marker: '"' | "'"
{
    // Get the value delimiter (quote or apostroph)
    Char cQuote = parser.NextChar();
    if(cQuote != TEXT('\'') && cQuote != TEXT('\"'))
    {
        parser.PreviousChar();
        return(false);
    }

    // Chunk of value (text)
    String strChunk;

    // Record the current position to extract later the text
    Bookmark bookmark(parser);

    Char c = parser.NextChar();
    // Search the end of the value
    while(c != cQuote)
    {
        switch(c)
        {
        case 0: // end of document
            parser.SyntaxError(ERROR_END_OF_DOC);
            break;

        case TEXT('<'): // Tag
            parser.SyntaxError(ERROR_UNEXPECTED_TAG);
            break;

        case TEXT('&'): // Reference
            {
                parser.PreviousChar();
                // Put what we already have
                AddText(bookmark);

                auto_ptr<Reference> pReference(Reference::Parse(parser, advself));
                if(pReference.get() == NULL) // Failed ?
                    parser.SyntaxError(ERROR_INVALID_REFERENCE);

                // Add it to the attribute
                Add(pReference.release());

                // Record the new position (after the reference)
                bookmark.Reset();
            }
            break;

        default:
            if(!IsXmlChar(c))
                parser.SyntaxError(ERROR_INVALID_CHAR);
            break;
        }

        // Next character
        c = parser.NextChar();
    }

    // Put the remaining of the value (without the Quote)
    AddText(bookmark, 1);
    return(true);
}

// ===================================================================
// References
// ===================================================================

Reference* Reference::Parse(Parser& parser, NodeContainer& parent)
// Parse Reference
// [67] Reference ::= EntityRef | CharRef
// Marker: '&'
// Here I choose not to follow the grammar. CharRef are not treated as
// entity, only as characters. So they are not handled here but in
// 
{
    // Begin like a reference ?
    if(!parser.ParseChar(TEXT('&')))
        return(NULL);

    parser.PreviousChar();

    // A CharRef ?
    auto_ptr<Reference> pReference(CharRef::Parse(parser, parent));
    if(NULL != pReference.get())
        return(pReference.release());

    // Avoid to use auto_ptr::release here since it's not defined
    // by Microsoft STL.

    // Must be an EntityRef
    // NULL if no EntityRef
    return(EntityRef::Parse(parser, parent));
}

CharRef* CharRef::Parse(Parser& parser, NodeContainer& parent)
// Parse CharRef
// [66] CharRef ::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';' 
// Marker: '&#'
{
    // Begin like a CharRef ?
    if(!parser.ParseString(TEXT("&#")))
        return(NULL);

    // Compute the value (character code)
    int nNum = 0;

    // Hexadecimal ?
    Char c = parser.NextChar();
    if(c == TEXT('x'))
    {
        // Get the value
        if(!parser.ParseHexNumber(nNum))
            parser.SyntaxError(ERROR_INVALID_HEX_NUMBER);
    }
    else // Decimal
    {
        parser.PreviousChar();
        // Get the value
        if(!parser.ParseNumber(nNum))
            parser.SyntaxError(ERROR_INVALID_NUMBER);
    }

    // Check the end of the reference
    if(!parser.ParseChar(TEXT(';')))
        parser.SyntaxError(ERROR_INVALID_REFERENCE);

    if(!IsXmlChar(nNum))
        parser.SyntaxError(ERROR_INVALID_CHAR);

    return(new CharRef(parent, nNum));
}

EntityRef* EntityRef::Parse(Parser& parser, NodeContainer& parent)
// Parse EntityRef
// [68] EntityRef ::= '&' Name ';'
// Marker: '&'
{
    // Begin like a reference ?
    if(!parser.ParseChar(TEXT('&')))
        return(NULL);

    // Get the name of the reference and check the end (';')
    String strReferenceName;
    if(!parser.ParseName(strReferenceName) || !parser.ParseChar(TEXT(';')))
        parser.SyntaxError(ERROR_INVALID_REFERENCE);

    auto_ptr<EntityRef> pRef(new EntityRef(parent, strReferenceName));
    if(!pRef->MapReferenceName())
        parser.SyntaxError(ERROR_UNKNOWN_REFERENCE);

    return(pRef.release());
}


// ===================================================================

}

