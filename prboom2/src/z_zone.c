/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: z_zone.c,v 1.16 2002/08/05 21:59:55 cph Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Zone Memory Allocation. Neat.
 *
 * Neat enough to be rewritten by Lee Killough...
 *
 * Must not have been real neat :)
 *
 * Made faster and more general, and added wrappers for all of Doom's
 * memory allocation functions, including malloc() and similar functions.
 * Added line and file numbers, in case of error. Added performance
 * statistics and tunables.
 *-----------------------------------------------------------------------------
 */

static const char rcsid[] = "$Id: z_zone.c,v 1.16 2002/08/05 21:59:55 cph Exp $";

// use config.h if autoconf made one -- josh
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "z_zone.h"
#include "doomstat.h"
#include "m_argv.h"
#include "lprintf.h"

#ifdef DJGPP
#include <dpmi.h>
#endif

// Tunables

// Alignment of zone memory (benefit may be negated by HEADER_SIZE, CHUNK_SIZE)
#define CACHE_ALIGN 32

// Minimum chunk size at which blocks are allocated
#define CHUNK_SIZE 32

// Minimum size a block must be to become part of a split
#define MIN_BLOCK_SPLIT (1024)

// How much RAM to leave aside for other libraries
#define LEAVE_ASIDE (128*1024)

// Amount to subtract when retrying failed attempts to allocate initial pool
#define RETRY_AMOUNT (256*1024)

// signature for block header
#define ZONEID  0x931d4a11

// Number of mallocs & frees kept in history buffer (must be a power of 2)
#define ZONE_HISTORY 4

// End Tunables

typedef struct memblock {

#ifdef ZONEIDCHECK
  unsigned id;
#endif

  struct memblock *next,*prev;
  size_t size;
  void **user;
  unsigned char tag,vm;

#ifdef INSTRUMENTED
  unsigned short extra;
  const char *file;
  int line;
#endif

} memblock_t;

/* size of block header
 * cph - base on sizeof(memblock_t), which can be larger than CHUNK_SIZE on
 * 64bit architectures */
static const size_t HEADER_SIZE = (sizeof(memblock_t)+CHUNK_SIZE-1) & ~(CHUNK_SIZE-1);

static memblock_t *rover;                // roving pointer to memory blocks
static memblock_t *zone;                 // pointer to first block
static memblock_t *zonebase;             // pointer to entire zone memory
static memblock_t *blockbytag[PU_MAX];

#ifdef INSTRUMENTED

// statistics for evaluating performance
static size_t free_memory;
static size_t active_memory;
static size_t purgable_memory;
static size_t inactive_memory;
static size_t virtual_memory;

static void Z_PrintStats(void)            // Print allocation statistics
{
  unsigned long total_memory = free_memory + active_memory +
                               purgable_memory + inactive_memory +
                               virtual_memory;
  double s = 100.0 / total_memory;

  doom_printf("%-5u\t%6.01f%%\tstatic\n"
          "%-5u\t%6.01f%%\tpurgable\n"
          "%-5u\t%6.01f%%\tfree\n"
          "%-5u\t%6.01f%%\tfragmentary\n"
          "%-5u\t%6.01f%%\tvirtual\n"
          "%-5lu\t\ttotal\n",
          active_memory,
          active_memory*s,
          purgable_memory,
          purgable_memory*s,
          free_memory,
          free_memory*s,
          inactive_memory,
          inactive_memory*s,
          virtual_memory,
          virtual_memory*s,
          total_memory
          );
}

#ifdef HEAPDUMP
void W_PrintLump(FILE* fp, void* p);

void Z_DumpMemory(void)
{
  static int dump;
  memblock_t* block = zone;
  char buf[80];
  FILE* fp;
  size_t total_cache = 0, total_free = 0, total_malloc = 0;

  sprintf(buf, "memdump.%d", dump++);
  fp = fopen(buf, "w");
  if (block)
  do {
    switch (block->tag) {
    case PU_FREE: 
      fprintf(fp, "free %d\n", block->size);
      total_free += block->size;
      break;
    case PU_CACHE:
      fprintf(fp, "cache %s:%d:%d\n", block->file, block->line, block->size);
      total_cache += block->size;
      break;
    case PU_LEVEL:
      fprintf(fp, "level %s:%d:%d\n", block->file, block->line, block->size);
      total_malloc += block->size;
      break;
    default:
      fprintf(fp, "malloc %s:%d:%d", block->file, block->line, block->size);
      total_malloc += block->size;
      if (block->file)
        if (strstr(block->file,"w_wad.c"))
	  W_PrintLump(fp, (char*)block + HEADER_SIZE);
      fputc('\n', fp);
      break;
    }
    block=block->next;
    if (((int)block->file < 0x00001000) && (block->file != NULL) && (block->tag != 0)) {
	    block->file = NULL;
    }
  } while (block != zone);
  fprintf(fp, "malloc %d, cache %d, free %d, total %d\n",
	  total_malloc, total_cache, total_free, 
	  total_malloc + total_cache + total_free);
  fclose(fp);
}
#endif
#endif

