#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "json.hpp"

// ...existing code...
using namespace std;
using json = nlohmann::json;

#ifndef PREPROCESSING_HPP
#define PREPROCESSING_HPP

//put all functions prototypes, structs, classes related to preprocessing here
unordered_map<string, json> create_hashtable_title();
unordered_map<int, vector<string>> create_hashtable_duration();

vector<string> hashtovector(unordered_map<string, json> & song_title);
vector<int> hashtovector_int(unordered_map<int, vector<string>> & duration_titles);

template <typename T>
struct TreeNode {
    T data;
    TreeNode<T>* left;
    TreeNode<T>* right;
    int height;
    TreeNode(T data) {
        this->data = data;
        left = right = nullptr;
        height = 1;
    }
};

void inorderdurdes(TreeNode<int>* temp, unordered_map<int, vector<string>> &arr, vector<string> &arr2);
void inorderdurasc(TreeNode<int>* temp, unordered_map<int, vector<string>> &arr, vector<string> &arr2);

template <typename A>
class AVL {
public:
    TreeNode<A> * root;
    AVL();
    int BalanceFactor(TreeNode<A>* temp);
    int UpdateHeight(TreeNode<A>* temp);
    TreeNode<A>* llrotate(TreeNode<A>* var);
    TreeNode<A>* lrrotate(TreeNode<A>* var);
    TreeNode<A>* rrrotate(TreeNode<A>* var);
    TreeNode<A>* rlrotate(TreeNode<A>* var);
    TreeNode<A>* addnode(TreeNode<A>* temp, A var);
    TreeNode<A>* deleteSong(TreeNode<A>* temp, A var); 
    void createtree(const vector<A>& v, int size) ;
    void inorder(TreeNode<A>* node,vector<string> & arr);
    void displayinorder(vector<string> & arr);
    void inorderdescend(TreeNode<A>* node,vector<string> & arr);
    void displayinorderdescend(vector<string> & arr) ;
};

#endif // PREPROCESSING_HPP

//put all function definitions related to preprocessing here
unordered_map<string,json> create_hashtable_title() {
    unordered_map<string, json> songhash;

    // Open the JSON file
    ifstream info("Info_files/info.json");
    if (!info.is_open()) {
        cerr << "The file cannot be opened." << endl;
        return songhash;
    }

    json data;
    try {
        if (info.peek() == ifstream::traits_type::eof()) {
            cerr << "info.json is empty. Initializing with an empty array.\n";
            data = json::array();
        } else {
            info >> data;
        }
    } catch (json::parse_error& e) {
        cerr << "Error parsing info.json: " << e.what() << endl;
        data = json::array(); // fallback to empty
    }

    // Accept either: top-level array OR object with "songs": [...]
    const json* songs_ptr = nullptr;
    if (data.is_array()) {
        songs_ptr = &data;
    }
    else {
        cerr << "JSON does not contain a valid 'songs' array." << endl;
        return songhash;
    }

    // Insert each song into the hash table
    for (const auto& song : *songs_ptr) {
        if (!song.is_object() || !song.contains("title")) {
            cerr << "Skipping entry without a title." << endl;
            continue;
        }
        string key = song.at("id").get<string>();
        songhash[key] = song;
    }
    return songhash;
}

unordered_map<int,vector<string>> create_hashtable_duration() {
    unordered_map<int,vector<string>> durationhash;

    // Open the JSON file
    ifstream info("Info_files/info.json");
    if (!info.is_open()) {
        cerr << "The file cannot be opened." << endl;
        return durationhash;
    }

    json data;
    try {
        if (info.peek() == ifstream::traits_type::eof()) {
            cerr << "info.json is empty. Initializing with an empty array.\n";
            data = json::array();
        } else {
            info >> data;
        }
    } catch (json::parse_error& e) {
        cerr << "Error parsing info.json: " << e.what() << endl;
        data = json::array(); // fallback to empty
    }

    // Accept either: top-level array OR object with "songs": [...]
    const json* songs_ptr = nullptr;
    if (data.is_array()) {
        songs_ptr = &data;
    }
    else {
        cerr << "JSON does not contain a valid 'songs' array." << endl;
        return durationhash;
    }

    for (const auto& song : *songs_ptr) {
        if (!song.is_object() || !song.contains("duration")) {
            cerr << "Skipping entry without any duration." << endl;
            continue;
        }
        int key = stoi(song.at("duration").get<string>());
        durationhash[key].push_back(song.at("id").get<string>());
    }
    return durationhash;
}

vector<int> hashtovector_int(unordered_map<int, vector<string>> & duration_table){
    vector<int> durations;
    for (const auto& pair : duration_table) {
        durations.push_back(pair.first);
    }
    return durations;
}

vector<string> hashtovector(unordered_map<string, json> &song_table){
    vector<string> titles;
    for (const auto& pair : song_table) {
        titles.push_back(pair.first);
    }
    return titles;
}

template <typename A>
AVL<A>::AVL() {
    root = nullptr;
}

