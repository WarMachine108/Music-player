#include <iostream>
#include <fstream>
#include <unordered_map>
#include <queue> 
#include <vector>
#include "json.hpp"
#include"preprocessing.hpp"
using namespace std;

#ifndef MAKER_HPP
#define MAKER_HPP

struct ListNode{
    string data;
    ListNode*next;
    ListNode*prev;
    ListNode(string data);
};

class Maker{
private:
    vector <string> titles;
    vector <string> recent_add; 
    AVL<string> title;
    AVL<int> duration;
    ListNode *head;
public:
    unordered_map<int, vector<string>> duration_table;
    unordered_map<string, json> song_table;
    unordered_map<string, string> title_to_id;
    Maker();
    void createDLL(vector<string> & arr);
    void deleteDoublyLinkedList(ListNode*& head);
    void deleteSong(const string& temp);
    ListNode*createAscendingList();
    ListNode*createDescendingList();
    ListNode*createDescendingList_Dur();
    ListNode*createAscendingList_Dur();
    ListNode*createAscendingList_rec();  
    ListNode*createDescendingList_rec();
};

#endif

ListNode::ListNode(string data){
    this->data=data;
    next=nullptr;
    prev=nullptr;
}


Maker::Maker(){ 
    head=nullptr;
    song_table=create_hashtable_title();
    duration_table=create_hashtable_duration(); 
    for (const auto& pair : song_table) {
        const string& id = pair.first;
        const json& song = pair.second;

        if (song.contains("title")) {
            string title_name = song["title"].get<string>();
            title_to_id[title_name] = id;
            titles.push_back(title_name);
        }
    }
    ifstream infofile("Info_files/info.json");
    if (infofile){ 
        json data; 
        infofile >> data; 
        for (auto& entry: data){ 
            if (entry.contains("id")){ 
                recent_add.push_back(entry["id"].get<string>());
            }
        }
    }
    title.createtree(titles,titles.size());
    vector<int> dur=hashtovector_int(duration_table);
    duration.createtree(dur,dur.size());
    titles.clear();
}
void Maker::createDLL(vector<string> & arr){
    head = nullptr;
    ListNode* temp = nullptr;
    ListNode* previous = nullptr;
    for(string s:arr){
        ListNode*newnode=new ListNode(s);
        if(!head){
            head=newnode;
            temp=head;
            previous=head;
            continue;
        }
        temp->next = newnode;
        newnode->prev = temp;
        temp = newnode;
    }
}
void Maker::deleteDoublyLinkedList(ListNode*& head) {
    ListNode* current = head;
    while (current != nullptr) {
        ListNode* nextNode = current->next;
        delete current;
        current = nextNode;
    }
    head = nullptr;
}

void Maker::deleteSong(const string& temp){ 
    if (title_to_id.find(temp) == title_to_id.end()){ 
        cout << "Song not found." << endl; 
        return;
    }
    string id = title_to_id[temp];
    json song = song_table[id];
    int dur = stoi(song["duration"].get<string>());

    title.root = title.deleteSong(title.root, temp);
    auto& ids = duration_table[dur];
    ids.erase(remove(ids.begin(), ids.end(), id), ids.end());
    if (ids.empty()) {
        duration_table.erase(dur);
        duration.root = duration.deleteSong(duration.root, dur);
    }

    title_to_id.erase(temp);
    song_table.erase(id);
    string songpath = "audioloc/" + id + ".mp3";
    string imagePath = "audioloc/" + id + ".webp";
    remove(songpath.c_str());
    remove(imagePath.c_str());

    ifstream in("Info_files/info.json");
    if (!in.is_open()) {
        cerr << "Could not open info.json for updating.\n";
        return;
    }
    json allSongs;
    in >> allSongs;
    in.close();
    json updated = json::array();
    for (auto& s : allSongs) {
        if (s["id"].get<string>() != id)
            updated.push_back(s);
    }
    ofstream out("Info_files/info.json");
    out << setw(4) << updated << endl;
    out.close();
    cout << "Successfully deleted: " << temp << endl;
}

ListNode* Maker::createAscendingList(){
    titles.clear();
    deleteDoublyLinkedList(head);

    vector<string> sorted_titles;
    title.displayinorder(sorted_titles);

    vector<string> sorted_ids;
    for (const string& t : sorted_titles) {
        if (title_to_id.count(t))
            sorted_ids.push_back(title_to_id[t]);
    }

    createDLL(sorted_ids);
    return head;
}
ListNode* Maker::createDescendingList(){
    titles.clear();
    deleteDoublyLinkedList(head);

    vector<string> sorted_titles;
    title.displayinorderdescend(sorted_titles);

    vector<string> sorted_ids;
    for (const string& t : sorted_titles) {
        if (title_to_id.count(t))
            sorted_ids.push_back(title_to_id[t]);
    }

    createDLL(sorted_ids);
    return head;
}
ListNode* Maker::createDescendingList_Dur(){
    titles.clear();
    deleteDoublyLinkedList(head);
    inorderdurdes(duration.root,duration_table,titles);
    createDLL(titles);
    return head;
}
ListNode* Maker::createAscendingList_Dur(){
    titles.clear();
    deleteDoublyLinkedList(head);
    inorderdurasc(duration.root,duration_table,titles);
    createDLL(titles);
    return head;
}
ListNode* Maker::createAscendingList_rec(){ 
    deleteDoublyLinkedList(head);
    createDLL(recent_add);
    return head;
}
ListNode* Maker::createDescendingList_rec() {
    deleteDoublyLinkedList(head);
    vector<string> reversed = recent_add;
    reverse(reversed.begin(), reversed.end());
    createDLL(reversed);
    return head;
}