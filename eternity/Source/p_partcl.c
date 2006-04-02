// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// This module is covered by the zdoom source distribution
// license, which is included in the Eternity source distribution,
// and is compatible with the terms of the GNU General Public
// License.
//
// See that license file for details.
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//
//   Code that ties particle effects to map objects, adapted
//   from zdoom. Thanks to Randy Heit.
//
//----------------------------------------------------------------------------

#include <math.h>
#include "z_zone.h"
#include "d_main.h"
#include "doomstat.h"
#include "doomtype.h"
#include "m_random.h"
#include "p_partcl.h"
#include "p_setup.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"
#include "w_wad.h"
#include "p_mobj.h"
#include "p_spec.h"
#include "c_runcmd.h"

// static integers to hold particle color values
static int grey1, grey2, grey3, grey4, red, green, blue, yellow, black,
           red1, green1, blue1, yellow1, purple, purple1, white,
           rblue1, rblue2, rblue3, rblue4, orange, yorange, dred, grey5,
           maroon1, maroon2, mdred;

static struct particleColorList {
   int *color, r, g, b;
} particleColors[] = {
   {&grey1,     85,  85,  85 },
   {&grey2,     171, 171, 171},
   {&grey3,     50,  50,  50 },
   {&grey4,     210, 210, 210},
   {&grey5,     128, 128, 128},
   {&red,       255, 0,   0  },
   {&green,     0,   200, 0  },
   {&blue,      0,   0,   255},
   {&yellow,    255, 255, 0  },
   {&black,     0,   0,   0  },
   {&red1,      255, 127, 127},
   {&green1,    127, 255, 127},
   {&blue1,     127, 127, 255},
   {&yellow1,   255, 255, 180},
   {&purple,    120, 0,   160},
   {&purple1,   200, 30,  255},
   {&white,     255, 255, 255},
   {&rblue1,    81,  81,  255},
   {&rblue2,    0,   0,   227},
   {&rblue3,    0,   0,   130},
   {&rblue4,    0,   0,   80 },
   {&orange,    255, 120, 0  },
   {&yorange,   255, 170, 0  },
   {&dred,      80,  0,   0  },
   {&maroon1,   154, 49,  49 },
   {&maroon2,   125, 24,  24 },
   {&mdred,     165, 0,   0  },
   {NULL}
};

#define	BEAMLENGTH       16
#define NUMVERTEXNORMALS 162
typedef float vec3_t[3];

static vec3_t avelocities[NUMVERTEXNORMALS];

