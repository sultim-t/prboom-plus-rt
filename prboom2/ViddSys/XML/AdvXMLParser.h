// ===================================================================
// AdvXMLParser
// -------------------------------------------------------------------
// Version:     1.1.4
// Date:        November 19, 2000
// OS:          Windows 2000 SP1
// Compiler:    Microsoft Visual C++ 6.0 SP4
// STL:         STLport 4.0
// -------------------------------------------------------------------
// Sebastien Andrivet grants Licensee a non-exclusive, non-transferable, 
// royalty-free license to use AdvXMLParser and its documentation (the
// 'Software') without fee.
// 
// By downloading, using, or copying the Software or any portion thereof, 
// Licensee agrees to abide by the intellectual property laws and all other 
// applicable laws, and to all of the terms and conditions of this Agreement.
// 
// Licensee shall maintain the following copyright and permission notices on 
// the Software sources and its documentation unchanged :
// 
// Copyright © 1999,2000 Sebastien Andrivet
// 
// THE SOFTWARE IS PROVIDED ''AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL SEBASTIEN ANDRIVET BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// 
// Permission to use or copy this software for any purpose is hereby granted
// without fee, provided the above notices are retained on all copies. 
// Permission to modify the code and to distribute modified code is granted,
// provided the above notices are retained, and a notice that the code was 
// modified is included with the above copyright notice. 
// 
// The Licensee may distribute binaries compiled with the Software (whether 
// original or modified) without any royalties or restrictions.
// 
// The Licensee may distribute original or modified the Software sources,
// provided that the conditions indicated in the above permission notice are met.
// 
// Except as contained in this notice, the name of Sebastien Andrivet
// shall not be used in advertising or otherwise to promote the sale, 
// use or other dealings in this Software without prior written 
// authorization from Sebastien Andrivet.
// ===================================================================

#ifndef INC_ADVXMLPARSER_H
#define INC_ADVXMLPARSER_H

#include "AdvXMLParserDefs.h"           // Some usefull macros
#include "AdvXMLParserConfig.h"         // Customization and parameters
#include "AdvXMLParserPrimitiveTypes.h" // Like vector, string, etc...

// ===================================================================
// Define our own namespace

namespace AdvXMLParser
{

// ===================================================================
// Forward declarations

class Node;
class NodeNull;
class NodeContainer;
class Attribute;
class Element;
class Comment;
class Pi;
class CData;
class Document;
class Parser;
class Bookmark;

typedef advstd::vector<Attribute*>  Attributes;     // Array of Attributes
typedef advstd::vector<Element*>    Elements;       // Array of Elements
typedef advstd::vector<Node*>       Nodes;          // Array of Nodes
typedef advstd::basic_string<Char>  String;         // String of characters
typedef advstd::map<String, Char>   MapReferences;  // Map references

// ===================================================================
// Different type of nodes

enum
{
    NULL_NODE                   = 0,
    ELEMENT_NODE                = 1,
    ATTRIBUTE_NODE              = 2,
    TEXT_NODE                   = 3,
    CDATA_SECTION_NODE          = 4,
    ENTITY_REFERENCE_NODE       = 5,
    ENTITY_NODE                 = 6,
    PROCESSING_INSTRUCTION_NODE = 7,
    COMMENT_NODE                = 8,
    DOCUMENT_NODE               = 9,
    DOCUMENT_TYPE_NODE          = 10,
    DOCUMENT_FRAGMENT_NODE      = 11,
    NOTATION_NODE               = 12
};

typedef int NODE_TYPE;

// ===================================================================
// Options for generation of XML

enum
{
    GENERATE_INDENTED = 0x0000,  // Try to produce nice XML with indentation
    GENERATE_PRESERVE = 0x0001   // Don't touch the XML. Generate what we have, nothing more
};

typedef unsigned long GENERATE;

// ===================================================================
// Value
// -------------------------------------------------------------------
// Instead of define a set of operators << and >> to manipulate the
// value of a Node (Attribute, Element), each appropriate Node is
// convertible to Value and only one set of operators is neccessary.
// ===================================================================

class Value
{
public:
    explicit Value(NodeContainer& node);

    // Get the value as a string
    String Get() const;
    // Add a string to the current value
    void Add(const Char* szValue);
    // Replace the value by the given string
    void Set(const Char* szValue);

private:
    // For the moment works only for NodeContainer but may change in
    // the future
    NodeContainer& m_node;
};

// ===================================================================
// ConstValue
// -------------------------------------------------------------------
// Same as Value but const (can't be modified)
// ===================================================================

class ConstValue
{
public:
    explicit ConstValue(const NodeContainer& node);
    // Get the value as a string
    String Get() const;

private:
    // For the moment works only for NodeContainer but may change in
    // the future
    const NodeContainer& m_node;
};

// ===================================================================
// GenerateContext
// -------------------------------------------------------------------
// Used to generate XML. It encapsulate the string generated and
// the current state of the generation (indentation, ...)
// ===================================================================

#ifndef ADVXMLPARSER_NO_WRITE // Only if writing is enabled

class GenerateContext
{
public:
    GenerateContext(String& strXml, const Char* szIndentation = NULL, GENERATE nOptions = GENERATE_INDENTED);
    GenerateContext(const GenerateContext& context);

