#include "rt_main.h"

#include <math.h>

#include "doomstat.h"
#include "e6y.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_sky.h"
#include "r_state.h"


// gld_CalcLightLevel_shaders
static float CalcLightLevel(int lightlevel)
{
  // muzzle flash
  //lightlevel += extralight << 5;

  int light = BETWEEN(0, 255, lightlevel);
  return (float)light / 255.0f;
}



// ----------- //
//    FLAT     //
// ----------- //



static void AddFlat(const int sectornum, dboolean ceiling, const visplane_t *plane)
{
  struct
  {
    float light; // the lightlevel of the flat
    float uoffs, voffs; // the texture coordinates
    float z; // the z position of the flat (height)
    const rt_texture_t *td;
  } flat = { 0 };

  sector_t tempsec; // needed for R_FakeFlat
  int floorlightlevel;      // killough 3/16/98: set floor lightlevel
  int ceilinglightlevel;    // killough 4/11/98

  if (sectornum < 0)
    return;
  sector_t *sector = &sectors[sectornum]; // get the sector
  // sector = R_FakeFlat(sector, &tempsec, &floorlightlevel, &ceilinglightlevel, false); // for boom effects

  if (!ceiling) // if it is a floor ...
  {
    if (sector->floorpic == skyflatnum) // don't draw if sky
      return;
    // get the texture. flattranslation is maintained by doom and
    // contains the number of the current animation frame
    flat.td = RT_Texture_GetFromFlatLump(flattranslation[plane->picnum]);
    if (!flat.td)
      return;
    // get the lightlevel from floorlightlevel
    flat.light = CalcLightLevel(plane->lightlevel);
    // flat.fogdensity = gld_CalcFogDensity(sector, plane->lightlevel, GLDIT_FLOOR);
    // calculate texture offsets
    if (sector->floor_xoffs | sector->floor_yoffs)
    {
      // flat.flags |= GLFLAT_HAVE_OFFSET;
      flat.uoffs = (float)sector->floor_xoffs / (float)(FRACUNIT * 64);
      flat.voffs = (float)sector->floor_yoffs / (float)(FRACUNIT * 64);
    }
    else
    {
      flat.uoffs = 0.0f;
      flat.voffs = 0.0f;
    }
  }
  else // if it is a ceiling ...
  {
    if (sector->ceilingpic == skyflatnum) // don't draw if sky
      return;
    // get the texture. flattranslation is maintained by doom and
    // contains the number of the current animation frame
    flat.td = RT_Texture_GetFromFlatLump(flattranslation[plane->picnum]);
    if (!flat.td)
      return;
    // get the lightlevel from ceilinglightlevel
    flat.light = CalcLightLevel(plane->lightlevel);
    // flat.fogdensity = gld_CalcFogDensity(sector, plane->lightlevel, GLDIT_CEILING);
    // calculate texture offsets
    if (sector->ceiling_xoffs | sector->ceiling_yoffs)
    {
      //flat.flags |= GLFLAT_HAVE_OFFSET;
      flat.uoffs = (float)sector->ceiling_xoffs / (float)(FRACUNIT * 64);
      flat.voffs = (float)sector->ceiling_yoffs / (float)(FRACUNIT * 64);
    }
    else
    {
      flat.uoffs = 0.0f;
      flat.voffs = 0.0f;
    }
  }

  // get height from plane
  flat.z = (float)plane->height / MAP_SCALE;


  // ---


  rtsectordata_t sector_geometry = RT_GetSectorGeometryData(sectornum, ceiling);

  RgGeometryUploadInfo info =
  {
    .uniqueID = RT_GetUniqueID_Flat(sectornum, ceiling),
    .flags = 0,
    .geomType = RG_GEOMETRY_TYPE_DYNAMIC,
    .passThroughType = RG_GEOMETRY_PASS_THROUGH_TYPE_OPAQUE,
    .visibilityType = RG_GEOMETRY_VISIBILITY_TYPE_WORLD_0,
    .vertexCount = sector_geometry.vertex_count,
    .pVertexData = sector_geometry.positions,
    .pNormalData = sector_geometry.normals,
    .pTexCoordLayerData = { sector_geometry.texcoords },
    .indexCount = sector_geometry.index_count,
    .pIndexData = sector_geometry.indices,
    .sectorID = sectornum,
    .layerColors = { RG_COLOR_WHITE },
    .defaultRoughness = 0.5f,
    .defaultMetallicity = 0.2f,
    .defaultEmission = RT_TEXTURE_EMISSION(flat.td),
    .geomMaterial = { flat.td->rg_handle },
    .transform = 
      {
        1,0,0,0,
        0,1,0,flat.z,
        0,0,1,0
      }
  };

  if (flat.td->flags & RT_TEXTURE_FLAG_IS_WATER_BIT)
  {
    // const rt_texture_t *bottom_td = RT_Texture_GetFromFlatLump(R_FlatNumForName("MFLR8_3"));
    info.flags = RG_GEOMETRY_UPLOAD_REFL_REFR_ALBEDO_MULTIPLY_BIT;
    info.passThroughType = RG_GEOMETRY_PASS_THROUGH_TYPE_MIRROR;
  }

  RgResult r = rgUploadGeometry(rtmain.instance, &info);
  RG_CHECK(r);
  // TODO RT: flat with texcoord offset


  if (ceiling && flat.light > 0.8f)
  {
    RgFloat3D center = { 0 };

    for (int j = 0; j < sector_geometry.vertex_count; j++)
    {
      float *v = sector_geometry.positions[j].data;
      center.data[0] += v[0];
      center.data[1] += v[1];
      center.data[2] += v[2];
    }
    center.data[0] /= (float)sector_geometry.vertex_count;
    center.data[1] /= (float)sector_geometry.vertex_count;
    center.data[2] /= (float)sector_geometry.vertex_count;

    center.data[1] += flat.z - 0.2f;

    RgSphericalLightUploadInfo light_info =
    {
      .uniqueID = RT_GetUniqueID_Flat(sectornum, ceiling),
      .color = {flat.light,flat.light,flat.light},
      .position = center,
      .sectorID = sectornum,
      .radius = 0.1f,
      .falloffDistance = 4
    };

    // RgResult r = rgUploadSphericalLight(rtmain.instance, &light_info);
    RG_CHECK(r);
  }
}