static vec3_t bytedirs[NUMVERTEXNORMALS] = 
{
   {-0.525731f, 0.000000f, 0.850651f}, 
   {-0.442863f, 0.238856f, 0.864188f}, 
   {-0.295242f, 0.000000f, 0.955423f}, 
   {-0.309017f, 0.500000f, 0.809017f}, 
   {-0.162460f, 0.262866f, 0.951056f}, 
   {0.000000f, 0.000000f, 1.000000f}, 
   {0.000000f, 0.850651f, 0.525731f}, 
   {-0.147621f, 0.716567f, 0.681718f}, 
   {0.147621f, 0.716567f, 0.681718f}, 
   {0.000000f, 0.525731f, 0.850651f}, 
   {0.309017f, 0.500000f, 0.809017f}, 
   {0.525731f, 0.000000f, 0.850651f}, 
   {0.295242f, 0.000000f, 0.955423f}, 
   {0.442863f, 0.238856f, 0.864188f}, 
   {0.162460f, 0.262866f, 0.951056f}, 
   {-0.681718f, 0.147621f, 0.716567f}, 
   {-0.809017f, 0.309017f, 0.500000f}, 
   {-0.587785f, 0.425325f, 0.688191f}, 
   {-0.850651f, 0.525731f, 0.000000f}, 
   {-0.864188f, 0.442863f, 0.238856f}, 
   {-0.716567f, 0.681718f, 0.147621f}, 
   {-0.688191f, 0.587785f, 0.425325f}, 
   {-0.500000f, 0.809017f, 0.309017f}, 
   {-0.238856f, 0.864188f, 0.442863f}, 
   {-0.425325f, 0.688191f, 0.587785f}, 
   {-0.716567f, 0.681718f, -0.147621f}, 
   {-0.500000f, 0.809017f, -0.309017f}, 
   {-0.525731f, 0.850651f, 0.000000f}, 
   {0.000000f, 0.850651f, -0.525731f}, 
   {-0.238856f, 0.864188f, -0.442863f}, 
   {0.000000f, 0.955423f, -0.295242f}, 
   {-0.262866f, 0.951056f, -0.162460f}, 
   {0.000000f, 1.000000f, 0.000000f}, 
   {0.000000f, 0.955423f, 0.295242f}, 
   {-0.262866f, 0.951056f, 0.162460f}, 
   {0.238856f, 0.864188f, 0.442863f}, 
   {0.262866f, 0.951056f, 0.162460f}, 
   {0.500000f, 0.809017f, 0.309017f}, 
   {0.238856f, 0.864188f, -0.442863f}, 
   {0.262866f, 0.951056f, -0.162460f}, 
   {0.500000f, 0.809017f, -0.309017f}, 
   {0.850651f, 0.525731f, 0.000000f}, 
   {0.716567f, 0.681718f, 0.147621f}, 
   {0.716567f, 0.681718f, -0.147621f}, 
   {0.525731f, 0.850651f, 0.000000f}, 
   {0.425325f, 0.688191f, 0.587785f}, 
   {0.864188f, 0.442863f, 0.238856f}, 
   {0.688191f, 0.587785f, 0.425325f}, 
   {0.809017f, 0.309017f, 0.500000f}, 
   {0.681718f, 0.147621f, 0.716567f}, 
   {0.587785f, 0.425325f, 0.688191f}, 
   {0.955423f, 0.295242f, 0.000000f}, 
   {1.000000f, 0.000000f, 0.000000f}, 
   {0.951056f, 0.162460f, 0.262866f}, 
   {0.850651f, -0.525731f, 0.000000f}, 
   {0.955423f, -0.295242f, 0.000000f}, 
   {0.864188f, -0.442863f, 0.238856f}, 
   {0.951056f, -0.162460f, 0.262866f}, 
   {0.809017f, -0.309017f, 0.500000f}, 
   {0.681718f, -0.147621f, 0.716567f}, 
   {0.850651f, 0.000000f, 0.525731f}, 
   {0.864188f, 0.442863f, -0.238856f}, 
   {0.809017f, 0.309017f, -0.500000f}, 
   {0.951056f, 0.162460f, -0.262866f}, 
   {0.525731f, 0.000000f, -0.850651f}, 
   {0.681718f, 0.147621f, -0.716567f}, 
   {0.681718f, -0.147621f, -0.716567f}, 
   {0.850651f, 0.000000f, -0.525731f}, 
   {0.809017f, -0.309017f, -0.500000f}, 
   {0.864188f, -0.442863f, -0.238856f}, 
   {0.951056f, -0.162460f, -0.262866f}, 
   {0.147621f, 0.716567f, -0.681718f}, 
   {0.309017f, 0.500000f, -0.809017f}, 
   {0.425325f, 0.688191f, -0.587785f}, 
   {0.442863f, 0.238856f, -0.864188f}, 
   {0.587785f, 0.425325f, -0.688191f}, 
   {0.688191f, 0.587785f, -0.425325f}, 
   {-0.147621f, 0.716567f, -0.681718f}, 
   {-0.309017f, 0.500000f, -0.809017f}, 
   {0.000000f, 0.525731f, -0.850651f}, 
   {-0.525731f, 0.000000f, -0.850651f}, 
   {-0.442863f, 0.238856f, -0.864188f}, 
   {-0.295242f, 0.000000f, -0.955423f}, 
   {-0.162460f, 0.262866f, -0.951056f}, 
   {0.000000f, 0.000000f, -1.000000f}, 
   {0.295242f, 0.000000f, -0.955423f}, 
   {0.162460f, 0.262866f, -0.951056f}, 
   {-0.442863f, -0.238856f, -0.864188f}, 
   {-0.309017f, -0.500000f, -0.809017f}, 
   {-0.162460f, -0.262866f, -0.951056f}, 
   {0.000000f, -0.850651f, -0.525731f}, 
   {-0.147621f, -0.716567f, -0.681718f}, 
   {0.147621f, -0.716567f, -0.681718f}, 
   {0.000000f, -0.525731f, -0.850651f}, 
   {0.309017f, -0.500000f, -0.809017f}, 
   {0.442863f, -0.238856f, -0.864188f}, 
   {0.162460f, -0.262866f, -0.951056f}, 
   {0.238856f, -0.864188f, -0.442863f}, 
   {0.500000f, -0.809017f, -0.309017f}, 
   {0.425325f, -0.688191f, -0.587785f}, 
   {0.716567f, -0.681718f, -0.147621f}, 
   {0.688191f, -0.587785f, -0.425325f}, 
   {0.587785f, -0.425325f, -0.688191f}, 
   {0.000000f, -0.955423f, -0.295242f}, 
   {0.000000f, -1.000000f, 0.000000f}, 
   {0.262866f, -0.951056f, -0.162460f}, 
   {0.000000f, -0.850651f, 0.525731f}, 
   {0.000000f, -0.955423f, 0.295242f}, 
   {0.238856f, -0.864188f, 0.442863f}, 
   {0.262866f, -0.951056f, 0.162460f}, 
   {0.500000f, -0.809017f, 0.309017f}, 
   {0.716567f, -0.681718f, 0.147621f}, 
   {0.525731f, -0.850651f, 0.000000f}, 
   {-0.238856f, -0.864188f, -0.442863f}, 
   {-0.500000f, -0.809017f, -0.309017f}, 
   {-0.262866f, -0.951056f, -0.162460f}, 
   {-0.850651f, -0.525731f, 0.000000f}, 
   {-0.716567f, -0.681718f, -0.147621f}, 
   {-0.716567f, -0.681718f, 0.147621f}, 
   {-0.525731f, -0.850651f, 0.000000f}, 
   {-0.500000f, -0.809017f, 0.309017f}, 
   {-0.238856f, -0.864188f, 0.442863f}, 
   {-0.262866f, -0.951056f, 0.162460f}, 
   {-0.864188f, -0.442863f, 0.238856f}, 
   {-0.809017f, -0.309017f, 0.500000f}, 
   {-0.688191f, -0.587785f, 0.425325f}, 
   {-0.681718f, -0.147621f, 0.716567f}, 
   {-0.442863f, -0.238856f, 0.864188f}, 
   {-0.587785f, -0.425325f, 0.688191f}, 
   {-0.309017f, -0.500000f, 0.809017f}, 
   {-0.147621f, -0.716567f, 0.681718f}, 
   {-0.425325f, -0.688191f, 0.587785f}, 
   {-0.162460f, -0.262866f, 0.951056f}, 
   {0.442863f, -0.238856f, 0.864188f}, 
   {0.162460f, -0.262866f, 0.951056f}, 
   {0.309017f, -0.500000f, 0.809017f}, 
   {0.147621f, -0.716567f, 0.681718f}, 
   {0.000000f, -0.525731f, 0.850651f}, 
   {0.425325f, -0.688191f, 0.587785f}, 
   {0.587785f, -0.425325f, 0.688191f}, 
   {0.688191f, -0.587785f, 0.425325f}, 
   {-0.955423f, 0.295242f, 0.000000f}, 
   {-0.951056f, 0.162460f, 0.262866f}, 
   {-1.000000f, 0.000000f, 0.000000f}, 
   {-0.850651f, 0.000000f, 0.525731f}, 
   {-0.955423f, -0.295242f, 0.000000f}, 
   {-0.951056f, -0.162460f, 0.262866f}, 
   {-0.864188f, 0.442863f, -0.238856f}, 
   {-0.951056f, 0.162460f, -0.262866f}, 
   {-0.809017f, 0.309017f, -0.500000f}, 
   {-0.864188f, -0.442863f, -0.238856f}, 
   {-0.951056f, -0.162460f, -0.262866f}, 
   {-0.809017f, -0.309017f, -0.500000f}, 
   {-0.681718f, 0.147621f, -0.716567f}, 
   {-0.681718f, -0.147621f, -0.716567f}, 
   {-0.850651f, 0.000000f, -0.525731f}, 
   {-0.688191f, 0.587785f, -0.425325f}, 
   {-0.587785f, 0.425325f, -0.688191f}, 
   {-0.425325f, 0.688191f, -0.587785f}, 
   {-0.425325f, -0.688191f, -0.587785f}, 
   {-0.587785f, -0.425325f, -0.688191f}, 
   {-0.688191f, -0.587785f, -0.425325f}, 
};

