// This file is included by AdvXmlParser.h in the middle of the class Element

// Why this ugly include ?
// Because currently Microsoft Visual C++ is not able to support template
// members that are not declared inlined (see the readme of VC).
// I don't want to put lot of code here so I choose this solution...

template<class N> N& AddInto(const N& node)
{
    if(IsNull())
        return(N::null);

    N* pNode = node.Clone(advself);
    Add(pNode);
    return(*pNode);
}

template<class N> N& Append(const N& node)
{
    if(IsNull())
        return(N::null);

    NodeContainer& parent = GetParent();
    ASSERT(!parent.IsNull());

    N* pNode = node.Clone(parent);
    parent.Add(pNode);
    return(*pNode);
}

template<class N> N& InsertBefore(const N& node)
{
    if(IsNull())
        return(N::null);

    NodeContainer& parent = GetParent();
    ASSERT(!parent.IsNull());

    N* pNode = node.Clone(parent);
    parent.InsertBefore(advself, pNode);
    return(*pNode);
}

template<class N> N& InsertAfter(const N& node)
{
    if(IsNull())
        return(N::null);

    NodeContainer& parent = GetParent();
    ASSERT(!parent.IsNull());

    N* pNode = node.Clone(parent);
    parent.InsertAfter(advself, pNode);
    return(*pNode);
}