void RT_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling)
{
  if (subsectornum < 0 || subsectornum >= numsubsectors)
  {
    assert(0);
    return;
  }


  subsector_t *subsector = &subsectors[subsectornum];
  if (subsector == NULL)
  {
    return;
  }

  if (floor != NULL)
  {
    AddFlat(subsector->sector->iSectorID, false, floor);
  }

  if (ceiling != NULL)
  {
    AddFlat(subsector->sector->iSectorID, true, ceiling);
  }
}



// ----------- //
//    WALL     //
// ----------- //



#define MAXCOORD (32767.0f / MAP_COEFF)
#define SMALLDELTA 0.001f


#define GLDWF_TOP 1
#define GLDWF_M1S 2
#define GLDWF_M2S 3
#define GLDWF_BOT 4
#define GLDWF_TOPFLUD 5 //e6y: project the ceiling plane into the gap
#define GLDWF_BOTFLUD 6 //e6y: project the floor plane into the gap
#define GLDWF_SKY 7
#define GLDWF_SKYFLIP 8

#define SKY_NONE    0
#define SKY_CEILING 1
#define SKY_FLOOR   2


typedef struct
{
  int lineID;
  dboolean double_sided;
  dboolean is_back_side;
  float ytop, ybottom;
  float ul, ur, vt, vb;
  float light;
  //float fogdensity;
  float alpha;
  float skyymid;
  float skyyaw;
  const rt_texture_t *rttexture;
  byte flag;
  seg_t *seg;
  int sectornum;
  int subsectornum;
} RTPWall;


typedef enum
{
  RTP_WALLTYPE_WALL,    // opaque wall
  RTP_WALLTYPE_MWALL,   // opaque mid wall
  RTP_WALLTYPE_FWALL,   // projected wall
  RTP_WALLTYPE_TWALL,   // transparent walls
  RTP_WALLTYPE_SWALL,   // sky walls

  RTP_WALLTYPE_AWALL,   // animated wall
  RTP_WALLTYPE_FAWALL,  // animated projected wall
} RTPWallType;


