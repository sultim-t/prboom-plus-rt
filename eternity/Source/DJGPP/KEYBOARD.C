/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves,
 *      1 Salisbury Road,
 *      Market Drayton,
 *      Shropshire,
 *      England, TF9 1AJ.
 *
 *      DOS keyboard routines.
 *
 *      Salvador Eduardo Tropea added support for extended scancodes,
 *      keyboard LED's, capslock and numlock, and alt+numpad input.
 *
 *      Fabian Nunez added support for the special Microsoft keys.
 *
 *      Sean Gugler added the set_leds() function.
 *
 *      Callback routine added by Peter Palotas.
 *
 *      Mathieu Lafon added support for the Pause and PrtScr keys and
 *      changed the key[] table to a normal/extended bitfield.
 *
 *      Lee Killough fixed some bugs and added support for lower-level
 *      callback functions which can detect key releases and special
 *      keys.
 *
 *      Julian Hope added support for different versions of Allegro
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <dpmi.h>
#include <dir.h>
#include <sys/movedata.h>

#include "allegro.h"
#include "internal.h"


#define KEYBOARD_INT          9

#define KB_SPECIAL_MASK       0x3F
#define KB_CTRL_ALT_FLAG      (KB_CTRL_FLAG | KB_ALT_FLAG)


int three_finger_flag = TRUE;
int key_led_flag = TRUE;

int (*keyboard_callback)(int key) = NULL;

void (*keyboard_lowlevel_callback)(int key) = NULL;  /* killough 3/21/98 */

static int keyboard_installed = FALSE; 

volatile char key[128];                   /* key pressed flags */

volatile int key_shifts = 0;

/* Julian: 6/6/2001: Allegro compatibility

   Declaration of some tables have changed:
   ALLEGRO_ET_VERSION==0 : unsigned char table[128]
   ALLEGRO_ET_VERSION==1 : unsigned char * table
   which are incompatible types (at least for the current gcc version)
   ALLEGRO_ET_VERSION==2 : have to determine how to hack

*/
#if(ALLEGRO_ET_VERSION==0)
  // Oldest allegro
  #define STATIC_NEEDED
  #define STATIC_HACK(name) name
  #define HACK_CONVERSION(name)
#else
  #define STATIC_NEEDED static
  #define STATIC_HACK(name) static_##name
  #define HACK_CONVERSION(name) \
  unsigned char * name = (unsigned char *)static_##name ;
#endif


STATIC_NEEDED unsigned char STATIC_HACK(key_ascii_table)[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,   9,       /* 0 */
   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,  0,   'a', 's',     /* 1 */
   'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,  '`', 0,   92,  'z', 'x', 'c', 'v',     /* 2 */
   'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   3,   3,   3,   3,   8,       /* 3 */
   3,   3,   3,   3,   3,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,       /* 4 */
   0,   0,   0,   127, 0,   0,   92,  3,   3,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,     /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0        /* 7 */
};
HACK_CONVERSION(key_ascii_table)

STATIC_NEEDED unsigned char STATIC_HACK(key_capslock_table)[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,   9,       /* 0 */
   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 13,  0,   'A', 'S',     /* 1 */
   'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', 39,  '`', 0,   92,  'Z', 'X', 'C', 'V',     /* 2 */
   'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   3,   3,   3,   3,   8,       /* 3 */
   3,   3,   3,   3,   3,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,       /* 4 */
   0,   0,   0,   127, 0,   0,   92,  3,   3,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,     /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0        /* 7 */
};
HACK_CONVERSION(key_capslock_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_shift_table)[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 126, 126,     /* 0 */
   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 126, 0,   'A', 'S',     /* 1 */
   'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34,  '~', 0,   '|', 'Z', 'X', 'C', 'V',     /* 2 */
   'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   1,   0,   1,   1,   1,   1,   1,       /* 3 */
   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,       /* 4 */
   0,   0,   1,   127, 0,   0,   0,   1,   1,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,     /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0        /* 7 */
};
HACK_CONVERSION(key_shift_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_control_table)[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   0,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0,   0,   127, 127,     /* 0 */
   17,  23,  5,   18,  20,  25,  21,  9,   15,  16,  2,   2,   10,  0,   1,   19,      /* 1 */
   4,   6,   7,   8,   10,  11,  12,  0,   0,   0,   0,   0,   26,  24,  3,   22,      /* 2 */
   2,   14,  13,  0,   0,   0,   0,   0,   0,   0,   0,   2,   2,   2,   2,   2,       /* 3 */
   2,   2,   2,   2,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 4 */
   0,   0,   2,   0,   0,   0,   0,   2,   2,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0        /* 7 */
};
HACK_CONVERSION(key_control_table)


