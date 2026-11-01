#pragma once
#include <algorithm>
#include <vector>

template <typename T> struct TreeNode {
    T data;
    TreeNode* left = nullptr;
    TreeNode* right = nullptr;
    int height = 1;
    explicit TreeNode(const T& value) : data(value) {}
};

// Custom AVL tree retained from the original project. Index keys are unique;
// songs use (normalized title, ID), so identical titles remain distinct.
template <typename T> class AVL {
    static int height(TreeNode<T>* n) { return n ? n->height : 0; }
    static void destroy(TreeNode<T>* n) { if (n) { destroy(n->left); destroy(n->right); delete n; } }
    TreeNode<T>* balance(TreeNode<T>* n) {
        if (!n) return nullptr;
        UpdateHeight(n);
        if (BalanceFactor(n) > 1) {
            if (BalanceFactor(n->left) < 0) n->left = rrrotate(n->left);
            return llrotate(n);
        }
        if (BalanceFactor(n) < -1) {
            if (BalanceFactor(n->right) > 0) n->right = llrotate(n->right);
            return rrrotate(n);
        }
        return n;
    }
    static void traverse(TreeNode<T>* n, std::vector<T>& out, bool asc) {
        if (!n) return;
        traverse(asc ? n->left : n->right, out, asc); out.push_back(n->data);
        traverse(asc ? n->right : n->left, out, asc);
    }
public:
    TreeNode<T>* root = nullptr;
    AVL() = default;
    ~AVL() { clear(); }
    AVL(const AVL&) = delete;
    AVL& operator=(const AVL&) = delete;
    void clear() { destroy(root); root = nullptr; }
    int BalanceFactor(TreeNode<T>* n) const { return n ? height(n->left) - height(n->right) : 0; }
    int UpdateHeight(TreeNode<T>* n) { return n ? n->height = 1 + std::max(height(n->left), height(n->right)) : 0; }
    TreeNode<T>* llrotate(TreeNode<T>* n) {
        auto* c = n->left; n->left = c->right; c->right = n;
        UpdateHeight(n); UpdateHeight(c); return c;
    }
    TreeNode<T>* rrrotate(TreeNode<T>* n) {
        auto* c = n->right; n->right = c->left; c->left = n;
        UpdateHeight(n); UpdateHeight(c); return c;
    }
    TreeNode<T>* addnode(TreeNode<T>* n, const T& key) {
        if (!n) return new TreeNode<T>(key);
        if (key < n->data) n->left = addnode(n->left, key);
        else if (n->data < key) n->right = addnode(n->right, key);
        else return n;
        return balance(n);
    }
    TreeNode<T>* deleteSong(TreeNode<T>* n, const T& key) {
        if (!n) return nullptr;
        if (key < n->data) n->left = deleteSong(n->left, key);
        else if (n->data < key) n->right = deleteSong(n->right, key);
        else {
            if (!n->left || !n->right) { auto* child = n->left ? n->left : n->right; delete n; return child; }
            auto* next = n->right; while (next->left) next = next->left;
            n->data = next->data; n->right = deleteSong(n->right, next->data);
        }
        // Deletion rotations depend on child balance (including zero), not key.
        return balance(n);
    }
    void insert(const T& key) { root = addnode(root, key); }
    void erase(const T& key) { root = deleteSong(root, key); }
    void displayinorder(std::vector<T>& out) const { traverse(root, out, true); }
    void displayinorderdescend(std::vector<T>& out) const { traverse(root, out, false); }
};
