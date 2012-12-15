// Copyright (c) 1993-2011 PrBoom developers (see AUTHORS)
// Licence: GPLv2 or later (see COPYING)

// Convert WAVE files to Doom sound format

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rd_util.h"
#include "rd_sound.h"

//
// wav_to_doom
//
// loads a wav and converts it into Doom's sound format
// makes lots of assumptions about the format of a wav file.
//

struct wav_header
{
  char riff[4];
  unsigned int size;
  char wave[4];
  char fmt[4];
  unsigned int fmtlen;
  unsigned short fmttag;
  unsigned short channels;
  unsigned int samplerate;
  unsigned int byterate;
  unsigned short blockalign;
  unsigned short bits;
  char data[4];
  unsigned int datalen;
  char samples[1];
} ATTR((packed));

struct doom_sound_header
{
  unsigned short log2bits;
  unsigned short rate;
  unsigned int length;
  char samples[1];
} ATTR((packed));

size_t wav_to_doom(void **lumpdata, const char *filename)
{
  void *data;
  size_t size = read_or_die(&data, filename);
  struct wav_header *header = data;
  struct doom_sound_header *out;

  if (size < sizeof(*header) - 1
      || memcmp(header->riff, "RIFF", 4) != 0
      || memcmp(header->wave, "WAVE", 4) != 0)
    die("Invalid WAV file: %s\n", filename);

  size = sizeof(*out) - 1 + LONG(header->datalen);
  out = xmalloc(size);

  out->log2bits = 3;
  out->rate = SHORT(LONG(header->samplerate));
  out->length = header->datalen;
  memmove(out->samples, header->samples, LONG(header->datalen));

  free(data);

  *lumpdata = out;
  return size;
}
