// ===================================================================
// AdvXMLParser
//
// See AdvXMLParser.h
// ===================================================================

// It's either me or some compilers, but I have some difficulties to
// make template specialization without defining the members in the
// declaration. I hate to do that (I prefer to separate the declations
// from the actual implementation of functions.
// TODO: Check the standard to know if it's me or the compiler(s)


// ===================================================================
// Iterator
// -------------------------------------------------------------------
// Used to enumerator the children of a given node. You can enumerate
// all the children (Node) or only one type (for ex. Element). In this
// case, the iterator returns a reference of the real type (Element).
// ===================================================================

template<class N> struct Iterator
{
    // Because of problems with SGI STL and MS VC++ 6, I can't use
    // the template 'iterator'. Instead, I directly define typedefs.
    typedef advstd::bidirectional_iterator_tag iterator_category;
    typedef N           value_type;
    typedef ptrdiff_t   difference_type; // I'm not sure...
    typedef N*          pointer;
    typedef N&          reference;

public:
    Iterator(const Node::IteratorRef& ref)
    :   m_nodes(ref.m_nodes),
        m_it(ref.m_it)
    {
        // Go to the first valid node
        FirstValid();
    }

    Iterator(const Iterator& it)
    :   m_nodes(it.m_nodes),
        m_it(it.m_it)
    {
    }

    Iterator& operator=(const Iterator& it)
    {
        ASSERT(&it.m_nodes == &m_nodes);
        m_it = it.m_it;
    }

    N& operator*() const
    {
        return(*static_cast<N*>(*m_it));
    }

    Iterator& operator++()
    {
        ++m_it;
        // Go to the first valid node
        FirstValid();
        return(advself);
    }

    Iterator  operator++(int)
    {
        Iterator tmp = advself;
        ++m_it;
        // Go to the first valid node
        FirstValid();
        return(tmp);
    }

    Iterator& operator--()
    {
        --m_it;
        LastValid();
        return(advself);
    }

    Iterator  operator--(int)
    {
        Iterator tmp = advself;
        --m_it;
        // Go to the last valid node
        LastValid();
        return(tmp);
    }

    bool operator==(const Iterator& it)     { return(m_it == it.m_it); }
    bool operator!=(const Iterator& it)     { return(m_it != it.m_it); }
    bool operator< (const Iterator& it)     { return(m_it <  it.m_it); }
    bool operator> (const Iterator& it)     { return(m_it >  it.m_it); }
    bool operator<=(const Iterator& it)     { return(m_it <= it.m_it); }
    bool operator>=(const Iterator& it)     { return(m_it >= it.m_it); }

public:
    // Since some compilers are not able to handle friend templates, I have to introduct this member function
    // (as public). It's only for implementation. Please don't use it.
    Nodes::const_iterator Internal_() const
    {
        return(m_it);
    }

    const Nodes& Internal2_() const
    {
        return(m_nodes);
    }

private:
    void FirstValid()
    // Go to the first valid node. General case
    {
        // Search the next node of the right type
        for(; m_it < m_nodes.end(); ++m_it)
        {
            if((*m_it)->IsKindOf(N::TYPE)) // Same type ?
                return;
        }

        // m_it == end
    }

    void LastValid()
    // Go to the last valid node. General case
    {
        // Search the previous node of the right type
        for(; m_it > m_nodes.begin(); --m_it)
        {
            if((*m_it)->IsKindOf(N::TYPE)) // Same type ?
                return;
        }

        // m_it == begin
    }

private:
    Nodes& m_nodes;
    Nodes::iterator m_it;
};

// ===================================================================
// ConstIterator
// -------------------------------------------------------------------
// Similar to Iterator but for const Nodes
// ===================================================================