static particle_t *JitterParticle(int ttl);
static void P_RunEffect(mobj_t *actor, int effects);
static void P_FlyEffect(mobj_t *actor);
static void P_BFGEffect(mobj_t *actor);
static void P_ExplosionParticles(mobj_t *actor, int color1, int color2);
static void P_RocketExplosion(mobj_t *actor);
static void P_BFGExplosion(mobj_t *actor);

static void P_GenVelocities(void)
{
   int i;

   for(i = 0; i < NUMVERTEXNORMALS*3; i++)
   {
      avelocities[0][i] = M_Random() * 0.01f;
   }
}

void P_InitParticleEffects(void)
{
   byte *palette;
   struct particleColorList *pc = particleColors;

   palette = W_CacheLumpName("PLAYPAL", PU_STATIC);

   // match particle colors to best fit and write back to
   // static variables
   while(pc->color)
   {
      *(pc->color) = V_FindBestColor(palette, pc->r, pc->g, pc->b);
      pc++;
   }

   Z_ChangeTag(palette, PU_CACHE);

   P_GenVelocities();
}

//
// P_UnsetParticlePosition
//
// haleyjd 02/20/04: maintenance of particle sector links,
// necessitated by portals.
//
static void P_UnsetParticlePosition(particle_t *ptcl)
{
   particle_t **sprev = ptcl->sprev;
   particle_t  *snext = ptcl->snext;

   if((*sprev = snext))    // unlink from chain
      snext->sprev = sprev;
}

