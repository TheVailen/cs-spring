#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_RECORDS 100

typedef struct {
    char key_text[100];
    int key_num;
    char value[100];
} KeyValuePair;

int read_table(const char *filename, KeyValuePair a[]) {
    FILE *file = fopen(filename, "r");
    int n = 0;

    if (file == NULL) {
        printf("Cannot open file");
        return -1;
    }


    while (n < MAX_RECORDS && fscanf(file, "%s %d %s", a[n].key_text, &a[n].key_num, a[n].value) == 3) {
        n++;
    }

    fclose(file);
    return n;
    
}

void print_table(KeyValuePair table[], int n) {
    for (int i = 0; i < n; i++) {
        printf("%s %d %s\n",
               table[i].key_text,
               table[i].key_num,
               table[i].value);
    }
}

int compare_records(const KeyValuePair *a, const KeyValuePair *b) {
    int text_compare = strcmp((*a).key_text, (*b).key_text);

    if (text_compare < 0) {
        return -1;
    } else if (text_compare > 0) {
        return 1;
    } 

    if ((*a).key_num < (*b).key_num) {
        return -1;
    }
    if ((*a).key_num > (*b).key_num) {
        return 1;
    }

    return 0;
}

void sort_records(KeyValuePair table[], int n) {
    int count[MAX_RECORDS];
    KeyValuePair sorted[MAX_RECORDS];

    for (int i = 0; i < n; i++) {
        count[i] = 0;
    }

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if(compare_records(&table[i], &table[j]) > 0) {
                count[i]++;
            } else {
                count[j]++;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        sorted[count[i]] = table[i];
    }

    for (int i = 0; i < n; i++) {
        table[i] = sorted[i];
    }

}

int binary_search(const KeyValuePair table[], int n, const KeyValuePair *key) {
    int left = 0;
    int right = n - 1;

    while (left <= right) {
        int mid = (left + right) / 2;
        int cmp = compare_records(&table[mid], key);

        if (cmp == 0) {
            return mid;
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return -1;
}

int main(int argc, char *argv[]) {
    KeyValuePair table[MAX_RECORDS];
    KeyValuePair key;
    int n; 
    int index;

    if (argc < 2) {
        printf("Not enough arguments for run\n");
        return 1;
    }

    n = read_table(argv[1], table);
    if (n < 0) {
        return 1;
    }

    printf("Исходная таблица:\n\n");
    print_table(table, n);
    
    printf("\n");

    printf("Отсортированная таблица:\n\n");
    sort_records(table, n);
    print_table(table, n);
    printf("\n");

    while (scanf("%99s %d", key.key_text, &key.key_num) == 2) {
        index = binary_search(table, n, &key);

        if (index >= 0) { 
            printf("%s %d %s\n", table[index].key_text, table[index].key_num, table[index].value);
        } else {
            printf("Key not found\n");
        }
    }

    return 0;

}    
