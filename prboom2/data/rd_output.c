// Copyright (c) 1993-2011 PrBoom developers (see AUTHORS)
// Licence: GPLv2 or later (see COPYING)

// Output wad construction - add lump data, build wad directory

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "rd_util.h"
#include "rd_output.h"

struct lump
{
  const char *name;
  const void *data;
  size_t size;
  unsigned int offset;
};

static unsigned int numlumps, dirsize;
static struct lump *dir;

//
// extract_lumpname
//

static char *extract_lumpname(const char *filename)
{
  const char *base;
  char *lumpname, *suffix, *c;

  // strip off directory prefix
  base = strrchr(filename, '/');
  if (!base)
    base = filename;
  else
    base += 1;

  if (!*base)
    die("Empty lumpname: %s\n", filename);

  // copy the name
  lumpname = xstrdup(base);

  suffix = strrchr(lumpname, '.');
  if (suffix)
    *suffix = '\0';

  for (c = lumpname; *c; c++)
    *c = toupper(*c);

  return lumpname;
}

//
// output_add - add lump to wad
//

void output_add(const char *filename, const void *data, size_t size)
{
  struct lump *newlump;

  if (numlumps >= dirsize)
  {
    dirsize = dirsize ? 2*dirsize : 256;
    dir = xrealloc(dir, dirsize * sizeof(*dir));
  }

  newlump = &dir[numlumps++];

  newlump->name = extract_lumpname(filename);
  newlump->data = data;
  newlump->size = size;
}

//
// write* - serialisation functions
//

// write a uint32_t, byteswapping if necessary
static void write_u32(FILE *f, unsigned int n)
{
  n = LONG(n);

  fwrite(&n, 4, 1, f);
}

// write a lump name (8 byte string)
static void write_ch8(FILE *f, const char *s)
{
  char buffer[9];

  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", s);

  fwrite(buffer, 8, 1, f);
}

//
// output_write - write wad to file
//

void output_write(const char *filename)
{
  unsigned int i;
  struct lump *lump;
  unsigned int pos = 12;
  FILE *out;

  // calculate wad directory offsets
  for (i = numlumps, lump = dir; i; i--, lump++)
  {
    lump->offset = pos;
    pos += lump->size;
  }

  out = fopen(filename, "wb");
  if (!out)
    die("Cannot open %s\n", filename);

  // write wad header
  fwrite("PWAD", 4, 1, out);
  write_u32(out, numlumps);
  write_u32(out, pos);

  // write lumps
  for (i = numlumps, lump = dir; i; i--, lump++)
    fwrite(lump->data, lump->size, 1, out);

  // write wad directory
  for (i = numlumps, lump = dir; i; i--, lump++)
  {
    write_u32(out, lump->offset);
    write_u32(out, lump->size);
    write_ch8(out, lump->name);
  }

  fclose(out);
}
