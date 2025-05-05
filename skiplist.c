#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<limits.h>
#include<math.h>
#define LOG2(x) (log(x) / log(2))
typedef struct {
    int found;
    int steps;
} SearchResult;
double log2_custom(double x) {
    return log(x) / log(2.0);
}

typedef struct Treapnode {
    int key;
    float priority;
    struct Treapnode *left;
    struct Treapnode *right;
} Treapnode;

Treapnode* rotate_right(Treapnode* root) {
    Treapnode* new_root = root->left;
    root->left = new_root->right;
    new_root->right = root;
    return new_root;
}

Treapnode* rotate_left(Treapnode* root) {
    Treapnode* new_root = root->right;
    root->right = new_root->left;
    new_root->left = root;
    return new_root;
}

Treapnode* treap_insert(Treapnode* root, int key) {
    if (!root) {
        Treapnode* node = (Treapnode*)malloc(sizeof(Treapnode));
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

SearchResult treap_search(Treapnode* root, int key, int steps) {
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

typedef struct node {
    int value;
    struct node** forward;
} node;

typedef struct {
    int max_level;
    int level;
    node* header;
} skiplist;

skiplist* skip_list_init(int max_level) {
    skiplist* list = (skiplist*)malloc(sizeof(skiplist));
    list->max_level = max_level;
    list->level = 1;
    list->header = (node*)malloc(sizeof(node));
    list->header->forward = (node**)calloc(max_level, sizeof(node*));
    list->header->value = INT_MIN;
    return list;
}

int random_level(int max_level) {
    int level = 1;
    while ((rand() % 2) && (level < max_level))
        level++;
    return level;
}
void free_skip_list(skiplist* list) {
    node* current = list->header;
    while (current) {
        node* next = current->forward[0]; 
        free(current->forward); 
        free(current); 
        current = next;
    }
    free(list); 
}
void free_treap(Treapnode* root) {
    if (!root) return;
    free_treap(root->left);  
    free_treap(root->right); 
    free(root);
}
void skip_list_insert(skiplist* list, int value) {
    node* update[list->max_level];
    node* current = list->header;

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

    node* new_node = (node*)malloc(sizeof(node));
    new_node->value = value;
    new_node->forward = (node**)calloc(new_level, sizeof(node*));
    for (int i = 0; i < new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
}

SearchResult skip_list_search(skiplist* list, int value) {
    int steps = 0;
    node* current = list->header;
    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->value <= value) {
            current = current->forward[i];
            steps++;
            if (current->value == value) {
                return (SearchResult){1, steps};
            }
        }
    }
    return (SearchResult){0, steps};
}

void benchmark_task2(int* n_values, int num_n, int search_ops) {
    for (int i = 0; i < num_n; i++) {
        int n = n_values[i];
        int* keys = (int*)malloc(n * sizeof(int));
        for (int j = 0; j < n; j++)
            keys[j] = rand() % (n * 10);

        Treapnode* treap = NULL;
        for (int j = 0; j < n; j++)
            treap = treap_insert(treap, keys[j]);

        int max_level = (int)(log2_custom(n) + 1);
        skiplist* list = skip_list_init(max_level);
        for (int j = 0; j < n; j++){
            skip_list_insert(list, keys[j]);
        }
        int treap_steps = 0, skip_steps = 0;
        
        for (int j = 0; j < search_ops; j++) {
            int key = (rand() % 2) ? keys[rand() % n] : rand() % (n * 10) + n * 10;
            SearchResult res = treap_search(treap, key, 0);
            treap_steps += res.steps;
            res = skip_list_search(list, key);
            skip_steps += res.steps;
        }

        printf("n=%d: Treap %.2f, SkipList %.2f\n", n, (double)treap_steps/search_ops, (double)skip_steps/search_ops);
        free_treap(treap);
        free_skip_list(list);
        free(keys);
    }
}

int main() {
    srand(time(NULL));
    printf("\nTask 2 Benchmarks:\n");
    int task2_n[] = {5000000, 10000000, 20000000, 50000000};
    benchmark_task2(task2_n, 4, 1000000);
    return 0;
}