#define EMPTY_TABLE                                                                                \
												   \
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */     \
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 0 */     \
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 1 */     \
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 2 */     \
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 3 */     \
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 4 */     \
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 5 */     \
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 6 */     \
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0        /* 7 */


STATIC_NEEDED unsigned char STATIC_HACK(key_altgr_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_altgr_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_accent1_lower_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_accent1_lower_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_accent1_upper_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_accent1_upper_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_accent1_shift_lower_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_accent1_shift_lower_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_accent1_shift_upper_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_accent1_shift_upper_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_accent2_lower_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_accent2_lower_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_accent2_upper_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_accent2_upper_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_accent2_shift_lower_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_accent2_shift_lower_table)


STATIC_NEEDED unsigned char STATIC_HACK(key_accent2_shift_upper_table)[128] =
{
   EMPTY_TABLE
};
HACK_CONVERSION(key_accent2_shift_upper_table)


// Julian: needn't to be hacked
unsigned char key_numlock_table[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 0 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 1 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 2 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 3 */
   0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', 0,   '4', '5', '6', 0,   '1',     /* 4 */
   '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0        /* 7 */
};

// Julian : end of hack
#undef STATIC_NEEDED
#undef STATIC_HACK
#undef HACK_CONVERSION

/*
 *  Mapping:
 *    0 = Use the second value as scan char, just like before.
 *    1 = Ignore the key.
 *    2 = Use the scan but put the shift flags instead of the ASCII.
 *    n = Use this value as the scancode.
 *
 *  Extended values:
 *    E0 1C = Enter
 *    E0 1D = RCtrl => fake Ctrl
 *    E0 2A = ?????? generated in conjuntion with Insert!! (and PrtScr)
 *    E0 35 = \
 *    E0 37 = PrintScreen (Alt PrtScr: 54)
 *            Warning: PrtScr     => key[KEY_PRTSCR] = 2
 *                     Alt PrtScr => key[KEY_PRTSCR] = 1
 *    E0 38 = AltGr => fake Alt
 *    E0 46 = Ctrl-Pause
 *    E0 47 = Home
 *    E0 48 = Up
 *    E0 4B = Left
 *    E0 4D = Right
 *    E0 4F = End
 *    E0 50 = Down
 *    E0 51 = Page-Down
 *    E0 52 = Insert
 *    E0 53 = Delete
 *
 *  Pause key:
 *    This is a very special key because it sends a stange code (E1), there
 *    is no release code and no autorepeat. So the key[KEY_PAUSE] is TRUE
 *    the first time you press it and become FALSE only when you press it
 *    a second time (like the numlock flag). When the handler get a E1 code, 
 *    it skips the 5 next codes which correspond to the pause key (sequence
 *    E1 1D 52 E1 9D D2). It must also change the key[KEY_PAUSE] flag when 
 *    it received the extended code 46 (CtrlPause).
 */
unsigned char key_extended_table[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 0 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   120, 0,   0,       /* 1 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,       /* 2 */
   0,   0,   0,   0,   0,   122, 1 /* <- killough RSHIFT */,   84,  121, 0,   0,   0,   0,   0,   0,   0,       /* 3 */
   0,   0,   0,   0,   0,   0,   123, 2,   2,   2,   0,   2,   2,   2,   0,   2,       /* 4 */
   2,   2,   2,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0        /* 7 */
};


