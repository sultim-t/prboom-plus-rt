/* config.h.  This is the config file for Windows  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
/* #define HAVE_SYS_WAIT_H 1 */

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if `sys_siglist' is declared by <signal.h>.  */
/* #define SYS_SIGLIST_DECLARED 1 */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* Define if the X Window System is missing or not being used.  */
/* #undef X_DISPLAY_MISSING */

/* Define to strcasecmp, if we have it */
#define stricmp strcasecmp

/* Define to strncasecmp, if we have it */
#define strnicmp strncasecmp

/* Define on I386 target */
#define I386 1

/* Define on targets supporting 386 Assembly */
#define I386_ASM 1

/* Define for high resolution support */
#define HIGHRES 1

/* Define to enable internal range checking */
/* #undef RANGECHECK */

/* Define this to see real-time memory allocation
 * statistics, and enable extra debugging features 
 */
/* #undef INSTRUMENTED */

/* Uncomment this to exhaustively run memory checks
 * while the game is running (this is EXTREMELY slow).
 * Only useful if INSTRUMENTED is also defined.
 */
/* #undef CHECKHEAP */

/* Uncomment this to cause heap dumps to be generated.
 * Only useful if INSTRUMENTED is also defined.
 */
/* #undef HEAPDUMP */

/* Uncomment this to perform id checks on zone blocks,
 * to detect corrupted and illegally freed blocks
 */
#define ZONEIDCHECK 1

/* CPhipps - some debugging macros for the new wad lump handling code */
/* Defining this causes quick checks which only impose an overhead if a 
 *  posible error is detected. */
#define SIMPLECHECKS 1

/* Defining this causes time stamps to be created each time a lump is locked, and 
 *  lumps locked for long periods of time are reported */
/* #undef TIMEDIAG */

/* Define this to revoke any setuid status at startup (except to be reenabled 
 * when SVGALib needs it). Ony relevant to the SVGALib/Linux version */
/* #define SECURE_UID 1 */

/* Define to be the path where Doom WADs are stored */
#define DOOMWADDIR "/usr/local/share/games/doom"

/* Define to be the path to the sound server */
/* #define SNDSERV_PATH "/usr/local/games/sndserv" */

/* Define to be the path to the lxmusserver */
/* #define MUSSERV_PATH "/usr/local/games/musserv" */

/* Define if you have library -lXext */
/* #define HAVE_LIBXEXT 1 */

/* Define if you have the SDL mixer library -lSDL_mixer */
/* #define HAVE_LIBSDL_MIXER 1 */

/* Define if you want networkg ame support */
/* #define HAVE_NET 1 */

/* Define if you have struct sockaddr_in6 */
/* #define HAVE_IPv6 1 */

/* Define if you want to build with OpenGL support */
/* #undef GL_DOOM */

/* Define if you have the inet_aton function.  */
/* #define HAVE_INET_ATON 1 */

/* Define if you have the inet_ntop function.  */
/* #define HAVE_INET_NTOP 1 */

/* Define if you have the inet_pton function.  */
/* #define HAVE_INET_PTON 1 */

/* Define if you have the setsockopt function.  */
/* #define HAVE_SETSOCKOPT 1 */

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1
#define snprintf _snprintf

/* Define if you have the vsnprintf function.  */
#define HAVE_VSNPRINTF 1
#define vsnprintf _vsnprintf

/* Define if you have the <asm/byteorder.h> header file.  */
/* #define HAVE_ASM_BYTEORDER_H 1 */

/* Define if you have the <linux/bitops.h> header file.  */
/* #define HAVE_LINUX_BITOPS_H 1 */

/* Define if you have the <linux/joystick.h> header file.  */
/* #define HAVE_LINUX_JOYSTICK_H 1 */

/* Define if you have the <machine/soundcard.h> header file.  */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define if you have the <soundcard.h> header file.  */
/* #undef HAVE_SOUNDCARD_H */

/* Define if you have the <sys/soundcard.h> header file.  */
/* #define HAVE_SYS_SOUNDCARD_H 1 */

/* Define if you have the <unistd.h> header file.  */
/* #define HAVE_UNISTD_H 1 */

/* Define if you want to compile with SDL  */
#define USE_SDL

/* Name of package */
#define PACKAGE "lxdoom"

/* Version number of package */
#define VERSION "1.4.5"
