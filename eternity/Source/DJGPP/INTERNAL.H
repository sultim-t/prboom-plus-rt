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
 *      Some definitions for internal use by the library code.
 *      This should not be included by user programs.
 *
 *      See readme.txt for copyright information.
 */


#ifndef INTERNAL_H
#define INTERNAL_H

#include "allegro.h"

#ifdef DJGPP
#include "interndj.h"
#else
#include "internli.h"
#endif


/* flag for how many times we have been initialised */
extern int _allegro_count;


/* some Allegro functions need a block of scratch memory */
extern void *_scratch_mem;
extern int _scratch_mem_size;

__INLINE__ void _grow_scratch_mem(int size)
{
   if (size > _scratch_mem_size) {
      size = (size+1023) & 0xFFFFFC00;
      _scratch_mem = realloc(_scratch_mem, size);
      _scratch_mem_size = size;
   }
}


/* list of functions to call at program cleanup */
void _add_exit_func(void (*func)());
void _remove_exit_func(void (*func)());


/* various bits of mouse stuff */
void _set_mouse_range();
extern BITMAP *_mouse_screen;


/* various bits of timer stuff */
extern int _timer_use_retrace;
extern volatile int _retrace_hpp_value;


/* asm joystick polling routine */
int _poll_joystick(int *x, int *y, int *x2, int *y2, int poll_mask);


/* caches and tables for svga bank switching */
extern int _last_bank_1, _last_bank_2; 
extern int *_gfx_bank; 


/* bank switching routines */
void _stub_bank_switch();
void _stub_bank_switch_end();


/* stuff for setting up bitmaps */
typedef struct _GFX_MODE_INFO
{
   int w, h;
   int bpp;
   int bios_num;
   int bios_int;
   int (*setter)(int w, int h, int bpp);
} _GFX_MODE_INFO;

void _check_gfx_virginity();
BITMAP *_gfx_mode_set_helper(int w, int h, int v_w, int v_h, int color_depth, GFX_DRIVER *driver, int (*detect)(), _GFX_MODE_INFO *mode_list, void (*set_width)(int w));
BITMAP *_make_bitmap(int w, int h, unsigned long addr, GFX_DRIVER *driver, int color_depth, int bpl);
void _sort_out_virtual_width(int *width, GFX_DRIVER *driver);

GFX_VTABLE *_get_vtable(int color_depth);

extern int _sub_bitmap_id_count;

#define BYTES_PER_PIXEL(bpp)     (((int)(bpp) + 7) / 8)

int _color_load_depth(int depth);

BITMAP *_fixup_loaded_bitmap(BITMAP *bmp, PALETTE pal, int bpp);


/* truecolor pixel blending information */
extern BLENDER_MAP *_blender_map15;
extern BLENDER_MAP *_blender_map16;
extern BLENDER_MAP *_blender_map24;

extern int _blender_col_15;
extern int _blender_col_16;
extern int _blender_col_24;
extern int _blender_col_32;

extern int _blender_alpha;


/* VGA register access routines */
void _vga_vsync();
void _vga_set_pallete_range(PALLETE p, int from, int to, int vsync);

extern int _crtc;


/* _read_vga_register:
 *  Reads the contents of a VGA register.
 */
__INLINE__ int _read_vga_register(int port, int index)
{
   if (port==0x3C0)
      inportb(_crtc+6); 

   outportb(port, index);
   return inportb(port+1);
}


/* _write_vga_register:
 *  Writes a byte to a VGA register.
 */
__INLINE__ void _write_vga_register(int port, int index, int v) 
{
   if (port==0x3C0) {
      inportb(_crtc+6);
      outportb(port, index);
      outportb(port, v);
   }
   else {
      outportb(port, index);
      outportb(port+1, v);
   }
}


/* _alter_vga_register:
 *  Alters specific bits of a VGA register.
 */
__INLINE__ void _alter_vga_register(int port, int index, int mask, int v)
{
   int temp;
   temp = _read_vga_register(port, index);
   temp &= (~mask);
   temp |= (v & mask);
   _write_vga_register(port, index, temp);
}


/* _vsync_out_h:
 *  Waits until the VGA is not in either a vertical or horizontal retrace.
 */
__INLINE__ void _vsync_out_h()
{
   do {
   } while (inportb(0x3DA) & 1);
}


/* _vsync_out_v:
 *  Waits until the VGA is not in a vertical retrace.
 */
__INLINE__ void _vsync_out_v()
{
   do {
   } while (inportb(0x3DA) & 8);
}