    void operator=(const GenerateContext& context);

    // Add some text to the XML
    void operator+=(const String& strXml);
    void operator+=(const Char* szXml);
    void operator+=(Char c);

    // Produce nice, indented XML or generate exactly what's in the nodes ?
    bool MustPreserve() const;
    // Generate indentation for the start tag (only if indentation is enabled)
    void GenerateStartTagIndentation();
    // Generate indentation for the end tag (only if indentation is enabled)
    void GenerateEndTagIndentation();
    // Indicate that we have reach an end tag but don't generate indentation
    void EndTag();
    // Generate a new line if neccessary
    void GenerateNewLine();

private:
    // Helper : generate indentation
    void GenerateIndentation();

private:
    GENERATE    m_nOptions;         // Options for the generation (see GENERATE)
    const Char* m_szIndentation;    // Text used for the indentation. By def. 2 spaces
    String&     m_strXml;           // Text generated (given by the user of this class)
    int         m_nLevel;           // Current level of identation
    bool        m_bEndTag;          // Last tag was an end tag ?
    bool        m_bNewLine;         // Last generation was a new line ?
};

#endif // ADVXMLPARSER_NO_WRITE

// ===================================================================
// Node (abstract)
// -------------------------------------------------------------------
// Single node in the document tree. Root of the hierarchy
// ===================================================================

class Node
{
public:
    // The null Node
    static NodeContainer& null;

public:
    Node(NodeContainer& parent, const String& strName);
    virtual ~Node();

    // Get the name of the node
    const String& GetName() const;
    // Is this node Null (equal to one of the null) ?
    bool IsNull() const;
    // Get the parent of this node
    NodeContainer& GetParent() const;

    // Delete this node (and remove it from the children of its parent)
    void Delete();

    // Get the raw data if any
    virtual String GetData() const = 0;
    // Get the value as a string after normalization
    virtual String GetValue() const = 0;
    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const = 0;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const = 0;

#ifndef ADVXMLPARSER_NO_WRITE
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const = 0;
#endif // ADVXMLPARSER_NO_WRITE

public:
    // ===============================================================
    // IteratorRef & ConstIteratorRef
    // ---------------------------------------------------------------
    // Used only to initialize Iterators.
    // 
    // Some nodes contain a collection of other nodes (Element, ...).
    // To enumerate their children, they return iterator. These
    // iterators are template to be able to enumerate only a type
    // of node (for ex. Element). IteratorRef is the link between
    // these templates and Node (or a sub-class of Node).
    // ---------------------------------------------------------------
    // I can't declare these IteratorRef and ConstIteratorRef as private
    // because some compilers (MSVC++ 6 in in particular) are not able 
    // to recognize template friends.
    // ===============================================================

    struct IteratorRef
    {
        IteratorRef(Nodes& nodes, Nodes::iterator it);

        Nodes& m_nodes;
        Nodes::iterator m_it;
    };

    struct ConstIteratorRef
    {
        ConstIteratorRef(const Nodes& nodes, Nodes::const_iterator it);

        const Nodes& m_nodes;
        Nodes::const_iterator m_it;
    };

protected:
    const String    m_strName;  // Name of the node
    NodeContainer&  m_parent;   // Parent of this node
};

// ===================================================================
// Text
// -------------------------------------------------------------------
// Textual content. For example to represent the text of an element,
// an attribute, ...
// ===================================================================

class Text : public Node
{
    typedef Node Super; // Superclass ("a la" Smalltalk)

public:
    // Type of this node
    enum { TYPE = TEXT_NODE };

public:
    explicit Text(const String& strText);
    Text(NodeContainer& parent, const String& strText);
    // Type-safe cloning of this node (not virtual)
    Text* Clone(NodeContainer& parent) const;

    // Add some text to this node
    void Concatenate(const String& strText);

    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const;
    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

#ifndef ADVXMLPARSER_NO_WRITE
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

private:
    String m_strText;   // Textual content
};

// ===================================================================
// NodeContainer (abstract)
// -------------------------------------------------------------------
// Node that contains other nodes (Element, Attribute, Document,...)
// ===================================================================

class NodeContainer : public Node
{
    typedef Node Super; // Superclass ("a la" Smalltalk)
    friend class Element;

public:
	typedef Nodes::size_type size_type;

public:
    NodeContainer(NodeContainer& parent, const String& strName);
    virtual ~NodeContainer();

    // ---------------------------------------------------------------
    // Iterators to enumerate the children
    // ---------------------------------------------------------------
    // First child
    ConstIteratorRef Begin() const;
    IteratorRef Begin();
    // After last child
    ConstIteratorRef End() const;
    IteratorRef End();

    // ---------------------------------------------------------------
    // Empty
    // ---------------------------------------------------------------
    // Is empty or has children ?
    bool IsEmpty() const;