unsigned short key_special_table[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 0 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,       /* 1 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,       /* 2 */
   0,   0,   0,   0,   0,   0,   1,   0,   4,   0,1024,   0,   0,   0,   0,   0,       /* 3 */
   0,   0,   0,   0,   0, 512, 256,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 4 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   8,  16,  32,   0,   0,       /* 5 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   2,   4,   0,   0,   0,   0,   0,   0        /* 7 */
};


#define KEY_BUFFER_SIZE    256

static volatile int key_buffer[KEY_BUFFER_SIZE]; 
static volatile int key_buffer_start = 0;
static volatile int key_buffer_end = 0;
static volatile int key_extended = 0;
static volatile int key_pad_seq = 0;
static volatile int key_pause_loop = 0;

static int (*keypressed_hook)() = NULL;
static int (*readkey_hook)() = NULL;



/* add_key:
 *  Helper function to add a keypress to the buffer.
 */
static inline void add_key(int c)
{
   if (keyboard_callback) {
      c = keyboard_callback(c);
      if (!c)
	 return;
   }

   key_buffer[key_buffer_end] = c;

   key_buffer_end++;
   if (key_buffer_end >= KEY_BUFFER_SIZE)
      key_buffer_end = 0;
   if (key_buffer_end == key_buffer_start) {    /* buffer full */
      key_buffer_start++;
      if (key_buffer_start >= KEY_BUFFER_SIZE)
	 key_buffer_start = 0;
   }
}



/* clear_keybuf:
 *  Clears the keyboard buffer.
 */
void clear_keybuf()
{
   int c;

   DISABLE();

   key_buffer_start = 0;
   key_buffer_end = 0;

   for (c=0; c<128; c++)
      key[c] = FALSE;

   ENABLE();

   if ((keypressed_hook) && (readkey_hook))
      while (keypressed_hook())
	 readkey_hook();
}



/* keypressed:
 *  Returns TRUE if there are keypresses waiting in the keyboard buffer.
 */
int keypressed()
{
   if (key_buffer_start == key_buffer_end) {
      if (keypressed_hook)
	 return keypressed_hook();
      else
	 return FALSE;
   }
   else
      return TRUE;
}



/* readkey:
 *  Returns the next character code from the keyboard buffer. If the
 *  buffer is empty, it waits until a key is pressed. The low byte of
 *  the return value contains the ASCII code of the key, and the high
 *  byte the scan code. 
 */
int readkey()
{
   int r;

   if ((!keyboard_installed) && (!readkey_hook))
      return 0;

   if ((readkey_hook) && (key_buffer_start == key_buffer_end))
      return readkey_hook();

   do {
   } while (key_buffer_start == key_buffer_end);  /* wait for a press */

   DISABLE();

   r = key_buffer[key_buffer_start];
   key_buffer_start++;
   if (key_buffer_start >= KEY_BUFFER_SIZE)
      key_buffer_start = 0;

   ENABLE();

   return r;
}



/* simulate_keypress:
 *  Pushes a key into the keyboard buffer, as if it has just been pressed.
 */
void simulate_keypress(int key)
{
   DISABLE();

   add_key(key);

   ENABLE();
}



/* kb_wait_for_write_ready:
 *  Wait for the keyboard controller to set the ready-for-write bit.
 */
static inline void kb_wait_for_write_ready()
{
   long i = 1000000L;

   while ((i--) && (inportb(0x64) & 2))
      ; /* wait */
}



/* kb_wait_for_read_ready:
 *  Wait for the keyboard controller to set the ready-for-read bit.
 */
static inline void kb_wait_for_read_ready()
{
   long i = 1000000L;

   while ((i--) && (!(inportb(0x64) & 0x01)))
      ; /* wait */
}



/* kb_send_data:
 *  Sends a byte to the keyboard controller. Returns 1 if all OK.
 */
