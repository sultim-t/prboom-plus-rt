// Copyright (c) 1993-2011 PrBoom developers (see AUTHORS)
// Licence: GPLv2 or later (see COPYING)

// Convert portable pixmap to Doom format

// convert ppm to doom patch format, with insertion point
size_t ppm_to_patch(void **lumpdata, const char *filename,
                    int insert_x, int insert_y);

// convert ppm to raw bitmap (for use with flats)
size_t ppm_to_bitmap(void **lumpdata, const char *filename);