    // ---------------------------------------------------------------
    // Add nodes
    // ---------------------------------------------------------------
    // Add the given text (either concatenate with the previous
    // Text node  or create a Text node)
    void AddText(const String& strText);
    // Add the text given by this bookmark (used by the parser)
    void AddText(Bookmark& bookmark, int nNbCharToSkip = 0);

    // ---------------------------------------------------------------
    // Delete nodes
    // ---------------------------------------------------------------
    // Remove all children and delete them
    void DeleteChildren();
    // Remove the given node and delete it
    void DeleteChild(const Node& node);

protected:
    // ---------------------------------------------------------------
    // Search a node matching some criteria
    // ---------------------------------------------------------------
    // Get the nth child
	Node& GetChild(size_type nIndex) const;
    // Get the nth child with the given type
	Node& GetChild(size_type nIndex, NODE_TYPE nType) const;
    // Get the nth child called szName and with the given type
	Node& GetChild(const Char* szName, size_type nIndex, NODE_TYPE nType) const;

    // ---------------------------------------------------------------
    // Add nodes
    // ---------------------------------------------------------------
    // Add a node
    void Add(Node* pNode);
    // Add a node (sibling) before another node
    void InsertBefore(const Node& node, Node* pNodeToAdd);
    // Add a node (sibling) after another node
    void InsertAfter(const Node& node, Node* pNodeToAdd);

    // ---------------------------------------------------------------
    // Cloning
    // ---------------------------------------------------------------
    // Clone the nodes of this node and put them in the given parameter
    void CloneChildren(NodeContainer& node) const;

    // ---------------------------------------------------------------
    // Delete nodes
    // ---------------------------------------------------------------
    // Delete the node at the given position
    void DeleteChild(Nodes::iterator it);

    // ---------------------------------------------------------------
    // Search a node matching some criteria (return an iterator)
    // ---------------------------------------------------------------
    // Find a child of the given type starting at it
    bool FindChild(NODE_TYPE nType, Nodes::iterator& it) const;
    // Find a child called szName starting at it
    bool FindChild(const Char* szName, Nodes::iterator& it) const;
    // Find a child called szName of the given type starting at it
    bool FindChild(const Char* szName, NODE_TYPE nType, Nodes::iterator& it) const;
    // Find a child starting at it
    bool FindChild(const Node& node, Nodes::iterator& it) const;

    // ---------------------------------------------------------------
    // Search a node matching some criteria (return a const iterator)
    // ---------------------------------------------------------------
    // Find a child of the given type starting at it
    bool FindChild(NODE_TYPE nType, Nodes::const_iterator& it) const;
    // Find a child called szName starting at it
    bool FindChild(const Char* szName, Nodes::const_iterator& it) const;
    // Find a child called szName of the given type starting at it
    bool FindChild(const Char* szName, NODE_TYPE nType, Nodes::const_iterator& it) const;
    // Find a child starting at it
    bool FindChild(const Node& node, Nodes::const_iterator& it) const;

    // ---------------------------------------------------------------
    // XML Generation
    // ---------------------------------------------------------------
#ifndef ADVXMLPARSER_NO_WRITE // Only if writing is enabled
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

protected:
    Nodes   m_children;     // Children of the node
    Text*   m_pLastText;    // Not NULL if last node was a Text
};

// ===================================================================
// Attribute
// -------------------------------------------------------------------
// Each element can have one or more attributes. Each attribute can
// itself contain text and/or references.
// ===================================================================

class Attribute : public NodeContainer
{
    typedef NodeContainer Super; // Superclass ("a la" Smalltalk)

public:
    // Type of this node
    enum { TYPE = ATTRIBUTE_NODE };
    // The (unique) null Attribute
    static Attribute null;

public:
    Attribute(NodeContainer& parent, const String& strName);
    // Type-safe cloning of this node (not virtual)
    Attribute* Clone(NodeContainer& parent) const;

    // Try to parse and create this node
    static Attribute* Parse(Parser& parser, Element& parent);
    // Parse the value of this attribute if any
    bool ParseAttValue(Parser& parser);

    // Attributes can be converted to Value or ConstValue
    operator ConstValue() const;
    operator ConstValue();
    operator Value();

    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const; // Get the value
    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

#ifndef ADVXMLPARSER_NO_WRITE // Only if writing is enabled
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

public:
    // ===============================================================
    // IteratorRef & ConstIteratorRef
    // ---------------------------------------------------------------
    // Used only to initialize Iterators.
    // 
    // Some nodes contain a collection of other nodes (Element, ...).
    // To enumerate their children, they return iterator. These
    // iterators are template to be able to enumerate only a type
    // of node (for ex. Element). IteratorRef is the link between
    // these templates and Node (or a sub-class of Node).
    // ---------------------------------------------------------------
    // I can't declare these IteratorRef and ConstIteratorRef as private
    // because some compilers (MSVC++ 6 in in particular) are not able 
    // to recognize template friends.
    // ===============================================================

    struct IteratorRef
    {
        IteratorRef(Attributes::iterator it);
        Attributes::iterator m_it;
    };

