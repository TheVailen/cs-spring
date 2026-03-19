#include <stdio.h>

#include "tree.h"

int main(void) {
  Node* root = NULL;

  int choice;
  int value;
  int parent_value;

  while (1) {
    printf("\n==============================\n");
    printf("МЕНЮ\n");
    printf("1. Добавить узел\n");
    printf("2. Вывести дерево\n");
    printf("3. Удалить поддерево\n");
    printf("4. Проверить вариант 10\n");
    printf("0. Выход\n");
    printf("==============================\n");
    printf("Ваш выбор: ");

    if (scanf("%d", &choice) != 1) {
      printf("Ошибка ввода. Программа завершена.\n");
      FreeTree(root);
      return 0;
    }

    switch (choice) {
      case 1:
        if (root == NULL) {
          printf("Дерево пустое. Введите значение корня: ");

          if (scanf("%d", &value) != 1) {
            printf("Ошибка ввода. Программа завершена.\n");
            FreeTree(root);
            return 0;
          }

          AddNode(&root, 0, value);
        } else {
          printf("Введите значение родителя: ");
          if (scanf("%d", &parent_value) != 1) {
            printf("Ошибка ввода. Программа завершена.\n");
            FreeTree(root);
            return 0;
          }

          printf("Введите значение нового узла: ");
          if (scanf("%d", &value) != 1) {
            printf("Ошибка ввода. Программа завершена.\n");
            FreeTree(root);
            return 0;
          }

          AddNode(&root, parent_value, value);
        }
        break;

      case 2:
        if (root == NULL) {
          printf("Дерево пустое.\n");
        } else {
          printf("Текущее дерево:\n");
          PrintTree(root, 0);
        }
        break;

      case 3:
        if (root == NULL) {
          printf("Дерево пустое, удалять нечего.\n");
        } else {
          printf("Введите значение узла, поддерево которого нужно удалить: ");

          if (scanf("%d", &value) != 1) {
            printf("Ошибка ввода. Программа завершена.\n");
            FreeTree(root);
            return 0;
          }

          DeleteSubtree(&root, value);
        }
        break;

      case 4:
        CheckLevelMonotone(root);
        break;

      case 0:
        FreeTree(root);
        printf("Память очищена. Программа завершена.\n");
        return 0;

      default:
        printf("Такого пункта меню не существует.\n");
        break;
    }
  }
}