size_t zone_size=8192;

#ifdef INSTRUMENTED

// killough 4/26/98: Add history information

enum {malloc_history, free_history, NUM_HISTORY_TYPES};

static const char *file_history[NUM_HISTORY_TYPES][ZONE_HISTORY];
static int line_history[NUM_HISTORY_TYPES][ZONE_HISTORY];
static int history_index[NUM_HISTORY_TYPES];
static const char *const desc[NUM_HISTORY_TYPES] = {"malloc()'s", "free()'s"};

void Z_DumpHistory(char *buf)
{
  int i,j;
  char s[1024];
  strcat(buf,"\n");
  for (i=0;i<NUM_HISTORY_TYPES;i++)
    {
      sprintf(s,"\nLast several %s:\n\n", desc[i]);
      strcat(buf,s);
      for (j=0; j<ZONE_HISTORY; j++)
        {
          int k = (history_index[i]-j-1) & (ZONE_HISTORY-1);
          if (file_history[i][k])
            {
              sprintf(s, "File: %s, Line: %d\n", file_history[i][k],
                      line_history[i][k]);
              strcat(buf,s);
            }
        }
    }
}
#else

void Z_DumpHistory(char *buf)
{
}

#endif

void Z_Close(void)
{
  (free)(zonebase);
  zone = rover = zonebase = NULL;
}

void Z_Init(void)
{
  size_t size = zone_size*1000;

#ifdef HAVE_MMAP
  return; /* cphipps - if we have mmap, we don't need our own heap */
#endif

#ifdef INSTRUMENTED
  if (!(HEADER_SIZE >= sizeof(memblock_t) && size > HEADER_SIZE)) 
    I_Error("Z_Init: Sanity check failed");
#endif

  size = (size+CHUNK_SIZE-1) & ~(CHUNK_SIZE-1);  // round to chunk size
  size += HEADER_SIZE + CACHE_ALIGN;

  // Allocate the memory

  zonebase=(malloc)(size);
  if (!zonebase)
    I_Error("Z_Init: Failed on allocation of %lu bytes", (unsigned long)size);

  lprintf(LO_INFO,"Z_Init : Allocated %lukb zone memory\n",
		  (long unsigned)size / 1000);

  // Align on cache boundary

  zone = (memblock_t *) ((char *) zonebase + CACHE_ALIGN -
                         ((unsigned) zonebase & (CACHE_ALIGN-1)));

  rover = zone;                            // Rover points to base of zone mem
  zone->next = zone->prev = zone;          // Single node
  zone->size = size;                       // All memory in one block
  zone->tag = PU_FREE;                     // A free block
  zone->vm  = 0;

#ifdef ZONEIDCHECK
  zone->id  = 0;
#endif

#ifdef INSTRUMENTED
  free_memory = size;
  /* cph - remove unnecessary initialisations to 0 */
#endif
#ifdef HEAPDUMP
  atexit(Z_DumpMemory);
#endif
}

/* Z_Malloc
 * You can pass a NULL user if the tag is < PU_PURGELEVEL.
 *
 * cph - the algorithm here was a very simple first-fit round-robin 
 *  one - just keep looping around, freeing everything we can until 
 *  we get a large enough space
 *
 * This has been changed now; we still do the round-robin first-fit, 
 * but we only free the blocks we actually end up using; we don't 
 * free all the stuff we just pass on the way.
 */