//
// P_SetParticlePosition
//
// haleyjd 02/20/04: maintenance of particle sector links,
// necessitated by portals. Maintaining a subsector_t
// field in the particle_t will be useful in the future,
// I am sure.
//
static void P_SetParticlePosition(particle_t *ptcl)
{
   subsector_t *ss = R_PointInSubsector(ptcl->x, ptcl->y);
   particle_t  **link = &ss->sector->ptcllist;
   particle_t  *snext = *link;

   ptcl->subsector = ss;

   if((ptcl->snext = snext))
      snext->sprev = &ptcl->snext;
   ptcl->sprev = link;
   *link = ptcl;
}

void P_ParticleThinker(void)
{
   int i;
   particle_t *particle, *prev;
   subsector_t *subsec;
   
   i = activeParticles;
   prev = NULL;
   while(i != -1) 
   {
      byte oldtrans;
      
      particle = Particles + i;
      i = particle->next;

      // haleyjd 09/01/02: zdoom translucency system is now
      // available, so this has been enabled
      
      oldtrans = particle->trans;
      particle->trans -= particle->fade;
      if(oldtrans < particle->trans || --particle->ttl == 0)
      {
         P_UnsetParticlePosition(particle);
         memset(particle, 0, sizeof(particle_t));
         if(prev)
            prev->next = i;
         else
            activeParticles = i;
         particle->next = inactiveParticles;
         inactiveParticles = particle - Particles;
         continue;
      }

      P_UnsetParticlePosition(particle);
      particle->x += particle->velx;
      particle->y += particle->vely;
      particle->z += particle->velz;
      P_SetParticlePosition(particle);

      subsec = particle->subsector;

      // handle special movement flags (post-position-set)

      // floor clipping
      if((particle->styleflags & PS_FLOORCLIP) &&
         particle->z < subsec->sector->floorheight)
      {
         particle->z = subsec->sector->floorheight;
         particle->accz = particle->velz = 0;
      }

      particle->velx += particle->accx;
      particle->vely += particle->accy;
      particle->velz += particle->accz;
      prev = particle;
   }
}

void P_RunEffects(void)
{
   int snum = 0;
   thinker_t *currentthinker = &thinkercap;

   if(camera)
   {
      subsector_t *ss = R_PointInSubsector(camera->x, camera->y);
      snum = (ss->sector - sectors) * numsectors;
   }
   else
   {
      subsector_t *ss = players[consoleplayer].mo->subsector;
      snum = (ss->sector - sectors) * numsectors;
   }

   while((currentthinker = currentthinker->next) != &thinkercap)
   {
      if(currentthinker->function == P_MobjThinker)
      {
         int rnum;
         mobj_t *mobj = (mobj_t *)currentthinker;
         rnum = snum + (mobj->subsector->sector - sectors);
         if(mobj->effects)
         {
            // run only if possibly visible
            if(!(rejectmatrix[rnum>>3] & (1<<(rnum&7))))
               P_RunEffect(mobj, mobj->effects);
         }
      }
   }
}

//
// haleyjd 05/19/02: partially rewrote to not make assumptions
// about struct member order and alignment in memory
//

#define FADEFROMTTL(a) (255/(a))

