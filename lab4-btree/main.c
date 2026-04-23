#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAX_KEY_LEN 6
#define INDENT_STEP 4

typedef struct BTreeNode {
    int n;
    int leaf;
    char **keys;
    double *values;
    struct BTreeNode **children;
} BTreeNode;

typedef struct {
    BTreeNode *root;
    int t;
} BTree;

static int is_valid_key(const char *s) {
    size_t len = strlen(s);
    if (len == 0 || len > MAX_KEY_LEN) return 0;
    for (size_t i = 0; i < len; i++) {
        if (!isalpha((unsigned char)s[i])) {
            return 0;
        }
    }
    return 1;
}

static BTreeNode *create_node(int t, int leaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    if (!node) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        exit(1);
    }
    
    node->n = 0;
    node->leaf = leaf;
    node->keys = (char **)calloc(2 * t - 1, sizeof(char *));
    node->values = (double *)calloc(2 * t - 1, sizeof(double));
    node->children = (BTreeNode **)calloc(2 * t, sizeof(BTreeNode *));
    
    if (!node->keys || !node->values || !node->children) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        exit(1);
    }
    
    return node;
}

static void free_node(BTreeNode *node, int t) {
    if (!node) return;
    
    if (!node->leaf) {
        for (int i = 0; i <= node->n; i++) {
            free_node(node->children[i], t);
        }
    }
    
    for (int i = 0; i < node->n; i++) {
        free(node->keys[i]);
    }
    
    free(node->keys);
    free(node->values);
    free(node->children);
    free(node);
}

static void btree_init(BTree *tree, int t) {
    tree->root = NULL;
    tree->t = t;
}

static int find_key_position(BTreeNode *node, const char *key) {
    int pos = 0;
    while (pos < node->n && strcmp(node->keys[pos], key) < 0) {
        pos++;
    }
    return pos;
}

static BTreeNode *search_node(BTreeNode *node, const char *key, int *index) {
    if (!node) return NULL;
    
    int i = 0;
    while (i < node->n && strcmp(key, node->keys[i]) > 0) {
        i++;
    }
    
    if (i < node->n && strcmp(key, node->keys[i]) == 0) {
        if (index) *index = i;
        return node;
    }
    
    if (node->leaf) return NULL;
    
    return search_node(node->children[i], key, index);
}

static void split_child(BTreeNode *parent, int index, int t) {
    BTreeNode *full_child = parent->children[index];
    BTreeNode *new_child = create_node(t, full_child->leaf);
    new_child->n = t - 1;
    
    for (int j = 0; j < t - 1; j++) {
        new_child->keys[j] = full_child->keys[j + t];
        new_child->values[j] = full_child->values[j + t];
        full_child->keys[j + t] = NULL;
    }
    
    if (!full_child->leaf) {
        for (int j = 0; j < t; j++) {
            new_child->children[j] = full_child->children[j + t];
            full_child->children[j + t] = NULL;
        }
    }
    
    full_child->n = t - 1;
    
    for (int j = parent->n; j >= index + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[index + 1] = new_child;
    
    for (int j = parent->n - 1; j >= index; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->values[j + 1] = parent->values[j];
    }
    
    parent->keys[index] = full_child->keys[t - 1];
    parent->values[index] = full_child->values[t - 1];
    full_child->keys[t - 1] = NULL;
    
    parent->n++;
}

static void insert_nonfull(BTreeNode *node, const char *key, double value, int t) {
    int i = node->n - 1;
    
    if (node->leaf) {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        
        node->keys[i + 1] = strdup(key);
        if (!node->keys[i + 1]) {
            fprintf(stderr, "Ошибка выделения памяти\n");
            exit(1);
        }
        node->values[i + 1] = value;
        node->n++;
    } else {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) {
            i--;
        }
        i++;
        
        if (node->children[i]->n == 2 * t - 1) {
            split_child(node, i, t);
            if (strcmp(key, node->keys[i]) > 0) {
                i++;
            }
        }
        insert_nonfull(node->children[i], key, value, t);
    }
}

