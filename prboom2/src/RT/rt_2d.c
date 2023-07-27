/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *  Copyright (C) 2022 by
 *  Sultim Tsyrendashiev
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
 *
 *---------------------------------------------------------------------
 */

#include "rt_main.h"

#include "i_video.h"
#include "v_video.h"


static float GetStatusBarScale()
{
  return 0.1f * BETWEEN(1, 10, rt_settings.statusbar_scale + 1);
}
static float GetHUDScale()
{
#if RT_SEPARATE_HUD_SCALE
  return 0.1f * BETWEEN(1, 10, rt_settings.hud_scale + 1);
#else
  return GetStatusBarScale();
#endif
}


static const float MATRIX_IDENTITY[] =
{
  1,0,0,0,
  0,1,0,0,
  0,0,1,0,
  0,0,0,1,
};


static void Swap(float *a, float *b)
{
  float t = *a;
  *a = *b;
  *b = t;
}


void RT_DrawLine(float x1, float y1, float x2, float y2, byte r, byte g, byte b)
{
  const float vw = (float)SCREENWIDTH;
  const float vh = (float)SCREENHEIGHT;
  
  x1 = x1 / vw * 2.0f - 1.0f;
  y1 = y1 / vh * 2.0f - 1.0f;
  x2 = x2 / vw * 2.0f - 1.0f;
  y2 = y2 / vh * 2.0f - 1.0f;

  uint32_t color = rgUtilPackColorByte4D(r, g, b, 255);

  // quad:  0 -- 3
  //        |    |
  //        1 -- 2
  RgPrimitiveVertex verts[] = {
      { .position = { x1, y1, 0 }, .color = color },
      { .position = { x2, y2, 0 }, .color = color },
  };

    // TODO RT: draw line
}


static void DrawQuad_Internal_T(const char* tex,
                                float x, float y, float width, float height,
                                float s1, float t1, float s2, float t2,
                                byte r, byte g, byte b, byte a)
{
  const float vw = (float)SCREENWIDTH;
  const float vh = (float)SCREENHEIGHT;

  float x1 = x / vw * 2.0f - 1.0f;
  float y1 = y / vh * 2.0f - 1.0f;
  float x2 = (x + width) / vw * 2.0f - 1.0f;
  float y2 = (y + height) / vh * 2.0f - 1.0f;

  uint32_t color = rgUtilPackColorByte4D(r, g, b, a);

  // quad:  0 -- 3
  //        |    |
  //        1 -- 2
  RgPrimitiveVertex verts[] = {
      { .position = { x1, y1, 0 }, .texCoord = { s1, t1 }, .color = color },
      { .position = { x1, y2, 0 }, .texCoord = { s1, t2 }, .color = color },
      { .position = { x2, y1, 0 }, .texCoord = { s2, t1 }, .color = color },
      { .position = { x2, y1, 0 }, .texCoord = { s2, t1 }, .color = color },
      { .position = { x1, y2, 0 }, .texCoord = { s1, t2 }, .color = color },
      { .position = { x2, y2, 0 }, .texCoord = { s2, t2 }, .color = color },
  };

  RgMeshPrimitiveForceRasterizedEXT raster = {
      .sType           = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_FORCE_RASTERIZED_EXT,
      .pNext           = NULL,
      .pViewport       = NULL,
      .pView           = NULL,
      .pProjection     = NULL,
      .pViewProjection = MATRIX_IDENTITY,
  };

  RgMeshPrimitiveInfo info = {
      .sType                = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_INFO,
      .pNext                = &raster,
      .flags                = RG_MESH_PRIMITIVE_TRANSLUCENT,
      .pPrimitiveNameInMesh = NULL,
      .primitiveIndexInMesh = 0,
      .pVertices            = verts,
      .vertexCount          = RG_ARRAY_SIZE(verts),
      .pIndices             = NULL,
      .indexCount           = 0,
      .pTextureName         = tex,
      .textureFrame         = 0,
      .color                = RG_PACKED_COLOR_WHITE,
      .emissive             = 0.0f,
  };

  RgResult _r = rgUploadMeshPrimitive(rtmain.instance, NULL, &info);
  RG_CHECK(_r);
}


static void DrawQuad_Internal(const char *tex, float x, float y, float width, float height, byte r, byte g, byte b, byte a)
{
  DrawQuad_Internal_T(tex, x, y, width, height, 0, 0, 1, 1, r, g, b, a);
}


void RT_DrawQuad(int x, int y, int width, int height, byte r, byte g, byte b, byte a)
{
  DrawQuad_Internal(NULL, (float)x, (float)y, (float)width, (float)height, r, g, b, a);
}


