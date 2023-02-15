#ifndef WRITE_BUFFER_H
#define WRITE_BUFFER_H

#include <stddef.h>

typedef struct WriteBuffer {
  int fd;
  int error;
  size_t pos;
  size_t capacity;
  unsigned char *data;
} WriteBuffer;

#define WRITE_BUFFER_ERROR(wb_ptr)((wb_ptr)->error)

int write_buffer_init(WriteBuffer *wb, int fd, size_t buffer_size);

/* all of the below return 0 if failed to write to file */
int write_buffer_write(WriteBuffer *wb, void const *data, size_t size);

int write_buffer_flush(WriteBuffer *wb);

/* flush before destroying */
int write_buffer_destroy(WriteBuffer *wb);

#endif