template<class N> struct ConstIterator
{
    // Because of problems with SGI STL and MS VC++ 6, I can't use
    // the template 'iterator'. Instead, I directly define typedefs.
    typedef advstd::bidirectional_iterator_tag iterator_category;
    typedef const N     value_type;
    typedef ptrdiff_t   difference_type; // I'm not sure...
    typedef const N*    pointer;
    typedef const N&    reference;

public:
    ConstIterator(const Node::ConstIteratorRef& ref)
    :   m_nodes(ref.m_nodes),
        m_it(ref.m_it)
    {
        // Go to the first valid node
        FirstValid();
    }

    ConstIterator(const Node::IteratorRef& ref)
    :   m_nodes(ref.m_nodes),
        m_it(ref.m_it)
    {
        // Go to the first valid node
        FirstValid();
    }

    ConstIterator(const ConstIterator& it)
    :   m_nodes(it.m_nodes),
        m_it(it.m_it)
    {
    }

    ConstIterator(const Iterator<N>& it)
    :   m_nodes(it.Internal2_()),
        m_it(it.Internal_())
    {
    }

    ConstIterator& operator=(const ConstIterator& it)
    {
        ASSERT(&it.m_nodes == &m_nodes);
        m_it = it.m_it;
		return(advself);
    }

    ConstIterator& operator=(const Iterator<N>& it)
    {
        ASSERT(&it.Internal_2() == &m_nodes);
        m_it = it.Internal_();
		return(advself);
    }

    const N& operator*() const
    {
        return(*static_cast<const N*>(*m_it));
    }

    ConstIterator& operator++()
    {
        ++m_it;
        // Go to the first valid node
        FirstValid();
        return(advself);
    }

    ConstIterator operator++(int)
    {
        ConstIterator tmp = advself;
        ++m_it;
        // Go to the first valid node
        FirstValid();
        return(tmp);
    }

    ConstIterator& operator--()
    {
        --m_it;
        // Go to the last valid node
        LastValid();
        return(advself);
    }

    ConstIterator operator--(int)
    {
        ConstIterator tmp = advself;
        --m_it;
        // Go to the last valid node
        LastValid();
        return(tmp);
    }

    bool operator==(const ConstIterator& it)    { return(m_it == it.m_it); }
    bool operator!=(const ConstIterator& it)    { return(m_it != it.m_it); }
    bool operator< (const ConstIterator& it)    { return(m_it <  it.m_it); }
    bool operator> (const ConstIterator& it)    { return(m_it >  it.m_it); }
    bool operator<=(const ConstIterator& it)    { return(m_it <= it.m_it); }
    bool operator>=(const ConstIterator& it)    { return(m_it >= it.m_it); }

private:
    void FirstValid()
    {
        for(; m_it < m_nodes.end(); ++m_it)
        {
            if((*m_it)->IsKindOf(N::TYPE)) // Same type ?
                return;
        }

        // m_it == end
    }

    void LastValid()
    {
        for(; m_it > m_nodes.begin(); --m_it)
        {
            if((*m_it)->IsKindOf(N::TYPE)) // Same type ?
                return;
        }

        // m_it == begin
    }

private:
    const Nodes& m_nodes;
    Nodes::const_iterator m_it;
};

// ===================================================================
// Iterator<Node>
// -------------------------------------------------------------------
// Specialization for Node. Since in this case all nodes are
// enumerated without looking at the type, the implementation can be
// optimized
// ===================================================================

template<> struct Iterator<Node>
{
    typedef advstd::bidirectional_iterator_tag iterator_category;
    typedef Node        value_type;
    typedef ptrdiff_t   difference_type;
    typedef Node*       pointer;
    typedef Node&       reference;

public:
    Iterator(const Node::IteratorRef& ref)
    :   m_it(ref.m_it)
    {
    }

    Iterator(const Iterator& it)
    :   m_it(it.m_it)
    {
    }

    Iterator& operator=(const Iterator& it)
    {
        m_it = it.m_it;
        return(advself);
    }

    Node& operator*() const
    {
        return(**m_it);
    }

    Iterator& operator++()
    {
        ++m_it;
        return(advself);
    }

    Iterator operator++(int)
    {
        Iterator tmp = advself;
        ++m_it;
        return(tmp);
    }

    Iterator& operator--()
    {
        --m_it;
        return(advself);
    }

    Iterator  operator--(int)
    {
        Iterator tmp = advself;
        --m_it;
        return(tmp);
    }

    bool operator==(const Iterator& it)     { return(m_it == it.m_it); }
    bool operator!=(const Iterator& it)     { return(m_it != it.m_it); }
    bool operator< (const Iterator& it)     { return(m_it <  it.m_it); }
    bool operator> (const Iterator& it)     { return(m_it >  it.m_it); }
    bool operator<=(const Iterator& it)     { return(m_it <= it.m_it); }
    bool operator>=(const Iterator& it)     { return(m_it >= it.m_it); }

public:
    // Since some compilers are not able to handle friend templates, I have to introduct this member function
    // (as public). It's only for implementation. Please don't use it.
    Nodes::const_iterator Internal_() const
    {
        return(m_it);
    }

private:
    Nodes::iterator m_it;
};