void RT_DrawQuad_Flat(int lump_flat, int x, int y, int width, int height, enum patch_translation_e flags)
{
  const rt_texture_t *td = RT_Texture_GetFromFlatLump(lump_flat);

  if (td == NULL)
  {
    return;
  }

  if (flags & VPT_STRETCH)
  {
    x = x * SCREENWIDTH / 320;
    y = y * SCREENHEIGHT / 200;
    width = width * SCREENWIDTH / 320;
    height = height * SCREENHEIGHT / 200;
  }

  float fU1 = 0;
  float fV1 = 0;

  float fU2 = (float)width / (float)td->width;
  float fV2 = (float)height / (float)td->height;

  DrawQuad_Internal_T(td->name, (float)x, (float)y, (float)width, (float)height, fU1, fV1, fU2, fV2, 255, 255, 255, 255);
}


void RT_DrawQuad_Patch(int lump, int x, int y, int width, int height, enum patch_translation_e flags)
{
  const rt_texture_t *td = RT_Texture_GetFromPatchLump(lump);

  if (td == NULL)
  {
    return;
  }

  x = x - td->leftoffset;
  y = y - td->topoffset;

  if (flags & VPT_STRETCH)
  {
    x = x * SCREENWIDTH / 320;
    y = y * SCREENHEIGHT / 200;
    width = width * SCREENWIDTH / 320;
    height = height * SCREENHEIGHT / 200;
  }

  DrawQuad_Internal(td->name, (float)x, (float)y, (float)width, (float)height, 255, 255, 255, 255);
}


void RT_TryApplyHUDCustomScale(enum patch_translation_e flags, float *p_xpos, float *p_ypos, float *p_width, float *p_height)
{
  float f = 1.0f;


  if (flags & VPT_STATUSBAR)
  {
    f = GetStatusBarScale();
  }
  else if (flags & VPT_HUD)
  {
    f = GetHUDScale();
  }
  else
  {
    return;
  }
  enum patch_translation_e algn = flags & VPT_ALIGN_MASK;

#if RT_ENABLE_STATUS_BAR_SCALE
  float xpos = *p_xpos;
  float ypos = *p_ypos;
  float width = *p_width;
  float height = *p_height;

  const float vw = (float)SCREENWIDTH;
  const float vh = (float)SCREENHEIGHT;

  // to [0..1]
  float x1 = (xpos) / vw;
  float x2 = (xpos + width) / vw;
  float y1 = (ypos) / vh;
  float y2 = (ypos + height) / vh;

#define ANCHOR_CENTER_X   (x1)=(x1)*2-1;(x2)=(x2)*2-1
#define ANCHOR_CENTER_Y   (y1)=(y1)*2-1;(y2)=(y2)*2-1
#define DEANCHOR_CENTER_X (x1)=((x1)+1)/2;(x2)=((x2)+1)/2
#define DEANCHOR_CENTER_Y (y1)=((y1)+1)/2;(y2)=((y2)+1)/2

#define ANCHOR_LEFT_X     (x1)=(x1);(x2)=(x2)
#define ANCHOR_TOP_Y      (y1)=(y1);(y2)=(y2)
#define DEANCHOR_LEFT_X   (x1)=(x1);(x2)=(x2)
#define DEANCHOR_TOP_Y    (y1)=(y1);(y2)=(y2)

#define ANCHOR_RIGHT_X    (x1)=1-(x1);(x2)=1-(x2)
#define ANCHOR_BOTTOM_Y   (y1)=1-(y1);(y2)=1-(y2)
#define DEANCHOR_RIGHT_X  (x1)=1-(x1);(x2)=1-(x2)
#define DEANCHOR_BOTTOM_Y (y1)=1-(y1);(y2)=1-(y2)

  // RT: very special case for shrinking the classic status bar
  if (algn == VPT_ALIGN_BOTTOM)
  {
    ANCHOR_CENTER_X;
    ANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_TOP)
  {
    ANCHOR_CENTER_X;
    ANCHOR_TOP_Y;
  }
  else if (algn == VPT_ALIGN_LEFT)
  {
    ANCHOR_LEFT_X;
    ANCHOR_CENTER_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT)
  {
    ANCHOR_RIGHT_X;
    ANCHOR_CENTER_Y;
  }
  else if (algn == VPT_ALIGN_LEFT_BOTTOM)
  {
    ANCHOR_LEFT_X;
    ANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_LEFT_TOP)
  {
    ANCHOR_LEFT_X;
    ANCHOR_TOP_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT_BOTTOM)
  {
    ANCHOR_RIGHT_X;
    ANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT_TOP)
  {
    ANCHOR_RIGHT_X;
    ANCHOR_TOP_Y;
  }
  else
  {
    assert(0);
    return;
  }


  {
    x1 *= f; x2 *= f; y1 *= f; y2 *= f;
  }


  // reset anchor
  if (algn == VPT_ALIGN_BOTTOM)
  {
    DEANCHOR_CENTER_X;
    DEANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_TOP)
  {
    DEANCHOR_CENTER_X;
    DEANCHOR_TOP_Y;
  }
  else if (algn == VPT_ALIGN_LEFT)
  {
    DEANCHOR_LEFT_X;
    DEANCHOR_CENTER_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT)
  {
    DEANCHOR_RIGHT_X;
    DEANCHOR_CENTER_Y;
  }
  else if (algn == VPT_ALIGN_LEFT_BOTTOM)
  {
    DEANCHOR_LEFT_X;
    DEANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_LEFT_TOP)
  {
    DEANCHOR_LEFT_X;
    DEANCHOR_TOP_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT_BOTTOM)
  {
    DEANCHOR_RIGHT_X;
    DEANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT_TOP)
  {
    DEANCHOR_RIGHT_X;
    DEANCHOR_TOP_Y;
  }
  else
  {
    assert(0);
    return;
  }

  // back to screen
  xpos   = x1 * vw;
  width  = x2 * vw - xpos;
  ypos   = y1 * vh;
  height = y2 * vh - ypos;

  *p_xpos = xpos;
  *p_ypos = ypos;
  *p_width = width;
  *p_height = height;
#endif
}