#define PARTICLE_VELRND ((FRACUNIT / 4096)  * (M_Random() - 128))
#define PARTICLE_ACCRND ((FRACUNIT / 16384) * (M_Random() - 128))

static particle_t *JitterParticle(int ttl)
{
   particle_t *particle = newParticle();
   
   if(particle) 
   {
      // Set initial velocities
      particle->velx = PARTICLE_VELRND;
      particle->vely = PARTICLE_VELRND;
      particle->velz = PARTICLE_VELRND;
      
      // Set initial accelerations
      particle->accx = PARTICLE_ACCRND;
      particle->accy = PARTICLE_ACCRND;
      particle->accz = PARTICLE_ACCRND;
      
      particle->trans = 255;	// fully opaque
      particle->ttl = ttl;
      particle->fade = FADEFROMTTL(ttl);
   }
   return particle;
}

static void MakeFountain(mobj_t *actor, int color1, int color2)
{
   particle_t *particle;
   
   if(!(leveltime & 1))
      return;
   
   particle = JitterParticle(51);
   
   if(particle)
   {
      angle_t an  = M_Random()<<(24-ANGLETOFINESHIFT);
      fixed_t out = FixedMul(actor->radius, M_Random()<<8);
      
      particle->x = actor->x + FixedMul(out, finecosine[an]);
      particle->y = actor->y + FixedMul(out, finesine[an]);
      particle->z = actor->z /*+ actor->height*/ + FRACUNIT;
      P_SetParticlePosition(particle);
      
      if(out < actor->radius/8)
         particle->velz += FRACUNIT*10/3;
      else
         particle->velz += FRACUNIT*3;
      
      particle->accz -= FRACUNIT/11;
      if(M_Random() < 30)
      {
         particle->size = 4;
         particle->color = color2;
      } 
      else 
      {
         particle->size = 6;
         particle->color = color1;
      }

      particle->styleflags = 0;
   }
}

static void P_RunEffect(mobj_t *actor, int effects)
{
   angle_t moveangle = R_PointToAngle2(0,0,actor->momx,actor->momy);
   particle_t *particle;

   if(effects & FX_FLIES)
   {
      P_FlyEffect(actor);
   }
   
   if((effects & FX_ROCKET) && drawrockettrails)
   {
      int i, speed;

      // Rocket trail
      fixed_t backx = 
         actor->x - FixedMul(finecosine[(moveangle)>>ANGLETOFINESHIFT], 
                             actor->radius*2);
      fixed_t backy = 
         actor->y - FixedMul(finesine[(moveangle)>>ANGLETOFINESHIFT], 
                             actor->radius*2);
      fixed_t backz = 
         actor->z - (actor->height>>3) * (actor->momz>>16) + 
         (2*actor->height)/3;
      
      angle_t an = (moveangle + ANG90) >> ANGLETOFINESHIFT;

      particle = JitterParticle(3 + (M_Random() & 31));
      if(particle)
      {
         fixed_t pathdist = M_Random()<<8;
         particle->x = backx - FixedMul(actor->momx, pathdist);
         particle->y = backy - FixedMul(actor->momy, pathdist);
         particle->z = backz - FixedMul(actor->momz, pathdist);
         P_SetParticlePosition(particle);

         speed = (M_Random () - 128) * (FRACUNIT/200);
         particle->velx += FixedMul(speed, finecosine[an]);
         particle->vely += FixedMul(speed, finesine[an]);
         particle->velz -= FRACUNIT/36;
         particle->accz -= FRACUNIT/20;
         particle->color = yellow;
         particle->size = 2;
         particle->styleflags = PS_FULLBRIGHT;
      }
      
      for(i = 6; i; i--)
      {
         particle_t *particle = JitterParticle (3 + (M_Random() & 31));
         if (particle)
         {
            fixed_t pathdist = M_Random()<<8;
            particle->x = backx - FixedMul(actor->momx, pathdist);
            particle->y = backy - FixedMul(actor->momy, pathdist);
            particle->z = backz - FixedMul(actor->momz, pathdist) + 
                          (M_Random() << 10);
            P_SetParticlePosition(particle);

            speed = (M_Random () - 128) * (FRACUNIT/200);
            particle->velx += FixedMul(speed, finecosine[an]);
            particle->vely += FixedMul(speed, finesine[an]);
            particle->velz += FRACUNIT/80;
            particle->accz += FRACUNIT/40;
            if(M_Random() & 7)
               particle->color = grey2;
            else
               particle->color = grey1;
            
            particle->size = 3;
            particle->styleflags = 0;
         } 
         else
         {
            break;
         }
      }
   }
   
   if((effects & FX_GRENADE) && drawgrenadetrails)
   {
      // Grenade trail
      
      P_DrawSplash2(6,
         actor->x - FixedMul (finecosine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2),
         actor->y - FixedMul (finesine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2),
         actor->z - (actor->height>>3) * (actor->momz>>16) + (2*actor->height)/3,
         moveangle + ANG180, 2, 2);
   }

   if((effects & FX_BFG) && drawbfgcloud)
   {
      P_BFGEffect(actor);
   }

   if((effects & FX_FOUNTAINMASK) && !(actor->flags2 & MF2_DORMANT))
   {
      // Particle fountain -- can be switched on and off via the
      // MF2_DORMANT flag
      
      static const int *fountainColors[16] = 
      { 
         &black,  &black,
         &red,    &red1,
         &green,  &green1,
         &blue,   &blue1,
         &yellow, &yellow1,
         &purple, &purple1,
         &black,  &grey3,
         &grey4,  &white
      };
      int color = (effects & FX_FOUNTAINMASK) >> 15;
      MakeFountain(actor, *fountainColors[color], 
                   *fountainColors[color+1]);
   }
}

