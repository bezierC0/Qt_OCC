
#pragma once
#include <TDF_Label.hxx>
using TreeNodeId = uint32_t;
static TreeNodeId lastNodeId = 0; // TreeNodeId
struct TreeNode
{
    TreeNodeId siblingPrevious;
    TreeNodeId siblingNext;
    TreeNodeId childFirst;
    TreeNodeId childLast;
    TreeNodeId parent;
    TDF_Label data;
    bool isDeleted;
};