#pragma once

#include <vector>

namespace TuxClocker {

template <typename T>
class FlatTreeNode {
	T value;
	std::vector<int> childIndices;
};
	
template <typename T>
class FlatTree {
	std::vector<FlatTreeNode<T>> nodes;
};

template <typename T>
class TreeNode {
private:
	T value;
	std::vector<TreeNode<T>> children;
};

};