static dboolean IsFacingCamera(dboolean invert_normal, const float *p0, const float *p1, const float *p2)
{
  const float *camera_pos = rtmain.mat_view_inverse[3];

  const float e1[] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
  const float e2[] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };

  float normal[] = {
    e1[1] * e2[2] - e1[2] * e2[1],
    e1[2] * e2[0] - e1[0] * e2[2],
    e1[0] * e2[1] - e1[1] * e2[0]
  };

  float len = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
  if (len < 0.001f)
  {
    return true;
  }
  if (invert_normal)
  {
    len *= -1;
  }
  normal[0] /= len; normal[1] /= len; normal[2] /= len;

  float d = normal[0] * p0[0] + normal[1] * p0[1] + normal[2] * p0[2];

  // substite camera_pos to plane equation
  return
    camera_pos[0] * normal[0] +
    camera_pos[1] * normal[1] +
    camera_pos[2] * normal[2] - d >= 0.0f;
}


static void DrawWall(RTPWallType itemtype, int drawwallindex, RTPWall *wall)
{
  rendered_segs++;

  // Do not repeat middle texture vertically
  // to avoid visual glitches for textures with holes

  /*unsigned int flags;
  if ((wall->flag == GLDWF_M2S) && (wall->flag < GLDWF_SKY))
  {
    flags = GLTEXTURE_CLAMPY;
  }
  else
  {
    flags = 0;
  }*/

  // not implemented
  assert(!(wall->flag == GLDWF_TOPFLUD) && !(wall->flag == GLDWF_BOTFLUD));


  // StaticLightAlpha(wall->light, wall->alpha);

  RgFloat4D color = RG_COLOR_WHITE;
  if (!wall->rttexture)
  {
    color.data[0] = 1.0f;
    color.data[1] = 0.0f;
    color.data[2] = 0.0f;
  }

  float x1, z1;
  float x2, z2;
  RT_GetLineInfo(wall->lineID, &x1, &z1, &x2, &z2);


  // lower left corner
  RgFloat2D texcoord_0 = { wall->ul, wall->vb };
  RgFloat3D position_0 = { x1, wall->ybottom, z1 };

  // split left edge of wall
  //if (!wall->glseg->fracleft)
  //  gld_SplitLeftEdge(wall, false);

  // upper left corner
  RgFloat2D texcoord_1 = { wall->ul, wall->vt };
  RgFloat3D position_1 = { x1, wall->ytop, z1 };

  // upper right corner
  RgFloat2D texcoord_2 = { wall->ur, wall->vt };
  RgFloat3D position_2 = { x2, wall->ytop, z2 };

  // split right edge of wall
  //if (!wall->glseg->fracright)
  //  gld_SplitRightEdge(wall, false);

  // lower right corner
  RgFloat2D texcoord_3 = { wall->ur, wall->vb };
  RgFloat3D position_3 = { x2, wall->ybottom, z2 };


  // RT: 2 triangle fans, but in reverse order (counter clockwise)
  const RgFloat3D positions[] =
  {
    position_0, position_2, position_1,
    position_0, position_3, position_2,
  };
  const RgFloat2D texcoords[] =
  {
    texcoord_0, texcoord_2, texcoord_1,
    texcoord_0, texcoord_3, texcoord_2,
  };


  dboolean alpha_tested = wall->rttexture && (wall->rttexture->flags & RT_TEXTURE_FLAG_WITH_ALPHA_BIT);
  dboolean invert_normal = wall->is_back_side;

  if (alpha_tested && wall->double_sided)
  {
    // RT: don't upload alpha-tested walls from both sides
    if (wall->is_back_side)
    {
      return;
    }

    invert_normal = !IsFacingCamera(invert_normal, positions[0].data, positions[1].data, positions[2].data);
  }


  RgGeometryUploadInfo info =
  {
    .uniqueID = RT_GetUniqueID_Wall(wall->lineID, wall->subsectornum, drawwallindex),
    .flags = invert_normal ? RG_GEOMETRY_UPLOAD_GENERATE_INVERTED_NORMALS_BIT : 0,
    .geomType = RG_GEOMETRY_TYPE_DYNAMIC,
    .passThroughType = alpha_tested ? RG_GEOMETRY_PASS_THROUGH_TYPE_ALPHA_TESTED : RG_GEOMETRY_PASS_THROUGH_TYPE_OPAQUE,
    .visibilityType = itemtype == RTP_WALLTYPE_SWALL ? RG_GEOMETRY_VISIBILITY_TYPE_SKY : RG_GEOMETRY_VISIBILITY_TYPE_WORLD_0,
    .vertexCount = RG_ARRAY_SIZE(positions),
    .pVertexData = positions,
    .pNormalData = NULL,
    .pTexCoordLayerData = { texcoords },
    .sectorID = wall->sectornum,
    .layerColors = { color },
    .defaultRoughness = 0.5f,
    .defaultMetallicity = 0.2f,
    .defaultEmission = RT_TEXTURE_EMISSION(wall->rttexture),
    .geomMaterial = { wall->rttexture ? wall->rttexture->rg_handle : RG_NO_MATERIAL },
    .transform = RG_TRANSFORM_IDENTITY
  };

  RgResult r = rgUploadGeometry(rtmain.instance, &info);
  RG_CHECK(r);
}