// ===================================================================
// ConstIterator<Node>
// -------------------------------------------------------------------
// Specialization for Node. Similar to Iterator but for const Nodes.
// ===================================================================

template<> struct ConstIterator<Node>
{
    typedef advstd::bidirectional_iterator_tag iterator_category;
    typedef const Node  value_type;
    typedef ptrdiff_t   difference_type;
    typedef const Node* pointer;
    typedef const Node& reference;

public:
    ConstIterator(const Node::ConstIteratorRef& ref)
    :   m_it(ref.m_it)
    {
    }

    ConstIterator(const Node::IteratorRef& ref)
    :   m_it(ref.m_it)
    {
    }

    ConstIterator(const ConstIterator& it)
    :   m_it(it.m_it)
    {
    }

    ConstIterator(const Iterator<Node>& it)
    :   m_it(it.Internal_())
    {
    }

    ConstIterator& operator=(const ConstIterator& it)
    {
        m_it = it.m_it;
        return(advself);
    }

    ConstIterator& operator=(const Iterator<Node>& it)
    {
        m_it = it.Internal_();
        return(advself);
    }

    const Node& operator*() const
    {
        return(**m_it);
    }

    ConstIterator& operator++()
    {
        ++m_it;
        return(advself);
    }

    ConstIterator operator++(int)
    {
        ConstIterator tmp = advself;
        ++m_it;
        return(tmp);
    }

    ConstIterator& operator--()
    {
        --m_it;
        return(advself);
    }

    ConstIterator operator--(int)
    {
        ConstIterator tmp = advself;
        --m_it;
        return(tmp);
    }

    bool operator==(const ConstIterator& it)    { return(m_it == it.m_it); }
    bool operator!=(const ConstIterator& it)    { return(m_it != it.m_it); }
    bool operator< (const ConstIterator& it)    { return(m_it <  it.m_it); }
    bool operator> (const ConstIterator& it)    { return(m_it >  it.m_it); }
    bool operator<=(const ConstIterator& it)    { return(m_it <= it.m_it); }
    bool operator>=(const ConstIterator& it)    { return(m_it >= it.m_it); }

private:
    Nodes::const_iterator m_it;
};

// ===================================================================
// Iterator
// -------------------------------------------------------------------
// Specialization for Attribute
// ===================================================================

template<> struct Iterator<Attribute>
{
    typedef advstd::bidirectional_iterator_tag iterator_category;
    typedef Attribute   value_type;
    typedef ptrdiff_t   difference_type;
    typedef Attribute*  pointer;
    typedef Attribute&  reference;

public:
    Iterator(const Attribute::IteratorRef& ref)
    :   m_it(ref.m_it)
    {
    }

    Iterator(const Iterator& it)
    :   m_it(it.m_it)
    {
    }

    Iterator& operator=(const Iterator& it)
    {
        m_it = it.m_it;
        return(advself);
    }

    Attribute& operator*() const
    {
        return(**m_it);
    }

    Iterator& operator++()
    {
        ++m_it;
        return(advself);
    }

    Iterator  operator++(int)
    {
        Iterator tmp = advself;
        ++m_it;
        return(tmp);
    }

    Iterator& operator--()
    {
        --m_it;
        return(advself);
    }

    Iterator  operator--(int)
    {
        Iterator tmp = advself;
        --m_it;
        return(tmp);
    }

    bool operator==(const Iterator& it)     { return(m_it == it.m_it); }
    bool operator!=(const Iterator& it)     { return(m_it != it.m_it); }
    bool operator< (const Iterator& it)     { return(m_it <  it.m_it); }
    bool operator> (const Iterator& it)     { return(m_it >  it.m_it); }
    bool operator<=(const Iterator& it)     { return(m_it <= it.m_it); }
    bool operator>=(const Iterator& it)     { return(m_it >= it.m_it); }

public:
    // Since some compilers are not able to handle friend templates, I have to introduct this member function
    // (as public). It's only for implementation. Please don't use it.
    Attributes::const_iterator Internal_() const
    {
        return(m_it);
    }

private:
    Attributes::iterator m_it;
};

