#include "doomstat.h"
#include "w_wad.h"
#include "r_main.h"
#include "v_video.h"
#include "st_stuff.h"
#include "g_game.h"
#include "am_map.h"
#include "lprintf.h"
#include "r_draw.h"
#include "r_filter.h"

//---------------------------------------------------------------------------
unsigned int filter_fracu;
unsigned int filter_tempColor;
unsigned int filter_tempFracV;

//---------------------------------------------------------------------------
// The ordered dither matrix that is tesselated to create varying intensity
//---------------------------------------------------------------------------
#if (DITHER_TYPE == -1)
  #define DMR 4  // convert the filter_ditherMatrix range to 256
  byte filter_ditherMatrix[DITHER_DIM][DITHER_DIM] =  {    0*DMR,  8*DMR,  2*DMR, 10*DMR,  0*DMR,  8*DMR,  2*DMR, 10*DMR,    60*DMR, 28*DMR, 52*DMR, 20*DMR, 62*DMR, 30*DMR, 54*DMR, 22*DMR,    0*DMR,  8*DMR,  2*DMR, 10*DMR,  0*DMR,  8*DMR,  2*DMR, 10*DMR,    60*DMR, 28*DMR, 52*DMR, 20*DMR, 62*DMR, 30*DMR, 54*DMR, 22*DMR,    0*DMR,  8*DMR,  2*DMR, 10*DMR,  0*DMR,  8*DMR,  2*DMR, 10*DMR,    60*DMR, 28*DMR, 52*DMR, 20*DMR, 62*DMR, 30*DMR, 54*DMR, 22*DMR,    0*DMR,  8*DMR,  2*DMR, 10*DMR,  0*DMR,  8*DMR,  2*DMR, 10*DMR,    60*DMR, 28*DMR, 52*DMR, 20*DMR, 62*DMR, 30*DMR, 54*DMR, 22  };
  
#elif (DITHER_TYPE == 0)
  #define DMR 16
  byte filter_ditherMatrix[DITHER_DIM][DITHER_DIM] = {     0*DMR,  8*DMR,  2*DMR, 10*DMR,      12*DMR,  4*DMR, 14*DMR,  6*DMR,      3*DMR, 11*DMR,  1*DMR,  9*DMR,      15*DMR,  7*DMR, 13*DMR,  5  };

#elif (DITHER_TYPE == 1) 
  #define DMR 16
  byte filter_ditherMatrix[DITHER_DIM][DITHER_DIM] = { 0*DMR,  14*DMR,  3*DMR, 13*DMR, 11*DMR,  5*DMR, 8*DMR,  6*DMR, 12*DMR, 2*DMR,  15*DMR,  1*DMR, 7*DMR,  9*DMR, 4*DMR,  10*DMR };

#elif (DITHER_TYPE == 2)
  #define DMR 4
  byte filter_ditherMatrix[DITHER_DIM][DITHER_DIM] =  {   0*DMR, 32*DMR,  8*DMR, 40*DMR,  2*DMR, 34*DMR, 10*DMR, 42*DMR,  48*DMR, 16*DMR, 56*DMR, 24*DMR, 50*DMR, 18*DMR, 58*DMR, 26*DMR,  12*DMR, 44*DMR,  4*DMR, 36*DMR, 14*DMR, 46*DMR,  6*DMR, 38*DMR,  60*DMR, 28*DMR, 52*DMR, 20*DMR, 62*DMR, 30*DMR, 54*DMR, 22*DMR,  3*DMR, 35*DMR, 11*DMR, 43*DMR,  1*DMR, 33*DMR,  9*DMR, 41*DMR,  51*DMR, 19*DMR, 59*DMR, 27*DMR, 49*DMR, 17*DMR, 57*DMR, 25*DMR,  15*DMR, 47*DMR,  7*DMR, 39*DMR, 13*DMR, 45*DMR,  5*DMR, 37*DMR,  63*DMR, 31*DMR, 55*DMR, 23*DMR, 61*DMR, 29*DMR, 53*DMR, 21  };

#elif (DITHER_TYPE == 3)
  #define DMR 1  
  byte  filter_ditherMatrix[DITHER_DIM][DITHER_DIM] = { 52*DMR,  44*DMR,  36*DMR, 124*DMR, 132*DMR, 140*DMR, 148*DMR, 156*DMR,   60*DMR,   4*DMR,  28*DMR, 116*DMR, 220*DMR, 228*DMR, 236*DMR, 164*DMR,                           68*DMR,  12*DMR,  20*DMR, 108*DMR, 212*DMR, 252*DMR, 244*DMR, 172*DMR,                           76*DMR,  84*DMR,  92*DMR, 100*DMR, 204*DMR, 196*DMR, 188*DMR, 180*DMR,                          132*DMR, 140*DMR, 148*DMR, 156*DMR,  52*DMR,  44*DMR,  36*DMR, 124*DMR,                          200*DMR, 228*DMR, 236*DMR, 164*DMR,  60*DMR,   4*DMR,  28*DMR, 116*DMR,                          212*DMR, 252*DMR, 244*DMR, 172*DMR,  68*DMR,  12*DMR,  20*DMR, 108*DMR,                          204*DMR, 196*DMR, 188*DMR, 180*DMR,  76*DMR,  84*DMR,  92*DMR, 100 };