static int btree_insert(BTree *tree, const char *key, double value, char *msg, size_t msg_sz) {
    if (!is_valid_key(key)) {
        snprintf(msg, msg_sz, "Ошибка: некорректный ключ");
        return 0;
    }
    
    if (search_node(tree->root, key, NULL)) {
        snprintf(msg, msg_sz, "Ошибка: ключ уже существует");
        return 0;
    }
    
    if (!tree->root) {
        tree->root = create_node(tree->t, 1);
        tree->root->keys[0] = strdup(key);
        tree->root->values[0] = value;
        tree->root->n = 1;
        snprintf(msg, msg_sz, "Ok");
        return 1;
    }
    
    if (tree->root->n == 2 * tree->t - 1) {
        BTreeNode *new_root = create_node(tree->t, 0);
        new_root->children[0] = tree->root;
        tree->root = new_root;
        split_child(new_root, 0, tree->t);
        insert_nonfull(new_root, key, value, tree->t);
    } else {
        insert_nonfull(tree->root, key, value, tree->t);
    }
    
    snprintf(msg, msg_sz, "Ok");
    return 1;
}

static void remove_from_leaf(BTreeNode *node, int idx) {
    free(node->keys[idx]);
    
    for (int i = idx + 1; i < node->n; i++) {
        node->keys[i - 1] = node->keys[i];
        node->values[i - 1] = node->values[i];
    }
    
    node->keys[node->n - 1] = NULL;
    node->n--;
}

static void get_predecessor(BTreeNode *node, int idx, char **key, double *value) {
    BTreeNode *current = node->children[idx];
    while (!current->leaf) {
        current = current->children[current->n];
    }
    *key = current->keys[current->n - 1];
    *value = current->values[current->n - 1];
}

static void get_successor(BTreeNode *node, int idx, char **key, double *value) {
    BTreeNode *current = node->children[idx + 1];
    while (!current->leaf) {
        current = current->children[0];
    }
    *key = current->keys[0];
    *value = current->values[0];
}

static void merge_children(BTreeNode *parent, int idx, int t) {
    BTreeNode *child = parent->children[idx];
    BTreeNode *sibling = parent->children[idx + 1];
    
    child->keys[t - 1] = parent->keys[idx];
    child->values[t - 1] = parent->values[idx];
    
    for (int i = 0; i < sibling->n; i++) {
        child->keys[i + t] = sibling->keys[i];
        child->values[i + t] = sibling->values[i];
        sibling->keys[i] = NULL;
    }
    
    if (!child->leaf) {
        for (int i = 0; i <= sibling->n; i++) {
            child->children[i + t] = sibling->children[i];
            sibling->children[i] = NULL;
        }
    }
    
    for (int i = idx + 1; i < parent->n; i++) {
        parent->keys[i - 1] = parent->keys[i];
        parent->values[i - 1] = parent->values[i];
    }
    
    for (int i = idx + 2; i <= parent->n; i++) {
        parent->children[i - 1] = parent->children[i];
    }
    
    parent->keys[parent->n - 1] = NULL;
    parent->children[parent->n] = NULL;
    
    child->n += sibling->n + 1;
    parent->n--;
    
    free(sibling->keys);
    free(sibling->values);
    free(sibling->children);
    free(sibling);
}

static void borrow_from_prev(BTreeNode *parent, int idx) {
    BTreeNode *child = parent->children[idx];
    BTreeNode *sibling = parent->children[idx - 1];
    
    for (int i = child->n - 1; i >= 0; i--) {
        child->keys[i + 1] = child->keys[i];
        child->values[i + 1] = child->values[i];
    }
    
    if (!child->leaf) {
        for (int i = child->n; i >= 0; i--) {
            child->children[i + 1] = child->children[i];
        }
    }
    
    child->keys[0] = parent->keys[idx - 1];
    child->values[0] = parent->values[idx - 1];
    
    if (!child->leaf) {
        child->children[0] = sibling->children[sibling->n];
        sibling->children[sibling->n] = NULL;
    }
    
    parent->keys[idx - 1] = sibling->keys[sibling->n - 1];
    parent->values[idx - 1] = sibling->values[sibling->n - 1];
    sibling->keys[sibling->n - 1] = NULL;
    
    child->n++;
    sibling->n--;
}

