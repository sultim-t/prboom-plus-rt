// ===================================================================
// AdvXMLParser
//
// See AdvXMLParser.h
// ===================================================================

// ===================================================================
// Value of Elements
// ===================================================================

inline Value::Value(NodeContainer& node)
:   m_node(node)
{
}

inline String Value::Get() const
{
    return(m_node.GetValue());
}

inline void Value::Add(const Char* szValue)
{
    m_node.AddText(szValue);
}

inline void Value::Set(const Char* szValue)
{
    m_node.DeleteChildren();
    m_node.AddText(szValue);
}

// ===================================================================

inline ConstValue::ConstValue(const NodeContainer& node)
:   m_node(node)
{
}

inline String ConstValue::Get() const
{
    return(m_node.GetValue());
}

// ===================================================================
// Node
// ===================================================================

inline bool Node::IsNull() const
{
    // Note: Node::null and Element::null are the same
    return
    (
        this == &Attribute::null    || 
        this == &Element::null      ||
        this == &Comment::null      ||
        this == &Pi::null           ||
        this == &CData::null
    );
}

inline const String& Node::GetName() const
{
    return(m_strName);
}

inline NodeContainer& Node::GetParent() const
{
    return(m_parent);
}

// ===================================================================
// Text - Textual content
// ===================================================================

inline void Text::Concatenate(const String& strText)
{
    m_strText += strText;
}

// ===================================================================
// Attribute of Elements
// ===================================================================

inline Attribute::operator ConstValue() const
{
	// Can't use an anonymous instance because some compilers (for ex.
	// Borland C++ 5.5.1 generates invalid code)
	ConstValue value(advself);
    return(value);
}

inline Attribute::operator ConstValue()
{
	// Can't use an anonymous instance because some compilers (for ex.
	// Borland C++ 5.5.1 generates invalid code)
	ConstValue value(advself);
    return(value);
}

inline Attribute::operator Value()
{
	// Can't use an anonymous instance because some compilers (for ex.
	// Borland C++ 5.5.1 generates invalid code)
	Value value(advself);
    return(value);
}

// ===================================================================
// Element
// ===================================================================

inline Element::operator ConstValue() const
{
	// Can't use an anonymous instance because some compilers (for ex.
	// Borland C++ 5.5.1 generates invalid code)
	ConstValue value(advself);
    return(value);
}

inline Element::operator ConstValue()
{
	// Can't use an anonymous instance because some compilers (for ex.
	// Borland C++ 5.5.1 generates invalid code)
	ConstValue value(advself);
    return(value);
}

inline Element::operator Value()
{
	// Can't use an anonymous instance because some compilers (for ex.
	// Borland C++ 5.5.1 generates invalid code)
	Value value(advself);
    return(value);
}

// ===================================================================

inline const Element& Element::operator()(Element::size_type nIndex) const
{
    return(GetElement(nIndex));
}

inline const Element& Element::operator()(const Char* szName, Element::size_type nIndex) const
{
    return(GetElement(szName, nIndex));
}

inline const Attribute& Element::operator[](const Char* szName) const
{
    return(GetAttribute(szName));
}

inline const Attribute& Element::operator[](Element::size_type nIndex) const
{
    return(GetAttribute(nIndex));
}

inline const Element& Element::GetElement(const Char* szName, Element::size_type nIndex) const
{
    Node& node = GetChild(szName, nIndex, ELEMENT_NODE);
    return(node.IsNull() ? Element::null : static_cast<Element&>(node));
}

// ===================================================================

inline Element& Element::operator()(const Char* szName, Element::size_type nIndex)
{
    return(CreateElement(szName, nIndex));
}

inline Attribute& Element::operator[](const Char* szName)
{
    return(CreateAttribute(szName));
}

inline const Element& Element::GetElement(Element::size_type nIndex) const
{
    Node& node = GetChild(nIndex, ELEMENT_NODE);
    return(node.IsNull() ? Element::null : static_cast<Element&>(node));
}

inline Element& Element::GetElement(const Char* szName, Element::size_type nIndex)
{
    Node& node = GetChild(szName, nIndex, ELEMENT_NODE);
    return(node.IsNull() ? Element::null : static_cast<Element&>(node));
}

inline Element& Element::GetElement(Element::size_type nIndex)
{
    Node& node = GetChild(nIndex, ELEMENT_NODE);
    return(node.IsNull() ? Element::null : static_cast<Element&>(node));
}

// ===================================================================
// Get Values
// ===================================================================

inline void operator<<(String& strValue, const ConstValue& value)
{
    strValue = value.Get();
}

inline void operator<<(String& strValue, const Node& node)
{
    strValue = node.GetValue();
}

// You can define these functions ourself if you prefer
#ifndef ADVXMLPARSER_NO_CONVERSION

inline void operator<<(int& nValue, const ConstValue& value)
{
    nValue = atoi(value.Get().c_str());
}

inline void operator<<(unsigned int& nValue, const ConstValue& value)
{
    nValue = atoi(value.Get().c_str());
}

inline void operator<<(double& dValue, const ConstValue& value)
{
    dValue = atof(value.Get().c_str());
}

#endif // ADVXMLPARSER_NO_CONVERSION

// ===================================================================
// Set Values
// ===================================================================

inline void operator<<(Value value, const Char* szValue)
{
    value.Set(szValue);
}

