// Copyright (c) 1993-2011 PrBoom developers (see AUTHORS)
// Licence: GPLv2 or later (see COPYING)

// Useful utility functions

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "rd_util.h"

void ATTR((noreturn)) die(const char *error, ...)
{
  va_list args;
  va_start(args, error);
  vfprintf(stderr, error, args);
  va_end(args);

  exit(1);
}

void *xmalloc(size_t size)
{
  void *ptr = malloc(size);

  if (!ptr)
    die("Failure to allocate %lu bytes\n", (unsigned long)size);

  return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
  ptr = realloc(ptr, size);

  if (!ptr)
    die("Failure to reallocate %lu bytes\n", (unsigned long)size);

  return ptr;
}

void *xcalloc(size_t n, size_t size)
{
  void *ptr = xmalloc(n * size);
  memset(ptr, 0, n * size);
  return ptr;
}

char *xstrdup(const char *s)
{
  size_t size = strlen(s)+1;
  void *ptr = xmalloc(size);
  memcpy(ptr, s, size);
  return ptr;
}

static const char **search_paths = NULL;
static size_t num_search_paths = 0;

// slurp an entire file into memory or kill yourself
size_t read_or_die(void **ptr, const char *file)
{
  size_t size = 0, length = 0;
  void *buffer = NULL, *pos = buffer;
  FILE *f;

  f = fopen(file, "rb");
  if (!f)
  {
    int i;
    size_t s;
    char *path;

    for (i = 0; i < num_search_paths; i++)
    {
      s = strlen(search_paths[i]) + 1 + strlen(file) + 1;
      path = xmalloc(s);
      snprintf(path, s, "%s/%s", search_paths[i], file);
      f = fopen(path, "rb");
      free(path);
    }
  }
  if (!f)
    die("Cannot open %s\n", file);

  while (!feof(f))
  {
    size_t size_read;

    if (size >= length)
    {
      size += 4096;
      buffer = xrealloc(buffer, size);
    }

    pos = (char *)buffer + length; // don't assume sizeof(void)==1
    size_read = fread(pos, 1, 4096, f);
    length += size_read;
  }

  *ptr = buffer;
  return length;
}

void search_path(const char *path)
{
  search_paths = xrealloc(search_paths,
                          (++num_search_paths)*sizeof(*search_paths));
  search_paths[num_search_paths-1] = xstrdup(path);
}
