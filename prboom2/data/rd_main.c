// Copyright (c) 1993-2011 PrBoom developers (see AUTHORS)
// Licence: GPLv2 or later (see COPYING)

// Main program, parse command line arguments

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "rd_util.h"
#include "rd_output.h"
#include "rd_sound.h"
#include "rd_palette.h"
#include "rd_graphic.h"

enum argtype
{
  ARG_NONE,
  ARG_OUTPUT,
  ARG_INCLUDE,
  ARG_PALETTE,
  ARG_MARKER,
  ARG_LUMP,
  ARG_GRAPHIC,
  ARG_SOUND,
  ARG_FLAT,
  ARG_SPRITE,
};

static void ATTR((noreturn)) usage(int exitcode)
{
  FILE *f = exitcode ? stderr : stdout;
  fprintf(f, "Usage: rdatawad <options...>\n"
          "  -o <output filename>\n"
          "  -I <search directory>\n"
          "  -palette <rgbfile>\n"
          "  -marker <lumpname>\n"
          "  -lumps <file>...\n"
          "  -graphics <ppmfile>...\n"
          "  -sounds <wavfile>...\n"
          "  -flats <ppmfile>...\n"
          "  -sprites <x,y,ppmfile>...\n");
  exit(exitcode);
}

int main(int argc, char **argv)
{
  enum argtype argtype = ARG_NONE;
  const char *output = NULL;

  if (argc <= 1)
    usage(0);

  while (*(++argv))
  {
    char *arg = *argv;

    if (*arg == '-')
    {
      if (*(arg+1) == '-') // allow both -switch and --switch
        arg++;

      if (!strcmp(arg, "-o"))
        argtype = ARG_OUTPUT;
      else if (!strcmp(arg, "-I"))
        argtype = ARG_INCLUDE;
      else if (!strcmp(arg, "-palette"))
        argtype = ARG_PALETTE;
      else if (!strcmp(arg, "-marker"))
        argtype = ARG_MARKER;
      else if (!strcmp(arg, "-lumps"))
        argtype = ARG_LUMP;
      else if (!strcmp(arg, "-graphics"))
        argtype = ARG_GRAPHIC;
      else if (!strcmp(arg, "-sounds"))
        argtype = ARG_SOUND;
      else if (!strcmp(arg, "-flats"))
        argtype = ARG_FLAT;
      else if (!strcmp(arg, "-sprites"))
        argtype = ARG_SPRITE;
      else if (!strcmp(arg, "-help") || !strcmp(arg, "-version"))
        usage(0);
      else
        usage(1);
    }
    else
      switch (argtype)
      {
      case ARG_NONE:
        usage(1);
        break;

      case ARG_OUTPUT:
        output = arg;
        argtype = ARG_NONE;
        break;

      case ARG_INCLUDE:
        search_path(arg);
        break;

      case ARG_PALETTE:
        palette_init(arg);
        argtype = ARG_NONE;
        break;

      case ARG_MARKER:
        output_add(arg, NULL, 0);
        break;

      case ARG_LUMP:
        {
          void *data = NULL;
          size_t size = read_or_die(&data, arg);
          output_add(arg, data, size);
        }
        break;

      case ARG_GRAPHIC:
        {
          void *data = NULL;
          size_t size = ppm_to_patch(&data, arg, 0, 0);
          output_add(arg, data, size);
        }
        break;

      case ARG_SOUND:
        {
          void *data = NULL;
          size_t size = wav_to_doom(&data, arg);
          output_add(arg, data, size);
        }
        break;

      case ARG_FLAT:
        {
          void *data = NULL;
          size_t size = ppm_to_bitmap(&data, arg);
          output_add(arg, data, size);
        }
        break;

      case ARG_SPRITE:
        {
          void *data = NULL;
          char *pos = arg;
          size_t size;
          int x, y;

          x = strtol(pos, &pos, 0);
          if (*pos == ',') pos++;
          else if (!isspace(*pos) && !isdigit(*pos)) usage(1);

          y = strtol(pos, &pos, 0);
          if (*pos == ',') pos++;
          else if (!isspace(*pos) && !isdigit(*pos)) usage(1);

          size = ppm_to_patch(&data, pos, x, y);
          output_add(pos, data, size);
        }
        break;
      }
  }

  if (output)
    output_write(output);
  else
    die("No output file specified\n");

  return 0;
}
