// ===================================================================
// AdvXMLParser
//
// See AdvXMLParser.h
// ===================================================================

// ===================================================================
// Helpers
// ===================================================================

inline bool IsSpace(Char c)
// Space, tabulation, line feed or return
{
    return(c == TEXT(' ') || c == TEXT('\t') || c == TEXT('\r') || c == TEXT('\n'));
}

inline bool IsAlpha(Char c)
// [a-zA-Z]
{
    return((c >= TEXT('a') && c <= TEXT('z')) || (c >= TEXT('A') && c <= TEXT('Z')));
}

inline bool IsDigit(Char c)
// For the moment, recognize only Basic Latin
// [0-9]
{
    return(c >= TEXT('0') && c <= TEXT('9'));
}

inline bool IsHexDigit(Char c)
// [0-9a-fA-F]
{
    return(IsDigit(c) || (c >= TEXT('a') && c <= TEXT('f')) || (c >= TEXT('A') && c <= TEXT('F')));
}

inline int HexDigitValue(Char c)
// [0-9a-fA-F]
{
    return((c >= TEXT('0') && c <= TEXT('9')) ? c - TEXT('0')
        : ((c >= TEXT('a') && c <= TEXT('f')) ? c - TEXT('a') + 10
        : c - TEXT('A') + 10));
        
}

inline bool IsAlphaDigit(Char c)
// [a-zA-Z0-9]
{
    return(IsAlpha(c) || IsDigit(c));
}

inline bool IsLetter(Char c_)
// For the moment, recognize only Basic Latin and Latin-1 Supplement
{
    // if c is signed, c >= 0xD8 is always false, ... so cast it
    unsigned char c = c_;

    return
    (
        (c >= 0x41 && c <= 0x5A) || // [A-Z]
        (c >= 0x61 && c <= 0x7A) || // [a-z]
        (c >= 0xC0 && c <= 0xD6) || // Latin-1 Supplement
        (c >= 0xD8 && c <= 0xF6) || // "
        (c >= 0xF8 && c <= 0xFF)    // "
    );
}

inline bool IsCombiningChar(Char c)
// For the moment, not recognized
{
    return(false);
}

inline bool IsExtender(Char c_)
// For the moment, not recognized except 0xB7 (middle point)
{
    // if c is signed, c >= 0xD8 is always false, ... so cast it
    unsigned char c = c_;
    return(c == 0xB7);
}

inline Char LowCase(Char c)
{
    return(c >= TEXT('A') && c <= TEXT('Z') ? static_cast<Char>(c - TEXT('A') + TEXT('a')) : c);
}

inline bool IsNameChar(Char c)
// [4] NameChar ::=  Letter | Digit | '.' | '-' | '_' | ':' | CombiningChar | Extender 
{
    return
    (
        IsLetter(c) || IsDigit(c) ||
        c == TEXT('.') || c == TEXT('-') || c == TEXT('_') || c == TEXT(':') ||
        IsCombiningChar(c) || IsExtender(c)
    );
}
// ===================================================================

inline bool IsXmlChar(Char c)
{
    return(c == 0x9 || c == 0xa || c == 0xd  || c >= 0x20);
}


// ===================================================================
// Document - whole XML document
// ===================================================================

inline Element& Document::GetRoot() const
{
    return(m_pRoot == NULL ? Element::null : *m_pRoot);
}

// ===================================================================
// ParsingException
// ===================================================================

inline ParsingException::ParsingException(PARSER_ERROR nError, int nLine, int nColumn)
:   m_nError(nError),
    m_nLine(nLine),
    m_nColumn(nColumn)
{
}

inline int ParsingException::GetLine() const
{
    return(m_nLine);
}

inline int ParsingException::GetColumn() const
{
    return(m_nColumn);
}

// ===================================================================
// Bookmark
// ===================================================================

inline Bookmark::Bookmark(Parser& reader)
:   m_reader(reader),
    m_szSourceCurrent(reader.m_szSourceCurrent),
    m_nLine(reader.m_nLine),
    m_nColumn(reader.m_nColumn)
{
}

inline void Bookmark::Restore()
{
    m_reader.m_szSourceCurrent  = m_szSourceCurrent;
    m_reader.m_nLine            = m_nLine;
    m_reader.m_nColumn          = m_nColumn;
}

inline void Bookmark::GetSubString(String& strString, int nNumEndSkip)
{
    ASSERT(m_reader.m_szSourceCurrent + nNumEndSkip >= m_szSourceCurrent);
    strString = String(m_szSourceCurrent, m_reader.m_szSourceCurrent - m_szSourceCurrent - nNumEndSkip);
}

inline void Bookmark::Reset()
{
    m_szSourceCurrent   = m_reader.m_szSourceCurrent;
    m_nLine             = m_reader.m_nLine;
    m_nColumn           = m_reader.m_nColumn;
}

// ===================================================================


