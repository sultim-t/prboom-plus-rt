#include <math.h>

#include "doomstat.h"
#include "e6y.h"
#include "lprintf.h"
#include "rt_main.h"
#include "r_fps.h"
#include "r_main.h"
#include "r_patch.h"
#include "r_things.h"


#define SET_RGVEC2(vec, x, y)    (vec).data[0]=x;(vec).data[1]=y
#define SET_RGVEC3(vec, x, y, z) (vec).data[0]=x;(vec).data[1]=y;(vec).data[2]=z


typedef struct
{
  int cm;
  float x, y, z;
  float vt, vb;
  float ul, ur;
  float x1, y1;
  float x2, y2;
  float light;
  // float fogdensity;
  fixed_t scale;
  const rt_texture_t *td;
  uint_64_t flags;
  // int index;
  int xy;
  fixed_t fx, fy;
} rt_sprite_t;



// gld_CalcLightLevel_shaders
static float CalcLightLevel(int lightlevel)
{
  int light = BETWEEN(0, 255, lightlevel);

  return (float)light / 255.0f;
}


static const RgFloat3D normals[6] = 
{
  { 0, 1, 0 },
  { 0, 1, 0 },
  { 0, 1, 0 },
  { 0, 1, 0 },
  { 0, 1, 0 },
  { 0, 1, 0 }
};


