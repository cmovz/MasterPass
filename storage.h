#ifndef STORAGE_H
#define STORAGE_H

#include "storage_node.h"
#include <stddef.h>

typedef struct Storage {
  StorageNode* root;
  char* file_name;
  char* temp_name;  /* this allows to replace atomically */
  size_t name_len;
  char changed;
  char is_new;
  char loaded;
} Storage;

typedef void(* StorageCb)(StorageNode const* node, StorageNodeArg user_arg);

void storage_init(Storage* storage);

/* return 0 only if it fails to read or allocate memory. it returns 1 even if
  the file doesn't exist, is_new == 0 implies the file was found and loaded,
  is_new == 1 means the file was not found and will be created on the first call
  to storage_save */
int storage_load(Storage* storage, char const* file);

/* save to disk */
int storage_save(Storage* storage);

/* insert if it doesn't exist, return 0 only if it fails to insert, return 1 if
  key is already there and update value */
int storage_insert(Storage* storage, unsigned char const* key,
                   unsigned char const* value);

int storage_find(Storage* storage, unsigned char const* key,
                 unsigned char* value);

void storage_traverse(Storage* storage, StorageCb cb, StorageNodeArg user_arg);

/* remove key if it exists */
void storage_remove(Storage* storage, unsigned char* key);

void storage_destroy(Storage* storage);

#endif