#elif (DITHER_TYPE == 6)
  #define DMR 4
  byte filter_ditherMatrix[DITHER_DIM][DITHER_DIM] = {     1*DMR, 59*DMR, 15*DMR, 55*DMR,  2*DMR, 56*DMR, 12*DMR, 52*DMR,    33*DMR, 17*DMR, 47*DMR, 31*DMR, 34*DMR, 18*DMR, 44*DMR, 28*DMR,     9*DMR, 49*DMR,  5*DMR, 63*DMR, 10*DMR, 50*DMR,  6*DMR, 60*DMR,    41*DMR, 25*DMR, 37*DMR, 21*DMR, 42*DMR, 26*DMR, 38*DMR, 22*DMR,     3*DMR, 57*DMR, 13*DMR, 53*DMR,  0*DMR, 58*DMR, 14*DMR, 54*DMR,    35*DMR, 19*DMR, 45*DMR, 29*DMR, 32*DMR, 16*DMR, 46*DMR, 30*DMR,    11*DMR, 51*DMR,  7*DMR, 61*DMR,  8*DMR, 48*DMR,  4*DMR, 62*DMR,    43*DMR, 27*DMR, 39*DMR, 23*DMR, 40*DMR, 24*DMR, 36*DMR, 20 };

#elif (DITHER_TYPE == 7)
  #define DMR 1  
  byte filter_ditherMatrix[DITHER_DIM][DITHER_DIM] = {      1*DMR,235*DMR, 59*DMR,219*DMR, 15*DMR,231*DMR, 55*DMR,215*DMR,  2*DMR,232*DMR, 56*DMR,216*DMR, 12*DMR,228*DMR, 52*DMR,212*DMR,    129*DMR, 65*DMR,187*DMR,123*DMR,143*DMR, 79*DMR,183*DMR,119*DMR,130*DMR, 66*DMR,184*DMR,120*DMR,140*DMR, 76*DMR,180*DMR,116*DMR,     33*DMR,193*DMR, 17*DMR,251*DMR, 47*DMR,207*DMR, 31*DMR,247*DMR, 34*DMR,194*DMR, 18*DMR,248*DMR, 44*DMR,204*DMR, 28*DMR,244*DMR,    161*DMR, 97*DMR,145*DMR, 81*DMR,175*DMR,111*DMR,159*DMR, 95*DMR,162*DMR, 98*DMR,146*DMR, 82*DMR,172*DMR,108*DMR,156*DMR, 92*DMR,      9*DMR,225*DMR, 49*DMR,209*DMR,  5*DMR,239*DMR, 63*DMR,223*DMR, 10*DMR,226*DMR, 50*DMR,210*DMR,  6*DMR,236*DMR, 60*DMR,220*DMR,    137*DMR, 73*DMR,177*DMR,113*DMR,133*DMR, 69*DMR,191*DMR,127*DMR,138*DMR, 74*DMR,178*DMR,114*DMR,134*DMR, 70*DMR,188*DMR,124*DMR,     41*DMR,201*DMR, 25*DMR,241*DMR, 37*DMR,197*DMR, 21*DMR,255*DMR, 42*DMR,202*DMR, 26*DMR,242*DMR, 38*DMR,198*DMR, 22*DMR,252*DMR,    169*DMR,105*DMR,153*DMR, 89*DMR,165*DMR,101*DMR,149*DMR, 85*DMR,170*DMR,106*DMR,154*DMR, 90*DMR,166*DMR,102*DMR,150*DMR, 86*DMR,      3*DMR,233*DMR, 57*DMR,217*DMR, 13*DMR,229*DMR, 53*DMR,213*DMR,  0*DMR,234*DMR, 58*DMR,218*DMR, 14*DMR,230*DMR, 54*DMR,214*DMR,    131*DMR, 67*DMR,185*DMR,121*DMR,141*DMR, 77*DMR,181*DMR,117*DMR,128*DMR, 64*DMR,186*DMR,122*DMR,142*DMR, 78*DMR,182*DMR,118*DMR,     35*DMR,195*DMR, 19*DMR,249*DMR, 45*DMR,205*DMR, 29*DMR,245*DMR, 32*DMR,192*DMR, 16*DMR,250*DMR, 46*DMR,206*DMR, 30*DMR,246*DMR,    163*DMR, 99*DMR,147*DMR, 83*DMR,173*DMR,109*DMR,157*DMR, 93*DMR,160*DMR, 96*DMR,144*DMR, 80*DMR,174*DMR,110*DMR,158*DMR, 94*DMR,     11*DMR,227*DMR, 51*DMR,211*DMR,  7*DMR,237*DMR, 61*DMR,221*DMR,  8*DMR,224*DMR, 48*DMR,208*DMR,  4*DMR,238*DMR, 62*DMR,222*DMR,    139*DMR, 75*DMR,179*DMR,115*DMR,135*DMR, 71*DMR,189*DMR,125*DMR,136*DMR, 72*DMR,176*DMR,112*DMR,132*DMR, 68*DMR,190*DMR,126*DMR,     43*DMR,203*DMR, 27*DMR,243*DMR, 39*DMR,199*DMR, 23*DMR,253*DMR, 40*DMR,200*DMR, 24*DMR,240*DMR, 36*DMR,196*DMR, 20*DMR,254*DMR,    171*DMR,107*DMR,155*DMR, 91*DMR,167*DMR,103*DMR,151*DMR, 87*DMR,168*DMR,104*DMR,152*DMR, 88*DMR,164*DMR,100*DMR,148*DMR, 84 };

