
#ifndef ADVXMLPARSER_NO_WRITE

// ===================================================================
// GenerateContext - Context of a generation of XML (see GenerateXML)
// ===================================================================

inline bool GenerateContext::MustPreserve() const
{
    return(m_nOptions & GENERATE_PRESERVE);
}

inline void GenerateContext::operator+=(const String& strXml)
{
    m_bNewLine = false;
    m_strXml += strXml;
}

inline void GenerateContext::operator+=(const Char* szXml)
{
    m_bNewLine = false;
    m_strXml += szXml;
}

inline void GenerateContext::operator+=(Char c)
{
    m_bNewLine = false;
    m_strXml += c;
}

// ===================================================================
// Document - whole XML document
// ===================================================================

inline void Document::GenerateXML(String& strXML) const
{
    GenerateContext context(strXML);
    GenerateXML(context);
}

inline String Document::GenerateXML() const
{
    String strXML;
    GenerateXML(strXML);
    return(strXML);
}

#endif // ADVXMLPARSER_NO_WRITE