static inline int kb_send_data(unsigned char data)
{
   long i;
   int resends = 4;
   int temp;

   do {
      kb_wait_for_write_ready();

      outportb(0x60, data);
      i = 2000000L;

      while (--i) {
	 kb_wait_for_read_ready();
	 temp = inportb(0x60);

	 if (temp == 0xFA)
	    return 1;
	 else if (temp == 0xFE)
	    break;
      }
   }
   while ((resends-- > 0) && (i));

   return 0;
}



/* update_leds:
 *  Sets the state of the keyboard LED indicators.
 */
static inline void update_leds()
{
   if (os_type == OSTYPE_WINNT)
      return;

   if ((!kb_send_data(0xED)) || (!kb_send_data((key_shifts>>8) & 7)))
      kb_send_data(0xF4);
}



/* my_keyint:
 *  Hardware level keyboard interrupt (int 9) handler.
 */
static int my_keyint()
{
   int i, t, temp, release, flag, mask, scan;   /* killough 3/22/98 */
   unsigned char *table;
   temp = inportb(0x60);            /* read keyboard byte */
   scan = temp;                     /* killough 3/22/98 */
   if (key_pause_loop) {            /* skip these codes */
      key_pause_loop--;
   }
   else if (temp == 0xE1) {         /* pause */
      key_pause_loop = 5;           /* we must skip the next 5 codes */
      key[KEY_PAUSE] ^= KB_EXTENDED;
      t = (KEY_PAUSE<<8);
      add_key(t);

      /* killough 3/22/98: Allow low-level handler */
      if (keyboard_lowlevel_callback)
	keyboard_lowlevel_callback(KEY_PAUSE);
   }
   else if (temp == 0xE0) {
      key_extended = 1; 
   }
   else {
      release = (temp & 0x80);      /* bit 7 means key was released */
      temp &= 0x7F;                 /* bits 0-6 is the scan code */
      mask = key_extended ? KB_EXTENDED : KB_NORMAL;

      if (key_extended) {           /* if is an extended code */
	 key_extended = 0;

	 if (((temp == KEY_END) || (temp == KEY_DEL)) && 
	     ((key_shifts & KB_CTRL_ALT_FLAG) == KB_CTRL_ALT_FLAG) && 
	     (!release) && (three_finger_flag)) {
	    asm (
	       "  movb $0x79, %%al ; "
	       "  call ___djgpp_hw_exception "
	    : : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory"
	    );
	    goto exit_keyboard_handler;
	 }

	 switch (key_extended_table[temp]) {

	    case 0:
	       /* process as normal */
	       break;

	    case 1:
	       /* ignore the key */
	       goto exit_keyboard_handler; 

	    case 2:

	      /* killough 3/22/98: Allow low-level handler */
	      if (keyboard_lowlevel_callback)
		keyboard_lowlevel_callback(scan);

	       if (release) {
		  key[temp] &= ~mask;
	       }
	       else { 
		  /* report the scan code + the shift state */
		  key[temp] |= mask;
		  t = (temp<<8) | (key_shifts & KB_SPECIAL_MASK);
		  add_key(t);
	       }
	       goto exit_keyboard_handler;

	    case KEY_PAUSE: 
	       /* pause (with ctrl) */
	       if (!release) {
		  key[KEY_PAUSE] ^= KB_EXTENDED;
		  t = (KEY_PAUSE<<8);
		  add_key(t);
	       }
	       goto exit_keyboard_handler;

	    default:
	       /* use a replacement value from the LUT */
	       temp = key_extended_table[temp];
	       break;
	 }
      } 

      /* killough 3/22/98: Allow low-level handler */
      if (keyboard_lowlevel_callback)
	keyboard_lowlevel_callback(scan);

      if (release) {                /* key was released */
	 key[temp] &= ~mask;

	 if ((flag = key_special_table[temp]) != 0) {
	    if ((flag < KB_SCROLOCK_FLAG) && (flag != KB_ALT_FLAG)) {
		key_shifts &= ~flag; 
	    }
	    else if (flag == KB_ALT_FLAG) {
	       key_shifts &= ~flag;
	       if (key_shifts & KB_INALTSEQ_FLAG) {
		  key_shifts &= ~KB_INALTSEQ_FLAG;
		  add_key(key_pad_seq & 0xFF);
	       }
	    }
	 }
      }
      else {                        /* key was pressed */
	 key[temp] |= mask;

	 if ((flag = key_special_table[temp]) != 0) {
	    if ((flag >= KB_SCROLOCK_FLAG) && (flag < KB_INALTSEQ_FLAG)) {
	       if (key_led_flag) {
		  key_shifts ^= flag;
		  update_leds();
		  goto exit_keyboard_handler;
	       }
	    }
	    else if (flag >= KB_ACCENT1_FLAG) {
	       if (key[KEY_ALTGR]) {
		  t = SCANCODE_TO_ALTGR(temp);
		  add_key(t);
		  goto exit_keyboard_handler; 
	       }
	       else {
		  if (key_shifts & KB_SHIFT_FLAG)
		     key_shifts |= (flag<<1);
		  else
		     key_shifts |= flag;
	       }
	    }
	    else
	       key_shifts |= flag;
	 }
	 else { 
	    /* accented character input */
	    if (key_shifts & (KB_ACCENT1_FLAG | KB_ACCENT1_S_FLAG | 
			      KB_ACCENT2_FLAG | KB_ACCENT2_S_FLAG)) {

	       if (((key_shifts & KB_SHIFT_FLAG) != 0) ^ 
		   ((key_shifts & KB_CAPSLOCK_FLAG) != 0)) {

		  if (key_shifts & KB_ACCENT1_FLAG)
		     table = key_accent1_upper_table;
		  else if (key_shifts & KB_ACCENT1_S_FLAG)
		     table = key_accent1_shift_upper_table;
		  else if (key_shifts & KB_ACCENT2_FLAG)
		     table = key_accent2_upper_table;
		  else if (key_shifts & KB_ACCENT2_S_FLAG)
		     table = key_accent2_shift_upper_table;
		  else
		     table = NULL;
	       }
	       else {
		  if (key_shifts & KB_ACCENT1_FLAG)
		     table = key_accent1_lower_table;
		  else if (key_shifts & KB_ACCENT1_S_FLAG)
		     table = key_accent1_shift_lower_table;
		  else if (key_shifts & KB_ACCENT2_FLAG)
		     table = key_accent2_lower_table;
		  else if (key_shifts & KB_ACCENT2_S_FLAG)
		     table = key_accent2_shift_lower_table;
		  else
		     table = NULL;
	       }

	       if (table[temp]) {
		  /* accented char */
		  t = (temp<<8) + table[temp];

		  key_shifts &= ~(KB_ACCENT1_FLAG | KB_ACCENT1_S_FLAG | 
				  KB_ACCENT2_FLAG | KB_ACCENT2_S_FLAG);

		  add_key(t);
		  goto exit_keyboard_handler;
	       }
	       else {
		  /* accent + normal char */
		  if (key_shifts & (KB_ACCENT1_FLAG | KB_ACCENT1_S_FLAG))
		     flag = KB_ACCENT1_FLAG;
		  else
		     flag = KB_ACCENT2_FLAG;

		  for (i=0; i<128; i++) {
		     if (key_special_table[i] & flag) {
			if (key_shifts & flag)
			   add_key(SCANCODE_TO_KEY(i));
			else
			   add_key(SCANCODE_TO_SHIFT(i));
			break;
		     }
		  }

		  key_shifts &= ~(KB_ACCENT1_FLAG | KB_ACCENT1_S_FLAG | 
				  KB_ACCENT2_FLAG | KB_ACCENT2_S_FLAG);
	       }
	    }
	    /* alt+key processing */
	    if (key_shifts & KB_ALT_FLAG) {
	       if ((temp >= 0x47) && (key_extended_table[temp] == 2)) { 
		  if (key_shifts & KB_INALTSEQ_FLAG) {
		     key_pad_seq = key_pad_seq*10 + key_numlock_table[temp]-'0';
		  }
		  else {
		     key_shifts |= KB_INALTSEQ_FLAG;
		     key_pad_seq = key_numlock_table[temp] - '0';
		  }
		  goto exit_keyboard_handler;
	       }
	       else {
		  if (key[KEY_ALTGR])
		     t = SCANCODE_TO_ALTGR(temp);
		  else
		     t = SCANCODE_TO_ALT(temp);
	       }
	    }
	    /* normal processing */
	    else if (key_shifts & KB_CTRL_FLAG)
	       t = SCANCODE_TO_CONTROL(temp);
	    else if (key_shifts & KB_SHIFT_FLAG) {
	       if (key_shifts & KB_CAPSLOCK_FLAG) {
		  if (key_ascii_table[temp] == key_capslock_table[temp])
		     t = SCANCODE_TO_SHIFT(temp);
		  else
		     t = SCANCODE_TO_KEY(temp);
	       }
	       else
		  t = SCANCODE_TO_SHIFT(temp);
	    }
	    else if ((key_shifts & KB_NUMLOCK_FLAG) && (key_numlock_table[temp]))
	       t = (KEY_PAD<<8) | key_numlock_table[temp];
	    else if (key_shifts & KB_CAPSLOCK_FLAG)
	       t = SCANCODE_TO_CAPS(temp);
	    else
	       t = SCANCODE_TO_KEY(temp);

	    key_shifts &= ~KB_INALTSEQ_FLAG;

	    add_key(t);
	 }
      }
   }

   exit_keyboard_handler:

   outportb(0x20,0x20);       /* ack. the interrupt */
   return 0;
}