static void DrawSprite(const mobj_t *thing, const rt_sprite_t *sprite, int sectornum)
{
  dboolean no_depth_test            = !!(sprite->flags & MF_NO_DEPTH_TEST);
  dboolean is_partial_invisibility  = !!(sprite->flags & MF_SHADOW);
  dboolean is_translucent           = !!(sprite->flags & MF_TRANSLUCENT);

  dboolean is_rasterized = no_depth_test || is_translucent;


  RgFloat3D positions[6];
  RgFloat2D texcoords[6];


  float yaw = 270.0f - (float)(viewangle >> ANGLETOFINESHIFT) * 360.0f / FINEANGLES;
  float inv_yaw = 180.0f - yaw;
  float cos_inv_yaw = cosf(inv_yaw * (float)M_PI / 180.f);
  float sin_inv_yaw = sinf(inv_yaw * (float)M_PI / 180.f);

  if (sprite->flags & (MF_SOLID | MF_SPAWNCEILING))
  {
    float x1 = +(sprite->x1 * cos_inv_yaw) + sprite->x;
    float x2 = +(sprite->x2 * cos_inv_yaw) + sprite->x;

    float y1 = sprite->y + sprite->y1;
    float y2 = sprite->y + sprite->y2;

    float z2 = -(sprite->x1 * sin_inv_yaw) + sprite->z;
    float z1 = -(sprite->x2 * sin_inv_yaw) + sprite->z;

    SET_RGVEC3(positions[0], x1, y1, z2); SET_RGVEC2(texcoords[0], sprite->ul, sprite->vt); // 0
    SET_RGVEC3(positions[1], x2, y1, z1); SET_RGVEC2(texcoords[1], sprite->ur, sprite->vt); // 1
    SET_RGVEC3(positions[2], x1, y2, z2); SET_RGVEC2(texcoords[2], sprite->ul, sprite->vb); // 2
    SET_RGVEC3(positions[3], x2, y1, z1); SET_RGVEC2(texcoords[3], sprite->ur, sprite->vt); // 1
    SET_RGVEC3(positions[4], x2, y2, z1); SET_RGVEC2(texcoords[4], sprite->ur, sprite->vb); // 3
    SET_RGVEC3(positions[5], x1, y2, z2); SET_RGVEC2(texcoords[5], sprite->ul, sprite->vb); // 2
  }
  else
  {
    const float cos_paperitems_pitch = cosf(0);
    const float sin_paperitems_pitch = sinf(0);

    float ycenter = fabsf(sprite->y1 - sprite->y2) * 0.5f;
    float y1c = sprite->y1 - ycenter;
    float y2c = sprite->y2 - ycenter;
    float cy = sprite->y + ycenter;

    float y1z2_y = -(y1c * sin_paperitems_pitch);
    float y2z2_y = -(y2c * sin_paperitems_pitch);

    float x1 = +(sprite->x1 * cos_inv_yaw - y1z2_y * sin_inv_yaw) + sprite->x;
    float x2 = +(sprite->x2 * cos_inv_yaw - y1z2_y * sin_inv_yaw) + sprite->x;
    float x3 = +(sprite->x1 * cos_inv_yaw - y2z2_y * sin_inv_yaw) + sprite->x;
    float x4 = +(sprite->x2 * cos_inv_yaw - y2z2_y * sin_inv_yaw) + sprite->x;

    float y1 = +(y1c * cos_paperitems_pitch) + cy;
    float y2 = +(y2c * cos_paperitems_pitch) + cy;

    float z1 = -(sprite->x1 * sin_inv_yaw + y1z2_y * cos_inv_yaw) + sprite->z;
    float z2 = -(sprite->x2 * sin_inv_yaw + y1z2_y * cos_inv_yaw) + sprite->z;
    float z3 = -(sprite->x1 * sin_inv_yaw + y2z2_y * cos_inv_yaw) + sprite->z;
    float z4 = -(sprite->x2 * sin_inv_yaw + y2z2_y * cos_inv_yaw) + sprite->z;
    
    SET_RGVEC3(positions[0], x1, y1, z1); SET_RGVEC2(texcoords[0], sprite->ul, sprite->vt); // 0
    SET_RGVEC3(positions[1], x2, y1, z2); SET_RGVEC2(texcoords[1], sprite->ur, sprite->vt); // 1
    SET_RGVEC3(positions[2], x3, y2, z3); SET_RGVEC2(texcoords[2], sprite->ul, sprite->vb); // 2
    SET_RGVEC3(positions[3], x2, y1, z2); SET_RGVEC2(texcoords[3], sprite->ur, sprite->vt); // 1
    SET_RGVEC3(positions[4], x4, y2, z4); SET_RGVEC2(texcoords[4], sprite->ur, sprite->vb); // 3
    SET_RGVEC3(positions[5], x3, y2, z3); SET_RGVEC2(texcoords[5], sprite->ul, sprite->vb); // 2
  }


  if (!is_rasterized)
  {
    RgGeometryUploadInfo info =
    {
      .uniqueID = RT_GetUniqueID_Thing(thing),
      .flags = is_partial_invisibility ? RG_GEOMETRY_UPLOAD_REFL_REFR_ALBEDO_MULTIPLY_BIT : 0,
      .geomType = RG_GEOMETRY_TYPE_DYNAMIC,
      .passThroughType = is_partial_invisibility ? RG_GEOMETRY_PASS_THROUGH_TYPE_MIRROR : RG_GEOMETRY_PASS_THROUGH_TYPE_ALPHA_TESTED,
      .visibilityType = RG_GEOMETRY_VISIBILITY_TYPE_WORLD_0,
      .vertexCount = 6,
      .pVertexData = positions,
      .pNormalData = normals,
      .pTexCoordLayerData = { texcoords },
      .sectorID = sectornum,
      .layerColors = { RG_COLOR_WHITE },
      .defaultRoughness = 0.5f,
      .defaultMetallicity = 0.1f,
      .defaultEmission = 0.0f,
      .geomMaterial = { sprite->td->rg_handle },
      .transform = RG_TRANSFORM_IDENTITY
    };

    RgResult r = rgUploadGeometry(rtmain.instance, &info);
    RG_CHECK(r);
  }
  else
  {
    RgRasterizedGeometryVertexArrays arr =
    {
      .pVertexData = positions,
      .pTexCoordData = texcoords,
      .pColorData = NULL,
      .vertexStride = sizeof(RgFloat3D),
      .texCoordStride = sizeof(RgFloat2D),
      .colorStride = 0
    };

    RgRasterizedGeometryUploadInfo info =
    {
      .renderType = RG_RASTERIZED_GEOMETRY_RENDER_TYPE_DEFAULT,
      .vertexCount = 6,
      .pArrays = &arr,
      .transform = RG_TRANSFORM_IDENTITY,
      .color = RG_COLOR_WHITE,
      .material = sprite->td->rg_handle,
      .blendEnable = true,
      .blendFuncSrc = RG_BLEND_FACTOR_SRC_ALPHA,
      .blendFuncDst = RG_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .depthTest = !no_depth_test,
      .depthWrite = !no_depth_test
    };

    RgResult r = rgUploadRasterizedGeometry(rtmain.instance, &info, NULL, NULL);
    RG_CHECK(r);
  }
}