// based on cm2RGB from gl_main
static float rt_cm2RGB[CR_LIMIT + 1][4] =
{
  {0.50f ,0.00f, 0.00f, 1.00f}, //CR_BRICK
  {1.00f ,1.00f, 1.00f, 1.00f}, //CR_TAN
  {0.85f ,0.85f, 0.85f, 1.00f}, //CR_GRAY     // RT: darker
  {0.40f ,1.00f, 0.25f, 1.00f}, //CR_GREEN    // RT: match original green 
  {0.50f ,0.20f, 1.00f, 1.00f}, //CR_BROWN
  {1.00f ,1.00f, 0.00f, 1.00f}, //CR_GOLD
  {1.00f ,0.00f, 0.00f, 1.00f}, //CR_RED
  {0.80f ,0.80f, 1.00f, 1.00f}, //CR_BLUE
  {1.00f ,0.50f, 0.25f, 1.00f}, //CR_ORANGE
  {1.00f ,1.00f, 0.00f, 1.00f}, //CR_YELLOW
  {0.50f ,0.50f, 1.00f, 1.00f}, //CR_BLUE2
  {0.00f ,0.00f, 0.00f, 1.00f}, //CR_BLACK
  {0.50f ,0.00f, 0.50f, 1.00f}, //CR_PURPLE
  {1.00f ,1.00f, 1.00f, 1.00f}, //CR_WHITE
  {1.00f ,1.00f, 1.00f, 1.00f}, //CR_LIMIT
};


void RT_DrawQuad_NumPatch(float x, float y, int lump, int cm, enum patch_translation_e flags)
{
  const rt_texture_t *td = RT_Texture_GetFromPatchLump(lump);

  if (td == NULL)
  {
    return;
  }

  float leftoffset, topoffset;

  if (flags & VPT_NOOFFSET)
  {
    leftoffset = 0;
    topoffset = 0;
  }
  else
  {
    leftoffset = (float)td->leftoffset;
    topoffset = (float)td->topoffset;
  }

  // [FG] automatically center wide patches without horizontal offset
  if (td->width > 320 && leftoffset == 0)
  {
    x -= (float)(td->width - 320) / 2;
  }

  float xpos, ypos;
  float width, height;

  if (flags & VPT_STRETCH_MASK)
  {
    const stretch_param_t *params = &stretch_params[flags & VPT_ALIGN_MASK];

    xpos = (x - leftoffset) * (float)params->video->width / 320.0f + (float)params->deltax1;
    ypos = (y - topoffset) * (float)params->video->height / 200.0f + (float)params->deltay1;
    width = (float)(td->width * params->video->width) / 320.0f;
    height = (float)(td->height * params->video->height) / 200.0f;
  }
  else
  {
    xpos = x - leftoffset;
    ypos = y - topoffset;
    width = (float)td->width;
    height = (float)td->height;
  }

  RT_TryApplyHUDCustomScale(flags, &xpos, &ypos, &width, &height);


  uint8_t rgba[4] = { 255,255,255,255 };
  if (td->flags & RT_TEXTURE_FLAG_MONOCHROME_FOR_COLORMAPS_BIT)
  {
    const float *d = rt_cm2RGB[cm];

    rgba[0] = (uint8_t)BETWEEN(0, 255, (int)(d[0] * 255));
    rgba[1] = (uint8_t)BETWEEN(0, 255, (int)(d[1] * 255));
    rgba[2] = (uint8_t)BETWEEN(0, 255, (int)(d[2] * 255));
    rgba[3] = (uint8_t)BETWEEN(0, 255, (int)(d[3] * 255));
  }


  DrawQuad_Internal(td->name, xpos, ypos, width, height, rgba[0], rgba[1], rgba[2], rgba[3]);
}


