#include "write_buffer.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int write_buffer_init(WriteBuffer *wb, int fd, size_t buffer_size)
{
  wb->data = malloc(buffer_size);
  if (!wb->data)
    return 0;

  wb->fd = fd;
  wb->error = 0;
  wb->pos = 0;
  wb->capacity = buffer_size;

  return 1;
}

/* all of the below return 0 if failed to write to file */
int write_buffer_write(WriteBuffer *wb, void const *data, size_t size)
{
  size_t available;
  ssize_t w;

  if (wb->error)
    return 0;

  available = wb->capacity - wb->pos;

  /* fits */
  if (size < available) {
    memcpy(wb->data + wb->pos, data, size);
    wb->pos += size;
  }

    /* do 1 flush */
  else if (size < wb->capacity) {
    memcpy(wb->data + wb->pos, data, available);
    w = write(wb->fd, wb->data, wb->capacity);
    if (w != (ssize_t) wb->capacity) {
      wb->error = 1;
      return 0;
    }

    wb->pos = size - available;
    memcpy(wb->data, (char const *) data + available, wb->pos);
  }

    /* flush and write all the new data too */
  else {
    w = write(wb->fd, wb->data, wb->pos);
    if (w != (ssize_t) wb->pos) {
      wb->error = 1;
      return 0;
    }

    w = write(wb->fd, data, size);
    if (w != (ssize_t) size) {
      wb->error = 1;
      return 0;
    }

    wb->pos = 0;
  }

  return 1;
}

int write_buffer_flush(WriteBuffer *wb)
{
  ssize_t w;

  if (wb->error)
    return 0;

  w = write(wb->fd, wb->data, wb->pos);
  if (w != (ssize_t) wb->pos) {
    wb->error = 1;
    return 0;
  }

  wb->pos = 0;
  return 1;
}

int write_buffer_destroy(WriteBuffer *wb)
{
  int status = 1;

  if (wb->pos)
    status = write_buffer_flush(wb);

  /* it's better to allow multiple destruction in this case */
  wb->pos = 0;
  free(wb->data);
  wb->data = NULL;

  return status;
}
