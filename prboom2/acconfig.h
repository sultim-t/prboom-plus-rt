/* acconfig.h for LxDoom
 *
 * $Id: acconfig.h,v 1.6 2000/05/18 10:00:18 cph Exp $
 * Parts Copyright (C) 1993-1996 by id Software, Inc.
 *
 * Process this file with autoheader to produce config.h.in,
 * which config.status then translates to config.h.  */

/* Define to the name of the distribution.  */
#undef PACKAGE

/* Define to the version of the distribution.  */
#undef VERSION

/* Define to strcasecmp, if we have it */
#undef stricmp

/* Define to strncasecmp, if we have it */
#undef strnicmp

/* Define for gnu-linux target */
#undef LINUX

/* Define for freebsd target */
#undef FREEBSD

/* Define on I386 target */
#undef I386

/* Define on targets supporting 386 Assembly */
#undef I386_ASM

/* Define on big endian target */
#undef __BIG_ENDIAN__

/* Define for high resolution support */
#undef HIGHRES

/* Define for support for MBF helper dogs */
#undef DOGS

/* Define to enable internal range checking */
#undef RANGECHECK

/* Define this to see real-time memory allocation
 * statistics, and enable extra debugging features 
 */
#undef INSTRUMENTED

/* Uncomment this to exhaustively run memory checks
 * while the game is running (this is EXTREMELY slow).
 * Only useful if INSTRUMENTED is also defined.
 */
#undef CHECKHEAP

/* Uncomment this to cause heap dumps to be generated.
 * Only useful if INSTRUMENTED is also defined.
 */
#undef HEAPDUMP

/* Uncomment this to perform id checks on zone blocks,
 * to detect corrupted and illegally freed blocks
 */
#undef ZONEIDCHECK

/* CPhipps - some debugging macros for the new wad lump handling code */
/* Defining this causes quick checks which only impose an overhead if a 
 *  posible error is detected. */
#undef SIMPLECHECKS

/* Defining this causes time stamps to be created each time a lump is locked, and 
 *  lumps locked for long periods of time are reported */
#undef TIMEDIAG

/* Define this to revoke any setuid status at startup (except to be reenabled 
 * when SVGALib needs it). Ony relevant to the SVGALib/Linux version */
#undef SECURE_UID

/* Define to be the path where Doom WADs are stored */
#undef DOOMWADDIR

/* Define to be the path to the sound server */
#undef SNDSERV_PATH

/* Define to be the path to the lxmusserver */
#undef MUSSERV_PATH

/* Define if you have library -lXext */
#undef HAVE_LIBXEXT

/* Define if you have the DGA library -lXxf86dga */
#undef HAVE_LIBXXF86DGA

/* Define if you have the SDL mixer library -lSDL_mixer */
#undef HAVE_LIBSDL_MIXER

/* Define if you want networkg ame support */
#undef HAVE_NET

/* Define if you have struct sockaddr_in6 */
#undef HAVE_IPv6

/* Define if you want to build with OpenGL support */
#undef GL_DOOM