// Based on M_GetKeyString
static int GetKeyString(char text_buffer[128], int c)
{
  int offset = 0;
  const char *s;

  if (c >= 33 && c <= 126)
  {

    // The '=', ',', and '.' keys originally meant the shifted
    // versions of those keys, but w/o having to shift them in
    // the game. Any actions that are mapped to these keys will
    // still mean their shifted versions. Could be changed later
    // if someone can come up with a better way to deal with them.

    if (c == '=')      // probably means the '+' key?
      c = '+';
    else if (c == ',') // probably means the '<' key?
      c = '<';
    else if (c == '.') // probably means the '>' key?
      c = '>';
    text_buffer[offset++] = (char)toupper(c); // Just insert the ascii key
    text_buffer[offset] = 0;
  }
  else
  {

    // Retrieve 4-letter (max) string representing the key

    // cph - Keypad keys, general code reorganisation to
    //  make this smaller and neater.
    if ((0x100 <= c) && (c < 0x200))
    {
      if (c == KEYD_KEYPADENTER)
        s = "PADE";
      else
      {
        strcpy(&text_buffer[offset], "PAD");
        offset += 4;
        text_buffer[offset - 1] = c & 0xff;
        text_buffer[offset] = 0;
      }
    }
    else if ((KEYD_F1 <= c) && (c < KEYD_F10))
    {
      text_buffer[offset++] = 'F';
      text_buffer[offset++] = '1' + c - KEYD_F1;
      text_buffer[offset] = 0;
    }
    else
    {
      switch (c)
      {
        case KEYD_TAB:      s = "TAB";  break;
        case KEYD_ENTER:      s = "ENTR"; break;
        case KEYD_ESCAPE:     s = "ESC";  break;
        case KEYD_SPACEBAR:   s = "SPAC"; break;
        case KEYD_BACKSPACE:  s = "BACK"; break;
        case KEYD_RCTRL:      s = "CTRL"; break;
        case KEYD_LEFTARROW:  s = "LARR"; break;
        case KEYD_UPARROW:    s = "UARR"; break;
        case KEYD_RIGHTARROW: s = "RARR"; break;
        case KEYD_DOWNARROW:  s = "DARR"; break;
        case KEYD_RSHIFT:     s = "SHFT"; break;
        case KEYD_RALT:       s = "ALT";  break;
        case KEYD_CAPSLOCK:   s = "CAPS"; break;
        case KEYD_SCROLLLOCK: s = "SCRL"; break;
        case KEYD_HOME:       s = "HOME"; break;
        case KEYD_PAGEUP:     s = "PGUP"; break;
        case KEYD_END:        s = "END";  break;
        case KEYD_PAGEDOWN:   s = "PGDN"; break;
        case KEYD_INSERT:     s = "INST"; break;
        case KEYD_DEL:        s = "DEL"; break;
        case KEYD_F10:        s = "F10";  break;
        case KEYD_F11:        s = "F11";  break;
        case KEYD_F12:        s = "F12";  break;
        case KEYD_PAUSE:      s = "PAUS"; break;
        case KEYD_MWHEELDOWN: s = "MWDN"; break;
        case KEYD_MWHEELUP:   s = "MWUP"; break;
        case KEYD_PRINTSC:    s = "PRSC"; break;
        case 0:               s = "NONE"; break;
        default:              s = "JUNK"; break;
      }

      if (s)
      { // cph - Slight code change
        strcpy(&text_buffer[offset], s); // string to display
        offset += strlen(s);
      }
    }
  }
  return offset;
}

const char *RT_GetFlashlightHintString(int key_flashlight)
{
  char key_text[128];
  GetKeyString(key_text, key_flashlight);
  key_text[sizeof(key_text) - 1] = '\0';

  static char hint_text[256];
  snprintf(hint_text, sizeof(hint_text), RG_STR_FLASHLIGHT_HINT, key_text);

  return hint_text;
}