static int rt_drawwallindex = 0;


static void AddDrawWallItem(RTPWallType itemtype, RTPWall *wall)
{
  // RT: force gl_blend_animations=false

  DrawWall(itemtype, rt_drawwallindex, wall);
  rt_drawwallindex++;
}


static void AddSkyTexture(RTPWall *wall, int sky1, int sky2, int skytype)
{
  const dboolean rt_mlook_or_fov = true;

  side_t *s = NULL;
  line_t *l = NULL;
  wall->rttexture = NULL;

  if ((sky1)&PL_SKYFLAT)
  {
    l = &lines[sky1 & ~PL_SKYFLAT];
  }
  else
  {
    if ((sky2)&PL_SKYFLAT)
    {
      l = &lines[sky2 & ~PL_SKYFLAT];
    }
  }

  if (l)
  {
    s = *l->sidenum + sides;
    //rt_skybox_params.side = s;
    wall->rttexture = RT_Texture_GetFromTexture(texturetranslation[s->toptexture]
    /*, false, texturetranslation[s->toptexture] == skytexture || l->special == 271 || l->special == 272*/);
    if (wall->rttexture)
    {
      if (!rt_mlook_or_fov)
      {
        wall->skyyaw = -2.0f * ((-(float)((viewangle + s->textureoffset) >> ANGLETOFINESHIFT) * 360.0f / FINEANGLES) / 90.0f);
        wall->skyymid = 200.0f / 319.5f * (((float)s->rowoffset / (float)FRACUNIT - 28.0f) / 100.0f);
      }
      else
      {
        wall->skyyaw = -2.0f * (((270.0f - (float)((viewangle + s->textureoffset) >> ANGLETOFINESHIFT) * 360.0f / FINEANGLES) + 90.0f) / 90.0f / skyscale);
        wall->skyymid = skyYShift + (((float)s->rowoffset / (float)FRACUNIT + 28.0f) / wall->rttexture->height) / skyscale;
      }
      wall->flag = (l->special == 272 ? GLDWF_SKY : GLDWF_SKYFLIP);
    }
  }
  else
  {
    wall->rttexture = RT_Texture_GetFromTexture(skytexture);
    if (wall->rttexture)
    {
      wall->skyyaw = skyXShift;
      wall->skyymid = skyYShift;
      wall->flag = GLDWF_SKY;
    }
  }

  if (wall->rttexture)
  {
    //rtmain.sky.type |= skytype;
    //wall->rttexture->flags |= GLTEXTURE_SKY;

    AddDrawWallItem(RTP_WALLTYPE_SWALL, wall);

    if (rtmain.sky.texture == NULL)
    {
      rtmain.sky.texture = wall->rttexture;

      // RT: force gl_drawskys=skytype_skydome

      if (s)
      {
        rtmain.sky.x_offset = (float)s->textureoffset * 180.0f / (float)ANG180;
        rtmain.sky.y_offset = (float)s->rowoffset / (float)FRACUNIT;
        rtmain.sky.gldwf_skyflip = wall->flag == GLDWF_SKYFLIP;
      }
    }
  }
}


#define LINE seg->linedef
#define CALC_Y_VALUES(w, lineheight, floor_height, ceiling_height)\
  (w).ytop=((float)(ceiling_height)/(float)MAP_SCALE)+SMALLDELTA;\
  (w).ybottom=((float)(floor_height)/(float)MAP_SCALE)-SMALLDELTA;\
  (lineheight)=((float)fabs(((ceiling_height)/(float)FRACUNIT)-((floor_height)/(float)FRACUNIT)))

