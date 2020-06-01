#pragma once

#include <functional>
#include <iostream>
#include <vector>

namespace TuxClocker {

template <typename T>
struct FlatTreeNode {
	T value;
	std::vector<int> childIndices;
};
	
template <typename T>
class TreeNode;

template <typename T>
struct FlatTree {
	std::vector<FlatTreeNode<T>> nodes;
	
	static TreeNode<T> toTree(FlatTree<T> flatTree) {
		std::function<void(TreeNode<T>*, uint)> recur;
		recur = [&recur, flatTree](TreeNode<T> *node, uint i) {
			auto c_indices = flatTree.nodes[i].childIndices;
			for (auto index : c_indices)
				node->appendChild(TreeNode<T>(flatTree.nodes[index].value));
			
			uint j = 0;
			for (auto &c_node : *(node->childrenPtr())) {
				recur(&c_node, c_indices[j]);
				j++;
			}
		};
		TreeNode<T> root;
		if (flatTree.nodes.size() > 0) {
			auto realRoot = TreeNode<T>(flatTree.nodes.front().value);
			recur(&realRoot, 0);
			root = realRoot;
		}
		return root;
	}
};

template <typename T>
class TreeNode {
public:
	TreeNode() {};
	TreeNode(T value) {m_value = value;}
	void appendChild(T value) {m_children.push_back(TreeNode{value});}
	void appendChild(TreeNode<T> node) {m_children.push_back(node);}
	std::vector<TreeNode<T>> children() {return m_children;}
	// Needed for recursive tree construction
	std::vector<TreeNode<T>> *childrenPtr() {return &m_children;}
	static void preorder(const TreeNode<T> node, std::function<void(const T)> func) {
		func(node.m_value);
		for (const auto child : node.m_children) {
			preorder(child, func);
		}
	}
	T value() {return m_value;}
	// Convert tree to array
	FlatTree<T> toFlatTree() {
		std::vector<FlatTreeNode<T>> nodes;
		auto node_ptr = this;
		preorderByRef(node_ptr, [&nodes, node_ptr](TreeNode<T> *n) {
			FlatTreeNode<T> node;
			node.value = n->value();
			for (auto &c_node : *(n->childrenPtr())) {
				// Find the index of this child node
				int j = 0, index = 0;
				
				preorderByRef(node_ptr, [&j, &index, &c_node](TreeNode<T> *n) {
					if (n == &c_node)
						index = j;
					j++;
				});
				node.childIndices.push_back(index);
			}
			nodes.push_back(node);
		});
		return FlatTree<T>{nodes};
	}
private:
	T m_value;
	std::vector<TreeNode<T>> m_children;
	static void preorderByRef(TreeNode<T> *node,
			std::function<void(TreeNode<T>*)> func) {
		func(node);
		for (auto &c_node : *(node->childrenPtr()))
			preorderByRef(&c_node, func);
	}
};

};