template <typename A>
int AVL<A>::BalanceFactor(TreeNode<A>* temp) {
    if (!temp) return 0;
    int lh = (temp->left) ? temp->left->height : 0;
    int rh = (temp->right) ? temp->right->height : 0;
    return lh - rh;
}

template <typename A>
int AVL<A>::UpdateHeight(TreeNode<A>* temp) {
    if (temp == nullptr) return 0;
    int lh = (temp->left) ? temp->left->height : 0;
    int rh = (temp->right) ? temp->right->height : 0;
    temp->height = 1 + max(lh, rh);
    return temp->height;
}

template <typename A>
TreeNode<A>* AVL<A>::llrotate(TreeNode<A>* var) {
    TreeNode<A>* child = var->left;
    TreeNode<A>* childRight = child->right;
    child->right = var;
    var->left = childRight;

    UpdateHeight(var);
    UpdateHeight(child);
    return child;
}

template <typename A>
TreeNode<A>* AVL<A>::lrrotate(TreeNode<A>* var) {
    var->left = rrrotate(var->left);
    return llrotate(var);
}

template <typename A>
TreeNode<A>* AVL<A>::rrrotate(TreeNode<A>* var) {
    TreeNode<A>* child = var->right;
    TreeNode<A>* childLeft = child->left;
    child->left = var;
    var->right = childLeft;

    UpdateHeight(var);
    UpdateHeight(child);
    return child;
}

template <typename A>
TreeNode<A>* AVL<A>::rlrotate(TreeNode<A>* var) {
    var->right = llrotate(var->right);
    return rrrotate(var);
}

template <typename A>
TreeNode<A>* AVL<A>::addnode(TreeNode<A>* temp, A var) {
    if (temp == nullptr)
        return new TreeNode<A>(var);

    if (var < temp->data)
        temp->left = addnode(temp->left, var);
    else if (var >= temp->data)
        temp->right = addnode(temp->right, var);

    UpdateHeight(temp);
    int balance = BalanceFactor(temp);

    if (balance > 1 && var < temp->left->data)
        return llrotate(temp);
    if (balance > 1 && var >= temp->left->data)
        return lrrotate(temp);
    if (balance < -1 && var >= temp->right->data)
        return rrrotate(temp);
    if (balance < -1 && var < temp->right->data)
        return rlrotate(temp);

    return temp;
}

template <typename A>
TreeNode<A>* AVL<A>::deleteSong(TreeNode<A>* temp, A key) {
    if (temp == nullptr)
        return nullptr;

    if (key < temp->data)
        temp->left = deleteSong(temp->left, key);
    else if (key > temp->data)
        temp->right = deleteSong(temp->right, key);
    else {
        if (temp->left == nullptr || temp->right == nullptr) {
            TreeNode<A>* child = temp->left ? temp->left : temp->right;
            delete temp;
            return child;
        } else {
            TreeNode<A>* successor = temp->right;
            while (successor->left != nullptr)
                successor = successor->left;

            temp->data = successor->data;
            temp->right = deleteSong(temp->right, successor->data);
        }
    }

    temp->height = 1 + max(
        (temp->left ? temp->left->height : 0),
        (temp->right ? temp->right->height : 0)
    );

    int balance = BalanceFactor(temp);

    if (balance > 1 && key < temp->left->data)
        return llrotate(temp);
    if (balance > 1 && key > temp->left->data)
        return lrrotate(temp);
    if (balance < -1 && key > temp->right->data)
        return rrrotate(temp);
    if (balance < -1 && key < temp->right->data)
        return rlrotate(temp);

    return temp;
}

template <typename A>
void AVL<A>::createtree(const vector<A>& v, int size) {
    if (size == 0) return;
    for (int i = 0; i < size; i++)
        root = addnode(root, v[i]);
}

// ASCENDING TITLE
template <typename A>
void AVL<A>::inorder(TreeNode<A>* node,vector<string> & arr) {
    if (!node) return;
    inorder(node->left,arr);
    arr.push_back(node->data);
    inorder(node->right,arr);
    
}

template <typename A>
void AVL<A>::displayinorder(vector<string> & arr) {
    inorder(root,arr);
}

//DESCENDING TITLE
template <typename A>
void AVL<A>::inorderdescend(TreeNode<A>* node,vector<string> & arr) {
    if (!node) return;
    inorderdescend(node->right,arr);
    arr.push_back(node->data);
    inorderdescend(node->left,arr);
}

template <typename A>
void AVL<A>::displayinorderdescend(vector<string> & arr) {
    inorderdescend(root,arr);
}

void inorderdurasc(TreeNode<int>* temp, unordered_map<int, vector<string>> &arr, vector<string> &arr2){ 
    if (!temp) return;
    inorderdurasc(temp->left, arr, arr2);
    for (string i: arr[temp->data])
        arr2.push_back(i);
    inorderdurasc(temp->right, arr, arr2);
}

void inorderdurdes(TreeNode<int>* temp, unordered_map<int, vector<string>> &arr, vector<string> &arr2){ 
    if (!temp) return;
    inorderdurdes(temp->right, arr, arr2);
    for (string i: arr[temp->data])
        arr2.push_back(i);
    inorderdurdes(temp->left, arr, arr2);
}