    struct ConstIteratorRef
    {
        ConstIteratorRef(Attributes::const_iterator it);
        Attributes::const_iterator m_it;
    };
};

// ===================================================================
// References
// -------------------------------------------------------------------
// Some nodes (like Elements, Attributes) may contain references 
// (character references, general entity references).
// ===================================================================

class Reference : public Node
{
    typedef Node Super; // Superclass ("a la" Smalltalk)

public:
    // Type of this node
    enum { TYPE = ENTITY_REFERENCE_NODE };

public:
    Reference(NodeContainer& parent, const String& strName);

    // Try to parse and create this node
    static Reference* Parse(Parser& parser, NodeContainer& parent);

    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const;
};

// ===================================================================
// CharRef
// -------------------------------------------------------------------
// Character Reference: Encoding of a single character.
// Note: for the moment this kind of reference is part of the tree
// representing a document. This may change in the future. In general,
// character reference are parsed and added as text, not as an object.
// ===================================================================

class CharRef : public Reference
{
    typedef Reference Super; // Superclass ("a la" Smalltalk)

public:
    explicit CharRef(int nNum);
    CharRef(NodeContainer& parent, int nNum);
    // Type-safe cloning of this node (not virtual)
    CharRef* Clone(NodeContainer& parent) const;

    // Try to parse and create this node
    static CharRef* Parse(Parser& parser, NodeContainer& parent);

    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

#ifndef ADVXMLPARSER_NO_WRITE
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

private:
    Char m_c; // Character represented by this reference
};

// ===================================================================
// EntityRef
// -------------------------------------------------------------------
// General Entity Reference.
// Note: for the moment, I only support predefined entities (that
// represent single characters). In the future, it will change and
// this class will become a container (instead of having m_c).
// ===================================================================

class EntityRef : public Reference
{
    typedef Reference Super; // Superclass ("a la" Smalltalk)

public:
    // Use this constructor to build a standalone entity reference
    explicit EntityRef(const String& strName);
    EntityRef(NodeContainer& parent, const String& strName);
    // Type-safe cloning of this node (not virtual)
    EntityRef* Clone(NodeContainer& parent) const;

    // Try to parse and create this node
    static EntityRef* Parse(Parser& parser, NodeContainer& parent);

    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

#ifndef ADVXMLPARSER_NO_WRITE
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

private:
    bool MapReferenceName();

private:
    Char m_c; // Character represented by this reference (see Note)
};

// ===================================================================
// Element
// -------------------------------------------------------------------
// Represent an Element of the document (Start Tag / End Tag). It
// contains children (nodes) and also attributes
// ===================================================================

class Element : public NodeContainer
{
    typedef NodeContainer Super; // Superclass ("a la" Smalltalk)

public:
    // Type of this node
    enum { TYPE = ELEMENT_NODE };
    // The (unique) null Element
    static Element null;

	typedef NodeContainer::size_type size_type;

public:
    explicit Element(const String& strName);
    Element(NodeContainer& parent, const String& strName);
    virtual ~Element();
    // Type-safe cloning of this node (not virtual)
    Element* Clone(NodeContainer& parent) const;

    // ---------------------------------------------------------------
    // Parsing (internal use only)
    // ---------------------------------------------------------------
    // Parse element and construct an object
    static Element* Parse(Parser& parser, NodeContainer& parent);

    // ---------------------------------------------------------------
    // Value
    // ---------------------------------------------------------------
    // Elements can be converted to Value or ConstValue
    operator ConstValue() const;
    operator ConstValue();
    operator Value();

    // ---------------------------------------------------------------
    // Access sub-elements
    // ---------------------------------------------------------------
    // Has sub-elements (children of type Element)
    bool HasElements() const;
    // Get the nth element called szName
	const Element& GetElement(const Char* szName, size_type nIndex = 0) const;
    // Get the nth element
	const Element& GetElement(size_type nIndex) const;
    // Get the nth element called szName
	Element& GetElement(const Char* szName, size_type nIndex = 0);
    // Get the nth element
	Element& GetElement(size_type nIndex);

    // Get the nth element called szName
    Element& operator()(const Char* szName, size_type nIndex = 0);
    // Get the nth element
    const Element& operator()(size_type nIndex) const;
    // Get the nth element named szName
    const Element& operator()(const Char* szName, size_type nIndex = 0) const;

    // ---------------------------------------------------------------
    // Create sub-elements (internal use only)
    // ---------------------------------------------------------------
    // Create an element called szName at the given position
    Element& CreateElement(const Char* szName, size_type nIndex = 0);
    // Add a element (child of this element)
    Element& AddElementInto(const Char* szName);
    // Append an element at the end (sibling of this element)
    Element& AppendElement(const Char* szName);
    // Insert an element after this element (sibling)
    Element& InsertElementAfter(const Char* szName);
    // Insert an element before this element (sibling)
    Element& InsertElementBefore(const Char* szName);

    // ---------------------------------------------------------------
    // Delete sub-elements
    // ---------------------------------------------------------------
    // Delete a given element
    bool DeleteChildElement(const Char* szName, size_type nIndex = 0);

