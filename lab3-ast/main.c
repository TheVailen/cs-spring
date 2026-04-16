#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 64
#define MAX_TOKENS 256
#define MAX_EXPR_LEN 1024

typedef enum {
    NODE_NUMBER,
    NODE_VARIABLE,
    NODE_OPERATOR
} NodeType;

typedef struct Node {
    NodeType type;
    char value[MAX_TOKEN_LEN];
    struct Node *left;
    struct Node *right;
} Node;

typedef struct {
    char items[MAX_TOKENS][MAX_TOKEN_LEN];
    int count;
    int pos;
} TokenList;

static Node *create_node(NodeType type, const char *value) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "Ошибка: не удалось выделить память\n");
        exit(1);
    }

    node->type = type;
    strncpy(node->value, value, MAX_TOKEN_LEN - 1);
    node->value[MAX_TOKEN_LEN - 1] = '\0';
    node->left = NULL;
    node->right = NULL;
    return node;
}

static Node *copy_tree(const Node *node) {
    if (!node) return NULL;

    Node *copy = create_node(node->type, node->value);
    copy->left = copy_tree(node->left);
    copy->right = copy_tree(node->right);
    return copy;
}

static void free_tree(Node *node) {
    if (!node) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

static int is_operator_char(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

static void tokenize(const char *expr, TokenList *tokens) {
    int i = 0;
    tokens->count = 0;
    tokens->pos = 0;

    while (expr[i] != '\0') {
        if (isspace((unsigned char)expr[i])) {
            i++;
            continue;
        }

        if (isalnum((unsigned char)expr[i]) || expr[i] == '_') {
            int j = 0;
            while (isalnum((unsigned char)expr[i]) || expr[i] == '_') {
                if (j < MAX_TOKEN_LEN - 1) {
                    tokens->items[tokens->count][j++] = expr[i];
                }
                i++;
            }
            tokens->items[tokens->count][j] = '\0';
            tokens->count++;
        } else if (is_operator_char(expr[i]) || expr[i] == '(' || expr[i] == ')') {
            tokens->items[tokens->count][0] = expr[i];
            tokens->items[tokens->count][1] = '\0';
            tokens->count++;
            i++;
        } else {
            fprintf(stderr, "Ошибка: недопустимый символ '%c'\n", expr[i]);
            exit(1);
        }

        if (tokens->count >= MAX_TOKENS) {
            fprintf(stderr, "Ошибка: слишком длинное выражение\n");
            exit(1);
        }
    }
}

static Node *parse_expression(TokenList *tokens);

static Node *parse_factor(TokenList *tokens) {
    if (tokens->pos >= tokens->count) {
        fprintf(stderr, "Ошибка: неожиданный конец выражения\n");
        return NULL;
    }

    const char *token = tokens->items[tokens->pos];

    if (strcmp(token, "(") == 0) {
        tokens->pos++;
        Node *node = parse_expression(tokens);
        if (!node) return NULL;

        if (tokens->pos >= tokens->count || strcmp(tokens->items[tokens->pos], ")") != 0) {
            fprintf(stderr, "Ошибка: ожидалась закрывающая скобка\n");
            free_tree(node);
            return NULL;
        }
        tokens->pos++;
        return node;
    }

    if (strcmp(token, "-") == 0 &&
        (tokens->pos == 0 ||
         strcmp(tokens->items[tokens->pos - 1], "(") == 0 ||
         is_operator_char(tokens->items[tokens->pos - 1][0]))) {

        tokens->pos++;
        Node *operand = parse_factor(tokens);
        if (!operand) return NULL;

        Node *node = create_node(NODE_OPERATOR, "neg");
        node->left = operand;
        return node;
    }

    tokens->pos++;

    if (isdigit((unsigned char)token[0])) {
        return create_node(NODE_NUMBER, token);
    }
    return create_node(NODE_VARIABLE, token);
}

static Node *parse_term(TokenList *tokens) {
    Node *left = parse_factor(tokens);
    if (!left) return NULL;

    while (tokens->pos < tokens->count) {
        const char *op = tokens->items[tokens->pos];
        if (strcmp(op, "*") != 0 && strcmp(op, "/") != 0) {
            break;
        }

        tokens->pos++;
        Node *right = parse_factor(tokens);
        if (!right) {
            free_tree(left);
            return NULL;
        }

        Node *node = create_node(NODE_OPERATOR, op);
        node->left = left;
        node->right = right;
        left = node;
    }

    return left;
}

static Node *parse_expression(TokenList *tokens) {
    Node *left = parse_term(tokens);
    if (!left) return NULL;

    while (tokens->pos < tokens->count) {
        const char *op = tokens->items[tokens->pos];
        if (strcmp(op, "+") != 0 && strcmp(op, "-") != 0) {
            break;
        }

        tokens->pos++;
        Node *right = parse_term(tokens);
        if (!right) {
            free_tree(left);
            return NULL;
        }

        Node *node = create_node(NODE_OPERATOR, op);
        node->left = left;
        node->right = right;
        left = node;
    }

    return left;
}

static void print_tree_impl(const Node *node, const char *prefix, int is_tail) {
    if (!node) return;

    printf("%s%s%s\n", prefix, is_tail ? "└── " : "├── ", node->value);

    char next_prefix[512];
    snprintf(next_prefix, sizeof(next_prefix), "%s%s", prefix, is_tail ? "    " : "│   ");

    if (node->left && node->right) {
        print_tree_impl(node->left, next_prefix, 0);
        print_tree_impl(node->right, next_prefix, 1);
    } else if (node->left) {
        print_tree_impl(node->left, next_prefix, 1);
    } else if (node->right) {
        print_tree_impl(node->right, next_prefix, 1);
    }
}

static void print_tree(const Node *node) {
    if (!node) {
        printf("(пусто)\n");
        return;
    }
    print_tree_impl(node, "", 1);
}

static int is_low_priority_op(const Node *node) {
    if (!node || node->type != NODE_OPERATOR) return 0;
    return strcmp(node->value, "+") == 0 || strcmp(node->value, "-") == 0;
}

static void tree_to_string(const Node *node, char *result) {
    if (!node) {
        result[0] = '\0';
        return;
    }

    if (node->type == NODE_NUMBER || node->type == NODE_VARIABLE) {
        strcpy(result, node->value);
        return;
    }

    if (strcmp(node->value, "neg") == 0) {
        char arg[MAX_EXPR_LEN];
        tree_to_string(node->left, arg);

        if (node->left && node->left->type == NODE_OPERATOR &&
            strcmp(node->left->value, "neg") != 0) {
            snprintf(result, MAX_EXPR_LEN, "-(%s)", arg);
        } else {
            snprintf(result, MAX_EXPR_LEN, "-%s", arg);
        }
        return;
    }

    char left[MAX_EXPR_LEN];
    char right[MAX_EXPR_LEN];
    char left_wrapped[MAX_EXPR_LEN];
    char right_wrapped[MAX_EXPR_LEN];

    tree_to_string(node->left, left);
    tree_to_string(node->right, right);

    if ((strcmp(node->value, "*") == 0 || strcmp(node->value, "/") == 0) && is_low_priority_op(node->left)) {
        snprintf(left_wrapped, sizeof(left_wrapped), "(%s)", left);
    } else {
        strcpy(left_wrapped, left);
    }

    if ((strcmp(node->value, "*") == 0 || strcmp(node->value, "/") == 0) && is_low_priority_op(node->right)) {
        snprintf(right_wrapped, sizeof(right_wrapped), "(%s)", right);
    } else if (strcmp(node->value, "-") == 0 && is_low_priority_op(node->right)) {
        snprintf(right_wrapped, sizeof(right_wrapped), "(%s)", right);
    } else {
        strcpy(right_wrapped, right);
    }

    snprintf(result, MAX_EXPR_LEN, "%s %s %s", left_wrapped, node->value, right_wrapped);
}

static Node *apply_transformation(Node *node) {
    if (!node) return NULL;

    node->left = apply_transformation(node->left);
    node->right = apply_transformation(node->right);

    if (node->type == NODE_OPERATOR && strcmp(node->value, "*") == 0) {
        if (node->right &&
            node->right->type == NODE_OPERATOR &&
            strcmp(node->right->value, "-") == 0) {

            Node *x1 = copy_tree(node->left);
            Node *x2 = copy_tree(node->left);
            Node *a = copy_tree(node->right->left);
            Node *b = copy_tree(node->right->right);

            Node *mul1 = create_node(NODE_OPERATOR, "*");
            Node *mul2 = create_node(NODE_OPERATOR, "*");
            Node *sub = create_node(NODE_OPERATOR, "-");

            mul1->left = x1;
            mul1->right = a;

            mul2->left = x2;
            mul2->right = b;

            sub->left = mul1;
            sub->right = mul2;

            free_tree(node);
            return sub;
        }

        if (node->left &&
            node->left->type == NODE_OPERATOR &&
            strcmp(node->left->value, "-") == 0) {

            Node *a = copy_tree(node->left->left);
            Node *b = copy_tree(node->left->right);
            Node *x1 = copy_tree(node->right);
            Node *x2 = copy_tree(node->right);

            Node *mul1 = create_node(NODE_OPERATOR, "*");
            Node *mul2 = create_node(NODE_OPERATOR, "*");
            Node *sub = create_node(NODE_OPERATOR, "-");

            mul1->left = a;
            mul1->right = x1;

            mul2->left = b;
            mul2->right = x2;

            sub->left = mul1;
            sub->right = mul2;

            free_tree(node);
            return sub;
        }
    }

    return node;
}

int main(void) {
    char expr[MAX_EXPR_LEN];

    printf("Лабораторная работа 3. Вариант 10\n");
    printf("Преобразование: a * (b - c) -> a * b - a * c\n\n");
    printf("Введите выражение: ");

    if (!fgets(expr, sizeof(expr), stdin)) {
        fprintf(stderr, "Ошибка чтения ввода\n");
        return 1;
    }

    expr[strcspn(expr, "\n")] = '\0';

    TokenList tokens;
    tokenize(expr, &tokens);

    if (tokens.count == 0) {
        fprintf(stderr, "Ошибка: пустое выражение\n");
        return 1;
    }

    Node *tree = parse_expression(&tokens);
    if (!tree) {
        return 1;
    }

    if (tokens.pos != tokens.count) {
        fprintf(stderr, "Ошибка: лишние токены в выражении\n");
        free_tree(tree);
        return 1;
    }

    char source[MAX_EXPR_LEN];
    tree_to_string(tree, source);

    printf("\nИсходное выражение:\n%s\n", source);
    printf("\nДерево исходного выражения:\n");
    print_tree(tree);

    tree = apply_transformation(tree);

    char result[MAX_EXPR_LEN];
    tree_to_string(tree, result);

    printf("\nПосле преобразования:\n%s\n", result);
    printf("\nДерево преобразованного выражения:\n");
    print_tree(tree);

    free_tree(tree);
    return 0;
}