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
public:
	TreeNode() {};
	TreeNode(T value) {m_value = value;}
	void appendChild(T value) {m_children.push_back(TreeNode{value});}
	std::vector<TreeNode<T>> children() {return m_children;}
private:
	T m_value;
	std::vector<TreeNode<T>> m_children;
};

};