// ===================================================================
// ConstIterator
// -------------------------------------------------------------------
// Specialization for Attribute
// ===================================================================

template<> struct ConstIterator<Attribute>
{
    typedef advstd::bidirectional_iterator_tag iterator_category;
    typedef const Attribute     value_type;
    typedef ptrdiff_t           difference_type;
    typedef const Attribute*    pointer;
    typedef const Attribute&    reference;

public:
    ConstIterator(const Attribute::ConstIteratorRef& ref)
    :   m_it(ref.m_it)
    {
    }

    ConstIterator(const Attribute::IteratorRef& ref)
    :   m_it(ref.m_it)
    {
    }

    ConstIterator(const ConstIterator& it)
    :   m_it(it.m_it)
    {
    }

    ConstIterator(const Iterator<Attribute>& it)
    :   m_it(it.Internal_())
    {
    }

    ConstIterator& operator=(const ConstIterator& it)
    {
        m_it = it.m_it;
        return(advself);
    }

    ConstIterator& operator=(const Iterator<Attribute>& it)
    {
        m_it = it.Internal_();
        return(advself);
    }

    const Attribute& operator*() const
    {
        return(**m_it);
    }

    ConstIterator& operator++()
    {
        ++m_it;
        return(advself);
    }

    ConstIterator operator++(int)
    {
        ConstIterator tmp = advself;
        ++m_it;
        return(tmp);
    }

    ConstIterator& operator--()
    {
        --m_it;
        return(advself);
    }

    ConstIterator operator--(int)
    {
        ConstIterator tmp = advself;
        --m_it;
        return(tmp);
    }

    bool operator==(const ConstIterator& it)    { return(m_it == it.m_it); }
    bool operator!=(const ConstIterator& it)    { return(m_it != it.m_it); }
    bool operator< (const ConstIterator& it)    { return(m_it <  it.m_it); }
    bool operator> (const ConstIterator& it)    { return(m_it >  it.m_it); }
    bool operator<=(const ConstIterator& it)    { return(m_it <= it.m_it); }
    bool operator>=(const ConstIterator& it)    { return(m_it >= it.m_it); }

private:
    Attributes::const_iterator m_it;
};


// ===================================================================
// Node::ConstIteratorRef
// ===================================================================

inline Node::ConstIteratorRef::ConstIteratorRef(const Nodes& nodes, Nodes::const_iterator it)
:   m_nodes(nodes),
    m_it(it)
{
}

// ===================================================================
// Node::IteratorRef
// ===================================================================

inline Node::IteratorRef::IteratorRef(Nodes& nodes, Nodes::iterator it)
:   m_nodes(nodes),
    m_it(it)
{
}

// ===================================================================
// NodeContainer - Iterators to enumerate the children
// ===================================================================

inline Node::IteratorRef NodeContainer::Begin()
{
    return(Node::IteratorRef(m_children, m_children.begin()));
}

inline Node::IteratorRef NodeContainer::End()
{
    return(Node::IteratorRef(m_children, m_children.end()));
}

inline Node::ConstIteratorRef NodeContainer::Begin() const
{
    return(Node::ConstIteratorRef(m_children, m_children.begin()));
}

inline Node::ConstIteratorRef NodeContainer::End() const
{
    return(Node::ConstIteratorRef(m_children, m_children.end()));
}

// ===================================================================
// Attribute::IteratorRef & Attribute::ConstIteratorRef
// ===================================================================

inline Attribute::IteratorRef::IteratorRef(Attributes::iterator it)
:   m_it(it)
{
}

inline Attribute::ConstIteratorRef::ConstIteratorRef(Attributes::const_iterator it)
:   m_it(it)
{
}

// ===================================================================
// Element
// -------------------------------------------------------------------
// Enumerate Attributes
// ===================================================================

inline Attribute::IteratorRef Element::AttributesBegin()
{
    return(Attribute::IteratorRef(m_attributes.begin()));
}

inline Attribute::IteratorRef Element::AttributesEnd()
{
    return(Attribute::IteratorRef(m_attributes.end()));
}

inline Attribute::ConstIteratorRef Element::AttributesBegin() const
{
    return(Attribute::ConstIteratorRef(m_attributes.begin()));
}

inline Attribute::ConstIteratorRef Element::AttributesEnd() const
{
    return(Attribute::ConstIteratorRef(m_attributes.end()));
}

