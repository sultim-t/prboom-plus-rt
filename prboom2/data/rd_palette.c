// Copyright (c) 1993-2011 PrBoom developers (see AUTHORS)
// Licence: GPLv2 or later (see COPYING)

// Chained hash lookup to convert rgb triples to palette indices

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rd_util.h"
#include "rd_palette.h"

static unsigned char *palette_data;

#define PAL_SIZE 256

static struct {
  unsigned char rgb[3];
  int first, next;
} hash[PAL_SIZE];

#define HASH(c) ((int)((c)[0])+(int)((c)[1])+(int)((c)[2]))

//
// make_hash
//
// killough-style chained hash
//

static void make_hash(void)
{
  int i;
  unsigned char *rgb;

  for (i = PAL_SIZE, rgb = &palette_data[3*i]; rgb -= 3, --i >= 0; )
  {
    memmove(hash[i].rgb, rgb, 3);
    hash[i].next = hash[i].first = PAL_SIZE;
  }

  for (i = PAL_SIZE, rgb = &palette_data[3*i]; rgb -= 3, --i >= 0; )
  {
    int h = HASH(rgb) % PAL_SIZE;
    hash[i].next = hash[h].first;
    hash[h].first = i;
  }
}

//
// loadpal
//

static void loadpal(const char *filename)
{
  void *data = NULL;
  size_t size = read_or_die(&data, filename);

  if (size != 3*PAL_SIZE)
    die("Bad palette: %s\n", filename);

  palette_data = data;
}

//
// palette_init
//

void palette_init(const char *filename)
{
  loadpal(filename);
  make_hash();
}

//
// palette_getindex
//

int palette_getindex(const unsigned char *rgb)
{
  int i;

  if (!palette_data)
    die("No palette loaded - please specify one with -palette\n");

  i = hash[HASH(rgb) % PAL_SIZE].first;

  while (i < PAL_SIZE && memcmp(hash[i].rgb, rgb, 3) != 0)
    i = hash[i].next;

  return i < PAL_SIZE ? i : -1;
}
