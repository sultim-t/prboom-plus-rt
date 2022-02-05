#include "doomstat.h"
#include "rt_main.h"
#include "r_main.h"
#include "r_state.h"


void RT_StartDrawScene()
{}


void RT_DrawScene(player_t *player)
{}


void RT_EndDrawScene()
{}


// gld_CalcLightLevel_shaders
static float CalcLightLevel(int lightlevel)
{
  int light = BETWEEN(0, 255, lightlevel);

  return (float)light / 255.0f;
}


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
    flat.light = CalcLightLevel(plane->lightlevel + (extralight << 5));
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
    flat.light = CalcLightLevel(plane->lightlevel + (extralight << 5));
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


  rtsectordata_t sector_geometry = RT_CreateSectorGeometryData(sectornum, ceiling);

  RgGeometryUploadInfo info =
  {
    .uniqueID = (uint64_t)sectornum | (ceiling ? 1ull << 32ull : 0ull),
    .geomType = RG_GEOMETRY_TYPE_DYNAMIC,
    .passThroughType = RG_GEOMETRY_PASS_THROUGH_TYPE_OPAQUE,
    .visibilityType = RG_GEOMETRY_VISIBILITY_TYPE_WORLD_0,
    .vertexCount = sector_geometry.vertex_count,
    .pVertexData = sector_geometry.positions,
    .pNormalData = sector_geometry.normals,
    .pTexCoordLayerData = { sector_geometry.texcoords },
    .indexCount = sector_geometry.index_count,
    .pIndexData = sector_geometry.indices,
    .sectorID = 0, // sectornum,
    .layerColors = { RG_COLOR_WHITE },
    .defaultRoughness = 0.5f,
    .defaultMetallicity = 0.2f,
    .defaultEmission = 0,
    .geomMaterial = { flat.td->rg_handle },
    .transform = 
      {
        1,0,0,0,
        0,1,0,flat.z,
        0,0,1,0
      }
  };

  RgResult r = rgUploadGeometry(rtmain.instance, &info);
  RG_CHECK(r);

  RT_DestroySectorGeometryData(&sector_geometry);

  // TODO RT: flat with texcoord offset
}


void RT_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling)
{
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


void RT_AddWall(seg_t *seg)
{}