#elif (DITHER_TYPE == 10)
  // Order-8 clustered dithering matrix. 
  #define DMR 2
  byte filter_ditherMatrix[DITHER_DIM][DITHER_DIM] = { 64*DMR, 69*DMR, 77*DMR, 87*DMR, 86*DMR, 76*DMR, 68*DMR, 67*DMR, 63*DMR, 58*DMR, 50*DMR, 40*DMR, 41*DMR, 51*DMR, 59*DMR, 60*DMR,     70*DMR, 94*DMR,100*DMR,109*DMR,108*DMR, 99*DMR, 93*DMR, 75*DMR, 57*DMR, 33*DMR, 27*DMR, 18*DMR, 19*DMR, 28*DMR, 34*DMR, 52*DMR,     78*DMR,101*DMR,114*DMR,116*DMR,115*DMR,112*DMR, 98*DMR, 83*DMR, 49*DMR, 26*DMR, 13*DMR, 11*DMR, 12*DMR, 15*DMR, 29*DMR, 44*DMR,     88*DMR,110*DMR,123*DMR,124*DMR,125*DMR,118*DMR,107*DMR, 85*DMR, 39*DMR, 17*DMR,  4*DMR,  3*DMR,  2*DMR,  9*DMR, 20*DMR, 42*DMR,     89*DMR,111*DMR,122*DMR,127*DMR,126*DMR,117*DMR,106*DMR, 84*DMR, 38*DMR, 16*DMR,  5*DMR,  0*DMR,  1*DMR, 10*DMR, 21*DMR, 43*DMR,     79*DMR,102*DMR,119*DMR,121*DMR,120*DMR,113*DMR, 97*DMR, 82*DMR, 48*DMR, 25*DMR,  8*DMR,  6*DMR,  7*DMR, 14*DMR, 30*DMR, 45*DMR,     71*DMR, 95*DMR,103*DMR,104*DMR,105*DMR, 96*DMR, 92*DMR, 74*DMR, 56*DMR, 32*DMR, 24*DMR, 23*DMR, 22*DMR, 31*DMR, 35*DMR, 53*DMR,     65*DMR, 72*DMR, 80*DMR, 90*DMR, 91*DMR, 81*DMR, 73*DMR, 66*DMR, 62*DMR, 55*DMR, 47*DMR, 37*DMR, 36*DMR, 46*DMR, 54*DMR, 61*DMR,    63*DMR, 58*DMR, 50*DMR, 40*DMR, 41*DMR, 51*DMR, 59*DMR, 60*DMR, 64*DMR, 69*DMR, 77*DMR, 87*DMR, 86*DMR, 76*DMR, 68*DMR, 67*DMR,     57*DMR, 33*DMR, 27*DMR, 18*DMR, 19*DMR, 28*DMR, 34*DMR, 52*DMR, 70*DMR, 94*DMR,100*DMR,109*DMR,108*DMR, 99*DMR, 93*DMR, 75*DMR,     49*DMR, 26*DMR, 13*DMR, 11*DMR, 12*DMR, 15*DMR, 29*DMR, 44*DMR, 78*DMR,101*DMR,114*DMR,116*DMR,115*DMR,112*DMR, 98*DMR, 83*DMR,     39*DMR, 17*DMR,  4*DMR,  3*DMR,  2*DMR,  9*DMR, 20*DMR, 42*DMR, 88*DMR,110*DMR,123*DMR,124*DMR,125*DMR,118*DMR,107*DMR, 85*DMR,     38*DMR, 16*DMR,  5*DMR,  0*DMR,  1*DMR, 10*DMR, 21*DMR, 43*DMR, 89*DMR,111*DMR,122*DMR,127*DMR,126*DMR,117*DMR,106*DMR, 84*DMR,     48*DMR, 25*DMR,  8*DMR,  6*DMR,  7*DMR, 14*DMR, 30*DMR, 45*DMR, 79*DMR,102*DMR,119*DMR,121*DMR,120*DMR,113*DMR, 97*DMR, 82*DMR,     56*DMR, 32*DMR, 24*DMR, 23*DMR, 22*DMR, 31*DMR, 35*DMR, 53*DMR, 71*DMR, 95*DMR,103*DMR,104*DMR,105*DMR, 96*DMR, 92*DMR, 74*DMR,     62*DMR, 55*DMR, 47*DMR, 37*DMR, 36*DMR, 46*DMR, 54*DMR, 61*DMR, 65*DMR, 72*DMR, 80*DMR, 90*DMR, 91*DMR, 81*DMR, 73*DMR, 66 };
