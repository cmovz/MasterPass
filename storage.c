#include "storage.h"
#include "write_buffer.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

#define WRITE_BUFFER_SIZE 16384
#define TEMP_NAME_EXTRA ".master_pass_app_temp-XXXXXX"
#define TEMP_NAME_EXTRA_SIZE (sizeof TEMP_NAME_EXTRA - 1)
#define TEMP_NAME_TAIL_OFFSET (TEMP_NAME_EXTRA_SIZE - 6 - 1)
#define TEMP_NAME_TAIL "XXXXXX"
#define TEMP_NAME_TAIL_SIZE (sizeof TEMP_NAME_TAIL)

typedef struct DB_Entry {
  unsigned char key[STORAGE_KEY_SIZE];
  unsigned char value[STORAGE_VALUE_SIZE];
} DB_Entry;

/* load balanced entries */
static int db_to_storage(DB_Entry *start, DB_Entry *end, StorageNode **node)
{
  DB_Entry *mid;

  assert(NULL == *node);

  if (start == end)
    return 1;

  mid = start + (end - start) / 2;
  storage_node_insert(node, mid->key, mid->value);

  return db_to_storage(start, mid, &(*node)->left)
         && db_to_storage(mid + 1, end, &(*node)->right);
}

static void storage_to_db(StorageNode *node, StorageNodeArg arg)
{
  WriteBuffer *wb = arg.ptr;

  if (wb->error)
    return;

  /* allow key and value to be represented in any order and non-contiguously */
  write_buffer_write(wb, node->key, STORAGE_KEY_SIZE);
  write_buffer_write(wb, node->value, STORAGE_VALUE_SIZE);
}

/* Public */
void storage_init(Storage *s)
{
  s->root = NULL;
  s->file_name = NULL;
  s->temp_name = NULL;
  s->changed = 0;
  s->is_new = 1;
  s->loaded = 0;
}

/* return 0 only if it fails to read or allocate memory */
int storage_load(Storage *s, char const *file)
{
  size_t entry_count;
  DB_Entry *db;
  struct stat st;
  int fd;

  s->loaded = 1;

  /* save a copy of the file name so saves will work later */
  s->name_len = strlen(file);
  s->file_name = malloc(s->name_len + 1);
  if (!s->file_name)
    goto err;

  memcpy(s->file_name, file, s->name_len + 1);

  /* if we just saved to /tmp/something, rename() would fail across devices */
  s->temp_name = malloc(s->name_len + TEMP_NAME_EXTRA_SIZE + 1);
  if (!s->temp_name)
    goto err1;

  memcpy(s->temp_name, s->file_name, s->name_len);
  memcpy(s->temp_name + s->name_len, TEMP_NAME_EXTRA, TEMP_NAME_EXTRA_SIZE);

  /* if there's no such file, that's ok */
  if (-1 == stat(file, &st))
    return 1;

  s->is_new = 0;
  fd = open(file, O_RDONLY);
  if (-1 == fd)
    goto err2;

  db = malloc(st.st_size);
  if (!db)
    goto err3;

  if ((ssize_t) st.st_size != read(fd, db, st.st_size))
    goto err4;

  if (!db_to_storage(db, db + st.st_size / sizeof *db, &s->root))
    goto err5;

  close(fd);
  free(db);

  return 1;

  err5:
  storage_node_destroy_tree(&s->root);

  err4:
  free(db);

  err3:
  close(fd);

  err2:
  free(s->temp_name);
  s->temp_name = NULL;

  err1:
  free(s->file_name);
  s->file_name = NULL;
  s->name_len = 0;
  s->is_new = 1;

  err:
  s->loaded = 0;
  return 0;
}

/* save to disk */
int storage_save(Storage *s)
{
  WriteBuffer wb;
  StorageNodeArg arg;
  int temp_fd;

  /* do nothing if nothing has to be done */
  if (!s->loaded || (!s->changed && !s->is_new))
    return 1;

  /* append XXXXXX\0 to temp_name so mkstemp() will generate a random file */
  memcpy(s->temp_name + s->name_len + TEMP_NAME_TAIL_OFFSET,
         TEMP_NAME_TAIL, TEMP_NAME_TAIL_SIZE);

  temp_fd = mkstemp(s->temp_name);
  if (-1 == temp_fd)
    goto err;

  if (!write_buffer_init(&wb, temp_fd, WRITE_BUFFER_SIZE))
    goto err1;

  arg.ptr = &wb;
  storage_node_traverse(s->root, storage_to_db, arg);
  if (wb.error)
    goto err2;

  if (!write_buffer_flush(&wb))
    goto err2;

  /* atomically replace old version */
  if (-1 == rename(s->temp_name, s->file_name))
    goto err2;

  close(temp_fd);
  write_buffer_destroy(&wb);
  s->changed = 0;
  s->is_new = 0;

  return 1;

  err2:
  write_buffer_destroy(&wb);

  err1:
  close(temp_fd);
  unlink(s->temp_name);

  err:
  return 0;
}

int storage_insert(Storage *s, unsigned char const *key,
                   unsigned char const *value)
{
  StorageNode **node;

  node = storage_node_find(&s->root, key);
  if (NULL == *node)
    s->changed = 1;
  else
    s->changed |= memcmp((*node)->value, value, STORAGE_VALUE_SIZE);

  return storage_node_insert(node, key, value);
}

int storage_find(Storage *s, unsigned char const *key,
                 unsigned char *value_dest)
{
  StorageNode **node;

  node = storage_node_find(&s->root, key);
  if (NULL == *node)
    return 0;

  memcpy(value_dest, (*node)->value, STORAGE_VALUE_SIZE);
  return 1;
}

void storage_traverse(Storage *s, StorageCb cb, StorageNodeArg user_arg)
{
  storage_node_traverse(s->root,
                        (void (*)(StorageNode *, StorageNodeArg)) cb, user_arg);
}

void storage_remove(Storage *s, unsigned char *key)
{
  s->changed |= storage_node_remove(&s->root, key);
}

void storage_destroy(Storage *s)
{
  storage_node_destroy_tree(&s->root);
  free(s->file_name);
  free(s->temp_name);
}