    // ---------------------------------------------------------------
    // Enumerate Attributes
    // ---------------------------------------------------------------
    // First attribute
    Attribute::ConstIteratorRef AttributesBegin() const;
    Attribute::IteratorRef AttributesBegin();
    // After last attribute
    Attribute::ConstIteratorRef AttributesEnd() const;
    Attribute::IteratorRef AttributesEnd();

    // ---------------------------------------------------------------
    // Access attributes
    // ---------------------------------------------------------------
    // Get the attribute with the given name if any
	const Attribute& GetAttribute(const Char* szName) const;
    // Get the nth attribute if any
	const Attribute& GetAttribute(size_type nIndex) const;
    // Get the attribute with the given name if any
	Attribute& GetAttribute(const Char* szName);
    // Get the nth attribute if any
	Attribute& GetAttribute(size_type nIndex);

    // Get the attribute with the given name if any
    Attribute& operator[](const Char* szName);
    // Get the nth attribute
    Attribute& operator[](size_type nIndex);
    // Get the nth attribute named szName
    const Attribute& operator[](const Char* szName) const;
    // Get the nth attribute
    const Attribute& operator[](size_type nIndex) const;

    // ---------------------------------------------------------------
    // Create attributes (internal use only)
    // ---------------------------------------------------------------
    // Create an attribute with the given name
	Attribute& CreateAttribute(const Char* szName);

    // ---------------------------------------------------------------
    // Delete attributes
    // ---------------------------------------------------------------
    // Delete the attribute with the given name if any
    bool DeleteAttribute(const Char* szName);

    // ---------------------------------------------------------------
    // Access Comments
    // ---------------------------------------------------------------
    // Get the nth comment (child of this element)
    const Comment& GetComment(size_type nIndex) const;
    Comment& GetComment(size_type nIndex);

    // ---------------------------------------------------------------
    // Access CDATA sections
    // ---------------------------------------------------------------
    // Get the nth CDATA section (child of this element)
    const CData& GetCData(size_type nIndex) const;
    CData& GetCData(size_type nIndex);

    // ---------------------------------------------------------------
    // Access Processing instructions
    // ---------------------------------------------------------------
    // Get the nth Processing Instruction with the given name (child)
    const Pi& GetPi(const Char* szTarget, size_type nIndex = 0) const;
    // Get the nth Processing Instruction (child of this element)
    const Pi& GetPi(size_type nIndex) const;
    // Get the nth Processing Instruction with the given name (child)
    Pi& GetPi(const Char* szTarget, size_type nIndex = 0);
    // Get the nth Processing Instruction (child of this element)
    Pi& GetPi(size_type nIndex);

    // ---------------------------------------------------------------
    // Create sub-nodes (Comment, CDATA sections, ...)
    // ---------------------------------------------------------------
    // Why this ugly #include here in the middle of the class ?
    // Because currently Microsoft Visual C++ is not able to support template
    // members that are not declared inlined (see the readme of VC).
    // I don't want to put lot of code here so I choose this solution...
#include "TemplateMembers.inl"
    // This #include defines:
    // template<class N> N& AddInto(const N& node);
    // template<class N> N& Append(const N& node);
    // template<class N> N& InsertBefore(const N& node);
    // template<class N> N& InsertAfter(const N& node);

    // ---------------------------------------------------------------
    // Overloaded (virtual)
    // ---------------------------------------------------------------
    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const; // Get the value
    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

    // ---------------------------------------------------------------
    // XML Generation
    // ---------------------------------------------------------------
#ifndef ADVXMLPARSER_NO_WRITE // Only if writing is enabled
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

private:
    // Parse start tag and construct an element
    static Element* ParseTagBegining(Parser& parser, bool& bEmptyTag, NodeContainer& parent);
    // Parse end tag
    bool ParseETag(Parser& parser);
    // Parse element content and end tag
    void ParseContentETag(Parser& parser);
    void ParseReference(Parser& parser);
    // Parse markups like Comment, CDATA, element, or PI
    bool ParseMarkup(Parser& parser);
    // Special treatment for predefined attributes
    void HandleSpecialAttributes(const Attribute* pAttribute);

private:
    Attributes  m_attributes;   // Attributes
    bool        m_bPreserveWS;  // True if has attribute xml:space with value 'preserve'
};

// ===================================================================
// Comment
// ===================================================================

class Comment : public Node
{
    typedef Node Super; // Superclass ("a la" Smalltalk)

public:
    // Type of this node
    enum { TYPE = COMMENT_NODE };
    // The (unique) null Comment
    static Comment null;

public:
    explicit Comment(const String& strComment);
    Comment(NodeContainer& parent, const String& strComment);
    // Type-safe cloning of this node (not virtual)
    Comment* Clone(NodeContainer& parent) const;

    // Parse the Comment and create an object
    static Comment* Parse(Parser& parser, NodeContainer& parent);

    // ---------------------------------------------------------------
    // Overloaded (virtual)
    // ---------------------------------------------------------------
    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const;
    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