/* _vsync_in:
 *  Waits until the VGA is in the vertical retrace period.
 */
__INLINE__ void _vsync_in()
{
   if (_timer_use_retrace) {
      int t = retrace_count; 

      do {
      } while (t == retrace_count);
   }
   else {
      do {
      } while (!(inportb(0x3DA) & 8));
   }
}


/* _write_hpp:
 *  Writes to the VGA pelpan register.
 */
__INLINE__ void _write_hpp(int value)
{
   if (_timer_use_retrace) {
      _retrace_hpp_value = value;

      do {
      } while (_retrace_hpp_value == value);
   }
   else {
      do {
      } while (!(inportb(0x3DA) & 8));

      _write_vga_register(0x3C0, 0x33, value);
   }
}


int _test_vga_register(int port, int index, int mask);
int _test_register(int port, int mask);
void _set_vga_virtual_width(int old_width, int new_width);


/* current drawing mode */
extern int _drawing_mode;
extern BITMAP *_drawing_pattern;
extern int _drawing_x_anchor;
extern int _drawing_y_anchor;
extern unsigned int _drawing_x_mask;
extern unsigned int _drawing_y_mask;


/* graphics drawing routines */
void _normal_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
void _normal_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);

int  _linear_getpixel8(struct BITMAP *bmp, int x, int y);
void _linear_putpixel8(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline8(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline8(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_sprite8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_v_flip8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_h_flip8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_vh_flip8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_trans_sprite8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite8(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite8(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite8(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _linear_draw_character8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_textout_fixed8(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _linear_blit8(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_blit_backward8(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_masked_blit8(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_clear_to_color8(struct BITMAP *bitmap, int color);

#ifdef ALLEGRO_COLOR16

void _linear_putpixel15(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline15(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline15(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_trans_sprite15(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite15(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite15(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite15(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite15(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);

int  _linear_getpixel16(struct BITMAP *bmp, int x, int y);
void _linear_putpixel16(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline16(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline16(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_sprite16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_256_sprite16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_v_flip16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_h_flip16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_vh_flip16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_trans_sprite16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite16(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite16(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite16(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _linear_draw_character16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_textout_fixed16(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _linear_blit16(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_blit_backward16(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_masked_blit16(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_clear_to_color16(struct BITMAP *bitmap, int color);

#endif

#ifdef ALLEGRO_COLOR24

int  _linear_getpixel24(struct BITMAP *bmp, int x, int y);
void _linear_putpixel24(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline24(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline24(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_sprite24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_256_sprite24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_v_flip24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_h_flip24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_vh_flip24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_trans_sprite24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite24(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite24(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite24(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _linear_draw_character24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_textout_fixed24(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _linear_blit24(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_blit_backward24(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_masked_blit24(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_clear_to_color24(struct BITMAP *bitmap, int color);

#endif

#ifdef ALLEGRO_COLOR32

int  _linear_getpixel32(struct BITMAP *bmp, int x, int y);
void _linear_putpixel32(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline32(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline32(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_sprite32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_256_sprite32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_v_flip32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_h_flip32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_vh_flip32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_trans_sprite32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite32(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite32(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite32(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _linear_draw_character32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_textout_fixed32(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _linear_blit32(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_blit_backward32(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_masked_blit32(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_clear_to_color32(struct BITMAP *bitmap, int color);

#endif

int  _x_getpixel(struct BITMAP *bmp, int x, int y);
void _x_putpixel(struct BITMAP *bmp, int x, int y, int color);
void _x_vline(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _x_hline(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _x_draw_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_sprite_v_flip(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_sprite_h_flip(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_sprite_vh_flip(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_trans_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_lit_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _x_draw_rle_sprite(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _x_draw_trans_rle_sprite(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _x_draw_lit_rle_sprite(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _x_draw_character(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _x_textout_fixed(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _x_blit_from_memory(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_blit_to_memory(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_blit(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_blit_forward(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_blit_backward(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_masked_blit(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_clear_to_color(struct BITMAP *bitmap, int color);


/* asm helper for stretch_blit() */
void _do_stretch(BITMAP *source, BITMAP *dest, void *drawer, int sx, fixed sy, fixed syd, int dx, int dy, int dh, int color_depth);


/* information for polygon scanline fillers */
typedef struct POLYGON_SEGMENT
{
   fixed u, v, du, dv;              /* fixed point u/v coordinates */
   fixed c, dc;                     /* single color gouraud shade values */
   fixed r, g, b, dr, dg, db;       /* RGB gouraud shade values */
   float z, dz;                     /* polygon depth (1/z) */
   float fu, fv, dfu, dfv;          /* floating point u/v coordinates */
   unsigned char *texture;          /* the texture map */
   int umask, vmask, vshift;        /* texture map size information */
   int seg;                         /* destination bitmap selector */
} POLYGON_SEGMENT;


/* polygon scanline filler functions */
void _poly_scanline_flat(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_gcol(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_grgb(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit(unsigned long addr, int w, POLYGON_SEGMENT *info);


/* sound lib stuff */
extern int _digi_volume;
extern int _midi_volume;
extern int _flip_pan; 

extern int (*_midi_init)();
extern void (*_midi_exit)();

int _midi_allocate_voice(int min, int max);

extern volatile long _midi_tick;

int _digmid_find_patches(char *dir, char *file);

#define VIRTUAL_VOICES  256


typedef struct          /* a virtual (as seen by the user) soundcard voice */
{
   SAMPLE *sample;      /* which sample are we playing? (NULL = free) */
   int num;             /* physical voice number (-1 = been killed off) */
   int autokill;        /* set to free the voice when the sample finishes */
   long time;           /* when we were started (for voice allocation) */
   int priority;        /* how important are we? */
} VOICE;

extern VOICE _voice[VIRTUAL_VOICES];


typedef struct          /* a physical (as used by hardware) soundcard voice */
{
   int num;             /* the virtual voice currently using me (-1 = free) */
   int playmode;        /* are we looping? */
   int vol;             /* current volume (fixed point .12) */
   int dvol;            /* volume delta, for ramping */
   int target_vol;      /* target volume, for ramping */
   int pan;             /* current pan (fixed point .12) */
   int dpan;            /* pan delta, for sweeps */
   int target_pan;      /* target pan, for sweeps */
   int freq;            /* current frequency (fixed point .12) */
   int dfreq;           /* frequency delta, for sweeps */
   int target_freq;     /* target frequency, for sweeps */
} PHYS_VOICE;

extern PHYS_VOICE _phys_voice[DIGI_VOICES];


#define MIXER_DEF_SFX               8
#define MIXER_MAX_SFX               32

int _mixer_init(int bufsize, int freq, int stereo, int is16bit, int *voices);
void _mixer_exit();
void _mix_some_samples(unsigned long buf, unsigned short seg, int issigned);

void _mixer_init_voice(int voice, SAMPLE *sample);
void _mixer_release_voice(int voice);
void _mixer_start_voice(int voice);
void _mixer_stop_voice(int voice);
void _mixer_loop_voice(int voice, int loopmode);
int  _mixer_get_position(int voice);
void _mixer_set_position(int voice, int position);
int  _mixer_get_volume(int voice);
void _mixer_set_volume(int voice, int volume);
void _mixer_ramp_volume(int voice, int time, int endvol);
void _mixer_stop_volume_ramp(int voice);
int  _mixer_get_frequency(int voice);
void _mixer_set_frequency(int voice, int frequency);
void _mixer_sweep_frequency(int voice, int time, int endfreq);
void _mixer_stop_frequency_sweep(int voice);
int  _mixer_get_pan(int voice);
void _mixer_set_pan(int voice, int pan);
void _mixer_sweep_pan(int voice, int time, int endpan);
void _mixer_stop_pan_sweep(int voice);
void _mixer_set_echo(int voice, int strength, int delay);
void _mixer_set_tremolo(int voice, int rate, int depth);
void _mixer_set_vibrato(int voice, int rate, int depth);

/* dummy functions for the NoSound drivers */
int  _dummy_detect();
int  _dummy_init(int voices);
void _dummy_exit();
int  _dummy_mixer_volume(int volume);
void _dummy_init_voice(int voice, SAMPLE *sample);
void _dummy_noop1(int p);
void _dummy_noop2(int p1, int p2);
void _dummy_noop3(int p1, int p2, int p3);
int  _dummy_get_position(int voice);
int  _dummy_get(int voice);
void _dummy_raw_midi(unsigned char data);
int  _dummy_load_patches(char *patches, char *drums);
void _dummy_adjust_patches(char *patches, char *drums);
void _dummy_key_on(int inst, int note, int bend, int vol, int pan);


/* from djgpp's libc, needed to find which directory we were run from */
extern int __crt0_argc;
extern char **__crt0_argv;


#endif          /* ifndef INTERNAL_H */