#endif

/*
//---------------------------------------------------------------------------
int filter_getFilteredForColumn32(const byte *colormap, fixed_t texV, fixed_t nextRowTexV) {
  unsigned int fracv, fracu;
  byte c0, c1, c2, c3;

  //#define fracu filter_fracu
  fracu = filter_fracu;
  fracv = texV & 0xffff;
  
// 0 1
// 2 3
  // gather up the for texels that will be blended to produce the output color
  c0 = colormap[ dcvars.source[texV>>FRACBITS] ];
  c1 = colormap[ dcvars.nextsource[texV>>FRACBITS] ];
  c2 = colormap[ dcvars.source[nextRowTexV>>FRACBITS] ];
  c3 = colormap[ dcvars.nextsource[nextRowTexV>>FRACBITS] ];

  // blend (add) them using the proper weights
  return
    vid_intPalette[c3][(fracu*fracv)>>(32-VID_COLORWEIGHTBITS)] +
    vid_intPalette[c2][((0xffff-fracu)*fracv)>>(32-VID_COLORWEIGHTBITS)] +
    vid_intPalette[c0][((0xffff-fracu)*(0xffff-fracv))>>(32-VID_COLORWEIGHTBITS)] +
    vid_intPalette[c1][(fracu*(0xffff-fracv))>>(32-VID_COLORWEIGHTBITS)];
}

//---------------------------------------------------------------------------
short filter_getFilteredForColumn16(const byte *colormap, fixed_t texV, fixed_t nextRowTexV) {
  unsigned int color;
  color =  filter_getFilteredForColumn32(colormap, texV, nextRowTexV);
  return (short)(
    ((color&0xff0000) >> 8) & 0xf800 |
    ((color&0x00ff00) >> 5) & 0x07e0 |
    ((color&0x0000ff) >> 3) & 0x003f
  );
}

//---------------------------------------------------------------------------
int filter_getFilteredForSpan32(const byte *colormap, unsigned int texU, unsigned int texV) {
  unsigned int fracu = texU & 0xffff;
  unsigned int fracv = texV & 0xffff;
  byte c0, c1, c2, c3;
  
  c0 = colormap[ dsvars.source[ ((texU>>16)&0x3f) | ((texV>>10)&0xfc0) ] ];
  c1 = colormap[ dsvars.source[ (((texU+FRACUNIT)>>16)&0x3f) | ((texV>>10)&0xfc0) ] ];
  c2 = colormap[ dsvars.source[ ((texU>>16)&0x3f) | (((texV+FRACUNIT)>>10)&0xfc0) ] ];
  c3 = colormap[ dsvars.source[ (((texU+FRACUNIT)>>16)&0x3f) | (((texV+FRACUNIT)>>10)&0xfc0) ] ];

  return
    vid_intPalette[c3][(fracu*fracv)>>(32-VID_COLORWEIGHTBITS)] +
    vid_intPalette[c2][((0xffff-fracu)*fracv)>>(32-VID_COLORWEIGHTBITS)] +
    vid_intPalette[c0][((0xffff-fracu)*(0xffff-fracv))>>(32-VID_COLORWEIGHTBITS)] +
    vid_intPalette[c1][(fracu*(0xffff-fracv))>>(32-VID_COLORWEIGHTBITS)];
}

//---------------------------------------------------------------------------
short filter_getFilteredForSpan16(const byte *colormap, unsigned int texU, unsigned int texV) {
  unsigned int color;
  color =  filter_getFilteredForSpan32(colormap, texU, texV);
  return (short)(
    ((color&0xff0000) >> 8) & 0xf800 |
    ((color&0x00ff00) >> 5) & 0x07e0 |
    ((color&0x0000ff) >> 3) & 0x003f
  );
}
*/