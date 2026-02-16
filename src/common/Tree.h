#pragma once
#include <TDF_Label.hxx>

#include <vector>
#include "TreeNode.h"

template<typename T>
class Tree
{
public:
    Tree();

    TreeNodeId nodeSiblingPrevious( TreeNodeId id ) const;
    TreeNodeId nodeSiblingNext( TreeNodeId id ) const;
    TreeNodeId nodeChildFirst( TreeNodeId id ) const;
    TreeNodeId nodeChildLast( TreeNodeId id ) const;
    TreeNodeId nodeParent( TreeNodeId id ) const;
    TreeNodeId nodeRoot( TreeNodeId id ) const;
    const T& nodeData( TreeNodeId id ) const;
    bool nodeIsRoot( TreeNodeId id ) const;
    bool nodeIsLeaf( TreeNodeId id ) const;
    void clear();
    TreeNodeId appendChild(TreeNodeId parentId, const T& data);
    TreeNodeId appendChild(TreeNodeId parentId, T&& data);
    void removeRoot( TreeNodeId id );

    // private
    TreeNodeId lastNodeId() const;
    TreeNode* ptrNode( TreeNodeId id );
    const TreeNode* ptrNode( TreeNodeId id ) const;
    TreeNode* appendChild( TreeNodeId parentId );
    bool isNodeDeleted(TreeNodeId id) const;

public:
    std::vector<TreeNode> m_vecNode;
    std::vector<TreeNodeId> m_vecRoot;
};

template<typename T> Tree<T>::Tree()
{
}

template<typename T>
TreeNodeId Tree<T>::nodeSiblingPrevious(const TreeNodeId id ) const
{
    const TreeNode* node = this->ptrNode( id );
    return node ? node->siblingPrevious : 0;
}

template<typename T>
TreeNodeId Tree<T>::nodeSiblingNext( TreeNodeId id ) const
{
    const TreeNode* node = this->ptrNode( id );
    return node ? node->siblingNext : 0;
}

template<typename T>
TreeNodeId Tree<T>::nodeChildFirst( TreeNodeId id ) const
{
    const TreeNode* node = this->ptrNode( id );
    return node ? node->childFirst : 0;
}

template<typename T>
TreeNodeId Tree<T>::nodeChildLast( const TreeNodeId id ) const
{
    const TreeNode* node = this->ptrNode( id );
    return node ? node->childLast : 0;
}

template<typename T>
TreeNodeId Tree<T>::nodeParent( const TreeNodeId id ) const
{
    const TreeNode* node = this->ptrNode( id );
    return node ? node->parent : 0;
}

template<typename T>
TreeNodeId Tree<T>::nodeRoot( TreeNodeId id ) const
{
    while ( !this->nodeIsRoot( id ) )
      id = this->nodeParent( id );

    return id;
}

template<typename T>
const T& Tree<T>::nodeData( TreeNodeId id ) const
{
    static const T nullObject = {};
    const TreeNode* node = this->ptrNode( id );
    return node ? node->data : nullObject;
}

template<typename T>
inline bool Tree<T>::nodeIsRoot( const TreeNodeId id ) const
{
    const TreeNode* node = this->ptrNode( id );
    return node ? node->parent == 0 : false;
}

template<typename T>
TreeNode* Tree<T>::appendChild( const TreeNodeId parentId )
{
    m_vecNode.push_back( {} );
    const TreeNodeId nodeId = this->lastNodeId();
    TreeNode* node = &m_vecNode.back();
    node->parent = parentId;
    node->siblingPrevious = this->nodeChildLast( parentId );
    if ( parentId != 0 ) {
        TreeNode* parentNode = this->ptrNode( parentId );
        if ( parentNode->childFirst == 0 )
            parentNode->childFirst = nodeId;

        if ( parentNode->childLast != 0 )
            this->ptrNode( parentNode->childLast)->siblingNext = nodeId;

        parentNode->childLast = nodeId;
    }
    else {
        m_vecRoot.push_back( nodeId );
    }

    return node;
}

template<typename T>
inline bool Tree<T>::isNodeDeleted( const TreeNodeId id ) const
{
    const auto node = this->ptrNode( id );
    return !node || node->isDeleted;
}

template<typename T>
TreeNodeId Tree<T>::lastNodeId() const
{
    return static_cast<TreeNodeId>(m_vecNode.size());
}

template<typename T>
TreeNode* Tree<T>::ptrNode(const TreeNodeId id)
{
    return id != 0 && id <= m_vecNode.size() ? &m_vecNode.at(id - 1) : nullptr;
}


template<typename T>
TreeNodeId Tree<T>::appendChild(const TreeNodeId parentId, const T& data)
{
    TreeNode* node = this->appendChild(parentId);
    node->data = data;
    return this->lastNodeId();
}
template<typename T>
inline TreeNodeId Tree<T>::appendChild(const TreeNodeId parentId, T&& data )
{
    TreeNode* node = this->appendChild( parentId );
    node->data = std::forward<T>( data );
    return this->lastNodeId();
}
template <typename T>
inline void Tree<T>::removeRoot(TreeNodeId id)
{
    // TODO
}
template <typename T>
const TreeNode *Tree<T>::ptrNode(const TreeNodeId id) const
{
    return id != 0 && id <= m_vecNode.size() ? &m_vecNode.at(id - 1) : nullptr;
}