static END_OF_FUNCTION(my_keyint);



/* read_key_table:
 *  Reads a specific keymapping table from the config file.
 */
static void read_key_table(unsigned char table[128], char *section)
{
   char name[80];
   int i;

   for (i=0; i<128; i++) {
      sprintf(name, "key%d", i);
      table[i] = get_config_int(section, name, table[i]);
   }
}



/* read_config:
 *  Worker function for reading keyboard config data.
 */
static void read_config(char *filename)
{
   int c;

   push_config_state();
   set_config_file(filename);

   read_key_table(key_ascii_table,               "key_ascii");
   read_key_table(key_capslock_table,            "key_capslock");
   read_key_table(key_shift_table,               "key_shift");
   read_key_table(key_control_table,             "key_control");
   read_key_table(key_altgr_table,               "key_altgr");
   read_key_table(key_accent1_lower_table,       "key_accent1_lower");
   read_key_table(key_accent1_upper_table,       "key_accent1_upper");
   read_key_table(key_accent1_shift_lower_table, "key_accent1_shift_lower");
   read_key_table(key_accent1_shift_upper_table, "key_accent1_shift_upper");
   read_key_table(key_accent2_lower_table,       "key_accent2_lower");
   read_key_table(key_accent2_upper_table,       "key_accent2_upper");
   read_key_table(key_accent2_shift_lower_table, "key_accent2_shift_lower");
   read_key_table(key_accent2_shift_upper_table, "key_accent2_shift_upper");

   c = get_config_int("key_escape", "accent1", 0);
   if (c)
      key_special_table[c&127] = KB_ACCENT1_FLAG;

   c = get_config_int("key_escape", "accent2", 0);
   if (c)
      key_special_table[c&127] = KB_ACCENT2_FLAG;

   pop_config_state();
}



