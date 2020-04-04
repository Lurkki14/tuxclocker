#pragma once

#include <functional>
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
	void appendChild(TreeNode<T> node) {m_children.push_back(node);}
	std::vector<TreeNode<T>> children() {return m_children;}
	static void preorder(const TreeNode<T> node, std::function<void(const T)> func) {
		func(node.m_value);
		for (const auto child : node.m_children) {
			preorder(child, func);
		}
	}
	T value() {return m_value;}
private:
	T m_value;
	std::vector<TreeNode<T>> m_children;
};

};
