// Copyright (c) 1993-2011 PrBoom developers (see AUTHORS)
// Licence: GPLv2 or later (see COPYING)

// Useful utility functions

#ifdef __GNUC__
#define ATTR(x) __attribute__(x)
#else
#define ATTR(x)
#endif

#ifdef WORDS_BIGENDIAN
# ifdef __GNUC__
#define LONG(x) __builtin_bswap32((x))
#define SHORT(x) (__builtin_bswap32((x))>>16)
# else
#define LONG(x) ( (((x) & 0x000000FF) << 24) \
                 +(((x) & 0x0000FF00) <<  8) \
                 +(((x) & 0x00FF0000) >>  8) \
                 +(((x) & 0xFF000000) >> 24) )
#define SHORT(x) ( (((x) & 0x00FF) << 8) \
                  +(((x) & 0xFF00) >> 8) )
# endif
#else
#define LONG(x) (x)
#define SHORT(x) (x)
#endif

void ATTR((noreturn)) die(const char *error, ...);

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void *xcalloc(size_t n, size_t size);
char *xstrdup(const char *s);

// slurp an entire file into memory or kill yourself
size_t read_or_die(void **ptr, const char *file);
void search_path(const char *path);