static void borrow_from_next(BTreeNode *parent, int idx) {
    BTreeNode *child = parent->children[idx];
    BTreeNode *sibling = parent->children[idx + 1];
    
    child->keys[child->n] = parent->keys[idx];
    child->values[child->n] = parent->values[idx];
    
    if (!child->leaf) {
        child->children[child->n + 1] = sibling->children[0];
    }
    
    parent->keys[idx] = sibling->keys[0];
    parent->values[idx] = sibling->values[0];
    
    for (int i = 1; i < sibling->n; i++) {
        sibling->keys[i - 1] = sibling->keys[i];
        sibling->values[i - 1] = sibling->values[i];
    }
    
    if (!sibling->leaf) {
        for (int i = 1; i <= sibling->n; i++) {
            sibling->children[i - 1] = sibling->children[i];
        }
    }
    
    sibling->keys[sibling->n - 1] = NULL;
    if (!sibling->leaf) {
        sibling->children[sibling->n] = NULL;
    }
    
    child->n++;
    sibling->n--;
}

static void fill_child(BTreeNode *parent, int idx, int t) {
    if (idx != 0 && parent->children[idx - 1]->n >= t) {
        borrow_from_prev(parent, idx);
    } else if (idx != parent->n && parent->children[idx + 1]->n >= t) {
        borrow_from_next(parent, idx);
    } else {
        if (idx != parent->n) {
            merge_children(parent, idx, t);
        } else {
            merge_children(parent, idx - 1, t);
        }
    }
}

static void remove_from_nonleaf(BTreeNode *node, int idx, int t);
static int delete_from_node(BTreeNode *node, const char *key, int t);

static void remove_from_nonleaf(BTreeNode *node, int idx, int t) {
    char *key = node->keys[idx];
    
    if (node->children[idx]->n >= t) {
        char *pred_key;
        double pred_val;
        get_predecessor(node, idx, &pred_key, &pred_val);
        
        free(node->keys[idx]);
        node->keys[idx] = strdup(pred_key);
        node->values[idx] = pred_val;
        delete_from_node(node->children[idx], pred_key, t);
    } else if (node->children[idx + 1]->n >= t) {
        char *succ_key;
        double succ_val;
        get_successor(node, idx, &succ_key, &succ_val);
        
        free(node->keys[idx]);
        node->keys[idx] = strdup(succ_key);
        node->values[idx] = succ_val;
        delete_from_node(node->children[idx + 1], succ_key, t);
    } else {
        merge_children(node, idx, t);
        delete_from_node(node->children[idx], key, t);
        free(key);
    }
}

static int delete_from_node(BTreeNode *node, const char *key, int t) {
    int idx = find_key_position(node, key);
    
    if (idx < node->n && strcmp(node->keys[idx], key) == 0) {
        if (node->leaf) {
            remove_from_leaf(node, idx);
        } else {
            remove_from_nonleaf(node, idx, t);
        }
        return 1;
    }
    
    if (node->leaf) return 0;
    
    int is_in_last_child = (idx == node->n);
    
    if (node->children[idx]->n < t) {
        fill_child(node, idx, t);
    }
    
    if (is_in_last_child && idx > node->n) {
        return delete_from_node(node->children[idx - 1], key, t);
    }
    
    return delete_from_node(node->children[idx], key, t);
}

static int btree_delete(BTree *tree, const char *key, char *msg, size_t msg_sz) {
    if (!tree->root) {
        snprintf(msg, msg_sz, "Ошибка: дерево пустое");
        return 0;
    }
    
    if (!is_valid_key(key)) {
        snprintf(msg, msg_sz, "Ошибка: некорректный ключ");
        return 0;
    }
    
    if (!delete_from_node(tree->root, key, tree->t)) {
        snprintf(msg, msg_sz, "Ошибка: ключ не найден");
        return 0;
    }
    
    if (tree->root->n == 0) {
        BTreeNode *old_root = tree->root;
        if (old_root->leaf) {
            free(old_root->keys);
            free(old_root->values);
            free(old_root->children);
            free(old_root);
            tree->root = NULL;
        } else {
            tree->root = old_root->children[0];
            old_root->children[0] = NULL;
            free(old_root->keys);
            free(old_root->values);
            free(old_root->children);
            free(old_root);
        }
    }
    
    snprintf(msg, msg_sz, "Ok");
    return 1;
}

