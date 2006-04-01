/**/
#define PACKAGE "prboom"
#define VERSION "2.3.2"
#define DOGS 1
#define MONITOR_VISIBILITY 1
#define ALL_IN_ONE
#define SIMPLECHECKS
/*#define DISABLE_LUMP_CACHING*/

/**/
#define USE_SDL 1
#define HAVE_MIXER 1
#define HAVE_NET 1
#define USE_SDL_NET 1

/**/
#define HIGHRES 1
/*#define GL_DOOM 1*/
#define USE_GLU_TESS 1
#define USE_GLU_IMAGESCALE 1
#define USE_GLU_MIPMAP 1
#define DISABLE_DOUBLEBUFFER

/**/
#define STDC_HEADERS 1

#define stricmp strcasecmp
#define strnicmp strncasecmp

#define HAVE_INET_ATON 1
#define HAVE_INET_NTOP 1
#define HAVE_INET_PTON 1
#define HAVE_SETSOCKOPT 1

#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1

#define HAVE_MKSTEMPS 1

#define HAVE_IPv6 1

#define HAVE_UNISTD_H
#define HAVE_SYS_WAIT_H
#define HAVE_GETOPT
#define HAVE_NETINET_IN_H
#define SYS_SIGLIST_DECLARED

/**/
#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN
#endif

#ifdef __i386__
#define I386_ASM 1
#endif

#define PACKEDATTR __attribute__((packed))

#define HAVE_LIBKERN_OSBYTEORDER_H