    // ---------------------------------------------------------------
    // XML Generation
    // ---------------------------------------------------------------
#ifndef ADVXMLPARSER_NO_WRITE
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

private:
    String  m_strComment;   // The comment itself
};

// ===================================================================
// Processing instruction
// ===================================================================

class Pi : public Node
{
    typedef Node Super; // Superclass ("a la" Smalltalk)

public:
    // Type of this node
    enum { TYPE = PROCESSING_INSTRUCTION_NODE };
    // The (unique) null Comment
    static Pi null;

public:
    Pi(const String& strTarget, const String& strInstruction);
    Pi(NodeContainer& parent, const String& strTarget, const String& strInstruction);
    // Type-safe cloning of this node (not virtual)
    Pi* Clone(NodeContainer& parent) const;

    // Parse the Instruction and create an object
    static Pi* Parse(Parser& parser, NodeContainer& parent);

    // ---------------------------------------------------------------
    // Overloaded (virtual)
    // ---------------------------------------------------------------
    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const;
    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

    // ---------------------------------------------------------------
    // XML Generation
    // ---------------------------------------------------------------
#ifndef ADVXMLPARSER_NO_WRITE
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

private:
    // Parse the target name of the Processing instruction
    static bool ParsePITarget(Parser& parser, String& strTarget);

private:
    String  m_strInstruction;   // The instruction itself
};

// ===================================================================
// CDATA section
// ===================================================================

class CData : public Node
{
    typedef Node Super; // Superclass ("a la" Smalltalk)

public:
    // Type of this node
    enum { TYPE = CDATA_SECTION_NODE };
    // The (unique) null Comment
    static CData null;

public:
    explicit CData(const String& strSection);
    CData(NodeContainer& parent, const String& strSection);
    // Type-safe cloning of this node (not virtual)
    CData* Clone(NodeContainer& parent) const;

    // Parse the Section and create an object
    static CData* Parse(Parser& parser, NodeContainer& parent);

    // ---------------------------------------------------------------
    // Overloaded (virtual)
    // ---------------------------------------------------------------
    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const;
    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

    // ---------------------------------------------------------------
    // XML Generation
    // ---------------------------------------------------------------
#ifndef ADVXMLPARSER_NO_WRITE
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
#endif // ADVXMLPARSER_NO_WRITE

private:
    String  m_strSection;   // Content of the section
};

// ===================================================================
// Document Type Definition (DTD)
// -------------------------------------------------------------------
// Important node: currently, even if the code is here (for the
// internal subset), the DTD is not parsed. This is because I have
// not tested this code well. It will work in a next Beta release.
// ===================================================================

#if ADVXMLPARSER_DTD != 0

class Dtd
{
public:
    explicit Dtd(Document& document);

    bool ParseDoctypedecl(Parser& parser);
    bool ParsePEReference(Parser& parser);
    bool ParseSystemLiteral(Parser& parser);
    bool ParsePubidLiteral(Parser& parser);
    bool ParseMarkupdecl(Parser& parser);
    bool ParseElementDecl(Parser& parser);
    bool ParseContentspec(Parser& parser);
    bool ParseMixed(Parser& parser);
    bool ParseChildren(Parser& parser);
    bool ParseChoiceSeq(Parser& parser);
    bool ParseCp(Parser& parser);
    bool ParseAttlistDecl(Parser& parser);
    bool ParseAttDef(Parser& parser);
    bool ParseAttType(Parser& parser);
    bool ParseNotationType(Parser& parser);
    bool ParseEnumeration(Parser& parser);
    bool ParseDefaultDecl(Parser& parser);
    bool ParseEntityDecl(Parser& parser);
    bool ParseEntityDef(Parser& parser);
    bool ParseNDataDecl(Parser& parser);
    bool ParsePEDef(Parser& parser);
    bool ParseEntityValue(Parser& parser);
    bool ParseNotationDecl(Parser& parser);
    bool ParseExternalID(Parser& parser);
    bool ParsePublicID(Parser& parser);

private:
    Document&   m_document;
};

#endif // ADVXMLPARSER_DTD

// ===================================================================
// Document
// -------------------------------------------------------------------
// Whole XML document. Root of the tree representing the parsed XML
// document.
// ===================================================================

class Document : public NodeContainer
{
    typedef NodeContainer Super; // Superclass ("a la" Smalltalk)

public:
    // Type of this node
    enum { TYPE = DOCUMENT_NODE };

public:
    Document();
    explicit Document(const Char* szRootName);
    // Type-safe cloning of this node (not virtual)
    Document* Clone() const;
    
    // ---------------------------------------------------------------
    // Access nodes in the document
    // ---------------------------------------------------------------
    // Get the root (element)
    Element& GetRoot() const;

    // ---------------------------------------------------------------
    // Access Comment
    // ---------------------------------------------------------------
    // Get the nth comment (child of this element)
    const Comment& GetComment(size_type nIndex) const;
    Comment& GetComment(size_type nIndex);

