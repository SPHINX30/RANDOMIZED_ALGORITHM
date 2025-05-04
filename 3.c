#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
typedef struct {
    int found;
    int steps;
} SearchResult;

typedef struct CompactTreapNode {
    int key;
    float priority;
    struct CompactTreapNode *left;
    struct CompactTreapNode *right;
} CompactTreapNode;

CompactTreapNode* rotate_right(CompactTreapNode* root) {
    CompactTreapNode* new_root = root->left;
    root->left = new_root->right;
    new_root->right = root;
    return new_root;
}

CompactTreapNode* rotate_left(CompactTreapNode* root) {
    CompactTreapNode* new_root = root->right;
    root->right = new_root->left;
    new_root->left = root;
    return new_root;
}

CompactTreapNode* treap_insert(CompactTreapNode* root, int key) {
    if (!root) {
        CompactTreapNode* node = (CompactTreapNode*)malloc(sizeof(CompactTreapNode));
        node->key = key;
        node->priority = (float)rand() / RAND_MAX;
        node->left = node->right = NULL;
        return node;
    }
    if (key < root->key) {
        root->left = treap_insert(root->left, key);
        if (root->left->priority > root->priority)
            root = rotate_right(root);
    } else {
        root->right = treap_insert(root->right, key);
        if (root->right->priority > root->priority)
            root = rotate_left(root);
    }
    return root;
}


typedef struct SkipListNode {
    int value;
    struct SkipListNode** forward;
} SkipListNode;

typedef struct {
    int max_level;
    int level;
    SkipListNode* header;
} MemoryEfficientSkipList;
SearchResult treap_search(CompactTreapNode* root, int key, int steps) {
    if (!root) {
        SearchResult res = {0, steps};
        return res;
    }
    steps++;
    if (key == root->key) {
        SearchResult res = {1, steps};
        return res;
    }
    if (key < root->key)
        return treap_search(root->left, key, steps);
    else
        return treap_search(root->right, key, steps);
}
MemoryEfficientSkipList* skip_list_init(int max_level) {
    MemoryEfficientSkipList* list = (MemoryEfficientSkipList*)malloc(sizeof(MemoryEfficientSkipList));
    list->max_level = max_level;
    list->level = 1;
    list->header = (SkipListNode*)malloc(sizeof(SkipListNode));
    list->header->forward = (SkipListNode**)calloc(max_level, sizeof(SkipListNode*));
    list->header->value = INT_MIN;
    return list;
}

int random_level(int max_level) {
    int level = 1;
    while ((rand() % 4 == 0) && (level < max_level)) { 
        level++;
    }
    return level;
}

void skip_list_insert(MemoryEfficientSkipList* list, int value) {
    SkipListNode* update[list->max_level];
    SkipListNode* current = list->header;

    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->value < value)
            current = current->forward[i];
        update[i] = current;
    }

    int new_level = random_level(list->max_level);
    if (new_level > list->level) {
        for (int i = list->level; i < new_level; i++)
            update[i] = list->header;
        list->level = new_level;
    }

    SkipListNode* new_node = (SkipListNode*)malloc(sizeof(SkipListNode));
    new_node->value = value;
    new_node->forward = (SkipListNode**)calloc(new_level, sizeof(SkipListNode*));
    for (int i = 0; i < new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
}

SearchResult skip_list_search(MemoryEfficientSkipList* list, int value) {
    int steps = 0;
    SkipListNode* current = list->header;
    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->value <= value) {
            current = current->forward[i];
            steps++;
            if (current->value == value) {
                SearchResult res = {1, steps};
                return res;
            }
        }
    }
    SearchResult res = {0, steps};
    return res;
}
void benchmark_task1(int* n_values, int num_n, int iterations) {
    for (int i = 0; i < num_n; i++) {
        int n = n_values[i];
        char filename[256];
        sprintf(filename, "data_%d.bin", n);

        struct stat st;
        if (stat(filename, &st) != 0)
            generate_and_save_data(n, filename);
        int adjusted_iter = iterations;

        double qs_total = 0;
        for (int j = 0; j < adjusted_iter; j++) {
            int fd = open(filename, O_RDWR);
            uint32_t* arr = mmap(NULL, n * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            int k = rand() % n;
            clock_t start = clock();
            quickselect_memmap(arr, 0, n - 1, k);
            qs_total += (double)(clock() - start) / CLOCKS_PER_SEC;
            munmap(arr, n * sizeof(uint32_t));
            close(fd);
        }
        double avg_qs = qs_total / adjusted_iter;

        int fd = open(filename, O_RDWR);
        uint32_t* arr = mmap(NULL, n * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        clock_t start = clock();
        quicksort(arr, 0, n - 1);
        double qsort_time = (double)(clock() - start) / CLOCKS_PER_SEC;

        munmap(arr, n * sizeof(uint32_t));
        close(fd);

        printf("n=%d: QuickSelect %.4fs, QuickSort %.4fs\n", n, avg_qs, qsort_time);
        }
}
void benchmark_task2(int* n_values, int num_n, int search_ops) {
    for (int i = 0; i < num_n; i++) {
        int n = n_values[i];
        
        srand(0);
        
        CompactTreapNode* treap = NULL;
        MemoryEfficientSkipList* list = skip_list_init(12); 
        
        for (int j = 0; j < n; j++) {
            int key = rand() % (n * 10);
            treap = treap_insert(treap, key);
            skip_list_insert(list, key);
        }
        
        int treap_steps = 0, skip_steps = 0;
        srand(1); 
        for (int j = 0; j < search_ops; j++) {
            int key;
            if (rand() % 2) {
                int index = rand() % n;
                srand(0); 
                for (int k = 0; k <= index; k++) {
                    key = rand() % (n * 10);
                }
            } else {
                key = rand() % (n * 10) + n * 10;
            }
            
            SearchResult res = treap_search(treap, key, 0);
            treap_steps += res.steps;
            res = skip_list_search(list, key);
            skip_steps += res.steps;
        }
        
        printf("n=%d: Treap %.2f, SkipList %.2f\n", n, (double)treap_steps/search_ops, (double)skip_steps/search_ops);
    }
}

int main() {
    srand(time(NULL));
    printf("Task 1 Benchmarks:\n");
    int task1_n[] = {10000000, 20000000, 40000000, 80000000, 160000000};
    benchmark_task1(task1_n, 5, 1000);
    printf("\nTask 2 Benchmarks:\n");
    int task2_n[] = {5000000, 10000000, 20000000, 50000000};
    benchmark_task2(task2_n, 4, 100000);
    
    return 0;
}