void P_DrawSplash(int count, fixed_t x, fixed_t y, fixed_t z, 
                  angle_t angle, int kind)
{
   int color1, color2;
   
   switch(kind) 
   {
   case 1: // Spark
      color1 = orange;
      color2 = yorange;
      break;
   default:
      return;
   }
   
   for(; count; count--)
   {
      angle_t an;
      particle_t *p = JitterParticle(10);
            
      if(!p)
         break;
      
      p->size = 2;
      p->color = M_Random() & 0x80 ? color1 : color2;
      p->styleflags = PS_FULLBRIGHT;
      p->velz -= M_Random() * 512;
      p->accz -= FRACUNIT/8;
      p->accx += (M_Random() - 128) * 8;
      p->accy += (M_Random() - 128) * 8;
      p->z = z - M_Random() * 1024;
      an = (angle + (M_Random() << 21)) >> ANGLETOFINESHIFT;
      p->x = x + (M_Random() & 15)*finecosine[an];
      p->y = y + (M_Random() & 15)*finesine[an];
      P_SetParticlePosition(p);
   }
}

// haleyjd 05/08/03: custom particle blood colors

static struct bloodColor
{
   int *color1;
   int *color2;
} mobjBloodColors[NUMBLOODCOLORS] =
{
   { &red,    &dred },
   { &grey1,  &grey5 },
   { &green,  &green1 },
   { &blue,   &blue },
   { &yellow, &yellow },
   { &black,  &grey3 },
   { &purple, &purple1 },
   { &grey4,  &white },
   { &orange, &yorange },
};

void P_DrawSplash2(int count, fixed_t x, fixed_t y, fixed_t z, 
                   angle_t angle, int updown, int kind)
{   
   int color1, color2, zvel, zspread, zadd, bloodcolor = 0;

   if(kind & MBC_BLOODMASK)
   {
      bloodcolor = kind & ~MBC_BLOODMASK;
      if(bloodcolor < 0 || bloodcolor >= NUMBLOODCOLORS)
         bloodcolor = 0;
      kind = 0;
   }
   
   switch(kind)
   {
   case 0:              // Blood
      color1 = *(mobjBloodColors[bloodcolor].color1);
      color2 = *(mobjBloodColors[bloodcolor].color2);
      break;
   case 1:              // Gunshot
      // default: grey puff
      color1 = grey1;
      color2 = grey5;

      if(!comp[comp_terrain])
      {
         // 06/21/02: make bullet puff colors responsive to 
         // TerrainTypes -- this is very cool and Quake-2-like ^_^      
         
         int terrain = P_GetTerrainTypeForPt(x, y, updown);
         
         switch(terrain)
         {
         case FLOOR_WATER:
            color1 = blue1;
            color2 = blue;
            break;
         case FLOOR_LAVA:
            color1 = orange;
            color2 = mdred;
            break;
         default:
            break;
         }
      }
      break;
   case 2:		// Smoke
      color1 = grey3;
      color2 = grey1;
      break;
   default:
      return;
   }
   
   zvel = -128;
   zspread = (updown ? -6000 : 6000);
   zadd = ((updown == 2) ? -128 : 0);
   
   for(; count; count--)
   {
      particle_t *p = newParticle();
      angle_t an;
      
      if(!p)
         break;
      
      p->ttl = 12;
      p->fade = FADEFROMTTL(12);
      p->trans = 255;
      p->styleflags = 0;
      p->size = 4;
      p->color = M_Random() & 0x80 ? color1 : color2;
      p->velz = M_Random() * zvel;
      p->accz = -FRACUNIT/22;
      if(kind)
      {
         an = (angle + ((M_Random() - 128) << 23)) >> ANGLETOFINESHIFT;
         p->velx = (M_Random() * finecosine[an]) >> 11;
         p->vely = (M_Random() * finesine[an]) >> 11;
         p->accx = p->velx >> 4;
         p->accy = p->vely >> 4;
      }
      p->z = z + (M_Random() + zadd) * zspread;
      an = (angle + ((M_Random() - 128) << 22)) >> ANGLETOFINESHIFT;
      p->x = x + (M_Random() & 31)*finecosine[an];
      p->y = y + (M_Random() & 31)*finesine[an];
      P_SetParticlePosition(p);
   }
}

