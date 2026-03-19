#include <stdio.h>
#include <stdlib.h>

#include "tree.h"

static const int ROOT_DEPTH = 0;
static const int INDENT_WIDTH = 2;

static Node* FindNodeInternal(Node* root, int value) {
    if (root == NULL) {
        return NULL;
    }

    if (root->data == value) {
        return root;
    }

    Node* found_in_child = FindNodeInternal(root->first_child, value);
    if (found_in_child != NULL) {
        return found_in_child;
    }

    return FindNodeInternal(root->next_sibling, value);
}

static void DeleteSubtreeRecursive(Node** current_node_ptr, int value) {
    if (current_node_ptr == NULL || *current_node_ptr == NULL) {
        return;
    }

    if ((*current_node_ptr)->data == value) {
        Node* node_to_delete = *current_node_ptr;
        Node* next_sibling = node_to_delete->next_sibling;

        FreeTree(node_to_delete->first_child);
        free(node_to_delete);

        *current_node_ptr = next_sibling;
        return;
    }

    DeleteSubtreeRecursive(&(*current_node_ptr)->first_child, value);
    DeleteSubtreeRecursive(&(*current_node_ptr)->next_sibling, value);
}

static int GetMaxDepth(const Node* root, int depth) {
  if (root == NULL) {
    return depth - 1;
  }

  int max_depth = depth;
  const Node* child = root->first_child;

  while (child != NULL) {
    int child_depth = GetMaxDepth(child, depth + 1);
    if (child_depth > max_depth) {
      max_depth = child_depth;
    }
    child = child->next_sibling;
  }

  return max_depth;
}

static void CountNodesAtLevels(const Node* root, int depth, int* counts) {
  if (root == NULL) {
    return;
  }

  ++counts[depth];
  CountNodesAtLevels(root->first_child, depth + 1, counts);
  CountNodesAtLevels(root->next_sibling, depth, counts);
}

Node* CreateNode(int value) {
    Node* new_node = (Node*)malloc(sizeof(Node));

    if (new_node == NULL) {
        return NULL;
    }

    new_node->data = value;
    new_node->first_child = NULL;
    new_node->next_sibling = NULL;

    return new_node;
}

Node* FindNode(const Node* root, int value) {
    return FindNodeInternal((Node*)root, value);
}

void AddNode(Node ** root, int parent_value, int new_value) {
    if (root == NULL) {
        printf("Ошибка: неверный указатель на корень дерева\n");
        return;
    }

    if (*root == NULL) {
        *root = CreateNode(new_value);

        if (*root == NULL) {
            printf("Ошибка: не удалось выделить память под новый корень\n");
            return;
        }

        printf("Корень %d успешно создан.\n", new_value);
        return;
    }

    Node* parent_node = FindNodeInternal(*root, parent_value);
    if (parent_node == NULL) {
        printf("Ошибка: родитель со значением %d не найден\n", parent_value);
        return;
    }

    Node* new_node = CreateNode(new_value);
    if (new_node == NULL) {
        printf("Ошибка: не удалось выделить память под новый узел\n");
        return;
    }

    if (parent_node->first_child == NULL) {
        parent_node->first_child = new_node;
    } else {
        Node* last_child = parent_node->first_child;
        
        while (last_child->next_sibling != NULL) {
            last_child = last_child->next_sibling;
        }

        last_child->next_sibling = new_node;
    }

    printf("Узел %d добавлен к родителю %d\n", new_value, parent_value);
}

void FreeTree(Node* root) {
    if (root == NULL) {
        return;
    }

    FreeTree(root->first_child);
    FreeTree(root->next_sibling);
    free(root);
}

void DeleteSubtree(Node** root, int value) {
    if (root == NULL || *root == NULL) {
        printf("Дерево пустое, удалять нечего\n");
        return;
    }

    Node* found_node = FindNodeInternal(*root, value);
    if (found_node == NULL) {
        printf("Узел со значением %d не найден\n", value);
        return;
    }

    DeleteSubtreeRecursive(root, value);
    printf("Поддерево с корнем %d удалено\n", value);
}

void PrintTree(const Node* root, int depth) {
    if (root == NULL) {
        return;
    }

    for (int i = 0; i < depth * INDENT_WIDTH; ++i) {
        printf(" ");
    }

    printf("|-- %d\n", root->data);

    PrintTree(root->first_child, depth + 1);
    PrintTree(root->next_sibling, depth);
}

void CheckLevelMonotone(const Node* root) {
    if (root == NULL) {
        printf("Дерево пустое\n");
        return;
    }

    int max_depth = GetMaxDepth(root, ROOT_DEPTH);
    int level_count_size = max_depth + 1;
    int* counts = (int*)calloc((size_t)level_count_size, sizeof(int));

    if (counts == NULL) {
        printf("Ошибка: не удаолось выделить память для подсчёта уровней\n");
        return;
    }

    CountNodesAtLevels(root, ROOT_DEPTH, counts);

    printf("Ширина уровней дерева:\n");
    for (int level = 0; level <= max_depth; ++level) {
        printf("Уровень %d: %d узлов\n", level, counts[level]);
    }

    int is_monotony = 1;

    for (int level = 1; level <= max_depth; ++level) {
        if (counts[level] < counts[level - 1]) {
            is_monotony = 0;
            break;
        }
    }

    if (is_monotony) {
        printf("Ширина уровней дерева монотонно не убывает (верно)\n");
    } else {
        printf("Ширина уровней дерева не монотонна\n");
    }

    free(counts);
}