/* try_config_location:
 *  Tries to read keyboard config data from the specified location, looking
 *  both for a regular config file and a keyboard.dat containing the file.
 */
static int try_config_location(char *path, char *file)
{
   char buf[256], *s;

   /* try a regular file */ 
   sprintf(buf, "%s%s", path, file);

   if (file_exists(buf, FA_RDONLY | FA_ARCH, NULL)) {
      read_config(buf);
      return TRUE;
   }

   /* try a datafile member */ 
   if (!strpbrk(file, "\\/#")) {
      sprintf(buf, "%skeyboard.dat#%s", path, file);
      s = get_extension(buf);
      if ((s > buf) && (*(s-1) == '.'))
	 *(s-1) = '_';

      if (file_exists(buf, FA_RDONLY | FA_ARCH, NULL)) {
	 read_config(buf);
	 return TRUE;
      }
   }

   return FALSE;
}



/* read_keyboard_config:
 *  Reads in the keyboard config tables.
 */
static void read_keyboard_config()
{
   char *name = get_config_string(NULL, "keyboard", NULL);
   char buf[256], path[256], *s;

   if ((!name) || (!name[0]))
      return;

   /* fully qualified path? */
   if (strpbrk(name, "\\/#")) {
      try_config_location("", name);
      return;
   }

   /* try in same dir as the program */
   strcpy(buf, name);
   s = get_extension(buf);
   if ((s <= buf) || (*(s-1) != '.')) {
      *s = '.';
      strcpy(s+1, "cfg");
   }

   strcpy(path, __crt0_argv[0]);
   *get_filename(path) = 0;
   put_backslash(path);

   if (try_config_location(path, buf))
      return;

   /* try the ALLEGRO environment variable */
   s = getenv("ALLEGRO");
   if (s) {
      strcpy(path, s);
      put_backslash(path);

      if (try_config_location(path, buf))
	 return; 
   }
}