void RT_ProjectSprite(int sectornum, mobj_t *thing, int lightlevel)
{
  if (sectornum < 0 || sectornum >= numsectors)
  {
    assert(0);
    return;
  }


  spritedef_t *sprdef;
  spriteframe_t *sprframe;
  int lump;
  dboolean flip;

  fixed_t fx, fy, fz;

  rt_sprite_t sprite = { 0 };

  // int mlook = HaveMouseLook() || (render_fov > FOV90);

  if (!paused && movement_smooth)
  {
    fx = thing->PrevX + FixedMul(tic_vars.frac, thing->x - thing->PrevX);
    fy = thing->PrevY + FixedMul(tic_vars.frac, thing->y - thing->PrevY);
    fz = thing->PrevZ + FixedMul(tic_vars.frac, thing->z - thing->PrevZ);
  }
  else
  {
    fx = thing->x;
    fy = thing->y;
    fz = thing->z;
  }

  fixed_t tr_x = fx - viewx;
  fixed_t tr_y = fy - viewy;

  fixed_t gxt = FixedMul(tr_x, viewcos);
  fixed_t gyt = -FixedMul(tr_y, viewsin);

  fixed_t tz = gxt - gyt;

  // thing is behind view plane?
  //if (tz < r_near_clip_plane)
  //  return;

  //gxt = -FixedMul(tr_x, viewsin);
  //gyt =  FixedMul(tr_y, viewcos);
  //fixed_t tx = -(gyt + gxt);

  //e6y
  //if (!render_paperitems && mlook)
  //{
  //  if (tz >= MINZ && (D_abs(tx) >> 5) > tz)
  //    return;
  //}
  //else
  //{
  //  // too far off the side?
  //  if (D_abs(tx) > (tz << 2))
  //    return;
  //}

  // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
  if ((unsigned)thing->sprite >= (unsigned)numsprites)
    I_Error("RT_ProjectSprite: Invalid sprite number %i", thing->sprite);
#endif

  sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
  if ((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
    I_Error("RT_ProjectSprite: Invalid sprite frame %i : %i", thing->sprite, thing->frame);
#endif

  if (!sprdef->spriteframes)
    I_Error("RT_ProjectSprite: Missing spriteframes %i : %i", thing->sprite, thing->frame);

  sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

  if (sprframe->rotate)
  {
    // choose a different rotation based on player view
    angle_t rot;
    angle_t ang = R_PointToAngle2(viewx, viewy, fx, fy);
    if (sprframe->lump[0] == sprframe->lump[1])
    {
      rot = (ang - thing->angle + (angle_t)(ANG45 / 2) * 9) >> 28;
    }
    else
    {
      rot = (ang - thing->angle + (angle_t)(ANG45 / 2) * 9 -
             (angle_t)(ANG180 / 16)) >> 28;
    }
    lump = sprframe->lump[rot];
    flip = (dboolean)(sprframe->flip & (1 << rot));
  }
  else
  {
    // use single rotation for all views
    lump = sprframe->lump[0];
    flip = (dboolean)(sprframe->flip & 1);
  }
  lump += firstspritelump;

  const rpatch_t *patch = R_CachePatchNum(lump);
  thing->patch_width = patch->width;

  //// killough 4/9/98: clip things which are out of view due to height
  //if (!mlook)
  //{
  //  int x1, x2;
  //  fixed_t xscale = FixedDiv(projection, tz);
  //  /* calculate edges of the shape
  //  * cph 2003/08/1 - fraggle points out that this offset must be flipped
  //  * if the sprite is flipped; e.g. FreeDoom imp is messed up by this. */
  //  if (flip)
  //    tx -= (patch->width - patch->leftoffset) << FRACBITS;
  //  else
  //    tx -= patch->leftoffset << FRACBITS;

  //  x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;
  //  tx += patch->width << FRACBITS;
  //  x2 = ((centerxfrac + FixedMul(tx, xscale) - FRACUNIT / 2) >> FRACBITS);

  //  // off the side?
  //  if (x1 > viewwidth || x2 < 0)
  //    goto unlock_patch;
  //}

  // killough 3/27/98: exclude things totally separated
  // from the viewer, by either water or fake ceilings
  // killough 4/11/98: improve sprite clipping for underwater/fake ceilings

  //heightsec = thing->subsector->sector->heightsec;
  //if (heightsec != -1)   // only clip things which are in special sectors
  //{
  //  int phs = viewplayer->mo->subsector->sector->heightsec;
  //  fixed_t gzt = fz + (patch->topoffset << FRACBITS);
  //  if (phs != -1 && viewz < sectors[phs].floorheight ?
  //      fz >= sectors[heightsec].floorheight :
  //      gzt < sectors[heightsec].floorheight)
  //    goto unlock_patch;
  //  if (phs != -1 && viewz > sectors[phs].ceilingheight ?
  //      gzt < sectors[heightsec].ceilingheight && viewz >= sectors[heightsec].ceilingheight :
  //      fz >= sectors[heightsec].ceilingheight)
  //    goto unlock_patch;
  //}

  //e6y FIXME!!!
  if (thing == players[displayplayer].mo && walkcamera.type != 2)
    goto unlock_patch;

  sprite.x = -(float)fx / MAP_SCALE;
  sprite.y =  (float)fz / MAP_SCALE;
  sprite.z =  (float)fy / MAP_SCALE;

  // Bring items up out of floor by configurable amount times .01 Mead 8/13/03
  sprite.y += gl_sprite_offset;

  sprite.x2 = (float)patch->leftoffset / MAP_COEFF;
  sprite.x1 = sprite.x2 - ((float)patch->width / MAP_COEFF);
  sprite.y1 = (float)patch->topoffset / MAP_COEFF;
  sprite.y2 = sprite.y1 - ((float)patch->height / MAP_COEFF);

  // e6y
  // if the sprite is below the floor, and it's not a hanger/floater/missile, 
  // and it's not a fully dead corpse, move it up
  if ((gl_spriteclip != spriteclip_const) &&
      (sprite.y2 < 0) && (sprite.y2 >= (float)(-gl_spriteclip_threshold_f)) &&
      !(thing->flags & (MF_SPAWNCEILING | MF_FLOAT | MF_MISSILE | MF_NOGRAVITY)) &&
      ((gl_spriteclip == spriteclip_always) || !((thing->flags & MF_CORPSE) && thing->tics == -1)))
  {
    sprite.y1 -= sprite.y2;
    sprite.y2 = 0.0f;
  }

  //if (frustum_culling)
  //{
  //  if (!gld_SphereInFrustum(
  //    sprite.x + cos_inv_yaw * (sprite.x1 + sprite.x2) / 2.0f,
  //    sprite.y + (sprite.y1 + sprite.y2) / 2.0f,
  //    sprite.z - sin_inv_yaw * (sprite.x1 + sprite.x2) / 2.0f,
  //    //1.5 == sqrt(2) + small delta for MF_FOREGROUND
  //    (float)(MAX(patch->width, patch->height)) / MAP_COEFF / 2.0f * 1.5f))
  //  {
  //    goto unlock_patch;
  //  }
  //}

  sprite.scale = FixedDiv(projectiony, tz);
  if ((thing->frame & FF_FULLBRIGHT) || show_alive)
  {
    //sprite.fogdensity = 0.0f;
    sprite.light = 1.0f;
  }
  else
  {
    sprite.light = CalcLightLevel(lightlevel + (extralight << 5));
    //sprite.fogdensity = CalcFogDensity(thing->subsector->sector, lightlevel, GLDIT_SPRITE);
  }
  sprite.cm = CR_LIMIT + (int)((thing->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
  // [FG] colored blood and gibs
  if (thing->flags & MF_COLOREDBLOOD)
  {
    sprite.cm = thing->bloodcolor;
  }
  sprite.td = RT_Texture_GetFromPatchLump(lump); // sprite.cm
  if (!sprite.td)
    goto unlock_patch;
  sprite.flags = thing->flags;

  //if (thing->flags & MF_FOREGROUND)
  //  scene_has_overlapped_sprites = true;

  //sprite.index = gl_spriteindex++;
  sprite.xy = thing->x + (thing->y >> 16);
  sprite.fx = thing->x;
  sprite.fy = thing->y;

  sprite.vt = 0.0f;
  sprite.vb = 1.0f;
  if (flip)
  {
    sprite.ul = 0.0f;
    sprite.ur = 1.0f;
  }
  else
  {
    sprite.ul = 1.0f;
    sprite.ur = 0.0f;
  }

  DrawSprite(thing, &sprite, sectornum);

unlock_patch:
  R_UnlockPatchNum(lump);
}


static void Vec3_Normalize(float *v_0, float *v_1, float *v_2)
{
  float l = sqrtf((*v_0) * (*v_0) + (*v_1) * (*v_1) + (*v_2) * (*v_2));
  *v_0 /= l;
  *v_1 /= l;
  *v_2 /= l;
}


void RT_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel)
{
  dboolean is_partial_invisibility = false;

  const rt_texture_t *td = RT_Texture_GetFromPatchLump(firstspritelump + weaponlump);
  if (!td)
    return;

  float fU1 = 0;
  float fV1 = 0;
  float fU2 = 1;
  float fV2 = 1;

  int ix1 = viewwindowx + vis->x1;
  int ix2 = ix1 + (int)((float)td->width * pspritexscale_f);
  int iy1 = viewwindowy + centery - (int)(((float)vis->texturemid / (float)FRACUNIT) * pspriteyscale_f);
  int iy2 = iy1 + (int)((float)td->height * pspriteyscale_f) + 1;

  float x1 = (float)ix1 / SCREENWIDTH;
  float x2 = (float)ix2 / SCREENWIDTH;
  float y1 = (float)iy1 / SCREENHEIGHT;
  float y2 = (float)iy2 / SCREENHEIGHT;

  x1 = x1 * 2 - 1;
  x2 = x2 * 2 - 1;
  y1 = y1 * 2 - 1;
  y2 = y2 * 2 - 1;

  // TODO RT: why multiplication by 2 is needed for weapons on X axis
  x1 *= 2;
  x2 *= 2;

  y1 *= -1;
  y2 *= -1;

  if (/*(viewplayer->mo->flags & MF_SHADOW) && */!vis->colormap)
  {
    is_partial_invisibility = true;
  }
  else
  {
    // dboolean is_translucent = viewplayer->mo->flags & MF_TRANSLUCENT;
  }

  RgFloat3D v_0 = { x1, y1, 0 };
  RgFloat3D v_1 = { x1, y2, 0 };
  RgFloat3D v_2 = { x2, y1, 0 };
  RgFloat3D v_3 = { x2, y2, 0 };

  RgFloat2D t_0 = { fU1, fV1 };
  RgFloat2D t_1 = { fU1, fV2 };
  RgFloat2D t_2 = { fU2, fV1 };
  RgFloat2D t_3 = { fU2, fV2 };

  RgFloat3D positions[6] = 
  {
    v_0, v_1, v_2,
    v_1, v_3, v_2
  };
  RgFloat2D texcoords[6] =
  {
    t_0, t_1, t_2,
    t_1, t_3, t_2
  };

  
#define cam_pos (rtmain.mat_view_inverse[3])
#define cam_rot (rtmain.mat_view_inverse) // column-major memory layout

#define cam_dir (rtmain.mat_view_inverse[2])

  static float z = -0.1f;
  static float s = 0.07f;

  float weaponPosition[3] = { cam_pos[0] + cam_dir[0] * z, cam_pos[1] + cam_dir[1] * z, cam_pos[2] + cam_dir[2] * z };

  RgTransform transform = 
  {
    cam_rot[0][0] * s, cam_rot[1][0] * s, cam_rot[2][0] * s,  weaponPosition[0],
    cam_rot[0][1] * s, cam_rot[1][1] * s, cam_rot[2][1] * s,  weaponPosition[1],
    cam_rot[0][2] * s, cam_rot[1][2] * s, cam_rot[2][2] * s,  weaponPosition[2],
  };


  RgGeometryUploadInfo info =
  {
    .uniqueID = RT_GetUniqueID_FirstPersonWeapon(weaponlump),
    .flags = is_partial_invisibility ? RG_GEOMETRY_UPLOAD_REFL_REFR_ALBEDO_MULTIPLY_BIT : 0,
    .geomType = RG_GEOMETRY_TYPE_DYNAMIC,
    .passThroughType = is_partial_invisibility ? RG_GEOMETRY_PASS_THROUGH_TYPE_WATER_REFLECT_REFRACT : RG_GEOMETRY_PASS_THROUGH_TYPE_ALPHA_TESTED,
    .visibilityType = RG_GEOMETRY_VISIBILITY_TYPE_FIRST_PERSON,
    .vertexCount = 6,
    .pVertexData = positions,
    .pNormalData = normals,
    .pTexCoordLayerData = { texcoords },
    // get camera's sector
    .sectorID = RT_GetSectorNum_Fixed(viewx, viewy),
    .layerColors = { RG_COLOR_WHITE },
    .defaultRoughness = 0.5f,
    .defaultMetallicity = 0.1f,
    .defaultEmission = 0.0f,
    .geomMaterial = { td->rg_handle },
    .transform = transform
  };

  RgResult r = rgUploadGeometry(rtmain.instance, &info);
  RG_CHECK(r);
}