void P_DisconnectEffect(mobj_t *actor)
{
   int i;
   
   for(i = 64; i; i--)
   {
      particle_t *p = JitterParticle (TICRATE*2);
      
      if(!p)
         break;
      
      p->x = actor->x + 
             ((M_Random()-128)<<9) * (actor->radius>>FRACBITS);
      p->y = actor->y + 
             ((M_Random()-128)<<9) * (actor->radius>>FRACBITS);
      p->z = actor->z + (M_Random()<<8) * (actor->height>>FRACBITS);
      P_SetParticlePosition(p);

      p->accz -= FRACUNIT/4096;
      p->color = M_Random() < 128 ? maroon1 : maroon2;
      p->size = 4;
      p->styleflags = PS_FULLBRIGHT;
   }
}

#define FLYCOUNT 162

static void P_FlyEffect(mobj_t *actor)
{
   int i;
   particle_t *p;
   float angle;
   float sp, sy, cp, cy;
   vec3_t forward;
   float dist = 64;
   float ltime;

   ltime = (float)leveltime / 50.0f;
   
   for(i = 0; i < FLYCOUNT; i += 2)
   {
      if(!(p = newParticle()))
         break;

      angle = ltime * avelocities[i][0];
      sy = (float)sin(angle);
      cy = (float)cos(angle);
      angle = ltime * avelocities[i][1];
      sp = (float)sin(angle);
      cp = (float)cos(angle);
	
      forward[0] = cp*cy;
      forward[1] = cp*sy;
      forward[2] = -sp;

      dist = (float)sin(ltime + i)*64;
      p->x = actor->x + (int)((bytedirs[i][0]*dist + forward[0]*BEAMLENGTH)*FRACUNIT);
      p->y = actor->y + (int)((bytedirs[i][1]*dist + forward[1]*BEAMLENGTH)*FRACUNIT);
      p->z = actor->z + (int)((bytedirs[i][2]*dist + forward[2]*BEAMLENGTH)*FRACUNIT);
      P_SetParticlePosition(p);

      p->velx = p->vely = p->velz = 0;
      p->accx = p->accy = p->accz = 0;

      p->color = black;

      p->size = 4; // ???
      p->ttl = 1;
      p->trans = 255;
      p->styleflags = 0;
   }
}

static void P_BFGEffect(mobj_t *actor)
{
   int i;
   particle_t *p;
   float angle;
   float sp, sy, cp, cy;
   vec3_t forward;
   float dist = 64;
   float ltime;
	
   ltime = (float)leveltime / 30.0f;
   for(i = 0; i < NUMVERTEXNORMALS; i++)
   {
      if(!(p = newParticle()))
         break;

      angle = ltime * avelocities[i][0];
      sy = (float)sin(angle);
      cy = (float)cos(angle);
      angle = ltime * avelocities[i][1];
      sp = (float)sin(angle);
      cp = (float)cos(angle);
	
      forward[0] = cp*cy;
      forward[1] = cp*sy;
      forward[2] = -sp;
      
      dist = (float)sin(ltime + i)*64;
      p->x = actor->x + (int)((bytedirs[i][0]*dist + forward[0]*BEAMLENGTH)*FRACUNIT);
      p->y = actor->y + (int)((bytedirs[i][1]*dist + forward[1]*BEAMLENGTH)*FRACUNIT);
      p->z = actor->z + (int)((bytedirs[i][2]*dist + forward[2]*BEAMLENGTH)*FRACUNIT);
      P_SetParticlePosition(p);

      p->velx = p->vely = p->velz = 0;
      p->accx = p->accy = p->accz = 0;

      p->color = green;

      p->size = 4;
      p->ttl = 1;
      p->trans = 169;
      p->styleflags = PS_FULLBRIGHT;
   }
}