static int btree_search(BTree *tree, const char *key, double *out_value, char *msg, size_t msg_sz) {
    if (!tree->root) {
        snprintf(msg, msg_sz, "Не найдено");
        return 0;
    }
    
    int idx = -1;
    BTreeNode *node = search_node(tree->root, key, &idx);
    
    if (!node) {
        snprintf(msg, msg_sz, "Не найдено");
        return 0;
    }
    
    *out_value = node->values[idx];
    snprintf(msg, msg_sz, "Найдено %.10g", *out_value);
    return 1;
}

static void print_node(FILE *out, BTreeNode *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth * INDENT_STEP; i++) {
        fputc(' ', out);
    }
    
    fputc('[', out);
    for (int i = 0; i < node->n; i++) {
        fprintf(out, "%s:%.10g", node->keys[i], node->values[i]);
        if (i + 1 < node->n) {
            fprintf(out, " | ");
        }
    }
    fprintf(out, "]\n");
    
    if (!node->leaf) {
        for (int i = 0; i <= node->n; i++) {
            print_node(out, node->children[i], depth + 1);
        }
    }
}

static void process_file(BTree *tree, FILE *in, FILE *out) {
    char line[256];
    char key[64];
    double value;
    char result[128];
    int first_line = 1;
    
    while (fgets(line, sizeof(line), in)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') continue;
        
        if (!first_line) fputc('\n', out);
        first_line = 0;
        
        fprintf(out, "%s\n", p);
        
        int op;
        if (sscanf(p, "%d", &op) != 1) {
            fprintf(out, "Ошибка: некорректная команда\n");
            continue;
        }
        
        switch (op) {
            case 1: {
                char extra[64];
                int n = sscanf(p, "1 %6s %lf %63s", key, &value, extra);
                if (n != 2) {
                    fprintf(out, "Ошибка: некорректный формат вставки\n");
                } else {
                    btree_insert(tree, key, value, result, sizeof(result));
                    fprintf(out, "%s\n", result);
                }
                break;
            }
            case 2: {
                char extra[64];
                int n = sscanf(p, "2 %6s %63s", key, extra);
                if (n != 1) {
                    fprintf(out, "Ошибка: некорректный формат удаления\n");
                } else {
                    btree_delete(tree, key, result, sizeof(result));
                    fprintf(out, "%s\n", result);
                }
                break;
            }
            case 3: {
                char extra[64];
                int n = sscanf(p, "%d %63s", &op, extra);
                if (n != 1) {
                    fprintf(out, "Ошибка: некорректный формат печати\n");
                } else {
                    if (!tree->root) {
                        fprintf(out, "<пусто>\n");
                    } else {
                        print_node(out, tree->root, 0);
                    }
                }
                break;
            }
            case 4: {
                char extra[64];
                int n = sscanf(p, "4 %6s %63s", key, extra);
                if (n != 1) {
                    fprintf(out, "Ошибка: некорректный формат поиска\n");
                } else {
                    btree_search(tree, key, &value, result, sizeof(result));
                    fprintf(out, "%s\n", result);
                }
                break;
            }
            default:
                fprintf(out, "Ошибка: неизвестная операция\n");
                break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <входной_файл.txt> <выходной_файл.txt> <t>\n", argv[0]);
        fprintf(stderr, "  t - минимальная степень B-дерева, целое число >= 2\n");
        return 1;
    }
    
    char *endptr = NULL;
    errno = 0;
    long t_long = strtol(argv[3], &endptr, 10);
    
    if (errno != 0 || *endptr != '\0' || t_long < 2 || t_long > 1000) {
        fprintf(stderr, "Некорректное значение t. Должно быть целым числом >= 2.\n");
        return 1;
    }
    
    int t = (int)t_long;
    
    FILE *in = fopen(argv[1], "r");
    if (!in) {
        perror("Не удалось открыть входной файл");
        return 1;
    }
    
    FILE *out = fopen(argv[2], "w");
    if (!out) {
        perror("Не удалось открыть выходной файл");
        fclose(in);
        return 1;
    }
    
    BTree tree;
    btree_init(&tree, t);
    
    process_file(&tree, in, out);
    
    free_node(tree.root, tree.t);
    fclose(in);
    fclose(out);
    
    return 0;
}