/* install_keyboard:
 *  Installs Allegro's keyboard handler. You must call this before using 
 *  any of the keyboard input routines. Note that Allegro completely takes 
 *  over the keyboard, so the debugger will not work properly, and under 
 *  DOS even ctrl-alt-del will have no effect. Returns -1 on failure.
 */
int install_keyboard()
{
   int c;
   unsigned short shifts;

   if (keyboard_installed)
      return -1;

   read_keyboard_config();

   LOCK_VARIABLE(key_ascii_table);
   LOCK_VARIABLE(key_capslock_table);
   LOCK_VARIABLE(key_shift_table);
   LOCK_VARIABLE(key_control_table);
   LOCK_VARIABLE(key_altgr_table);
   LOCK_VARIABLE(key_accent1_lower_table);
   LOCK_VARIABLE(key_accent1_upper_table);
   LOCK_VARIABLE(key_accent1_shift_lower_table);
   LOCK_VARIABLE(key_accent1_shift_upper_table);
   LOCK_VARIABLE(key_accent2_lower_table);
   LOCK_VARIABLE(key_accent2_upper_table);
   LOCK_VARIABLE(key_accent2_shift_lower_table);
   LOCK_VARIABLE(key_accent2_shift_upper_table);
   LOCK_VARIABLE(key_numlock_table);
   LOCK_VARIABLE(key_extended_table);
   LOCK_VARIABLE(key_special_table);
   LOCK_VARIABLE(three_finger_flag);
   LOCK_VARIABLE(key_led_flag);
   LOCK_VARIABLE(key_shifts);
   LOCK_VARIABLE(key);
   LOCK_VARIABLE(key_buffer);
   LOCK_VARIABLE(key_buffer_start);
   LOCK_VARIABLE(key_buffer_end);
   LOCK_VARIABLE(key_extended);
   LOCK_VARIABLE(key_pad_seq);
   LOCK_VARIABLE(key_pause_loop);
   LOCK_VARIABLE(keyboard_callback);
   LOCK_VARIABLE(keyboard_lowlevel_callback);         /* killough 3/22/98 */
   LOCK_FUNCTION(my_keyint);

   /* killough 3/22/98: we must disable interrupts during entire setup */

   DISABLE();

   key_buffer_start = 0;
   key_buffer_end = 0;

   for (c=0; c<128; c++)
      key[c] = FALSE;

   if ((keypressed_hook) && (readkey_hook))
      while (keypressed_hook())
	 readkey_hook();

   /* transfer keys from keyboard buffer */
   while ((kbhit()) && (key_buffer_end < KEY_BUFFER_SIZE-1))
      key_buffer[key_buffer_end++] = getch();

   /* get state info from the BIOS */
   _dosmemgetw(0x417, 1, &shifts);

   key_shifts = 0;

   if (shifts & 1) {
      key_shifts |= KB_SHIFT_FLAG;
      key[KEY_RSHIFT] = TRUE;
   }
   if (shifts & 2) {
      key_shifts |= KB_SHIFT_FLAG;
      key[KEY_LSHIFT] = TRUE;
   }
   if (shifts & 4) {
      key_shifts |= KB_CTRL_FLAG;
      key[KEY_LCONTROL] = TRUE;
   }
   if (shifts & 8) {
      key_shifts |= KB_ALT_FLAG;
      key[KEY_ALT] = TRUE;
   }
   if (shifts & 16)
      key_shifts |= KB_SCROLOCK_FLAG;
   if (shifts & 32)
      key_shifts |= KB_NUMLOCK_FLAG;
   if (shifts & 64)
      key_shifts |= KB_CAPSLOCK_FLAG;

   _install_irq(KEYBOARD_INT, my_keyint);

   update_leds();

   _add_exit_func(remove_keyboard);
   keyboard_installed = TRUE;
   ENABLE();
   return 0;
}