    // ---------------------------------------------------------------
    // Access CDATA sections
    // ---------------------------------------------------------------
    const CData& GetCData(size_type nIndex) const;
    CData& GetCData(size_type nIndex);

    // ---------------------------------------------------------------
    // Access Processing instructions
    // ---------------------------------------------------------------
    const Pi& GetPi(const Char* szTarget, size_type nIndex = 0) const;
    const Pi& GetPi(size_type nIndex) const;
    Pi& GetPi(const Char* szTarget, size_type nIndex = 0);
    Pi& GetPi(size_type nIndex);

    // ---------------------------------------------------------------
    // Overloaded (virtual)
    // ---------------------------------------------------------------
    // Get the raw data if any
    virtual String GetData() const;
    // Get the value as a string after normalization
    virtual String GetValue() const;
    // Is this node of the given type
    virtual bool IsKindOf(NODE_TYPE nType) const;
    // Clone a node and return a copy with the given parent
    virtual Node* CloneNode(NodeContainer& parent) const;

    // ---------------------------------------------------------------
    // XML Generation
    // ---------------------------------------------------------------
#ifndef ADVXMLPARSER_NO_WRITE
    // Generate XML to represent this node
    virtual void GenerateXML(GenerateContext& xml) const;
    String GenerateXML() const;
    void GenerateXML(String& strXml) const;
#endif // ADVXMLPARSER_NO_WRITE

    // ---------------------------------------------------------------
    // Parsing
    // ---------------------------------------------------------------
    // Parse the document and create an object
    static Document* Parse(Parser& parser);
    // Parse Prolog
    void ParseProlog(Parser& parser);
    // Parse XML declaration (<?xml version="1.0" ... ?>
    bool ParseXMLDecl(Parser& parser);
    // Parse XML version
    bool ParseVersionInfo(Parser& parser, String& strVersion);
    // Parse XML version number
    bool ParseVersionNum(Parser& parser, String& strVersion);
    // Parse XML encoding declaration
    bool ParseEncodingDecl(Parser& parser, String& strEncoding);
    // Parse encoding name
    bool ParseEncName(Parser& parser, String& strEncoding);
    // Parse Standalone declaration
    bool ParseSDDecl(Parser& parser, bool& bStandalone);
    // Parse Comments, spaces, etc.
    bool ParseMisc(Parser& parser);
    // Parse several Misc
    void ParseMiscs(Parser& parser);

private:
    // Parse the root element
    void ParseRootElement(Parser& parser);

private:
    String      m_strXmlVersion;    // Version of the spec found in the declaration
    String      m_strEncoding;      // Encoding used for this document
    bool        m_bStandalone;      // Standalone status
    Element*    m_pRoot;            // Root element. Also part of NodeContainer::m_nodes

#if ADVXMLPARSER_DTD != 0
    Dtd         m_dtd;              // Document Type Definition
#endif // ADVXMLPARSER_DTD
};

// ===================================================================
// Parsing Errors
// ===================================================================

enum PARSER_ERROR
{
    ERROR_NONE,
    ERROR_END_OF_DOC,
    ERROR_INVALID_CHAR,
    ERROR_NO_EQUAL,
    ERROR_INVALID_ATTRIBUTE_VALUE,
    ERROR_UNEXPECTED_TAG,
    ERROR_INVALID_REFERENCE,
    ERROR_INVALID_HEX_NUMBER,
    ERROR_INVALID_NUMBER,
    ERROR_UNKNOWN_REFERENCE,
    ERROR_INVALID_TAG_NAME,
    ERROR_INVALID_EMPTY_TAG,
    ERROR_INVALID_TAG_END,
    ERROR_TAG_NAME_MISMATCH,
    ERROR_INVALID_MARKUP,
    ERROR_INVALID_CDATA_CHARS,
    ERROR_NO_ROOT,
    ERROR_INVALID_VERSION_INFO,
    ERROR_INVALID_DECLARATION_END,
    ERROR_INVALID_ENCODING_DECLARATION,
    ERROR_INVALID_ENCODING_NAME,
    ERROR_INVALID_STANDALONE_DECLARATION,
    ERROR_INVALID_STANDALONE_VALUE,
    ERROR_INVALID_COMMENT_CHAR,
    ERROR_INVALID_PI_TARGET,
    ERROR_RESERVED_PI_TARGET,
    ERROR_INVALID_DOCTYPE,
    ERROR_INVALID_PE_REFERENCE,
    ERROR_INVALID_PUBID_LITERAL,
    ERROR_INVALID_ELEMENT_DECL,
    ERROR_CHOICE_EXPECTED,
    ERROR_SEQ_EXPECTED,
    ERROR_INVALID_CP,
    ERROR_INVALID_ATTLIST_DECL,
    ERROR_INVALID_ATTDEF,
    ERROR_INVALID_NOTATION_TYPE,
    ERROR_INVALID_ENUMERATION,
    ERROR_INVALID_DEFAULT_DECL,
    ERROR_INVALID_ENTITY_DECL,
    ERROR_INVALID_NDATA_DECL,
    ERROR_INVALID_NOTATION_DECL,
    ERROR_INVALID_EXTERNAL_ID,
    ERROR_INVALID_PUBLIC_ID,
    ERROR_DATA_AT_END,
    ERROR_LAST // Not used, only to check the number of error 
};

// ===================================================================
// Exceptions (abstract)
// -------------------------------------------------------------------
// Base class for exceptions
// ===================================================================

class Exception
{
public:
    Exception();
    virtual ~Exception();
};

// ===================================================================
// ParsingException
// -------------------------------------------------------------------
// Error during parsing
// ===================================================================

class ParsingException : public Exception
{
private:
    PARSER_ERROR    m_nError;
    int             m_nLine;
    int             m_nColumn;

public:
    ParsingException(PARSER_ERROR nError, int nLine, int nColumn);