//
// haleyjd 05/20/02: frame-based particle events system
//
// A field, particle_evt, has been added to the state_t structure
// to provide a numerical indicator of what type of effect a frame
// should trigger
//

particle_event_t particleEvents[P_EVENT_NUMEVENTS] =
{
   { NULL, "pevt_none" },               // P_EVENT_NONE
   { P_RocketExplosion, "pevt_rexpl" }, // P_EVENT_ROCKET_EXPLODE
   { P_BFGExplosion, "pevt_bfgexpl" },  // P_EVENT_BFG_EXPLODE
};

//
// P_RunEvent
//
// Called from P_SetMobjState, immediately after the action function
// for the actor's current state has been executed.
//
void P_RunEvent(mobj_t *actor)
{
   long effectNum;
   
   effectNum = ((actor && actor->state) ? actor->state->particle_evt : 
                                          P_EVENT_NONE);

   if(effectNum < 0 || effectNum >= P_EVENT_NUMEVENTS)
   {
      doom_printf(FC_ERROR"P_RunEvent: Particle event number out of range");
      return;
   }

   if(effectNum != P_EVENT_NONE)
   {
      if(particleEvents[effectNum].enabled)
         particleEvents[effectNum].func(actor);
   }
}

//
// haleyjd 05/19: experimental explosion
//
static void P_ExplosionParticles(mobj_t *actor, int color1, int color2)
{
   int i, rnd;

   for(i = 0; i < 256; i++)
   {
      particle_t *p = newParticle();

      if(!p)
         break;

      p->ttl = 26;
      // fade slower than usual
      p->fade = FADEFROMTTL(26);
      p->trans = 255;

      // 2^11 = 2048, 2^12 = 4096
      p->x = actor->x + (((M_Random() % 32) - 16)*4096);
      p->y = actor->y + (((M_Random() % 32) - 16)*4096);
      p->z = actor->z + (((M_Random() % 32) - 16)*4096);
      P_SetParticlePosition(p);

      // note: was (rand() % 384) - 192 in Q2, but DOOM's RNG
      // only outputs numbers from 0 to 255, so it has to be
      // corrected to unbias it and get output from approx.
      // -192 to 191
      rnd = M_Random();
      p->velx = (rnd - 192 + (rnd/2))*2048;
      rnd = M_Random();
      p->vely = (rnd - 192 + (rnd/2))*2048;
      rnd = M_Random();
      p->velz = (rnd - 192 + (rnd/2))*2048;

      p->accx = p->accy = p->accz = 0;

      p->size = (M_Random() < 48) ? 6 : 4;

      p->color = (M_Random() & 0x80) ? color2 : color1;

      p->styleflags = PS_FULLBRIGHT;
   }
}

static void P_RocketExplosion(mobj_t *actor)
{
   P_ExplosionParticles(actor, orange, yorange);
}

static void P_BFGExplosion(mobj_t *actor)
{
   P_ExplosionParticles(actor, green, green);
}

// Generate console variables for the enabled flags on each event
void P_AddEventVars(void)
{
   int i;

   for(i = 1; i < P_EVENT_NUMEVENTS; i++)
   {
      variable_t *variable;
      command_t  *command;

      variable = Z_Malloc(sizeof(variable_t), PU_STATIC, NULL);
      variable->variable = &(particleEvents[i].enabled);
      variable->v_default = NULL;
      variable->type = vt_int;
      variable->min = 0;
      variable->max = 1;
      variable->defines = onoff;

      command = Z_Malloc(sizeof(command_t), PU_STATIC, NULL);
      command->name = particleEvents[i].name;
      command->type = ct_variable;
      command->flags = 0;
      command->variable = variable;
      command->handler = NULL;
      command->netcmd = 0;

      (C_AddCommand)(command);
   }
}

// EOF
