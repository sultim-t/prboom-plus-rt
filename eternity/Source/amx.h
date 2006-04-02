/*  Abstract Machine for the Small compiler
 *
 *  Copyright (c) ITB CompuPhase, 1997-2005
 *
 *  This software is provided "as-is", without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1.  The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 *
 *  Version: $Id: amx.h,v 1.52 2005-02-09 10:15:12+01 thiadmer Exp $
 */

#if defined FREEBSD && !defined __FreeBSD__
  #define __FreeBSD__
#endif
#if defined LINUX || defined __FreeBSD__ || defined __OpenBSD__
  /* haleyjd: this is a user include, not a system include */
  #include "sclinux.h"
#endif

#ifndef AMX_H_INCLUDED
#define AMX_H_INCLUDED

#if !defined HAVE_STDINT_H
  #if defined __LCC__ || defined __DMC__ || defined LINUX
    #if defined HAVE_INTTYPES_H
      #include <inttypes.h>
    #else
      #include <stdint.h>
    #endif
  #elif !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
    /* The ISO C99 defines the int16_t and int_32t types. If the compiler got
     * here, these types are probably undefined.
     */
    #if defined __MACH__
      #include <ppc/types.h>
      typedef unsigned short int  uint16_t;
      typedef unsigned long int   uint32_t;
    #elif defined __FreeBSD__
      #include <inttypes.h>
    #else
      typedef short int           int16_t;
      typedef unsigned short int  uint16_t;
      #if defined SN_TARGET_PS2
        typedef int               int32_t;
        typedef unsigned int      uint32_t;
      #else
        typedef long int          int32_t;
        typedef unsigned long int uint32_t;
      #endif
      #if defined __WIN32__ || defined _WIN32 || defined WIN32
        typedef __int64	          int64_t;
        typedef unsigned __int64  uint64_t;
        #define HAVE_I64
      #elif defined __GNUC__
        typedef long long         int64_t;
        typedef unsigned long long uint64_t;
        #define HAVE_I64
      #endif
    #endif
  #endif
  #if !defined HAVE_STDINT_H
    #define HAVE_STDINT_H
  #endif
#endif
#if defined _LP64 || defined WIN64 || defined _WIN64
  #if !defined __64BIT__
    #define __64BIT__
  #endif
#endif

#if HAVE_ALLOCA_H
  #include <alloca.h>
#endif
#if defined __WIN32__ || defined _WIN32 || defined WIN32 /* || defined __MSDOS__ */
  #if !defined alloca
    #define alloca(n)   _alloca(n)
  #endif
#endif