void *(Z_Malloc)(size_t size, int tag, void **user
#ifdef INSTRUMENTED
		 , const char *file, int line
#endif
		 )
{
  register memblock_t *block;
  memblock_t *start, *first_of_free;
  register size_t contig_free = 0;

#ifdef INSTRUMENTED
  size_t size_orig = size;
#ifdef CHECKHEAP
  Z_CheckHeap();
#endif

  file_history[malloc_history][history_index[malloc_history]] = file;
  line_history[malloc_history][history_index[malloc_history]++] = line;
  history_index[malloc_history] &= ZONE_HISTORY-1;
#endif

#ifdef ZONEIDCHECK
  if (tag >= PU_PURGELEVEL && !user)
    I_Error ("Z_Malloc: An owner is required for purgable blocks"
#ifdef INSTRUMENTED
             "Source: %s:%d", file, line
#endif
	     );
#endif

  if (!size)
    return user ? *user = NULL : NULL;           // malloc(0) returns NULL

  size = (size+CHUNK_SIZE-1) & ~(CHUNK_SIZE-1);  // round to chunk size

  block = rover;

  /* cph - allow memory allocation even before Z_Init;
   *  just drop through to the malloc(3) fallback
   */
  if (block) {
   if (block->prev->tag == PU_FREE)
    block = block->prev;

   start = block;
   first_of_free = NULL;

   do {
    /* If we just wrapped, we're not contiguous with the previous block */
    if (block == zone) contig_free = 0;

    if (block->tag < PU_PURGELEVEL && block->tag != PU_FREE) {
      /* Not free(able), so no free space here */
      contig_free = 0;
    } else {
      /* Add to contiguous chunk of free space */
      if (!contig_free) first_of_free = block;
      contig_free += block->size;

      /* First fit */
      if (contig_free >= size)
	break;
    }
   }
   while ((block = block->next) != start);   // detect cycles as failure
  }

  if (contig_free >= size) {
    /* We have a block of free(able) memory on the heap which will suffice */
    block = first_of_free;

    /* If the previous block is adjacent and free, step back and include it */
    if (block != zone && block->prev->tag == PU_FREE) 
      block = block->prev;

    /* Free current block if needed */
    if (block->tag != PU_FREE) Z_Free((char *) block + HEADER_SIZE);

    /* Note: guaranteed that block->prev is either 
     * not free or not contiguous 
     *
     * At every step, block->next must be not free, else it would 
     *  have been merged with our block 
     * No range check needed because we know it works by the previous loop */
    while (block->size < size)
      Z_Free((char *)(block->next) + HEADER_SIZE);

    /* Now, carve up the block */
    {
      size_t extra = block->size - size;
      if (extra >= MIN_BLOCK_SPLIT + HEADER_SIZE) {
	memblock_t *newb = (memblock_t *)((char *) block +
					  HEADER_SIZE + size);
	
	(newb->next = block->next)->prev = newb;
	(newb->prev = block)->next = newb;          // Split up block
	block->size = size;
	newb->size = extra - HEADER_SIZE;
	newb->tag = PU_FREE;
	newb->vm = 0;
	
#ifdef INSTRUMENTED
	inactive_memory += HEADER_SIZE;
	free_memory -= HEADER_SIZE;
#endif
      }
      
      rover = block->next;           // set roving pointer for next search
      
#ifdef INSTRUMENTED
      inactive_memory += block->extra = block->size - size_orig;
      if (tag >= PU_PURGELEVEL)
	purgable_memory += size_orig;
      else
	active_memory += size_orig;
      free_memory -= block->size;
#endif
    }
  } else {
    /* Allocate a vm block * 
     * We've run out of physical memory, or so we think.
     * Although less efficient, we'll just use ordinary malloc.
     * This will squeeze the remaining juice out of this machine
     * and start cutting into virtual memory if it has it.
     */
    
#ifdef HAVE_LIBDMALLOC
    while (!(block = _malloc_leap(file,line,size + HEADER_SIZE))) {
#else
    while (!(block = (malloc)(size + HEADER_SIZE))) {
#endif
      if (!blockbytag[PU_CACHE])
        I_Error ("Z_Malloc: Failure trying to allocate %lu bytes"
#ifdef INSTRUMENTED
                 "\nSource: %s:%d"
#endif
		 ,(unsigned long) size
#ifdef INSTRUMENTED
		 , file, line
#endif
		 );
      Z_FreeTags(PU_CACHE,PU_CACHE);
    }

    if ((block->next = blockbytag[tag]))
      block->next->prev = (memblock_t *) &block->next;
    blockbytag[tag] = block;
    block->prev = (memblock_t *) &blockbytag[tag];
    block->vm = 1;
    
#ifdef INSTRUMENTED
    virtual_memory += size + HEADER_SIZE;
#endif
    /* cph - the next line was lost in the #ifdef above, and also added an 
     *  extra HEADER_SIZE to block->size, which was incorrect */
    block->size = size; 
  }

#ifdef INSTRUMENTED
  block->file = file;
  block->line = line;
#endif
  
#ifdef ZONEIDCHECK
  block->id = ZONEID;         // signature required in block header
#endif
  block->tag = tag;           // tag
  block->user = user;         // user
  block = (memblock_t *)((char *) block + HEADER_SIZE);
  if (user)                   // if there is a user
    *user = block;            // set user to point to new block
  
#ifdef INSTRUMENTED
  Z_PrintStats();           // print memory allocation stats
  // scramble memory -- weed out any bugs
  memset(block, gametic & 0xff, size);
#endif
  return block;
}

void (Z_Free)(void *p
#ifdef INSTRUMENTED
	      , const char *file, int line
#endif
	      )
{
#ifdef INSTRUMENTED
#ifdef CHECKHEAP
  Z_CheckHeap();
#endif
  file_history[free_history][history_index[free_history]] = file;
  line_history[free_history][history_index[free_history]++] = line;
  history_index[free_history] &= ZONE_HISTORY-1;
#endif

  if (p)
    {
      memblock_t *other, *block = (memblock_t *)((char *) p - HEADER_SIZE);

#ifdef ZONEIDCHECK
      if (block->id != ZONEID)
        I_Error("Z_Free: freed a pointer without ZONEID"
#ifdef INSTRUMENTED
                "\nSource: %s:%d"
                "\nSource of malloc: %s:%d"
                , file, line, block->file, block->line
#endif
                );
      block->id = 0;              // Nullify id so another free fails
#endif

#ifdef INSTRUMENTED
      /* scramble memory -- weed out any bugs */
      memset(p, gametic & 0xff, block->size);
#endif

      if (block->user)            // Nullify user if one exists
        *block->user = NULL;

      if (block->vm)
        {
          if ((*(memblock_t **) block->prev = block->next))
            block->next->prev = block->prev;

#ifdef INSTRUMENTED
          virtual_memory -= block->size + HEADER_SIZE;
#endif
#ifdef HAVE_LIBDMALLOC
          _free_leap(file,line,block);
#else
          (free)(block);
#endif
        }
      else
        {

#ifdef INSTRUMENTED
          free_memory += block->size;
          inactive_memory -= block->extra;
          if (block->tag >= PU_PURGELEVEL)
            purgable_memory -= block->size - block->extra;
          else
            active_memory -= block->size - block->extra;
#endif

          block->tag = PU_FREE;       // Mark block freed

          if (block != zone)
            {
              other = block->prev;        // Possibly merge with previous block
              if (other->tag == PU_FREE)
                {
                  if (rover == block)  // Move back rover if it points at block
                    rover = other;
                  (other->next = block->next)->prev = other;
                  other->size += block->size + HEADER_SIZE;
                  block = other;

#ifdef INSTRUMENTED
                  inactive_memory -= HEADER_SIZE;
                  free_memory += HEADER_SIZE;
#endif
                }
            }

          other = block->next;        // Possibly merge with next block
          if (other->tag == PU_FREE && other != zone)
            {
              if (rover == other) // Move back rover if it points at next block
                rover = block;
              (block->next = other->next)->prev = block;
              block->size += other->size + HEADER_SIZE;

#ifdef INSTRUMENTED
              inactive_memory -= HEADER_SIZE;
              free_memory += HEADER_SIZE;
#endif
            }
        }

#ifdef INSTRUMENTED
      Z_PrintStats();           // print memory allocation stats
#endif
    }
}

void (Z_FreeTags)(int lowtag, int hightag
#ifdef INSTRUMENTED
		  , const char *file, int line
#endif
		  )
{
  /* cph - move rover to start of zone; we like to encourage static 
   * data to stay in one place, at the start of the heap
   */
  memblock_t *block = rover = zone;

#ifdef HEAPDUMP
  Z_DumpMemory();
#endif

  if (lowtag <= PU_FREE)
    lowtag = PU_FREE+1;

  if (block)
  do               // Scan through list, searching for tags in range
    if (block->tag >= lowtag && block->tag <= hightag)
      {
        memblock_t *prev = block->prev, *cur = block;
#ifdef INSTRUMENTED
        (Z_Free)((char *) block + HEADER_SIZE, file, line);
#else
        (Z_Free)((char *) block + HEADER_SIZE);
#endif
	/* cph - be more careful here, we were skipping blocks!
	 * If the current block was not merged with the previous, 
	 *  cur is still a valid pointer, prev->next == cur, and cur is 
	 *  already free so skip to the next.
	 * If the current block was merged with the previous, 
	 *  the next block to analyse is prev->next.
	 * Note that the while() below does the actual step forward 
	 */
        block = (prev->next == cur) ? cur : prev;
      }
  while ((block=block->next) != zone);

  if (hightag > PU_CACHE)
    hightag = PU_CACHE;

  for (;lowtag <= hightag; lowtag++)
    for (block = blockbytag[lowtag], blockbytag[lowtag] = NULL; block;)
      {
        memblock_t *next = block->next;

#ifdef ZONEIDCHECK
        if (block->id != ZONEID)
          I_Error("Z_Free: freed a pointer without ZONEID"
#ifdef INSTRUMENTED
                  "\nSource: %s:%d"
                  "\nSource of malloc: %s:%d"
                  , file, line, block->file, block->line
#endif
                  );

        block->id = 0;              // Nullify id so another free fails
#endif

#ifdef INSTRUMENTED
        virtual_memory -= block->size + HEADER_SIZE;
#endif

        if (block->user)            // Nullify user if one exists
          *block->user = NULL;

        (free)(block);              // Free the block

        block = next;               // Advance to next block
      }
}

void (Z_ChangeTag)(void *ptr, int tag
#ifdef INSTRUMENTED
		   , const char *file, int line
#endif
		   )
{
  memblock_t *block = (memblock_t *)((char *) ptr - HEADER_SIZE);

#ifdef INSTRUMENTED
#ifdef CHECKHEAP
  Z_CheckHeap();
#endif
#endif

#ifdef ZONEIDCHECK
  if (block->id != ZONEID)
    I_Error ("Z_ChangeTag: freed a pointer without ZONEID"
#ifdef INSTRUMENTED
             "\nSource: %s:%d"
             "\nSource of malloc: %s:%d"
             , file, line, block->file, block->line
#endif
             );

  if (tag >= PU_PURGELEVEL && !block->user)
    I_Error ("Z_ChangeTag: an owner is required for purgable blocks\n"
#ifdef INSTRUMENTED
             "Source: %s:%d"
             "\nSource of malloc: %s:%d"
             , file, line, block->file, block->line
#endif
             );

#endif // ZONEIDCHECK

  if (block->vm)
    {
      if ((*(memblock_t **) block->prev = block->next))
        block->next->prev = block->prev;
      if ((block->next = blockbytag[tag]))
        block->next->prev = (memblock_t *) &block->next;
      block->prev = (memblock_t *) &blockbytag[tag];
      blockbytag[tag] = block;
    }
  else
    {
#ifdef INSTRUMENTED
      if (block->tag < PU_PURGELEVEL && tag >= PU_PURGELEVEL)
        {
          active_memory -= block->size - block->extra;
          purgable_memory += block->size - block->extra;
        }
      else
        if (block->tag >= PU_PURGELEVEL && tag < PU_PURGELEVEL)
          {
            active_memory += block->size - block->extra;
            purgable_memory -= block->size - block->extra;
          }
#endif
    }
  block->tag = tag;
}

void *(Z_Realloc)(void *ptr, size_t n, int tag, void **user
#ifdef INSTRUMENTED
                  , const char *file, int line
#endif
		  )
{
  void *p = (Z_Malloc)(n, tag, user DA(file, line));
  if (ptr)
    {
      memblock_t *block = (memblock_t *)((char *) ptr - HEADER_SIZE);
      memcpy(p, ptr, n <= block->size ? n : block->size);
      (Z_Free)(ptr DA(file, line));
      if (user) // in case Z_Free nullified same user
        *user=p;
    }
  return p;
}

void *(Z_Calloc)(size_t n1, size_t n2, int tag, void **user
#ifdef INSTRUMENTED
                 , const char *file, int line
#endif
		 )
{
  return
    (n1*=n2) ? memset((Z_Malloc)(n1, tag, user DA(file, line)), 0, n1) : NULL;
}

char *(Z_Strdup)(const char *s, int tag, void **user
#ifdef INSTRUMENTED
                 , const char *file, int line
#endif
		 )
{
  return strcpy((Z_Malloc)(strlen(s)+1, tag, user DA(file, line)), s);
}

void (Z_CheckHeap)(
#ifdef INSTRUMENTED
		   const char *file, int line
#endif
		   )
{
  memblock_t *block = zone;   // Start at base of zone mem
  if (block)
  do {                        // Consistency check (last node treated special)
    if ((block->next != zone &&
         (memblock_t *)((char *) block+HEADER_SIZE+block->size) != block->next)
        || block->next->prev != block || block->prev->next != block)
      I_Error("Z_CheckHeap: Block size does not touch the next block\n"
#ifdef INSTRUMENTED
              "Source: %s:%d"
              "\nSource of offending block: %s:%d"
              , file, line, block->file, block->line
#endif
              );
    if (((int)block->file < 0x00001000) && (block->file != NULL) && (block->tag != 0)) {
	    block->file = NULL;
    }
  } while ((block=block->next) != zone);
}