    PARSER_ERROR GetErrorCode() const;
    const Char* GetErrorMessage() const;
    int GetLine() const;
    int GetColumn() const;
};

// ===================================================================
// InvalidRefException
// -------------------------------------------------------------------
// Invalid or unknown reference found.
// ===================================================================

class InvalidRefException : public Exception
{
public:
    explicit InvalidRefException(const String& strName);

private:
    String  m_strName;
};

// ===================================================================
// Bookmark
// -------------------------------------------------------------------
// Record the current position in the document
// ===================================================================

class Bookmark
{
private:
    Parser&         m_reader;           // Parser
    const Char*     m_szSourceCurrent;  // Position recorded
    int             m_nLine;            // Line recorded
    int             m_nColumn;          // Column recorded

public:
    explicit Bookmark(Parser& reader);
    // Change back the position 
    void Restore();
    // Get the sub-string between the current and 
    // the recorded positions
    void GetSubString(String& strString, int nNumEndSkip = 0); 
    // Record the current position
    void Reset();
};

// ===================================================================
// Parser
// -------------------------------------------------------------------
// XML Parser
// ===================================================================

class Parser  
{
    friend class Bookmark;

public:
	Parser();
	virtual ~Parser();

    // ---------------------------------------------------------------
    // Parse the document - Can throw exception
    // ---------------------------------------------------------------
	Document* Parse(const Char* szSource, int nSourceSize);

public:
    // ---------------------------------------------------------------
    // Errors
    // ---------------------------------------------------------------
    // Syntax error (throw exception)
    void SyntaxError(PARSER_ERROR nError);

    // ---------------------------------------------------------------
    // Low-level parsing
    // ---------------------------------------------------------------
    // Next char (next position)
    Char NextChar();
    // Previous position
    void PreviousChar();

    // All these following member functions can throw exceptions

    // ---------------------------------------------------------------
    // Parse strings, numbers, etc.
    // ---------------------------------------------------------------
    // Read a given char
    bool ParseChar(Char c);
    // Read the given string
    bool ParseString(const Char* pString);
    // Read the given string (not case sensitive)
    bool ParseStringNoCase(const Char* pString);
    // Read a (decimal) number
    bool ParseNumber(int& nNum);
    // Read an hexadecimal number
    bool ParseHexNumber(int& nNum);

    // ---------------------------------------------------------------
    // Utilities
    // ---------------------------------------------------------------
    // Parse a declaration (like: version = )
    bool ParseDeclBegining(const Char* szString);

    // ---------------------------------------------------------------
    // Basic Rules of the Grammar
    // ---------------------------------------------------------------
    // Read One or more spaces
    bool ParseSpaces();
    // Parse equal sign
    bool ParseEq();
    // Read a name (letters, digits and special chars)
    bool ParseName(String& strName);
    // Read a Name Token
    bool ParseNmtoken(String& strName);

private:
	const Char* m_szSource;         // XML document
    const Char* m_szSourceCurrent;  // Current position
    const Char* m_szSourceEnd;      // End of the document

    int         m_nLine;            // Current line
    int         m_nColumn;          // Current column
    int         m_nOldColumn;       // Previous column (for PreviousChar)
};

// ===================================================================
// Get Values
// ===================================================================

void operator<<(String& strValue, const ConstValue& value);
void operator<<(String& strValue, const Node& node);

// You can define these functions ourself if you prefer
#ifndef ADVXMLPARSER_NO_CONVERSION
void operator<<(int& nValue, const ConstValue& value);
void operator<<(unsigned int& nValue, const ConstValue& value);
void operator<<(double& dValue, const ConstValue& value);
#endif

// ===================================================================
// Set Values
// ===================================================================

void operator<<(Value value, const Char* szValue);
//void operator<<(Node& node, const Char* szValue); TODO

// You can define these functions ourself if you prefer
#ifndef ADVXMLPARSER_NO_CONVERSION
void operator<<(Value value, int nValue);
void operator<<(Value value, unsigned int nValue);
void operator<<(Value value, double dValue);
#endif // ADVXMLPARSER_NO_CONVERSION

// ===================================================================
// Implementation of inline functions

#include "Iterators.inl"
#include "Elements.inl"
#include "Reader.inl"
#include "Writer.inl"

// ===================================================================

}

#endif // INC_ADVXMLPARSER_H