#ifdef  __cplusplus
extern  "C" {
#endif

/* calling convention for native functions */
#if !defined AMX_NATIVE_CALL
  #define AMX_NATIVE_CALL
#endif
/* calling convention for all interface functions and callback functions */
#if !defined AMXAPI
  #if defined STDECL
    #define AMXAPI      __stdcall
  #elif defined CDECL
    #define AMXAPI      __cdecl
  #elif defined GCC_HASCLASSVISIBILITY
    #define AMXAPI __attribute__ ((visibility("default")))
  #else
    #define AMXAPI
  #endif
#endif
#if !defined AMXEXPORT
  #define AMXEXPORT
#endif

/* File format version                          Required AMX version
 *   0 (original version)                       0
 *   1 (opcodes JUMP.pri, SWITCH and CASETBL)   1
 *   2 (compressed files)                       2
 *   3 (public variables)                       2
 *   4 (opcodes SWAP.pri/alt and PUSHADDR)      4
 *   5 (tagnames table)                         4
 *   6 (reformatted header)                     6
 *   7 (name table, opcodes SYMTAG & SYSREQ.D)  7
 */
#define CUR_FILE_VERSION  7     /* current file version; also the current AMX version */
#define MIN_FILE_VERSION  6     /* lowest supported file format version for the current AMX version */
#define MIN_AMX_VERSION   7     /* minimum AMX version needed to support the current file format */

#if defined BIT16
  #define SMALL_CELL_SIZE 16    /* for backward compatibility */
#endif
#if !defined SMALL_CELL_SIZE
  #define SMALL_CELL_SIZE 32    /* by default, use 32-bit cells */
#endif
#if SMALL_CELL_SIZE==16
  typedef uint16_t  ucell;
  typedef int16_t   cell;
#elif SMALL_CELL_SIZE==32
  typedef uint32_t  ucell;
  typedef int32_t   cell;
#elif SMALL_CELL_SIZE==64
  typedef uint64_t  ucell;
  typedef int64_t   cell;
#else
  #error Unsupported cell size (SMALL_CELL_SIZE)
#endif

#define UNPACKEDMAX   ((1L << (sizeof(cell)-1)*8) - 1)

struct tagAMX;
typedef cell (AMX_NATIVE_CALL *AMX_NATIVE)(struct tagAMX *amx, cell *params);
typedef int (AMXAPI *AMX_CALLBACK)(struct tagAMX *amx, cell index,
                                   cell *result, cell *params);
typedef int (AMXAPI *AMX_DEBUG)(struct tagAMX *amx);
#if !defined _FAR
  #define _FAR
#endif

#if defined _MSC_VER
  #pragma warning(disable:4103)  /* disable warning message 4103 that complains
                                  * about pragma pack in a header file */
  #pragma warning(disable:4100)  /* "'%$S' : unreferenced formal parameter" */
#endif

/* Some compilers do not support the #pragma align, which should be fine. Some
 * compilers give a warning on unknown #pragmas, which is not so fine...
 */
#if defined SN_TARGET_PS2 || defined __GNUC__
  #define AMX_NO_ALIGN
#endif

#if defined __GNUC__
  #define PACKED        __attribute__((packed))
#else
  #define PACKED
#endif

#if !defined AMX_NO_ALIGN
  #if defined LINUX || defined __FreeBSD__
    #pragma pack(1)         /* structures must be packed (byte-aligned) */
  #elif defined MACOS && defined __MWERKS__
	#pragma options align=mac68k
  #else
    #pragma pack(push)
    #pragma pack(1)         /* structures must be packed (byte-aligned) */
    #if defined __TURBOC__
      #pragma option -a-    /* "pack" pragma for older Borland compilers */
    #endif
  #endif
#endif

typedef struct tagAMX_NATIVE_INFO {
  const char _FAR *name PACKED;
  AMX_NATIVE func       PACKED;
} AMX_NATIVE_INFO       PACKED;

#define AMX_USERNUM     4
#define sEXPMAX         19      /* maximum name length for file version <= 6 */
#define sNAMEMAX        31      /* maximum name length of symbol name */

typedef struct tagAMX_FUNCSTUB {
  ucell address         PACKED;
  char name[sEXPMAX+1]  PACKED;
} AMX_FUNCSTUB          PACKED;

typedef struct tagFUNCSTUBNT {
  ucell address         PACKED;
  uint32_t nameofs      PACKED;
} FUNCSTUBNT            PACKED;

/* The AMX structure is the internal structure for many functions. Not all
 * fields are valid at all times; many fields are cached in local variables.
 */
typedef struct tagAMX {
  unsigned char _FAR *base PACKED; /* points to the AMX header ("amxhdr") plus the code, optionally also the data */
  unsigned char _FAR *data PACKED; /* points to separate data+stack+heap, may be NULL */
  AMX_CALLBACK callback PACKED;
  AMX_DEBUG debug       PACKED; /* debug callback */
  /* for external functions a few registers must be accessible from the outside */
  cell cip              PACKED; /* instruction pointer: relative to base + amxhdr->cod */
  cell frm              PACKED; /* stack frame base: relative to base + amxhdr->dat */
  cell hea              PACKED; /* top of the heap: relative to base + amxhdr->dat */
  cell hlw              PACKED; /* bottom of the heap: relative to base + amxhdr->dat */
  cell stk              PACKED; /* stack pointer: relative to base + amxhdr->dat */
  cell stp              PACKED; /* top of the stack: relative to base + amxhdr->dat */
  int flags             PACKED; /* current status, see amx_Flags() */
  /* for assertions and debug hook */
  cell curline          PACKED;
  cell curfile          PACKED;
  int dbgcode           PACKED;
  cell dbgaddr          PACKED;
  cell dbgparam         PACKED;
  char _FAR *dbgname    PACKED;
  /* user data */
  long usertags[AMX_USERNUM] PACKED;
  void _FAR *userdata[AMX_USERNUM] PACKED;
  /* native functions can raise an error */
  int error             PACKED;
  /* the sleep opcode needs to store the full AMX status */
  cell pri              PACKED;
  cell alt              PACKED;
  cell reset_stk        PACKED;
  cell reset_hea        PACKED;
  cell sysreq_d         PACKED; /* relocated address/value for the SYSREQ.D opcode */
  #if defined JIT
    /* support variables for the JIT */
    int reloc_size      PACKED; /* required temporary buffer for relocations */
    long code_size      PACKED; /* estimated memory footprint of the native code */
  #endif
} AMX                   PACKED;

/* The AMX_HEADER structure is both the memory format as the file format. The
 * structure is used internaly.
 */
typedef struct tagAMX_HEADER {
  int32_t size          PACKED; /* size of the "file" */
  uint16_t magic        PACKED; /* signature */
  char    file_version  PACKED; /* file format version */
  char    amx_version   PACKED; /* required version of the AMX */
  int16_t flags         PACKED;
  int16_t defsize       PACKED; /* size of a definition record */
  int32_t cod           PACKED; /* initial value of COD - code block */
  int32_t dat           PACKED; /* initial value of DAT - data block */
  int32_t hea           PACKED; /* initial value of HEA - start of the heap */
  int32_t stp           PACKED; /* initial value of STP - stack top */
  int32_t cip           PACKED; /* initial value of CIP - the instruction pointer */
  int32_t publics       PACKED; /* offset to the "public functions" table */
  int32_t natives       PACKED; /* offset to the "native functions" table */
  int32_t libraries     PACKED; /* offset to the table of libraries */
  int32_t pubvars       PACKED; /* the "public variables" table */
  int32_t tags          PACKED; /* the "public tagnames" table */
  int32_t nametable     PACKED; /* name table, file version 7 only */
} AMX_HEADER            PACKED;

#if SMALL_CELL_SIZE==16
  #define AMX_MAGIC     0xf1e2
#elif SMALL_CELL_SIZE==32
  #define AMX_MAGIC     0xf1e0
#elif SMALL_CELL_SIZE==64
  #define AMX_MAGIC     0xf1e1
#endif

enum {
  AMX_ERR_NONE,
  /* reserve the first 15 error codes for exit codes of the abstract machine */
  AMX_ERR_EXIT,         /* forced exit */
  AMX_ERR_ASSERT,       /* assertion failed */
  AMX_ERR_STACKERR,     /* stack/heap collision */
  AMX_ERR_BOUNDS,       /* index out of bounds */
  AMX_ERR_MEMACCESS,    /* invalid memory access */
  AMX_ERR_INVINSTR,     /* invalid instruction */
  AMX_ERR_STACKLOW,     /* stack underflow */
  AMX_ERR_HEAPLOW,      /* heap underflow */
  AMX_ERR_CALLBACK,     /* no callback, or invalid callback */
  AMX_ERR_NATIVE,       /* native function failed */
  AMX_ERR_DIVIDE,       /* divide by zero */
  AMX_ERR_SLEEP,        /* go into sleepmode - code can be restarted */

  AMX_ERR_MEMORY = 16,  /* out of memory */
  AMX_ERR_FORMAT,       /* invalid file format */
  AMX_ERR_VERSION,      /* file is for a newer version of the AMX */
  AMX_ERR_NOTFOUND,     /* function not found */
  AMX_ERR_INDEX,        /* invalid index parameter (bad entry point) */
  AMX_ERR_DEBUG,        /* debugger cannot run */
  AMX_ERR_INIT,         /* AMX not initialized (or doubly initialized) */
  AMX_ERR_USERDATA,     /* unable to set user data field (table full) */
  AMX_ERR_INIT_JIT,     /* cannot initialize the JIT */
  AMX_ERR_PARAMS,       /* parameter error */
  AMX_ERR_DOMAIN,       /* domain error, expression result does not fit in range */
  AMX_ERR_GENERAL,      /* general error (unknown or unspecific error) */
};

enum {
  DBG_INIT,             /* query/initialize */
  DBG_FILE,             /* file number in curfile, filename in name */
  DBG_LINE,             /* line number in curline, file number in curfile */
  DBG_SYMBOL,           /* address in dbgaddr, class/type in dbgparam */
  DBG_CLRSYM,           /* stack address below which locals should be removed. stack address in stk */
  DBG_CALL,             /* function call, address jumped to in dbgaddr */
  DBG_RETURN,           /* function returns */
  DBG_TERMINATE,        /* program ends, code address in dbgaddr, reason in dbgparam */
  DBG_SRANGE,           /* symbol size and dimensions (arrays); level in dbgaddr (!); length in dbgparam */
  DBG_SYMTAG,           /* tag of the most recent symbol (if non-zero), tag in dbgparam */
};

/*      AMX_FLAG_CHAR16   0x01     no longer used */
#define AMX_FLAG_DEBUG    0x02  /* symbolic info. available */
#define AMX_FLAG_COMPACT  0x04  /* compact encoding */
#define AMX_FLAG_BYTEOPC  0x08  /* opcode is a byte (not a cell) */
#define AMX_FLAG_NOCHECKS 0x10  /* no array bounds checking */
#define AMX_FLAG_BROWSE 0x4000  /* browsing/relocating or executing */
#define AMX_FLAG_RELOC  0x8000  /* jump/call addresses relocated */

#define AMX_EXEC_MAIN   -1      /* start at program entry point */
#define AMX_EXEC_CONT   -2      /* continue from last address */

#define AMX_USERTAG(a,b,c,d)    ((a) | ((b)<<8) | ((long)(c)<<16) | ((long)(d)<<24))

#if !defined AMX_COMPACTMARGIN
  #define AMX_COMPACTMARGIN 64
#endif

/* for native functions that use floating point parameters, the following
 * two macros are convenient for casting a "cell" into a "float" type _without_
 * changing the bit pattern
 */
#if SMALL_CELL_SIZE==32
  #define amx_ftoc(f)   ( * ((cell*)&f) )   /* float to cell */
  #define amx_ctof(c)   ( * ((float*)&c) )  /* cell to float */
#elif SMALL_CELL_SIZE==64
  #define amx_ftoc(f)   ( * ((cell*)&f) )   /* float to cell */
  #define amx_ctof(c)   ( * ((double*)&c) ) /* cell to float */
#else
  #error Unsupported cell size
#endif

#define amx_StrParam(amx,param,result)                                      \
    do {                                                                    \
      cell *amx_cstr_; int amx_length_;                                     \
      amx_GetAddr((amx), (param), &amx_cstr_);                              \
      amx_StrLen(amx_cstr_, &amx_length_);                                  \
      if (amx_length_ > 0 &&                                                \
          ((result) = (void*)alloca((amx_length_ + 1) * sizeof(*(result)))) != NULL) \
        amx_GetString((char*)(result), amx_cstr_, sizeof(*(result))>1);     \
      else (result) = NULL;                                                 \
    } while (0)

uint16_t * AMXAPI amx_Align16(uint16_t *v);
uint32_t * AMXAPI amx_Align32(uint32_t *v);
#if defined _I64_MAX || defined HAVE_I64
  uint64_t * AMXAPI amx_Align64(uint64_t *v);
#endif
int AMXAPI amx_Allot(AMX *amx, int cells, cell *amx_addr, cell **phys_addr);
int AMXAPI amx_Callback(AMX *amx, cell index, cell *result, cell *params);
int AMXAPI amx_Cleanup(AMX *amx);
int AMXAPI amx_Clone(AMX *amxClone, AMX *amxSource, void *data);
int AMXAPI amx_Debug(AMX *amx); /* default debug procedure, does nothing */
int AMXAPI amx_Exec(AMX *amx, cell *retval, int index, int numparams, ...);
int AMXAPI amx_Execv(AMX *amx, cell *retval, int index, int numparams, cell params[]);
int AMXAPI amx_FindNative(AMX *amx, const char *name, int *index);
int AMXAPI amx_FindPublic(AMX *amx, const char *funcname, int *index);
int AMXAPI amx_FindPubVar(AMX *amx, const char *varname, cell *amx_addr);
int AMXAPI amx_FindTagId(AMX *amx, cell tag_id, char *tagname);
int AMXAPI amx_Flags(AMX *amx,uint16_t *flags);
int AMXAPI amx_GetAddr(AMX *amx,cell amx_addr,cell **phys_addr);
int AMXAPI amx_GetNative(AMX *amx, int index, char *funcname);
int AMXAPI amx_GetPublic(AMX *amx, int index, char *funcname);
int AMXAPI amx_GetPubVar(AMX *amx, int index, char *varname, cell *amx_addr);
int AMXAPI amx_GetString(char *dest,const cell *source, int use_wchar);
int AMXAPI amx_GetTag(AMX *amx, int index, char *tagname, cell *tag_id);
int AMXAPI amx_GetUserData(AMX *amx, long tag, void **ptr);
int AMXAPI amx_Init(AMX *amx, void *program);
int AMXAPI amx_InitJIT(AMX *amx, void *reloc_table, void *native_code);
int AMXAPI amx_MemInfo(AMX *amx, long *codesize, long *datasize, long *stackheap);
int AMXAPI amx_NameLength(AMX *amx, int *length);
AMX_NATIVE_INFO * AMXAPI amx_NativeInfo(const char *name,AMX_NATIVE func);
int AMXAPI amx_NumNatives(AMX *amx, int *number);
int AMXAPI amx_NumPublics(AMX *amx, int *number);
int AMXAPI amx_NumPubVars(AMX *amx, int *number);
int AMXAPI amx_NumTags(AMX *amx, int *number);
int AMXAPI amx_RaiseError(AMX *amx, int error);
int AMXAPI amx_Register(AMX *amx, AMX_NATIVE_INFO *nativelist, int number);
int AMXAPI amx_Release(AMX *amx, cell amx_addr);
int AMXAPI amx_SetCallback(AMX *amx, AMX_CALLBACK callback);
int AMXAPI amx_SetDebugHook(AMX *amx, AMX_DEBUG debug);
int AMXAPI amx_SetString(cell *dest, const char *source, int pack, int use_wchar);
int AMXAPI amx_SetUserData(AMX *amx, long tag, void *ptr);
int AMXAPI amx_StrLen(cell *cstring, int *length);
int AMXAPI amx_UTF8Get(const char *string, const char **endptr, cell *value);
int AMXAPI amx_UTF8Put(char *string, char **endptr, int maxchars, cell value);
int AMXAPI amx_UTF8Check(const char *string);

#if SMALL_CELL_SIZE==16
  #define amx_AlignCell(v) amx_Align16(v)
#elif SMALL_CELL_SIZE==32
  #define amx_AlignCell(v) amx_Align32(v)
#elif SMALL_CELL_SIZE==64 && (defined _I64_MAX || defined HAVE_I64)
  #define amx_AlignCell(v) amx_Align64(v)
#else
  #error Unsupported cell size
#endif


#if !defined AMX_NO_ALIGN
  #if defined LINUX || defined __FreeBSD__
    #pragma pack()    /* reset default packing */
  #elif defined MACOS && defined __MWERKS__
    #pragma options align=reset
  #else
    #pragma pack(pop) /* reset previous packing */
  #endif
#endif

#ifdef  __cplusplus
}
#endif

#endif /* AMX_H_INCLUDED */
