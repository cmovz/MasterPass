/* This is a simple binary tree, there's no reason to use a balanced tree
  because keys are random */

#ifndef STORAGE_NODE_H
#define STORAGE_NODE_H

#define STORAGE_KEY_SIZE 16
#define STORAGE_VALUE_SIZE 16

typedef struct StorageNode {
  struct StorageNode *left;
  struct StorageNode *right;
  unsigned char key[STORAGE_KEY_SIZE];
  unsigned char value[STORAGE_VALUE_SIZE];
} StorageNode;

typedef union StorageNodeArg {
  void *ptr;
  int i;
  unsigned int u;
} StorageNodeArg;

/* return 0 only if memory allocation fails */
int storage_node_insert(StorageNode **root, unsigned char const *key,
                        unsigned char const *value);

StorageNode **storage_node_find(StorageNode **root, unsigned char const *key);

/* return 1 if node was present, 0 otherwise */
int storage_node_remove(StorageNode **root, unsigned char const *key);

/* cb is only allowed to change value */
void storage_node_traverse(StorageNode *root,
                           void(*cb)(StorageNode *, StorageNodeArg),
                           StorageNodeArg user_data);

void storage_node_destroy_tree(StorageNode **root);

#endif
