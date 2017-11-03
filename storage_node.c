#include "storage_node.h"
#include <stdlib.h>
#include <string.h>

/* This is used to find nodes in the tree and receive the results */
typedef struct NodeQuery {
  StorageNode** node;
  unsigned char const* key;
  int to_the_left;
} StorageNodeQuery;

static void make_query(StorageNodeQuery* q, StorageNode** root,
                       unsigned char const* key)
{
  q->node = root;
  q->key = key;
  q->to_the_left = 0;
}

/* returns the element or where the element should be */
static void find_node(StorageNodeQuery* q)
{
  int cmp;

  if(NULL != *q->node){
    cmp = memcmp((*q->node)->key, q->key, STORAGE_KEY_SIZE);
    if(cmp < 0){
      q->node = &(*q->node)->left;
      q->to_the_left = 1;
      find_node(q);
    }
    else
    if(cmp > 0){
      q->node = &(*q->node)->right;
      q->to_the_left = 0;
      find_node(q);
    }
  }
}

static void insert_node(StorageNode** node, StorageNode* new_node)
{
  StorageNodeQuery q;

  make_query(&q, node, new_node->key);
  find_node(&q);
  free(*q.node);
  *q.node = new_node;
}

static void traverse_nodes(StorageNode* node,
                           void(*cb)(StorageNode*, StorageNodeArg),
                           StorageNodeArg user_data)
{
  StorageNode* right;

  if(!node)
    return;

  traverse_nodes(node->left, cb, user_data);
  right = node->right;

  /* this might free node */
  cb(node, user_data);

  traverse_nodes(right, cb, user_data);
}

static void free_node(StorageNode* node, StorageNodeArg user_data)
{
  free(node);
}

static void destroy_nodes(StorageNode* root)
{
  StorageNodeArg ignore;
  traverse_nodes(root, free_node, ignore);
}

static StorageNode** get_rightmost_child(StorageNode* node)
{
  if(NULL == node->right)
    return &node->right;

  return get_rightmost_child(node->right);
}

static StorageNode** get_leftmost_child(StorageNode* node)
{
  if(NULL == node->left)
    return &node->left;

  return get_leftmost_child(node->left);
}

static int remove_node(StorageNode** root, unsigned char const* key)
{
  StorageNodeQuery q;
  StorageNode* removed_node;

  make_query(&q, root, key);
  find_node(&q);

  /* not found */
  if(NULL == *q.node)
    return 0;

  removed_node = *q.node;

  if(removed_node->left){
    /* left and right children */
    if(removed_node->right){
      if(q.to_the_left){
        *q.node = removed_node->right;
        *get_leftmost_child(removed_node->right) = removed_node->left;
      }
      else {
        *q.node = removed_node->left;
        *get_rightmost_child(removed_node->left) = removed_node->right;
      }
    }

    /* only left child */
    else
      *q.node = removed_node->left;
  }

  /* only right child */
  else
  if(removed_node->right)
    *q.node = removed_node->right;

  /* no children */
  else
    *q.node = NULL;

  free(removed_node);
  return 1;
}


/* Public */
int storage_node_insert(StorageNode** root, unsigned char const* key,
                        unsigned char const* value)
{
  StorageNode* new_node = malloc(sizeof(StorageNode));
  if(!new_node)
    return 0;

  new_node->left = NULL;
  new_node->right = NULL;
  memcpy(new_node->key, key, STORAGE_KEY_SIZE);
  memcpy(new_node->value, value, STORAGE_VALUE_SIZE);

  insert_node(root, new_node);
  return 1;
}

StorageNode** storage_node_find(StorageNode** root, unsigned char const* key)
{
  StorageNodeQuery q;

  make_query(&q, root, key);
  find_node(&q);

  return q.node;
}

int storage_node_remove(StorageNode** root, unsigned char const* key)
{
  return remove_node(root, key);
}

void storage_node_traverse(StorageNode* root,
                           void(* cb)(StorageNode*, StorageNodeArg),
                           StorageNodeArg user_data)
{
  traverse_nodes(root, cb, user_data);
}

void storage_node_destroy_tree(StorageNode** root)
{
  destroy_nodes(*root);
  *root = NULL;
}