/* remove_keyboard:
 *  Removes the keyboard handler, returning control to the BIOS. You don't
 *  normally need to call this, because allegro_exit() will do it for you.
 */
void remove_keyboard()
{
   int c;
   unsigned short shifts;

   if (!keyboard_installed)
      return;

   DISABLE();

   update_leds();

   _remove_irq(KEYBOARD_INT);

   /* transfer state info to the BIOS */
   shifts = 0;

   if (key[KEY_RSHIFT])
      shifts |= 1;
   if (key[KEY_LSHIFT])
      shifts |= 2;
   if (key[KEY_LCONTROL])
      shifts |= 4;
   if (key[KEY_ALT])
      shifts |= 8;
   if (key_shifts & KB_SCROLOCK_FLAG)
      shifts |= 16;
   if (key_shifts & KB_NUMLOCK_FLAG)
      shifts |= 32;
   if (key_shifts & KB_CAPSLOCK_FLAG)
      shifts |= 64;

   _dosmemputw(&shifts, 1, 0x417);

   key_buffer_start = 0;
   key_buffer_end = 0;

   for (c=0; c<128; c++)
      key[c] = FALSE;

   if ((keypressed_hook) && (readkey_hook))
      while (keypressed_hook())
	 readkey_hook();

   _remove_exit_func(remove_keyboard);
   keyboard_installed = FALSE;

   ENABLE();
}



/* set_leds:
 *  Overrides the state of the keyboard LED indicators.
 *  Set to -1 to return to default behavior.
 */
void set_leds(int leds)
{
   if (os_type == OSTYPE_WINNT)
      return;
   DISABLE();	/* killough 3/22/98 */
   if (leds < 0) {
      key_led_flag = TRUE;
      update_leds();
   }
   else {
      key_led_flag = FALSE;
      if ((!kb_send_data(0xED)) || (!kb_send_data((leds>>8)&7)))
	 kb_send_data(0xF4);
   }
   ENABLE(); 	/* killough 3/22/98 */
}



/* install_keyboard_hooks:
 *  You should only use this function if you *aren't* using the rest of the 
 *  keyboard handler. It can be called in the place of install_keyboard(), 
 *  and lets you provide callback routines to detect and read keypresses, 
 *  which will be used by the main keypressed() and readkey() functions. This 
 *  can be useful if you want to use Allegro's GUI code with a custom 
 *  keyboard handler, as it provides a way for the GUI to access keyboard 
 *  input from your own code.
 */
void install_keyboard_hooks(int (*keypressed)(), int (*readkey)())
{
   keypressed_hook = keypressed;
   readkey_hook = readkey;
}