#define OU(w,seg) (((float)((seg)->sidedef->textureoffset)/(float)FRACUNIT)/(float)(w).rttexture->width)
#define OV(w,seg) (((float)((seg)->sidedef->rowoffset)/(float)FRACUNIT)/(float)(w).rttexture->height)
#define OV_PEG(w,seg,v_offset) (OV((w),(seg))-(((float)(v_offset)/(float)FRACUNIT)/(float)(w).rttexture->height))
#define URUL(w, seg, backseg, linelength)\
  if (backseg){\
    (w).ur=OU((w),(seg));\
    (w).ul=(w).ur+((linelength)/(float)(w).rttexture->width);\
  }else{\
    (w).ul=OU((w),(seg));\
    (w).ur=(w).ul+((linelength)/(float)(w).rttexture->width);\
  }

#define CALC_TEX_VALUES_TOP(w, seg, backseg, peg, linelength, lineheight)\
  (w).flag=GLDWF_TOP;\
  URUL(w, seg, backseg, linelength);\
  if (peg){\
    (w).vb=OV((w),(seg))+(/*(w).rttexture->scaleyfac*/ 1.0f);\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).rttexture->height));\
  }else{\
    (w).vt=OV((w),(seg));\
    (w).vb=(w).vt+((float)(lineheight)/(float)(w).rttexture->height);\
  }

#define CALC_TEX_VALUES_MIDDLE1S(w, seg, backseg, peg, linelength, lineheight)\
  (w).flag=GLDWF_M1S;\
  URUL(w, seg, backseg, linelength);\
  if (peg){\
    (w).vb=OV((w),(seg))+(/*(w).rttexture->scaleyfac*/ 1.0f);\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).rttexture->height));\
  }else{\
    (w).vt=OV((w),(seg));\
    (w).vb=(w).vt+((float)(lineheight)/(float)(w).rttexture->height);\
  }

#define CALC_TEX_VALUES_BOTTOM(w, seg, backseg, peg, linelength, lineheight, v_offset)\
  (w).flag=GLDWF_BOT;\
  URUL(w, seg, backseg, linelength);\
  if (peg){\
    (w).vb=OV_PEG((w),(seg),(v_offset))+(/*(w).rttexture->scaleyfac*/ 1.0f);\
    (w).vt=((w).vb-((float)(lineheight)/(float)(w).rttexture->height));\
  }else{\
    (w).vt=OV((w),(seg));\
    (w).vb=(w).vt+((float)(lineheight)/(float)(w).rttexture->height);\
  }


static sector_t *FakeFlat(sector_t *sec, sector_t *tempsec,
                            int *floorlightlevel, int *ceilinglightlevel,
                            dboolean back)
{
  return sec;
}


