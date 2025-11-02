#include <iostream>
#include <fstream>
#include <unordered_map>
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
    unordered_map<int, vector<string>> duration_table;
    unordered_map<string, json> song_table;
    vector <string> titles;
    AVL<string> title;
    AVL<int> duration;
    ListNode *  head;
    string type;
    bool aod;
public:
    Maker();
    void createDLL(vector<string> & arr);
    void deleteDoublyLinkedList(ListNode* head);
    ListNode* createAscendingList();
    ListNode*createDescendingList();
    ListNode*createDescendingList_Dur();
    ListNode*createAscendingList_Dur();
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
    titles=hashtovector(song_table);
    title.createtree(titles,titles.size());
    vector<int> dur=hashtovector_int(duration_table);
    duration.createtree(dur,dur.size());
    titles.clear();
}
void Maker::createDLL(vector<string> & arr){
    ListNode*temp=head;
    ListNode*previous=nullptr;
    for(string s:arr){
        ListNode*newnode=new ListNode(s);
        if(!head){
            head=newnode;
            temp=head;
            previous=head;
            continue;
        }
        temp->next=newnode;
        temp=temp->next;
        temp->prev=previous;
        previous=temp;
    }
}
void Maker::deleteDoublyLinkedList(ListNode* head) {
    if (head == nullptr) {
        return;
    }
    deleteDoublyLinkedList(head->next);
    delete head;
}
ListNode* Maker::createAscendingList(){
    titles.clear();
    deleteDoublyLinkedList(head);
    title.displayinorder(titles);
    createDLL(titles);
    return head;
}
ListNode* Maker::createDescendingList(){
    titles.clear();
    deleteDoublyLinkedList(head);
    title.displayinorderdescend(titles);
    createDLL(titles);
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