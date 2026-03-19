#pragma once 

typedef struct Node {
    int data;
    struct Node* first_child;
    struct Node* next_sibling;
} Node;

Node* CreateNode(int value);
Node* FindNode(const Node* root, int value);
void AddNode(Node** root, int parent_value, int new_value);
void FreeTree(Node* root);
void DeleteSubtree(Node** root, int value);
void PrintTree(const Node* root, int depth);
void CheckLevelMonotone(const Node* root);