void RT_AddWall(int subsectornum, seg_t *seg)
{
  if (subsectornum < 0 || subsectornum >= numsubsectors)
  {
    assert(0);
    return;
  }


  rt_drawwallindex = 0;


  const float tran_filter_pct = 66;
  // const float zCamera = (float)viewz / MAP_SCALE;

  RTPWall wall = { 0 };

  const rt_texture_t *temptex;
  sector_t *frontsector;
  sector_t *backsector;
  sector_t ftempsec; // needed for R_FakeFlat
  sector_t btempsec; // needed for R_FakeFlat
  float lineheight, linelength;
  int rellight = 0;
  int backseg;

  int side = (seg->sidedef == &sides[seg->linedef->sidenum[0]] ? 0 : 1);
  //if (linerendered[side][seg->linedef->iLineID] == rendermarker)
  //  return;
  //linerendered[side][seg->linedef->iLineID] = rendermarker;
  linelength = lines[seg->linedef->iLineID].texel_length;
  wall.lineID = seg->linedef->iLineID;
  wall.subsectornum = subsectornum;
  wall.sectornum = subsectors[subsectornum].sector->iSectorID;
  backseg = seg->sidedef != &sides[seg->linedef->sidenum[0]];
  wall.is_back_side = backseg;

  if (!seg->frontsector)
    return;
  frontsector = FakeFlat(seg->frontsector, &ftempsec, NULL, NULL, false); // for boom effects
  if (!frontsector)
    return;

  // e6y: fake contrast stuff
  // Original doom added/removed one light level ((1<<LIGHTSEGSHIFT) == 16) 
  // for walls exactly vertical/horizontal on the map
  if (fake_contrast)
  {
    //rellight = seg->linedef->dx == 0 ? +gl_rellight : seg->linedef->dy == 0 ? -gl_rellight : 0;
  }
  wall.light = CalcLightLevel(frontsector->lightlevel + rellight);
  //wall.fogdensity = CalcFogDensity(frontsector,
  //                                 frontsector->lightlevel + (gl_lightmode == gl_lightmode_fogbased ? rellight : 0),
  //                                 RTP_WALLTYPE_WALL);
  wall.alpha = 1.0f;
  wall.rttexture = NULL;
  wall.seg = seg; //e6y
  wall.double_sided = seg->backsector != NULL;

  if (!wall.double_sided) /* onesided */
  {
    if (frontsector->ceilingpic == skyflatnum)
    {
      wall.ytop = MAXCOORD;
      wall.ybottom = (float)frontsector->ceilingheight / MAP_SCALE;
      AddSkyTexture(&wall, frontsector->sky, frontsector->sky, SKY_CEILING);
    }
    if (frontsector->floorpic == skyflatnum)
    {
      wall.ytop = (float)frontsector->floorheight / MAP_SCALE;
      wall.ybottom = -MAXCOORD;
      AddSkyTexture(&wall, frontsector->sky, frontsector->sky, SKY_FLOOR);
    }
    temptex = RT_Texture_GetFromTexture(texturetranslation[seg->sidedef->midtexture]);
    if (temptex && frontsector->ceilingheight > frontsector->floorheight)
    {
      wall.rttexture = temptex;
      CALC_Y_VALUES(wall, lineheight, frontsector->floorheight, frontsector->ceilingheight);
      CALC_TEX_VALUES_MIDDLE1S(
        wall, seg, backseg, (LINE->flags & ML_DONTPEGBOTTOM) > 0,
        linelength, lineheight
      );
      AddDrawWallItem(RTP_WALLTYPE_WALL, &wall);
    }
  }
  else /* twosided */
  {
    sector_t *fs, *bs;
    int toptexture, midtexture, bottomtexture;
    fixed_t floor_height, ceiling_height;
    fixed_t max_floor, min_floor;
    fixed_t max_ceiling, min_ceiling;
    //fixed_t max_floor_tex, min_ceiling_tex;

    backsector = FakeFlat(seg->backsector, &btempsec, NULL, NULL, true); // for boom effects
    if (!backsector)
      return;

    if (frontsector->floorheight > backsector->floorheight)
    {
      max_floor = frontsector->floorheight;
      min_floor = backsector->floorheight;
    }
    else
    {
      max_floor = backsector->floorheight;
      min_floor = frontsector->floorheight;
    }

    if (frontsector->ceilingheight > backsector->ceilingheight)
    {
      max_ceiling = frontsector->ceilingheight;
      min_ceiling = backsector->ceilingheight;
    }
    else
    {
      max_ceiling = backsector->ceilingheight;
      min_ceiling = frontsector->ceilingheight;
    }

    //max_floor_tex = max_floor + seg->sidedef->rowoffset;
    //min_ceiling_tex = min_ceiling + seg->sidedef->rowoffset;

    if (backseg)
    {
      fs = backsector;
      bs = frontsector;
    }
    else
    {
      fs = frontsector;
      bs = backsector;
    }

    toptexture = texturetranslation[seg->sidedef->toptexture];
    midtexture = texturetranslation[seg->sidedef->midtexture];
    bottomtexture = texturetranslation[seg->sidedef->bottomtexture];

    /* toptexture */
    ceiling_height = frontsector->ceilingheight;
    floor_height = backsector->ceilingheight;
    if (frontsector->ceilingpic == skyflatnum)// || backsector->ceilingpic==skyflatnum)
    {
      wall.ytop = MAXCOORD;
      if (
        // e6y
        // There is no more HOM in the starting area on Memento Mori map29 and on map30.
        // Old code:
        // (backsector->ceilingheight==backsector->floorheight) &&
        // (backsector->ceilingpic==skyflatnum)
        (backsector->ceilingpic == skyflatnum) &&
        (backsector->ceilingheight <= backsector->floorheight)
        )
      {
        // e6y
        // There is no more visual glitches with sky on Icarus map14 sector 187
        // Old code: wall.ybottom=(float)backsector->floorheight/MAP_SCALE;
        wall.ybottom = ((float)(backsector->floorheight +
                        (seg->sidedef->rowoffset > 0 ? seg->sidedef->rowoffset : 0))) / MAP_SCALE;
        AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_CEILING);
      }
      else
      {
        if (bs->ceilingpic == skyflatnum && fs->ceilingpic != skyflatnum &&
            toptexture == NO_TEXTURE && midtexture == NO_TEXTURE)
        {
          wall.ybottom = (float)min_ceiling / MAP_SCALE;
          AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_CEILING);
        }
        else
        {
          if (((backsector->ceilingpic != skyflatnum && toptexture != NO_TEXTURE) && midtexture == NO_TEXTURE) ||
              backsector->ceilingpic != skyflatnum ||
              backsector->ceilingheight <= frontsector->floorheight)
          {
            wall.ybottom = (float)max_ceiling / MAP_SCALE;
            AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_CEILING);
          }
        }
      }
    }
    if (floor_height < ceiling_height)
    {
      if (!((frontsector->ceilingpic == skyflatnum) && (backsector->ceilingpic == skyflatnum)))
      {
        temptex = RT_Texture_GetFromTexture(toptexture);
      #if 0
        if (!temptex && /*gl_use_stencil &&*/ backsector &&
            !(seg->linedef->r_flags & RF_ISOLATED) &&
            /*frontsector->ceilingpic != skyflatnum && */backsector->ceilingpic != skyflatnum &&
            !(backsector->flags & NULL_SECTOR))
        {
          wall.ytop = ((float)(ceiling_height) / (float)MAP_SCALE) + SMALLDELTA;
          wall.ybottom = ((float)(floor_height) / (float)MAP_SCALE) - SMALLDELTA;
          if (wall.ybottom >= zCamera)
          {
            wall.flag = GLDWF_TOPFLUD;
            temptex = RT_Texture_GetFromFlatLump(flattranslation[seg->backsector->ceilingpic]);
            if (temptex)
            {
              wall.rttexture = temptex;
              gld_AddDrawWallItem(RTP_WALLTYPE_FWALL, &wall);
            }
          }
        }
        else
      #endif
        {
          if (temptex)
          {
            wall.rttexture = temptex;
            CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
            CALC_TEX_VALUES_TOP(
              wall, seg, backseg, (LINE->flags & (/*e6y ML_DONTPEGBOTTOM | */ML_DONTPEGTOP)) == 0,
              linelength, lineheight
            );
            AddDrawWallItem(RTP_WALLTYPE_WALL, &wall);
          }
        }
      }
    }

    /* midtexture */
    //e6y
    if (comp[comp_maskedanim])
      temptex = RT_Texture_GetFromTexture(seg->sidedef->midtexture);
    else

    // e6y
    // Animated middle textures with a zero index should be forced
    // See spacelab.wad (http://www.doomworld.com/idgames/index.php?id=6826)
    temptex = RT_Texture_GetFromTexture(midtexture);
    if (temptex && seg->sidedef->midtexture != NO_TEXTURE && backsector->ceilingheight > frontsector->floorheight)
    {
      int top, bottom;
      wall.rttexture = temptex;

      if ((LINE->flags & ML_DONTPEGBOTTOM) > 0)
      {
        //floor_height=max_floor_tex;
        floor_height = MAX(seg->frontsector->floorheight, seg->backsector->floorheight) + (seg->sidedef->rowoffset);
        ceiling_height = floor_height + (int)(wall.rttexture->height << FRACBITS);
      }
      else
      {
        //ceiling_height=min_ceiling_tex;
        ceiling_height = MIN(seg->frontsector->ceilingheight, seg->backsector->ceilingheight) + (seg->sidedef->rowoffset);
        floor_height = ceiling_height - (int)(wall.rttexture->height << FRACBITS);
      }

      // Depending on missing textures and possible plane intersections
      // decide which planes to use for the polygon
      if (seg->frontsector != seg->backsector ||
          seg->frontsector->heightsec != -1)
      {
        sector_t *f, *b;

        f = (seg->frontsector->heightsec == -1 ? seg->frontsector : &ftempsec);
        b = (seg->backsector->heightsec == -1 ? seg->backsector : &btempsec);

        // Set up the top
        if (frontsector->ceilingpic != skyflatnum ||
            backsector->ceilingpic != skyflatnum)
        {
          if (toptexture == NO_TEXTURE)
            // texture is missing - use the higher plane
            top = MAX(f->ceilingheight, b->ceilingheight);
          else
            top = MIN(f->ceilingheight, b->ceilingheight);
        }
        else
          top = ceiling_height;

        // Set up the bottom
        if (frontsector->floorpic != skyflatnum ||
            backsector->floorpic != skyflatnum ||
            frontsector->floorheight != backsector->floorheight)
        {
          if (seg->sidedef->bottomtexture == NO_TEXTURE)
            // texture is missing - use the lower plane
            bottom = MIN(f->floorheight, b->floorheight);
          else
            // normal case - use the higher plane
            bottom = MAX(f->floorheight, b->floorheight);
        }
        else
        {
          bottom = floor_height;
        }

        //let's clip away some unnecessary parts of the polygon
        if (ceiling_height < top)
          top = ceiling_height;
        if (floor_height > bottom)
          bottom = floor_height;
      }
      else
      {
        // both sides of the line are in the same sector
        top = ceiling_height;
        bottom = floor_height;
      }

      if (top <= bottom)
        goto bottomtexture;

      wall.ytop = (float)top / (float)MAP_SCALE;
      wall.ybottom = (float)bottom / (float)MAP_SCALE;

      wall.flag = GLDWF_M2S;
      URUL(wall, seg, backseg, linelength);

      wall.vt = (float)((-top + ceiling_height) >> FRACBITS) / (float)wall.rttexture->height;
      wall.vb = (float)((-bottom + ceiling_height) >> FRACBITS) / (float)wall.rttexture->height;

      if (seg->linedef->tranlump >= 0 && general_translucency)
        wall.alpha = (float)tran_filter_pct / 100.0f;
      AddDrawWallItem((wall.alpha == 1.0f ? RTP_WALLTYPE_MWALL : RTP_WALLTYPE_TWALL), &wall);
      wall.alpha = 1.0f;
    }
  bottomtexture:
    /* bottomtexture */
    ceiling_height = backsector->floorheight;
    floor_height = frontsector->floorheight;
    if (frontsector->floorpic == skyflatnum)
    {
      wall.ybottom = -MAXCOORD;
      if (
        (backsector->ceilingheight == backsector->floorheight) &&
        (backsector->floorpic == skyflatnum)
        )
      {
        wall.ytop = (float)backsector->floorheight / MAP_SCALE;
        AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_FLOOR);
      }
      else
      {
        if (bottomtexture == NO_TEXTURE && midtexture == NO_TEXTURE)
        {
          wall.ytop = (float)max_floor / MAP_SCALE;
          AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_CEILING);
        }
        else
        {
          if ((bottomtexture != NO_TEXTURE && midtexture == NO_TEXTURE) ||
              backsector->floorpic != skyflatnum ||
              backsector->floorheight >= frontsector->ceilingheight)
          {
            wall.ytop = (float)min_floor / MAP_SCALE;
            AddSkyTexture(&wall, frontsector->sky, backsector->sky, SKY_FLOOR);
          }
        }
      }
    }
    if (floor_height < ceiling_height)
    {
      temptex = RT_Texture_GetFromTexture(bottomtexture);
    #if 0
      if (!temptex && /*gl_use_stencil &&*/ backsector &&
          !(seg->linedef->r_flags & RF_ISOLATED) &&
          /*frontsector->floorpic != skyflatnum && */backsector->floorpic != skyflatnum &&
          !(backsector->flags & NULL_SECTOR))
      {
        wall.ytop = ((float)(ceiling_height) / (float)MAP_SCALE) + SMALLDELTA;
        wall.ybottom = ((float)(floor_height) / (float)MAP_SCALE) - SMALLDELTA;
        if (wall.ytop <= zCamera)
        {
          wall.flag = GLDWF_BOTFLUD;
          temptex = RT_Texture_GetFromFlatLump(flattranslation[seg->backsector->floorpic]);
          if (temptex)
          {
            wall.rttexture = temptex;
            gld_AddDrawWallItem(RTP_WALLTYPE_FWALL, &wall);
          }
        }
      }
      else
    #endif
      {
        if (temptex)
        {
          wall.rttexture = temptex;
          CALC_Y_VALUES(wall, lineheight, floor_height, ceiling_height);
          CALC_TEX_VALUES_BOTTOM(
            wall, seg, backseg, (LINE->flags & ML_DONTPEGBOTTOM) > 0,
            linelength, lineheight,
            floor_height - frontsector->ceilingheight
          );
          AddDrawWallItem(RTP_WALLTYPE_WALL, &wall);
        }
      }
    }
  }
}

#undef LINE
#undef CALC_Y_VALUES
#undef OU
#undef OV
#undef OV_PEG
#undef CALC_TEX_VALUES_TOP
#undef CALC_TEX_VALUES_MIDDLE1S
#undef CALC_TEX_VALUES_BOTTOM
#undef ADDWALL
