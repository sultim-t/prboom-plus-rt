/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
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
 * DESCRIPTION:  Platform-independent sound code
 *
 *-----------------------------------------------------------------------------*/

// killough 3/7/98: modified to allow arbitrary listeners in spy mode
// killough 5/2/98: reindented, removed useless code, beautified

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "doomstat.h"
#include "s_sound.h"
#include "i_sound.h"
#include "i_system.h"
#include "r_main.h"
#include "m_random.h"
#include "w_wad.h"
#include "lprintf.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "p_info.h"

#ifdef COMPILE_VIDD
#include "vidd/vidd.h"
#endif

// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST (1200<<FRACBITS)

// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
//
// killough 12/98: restore original
// #define S_CLOSE_DIST (160<<FRACBITS)

#define S_CLOSE_DIST (200<<FRACBITS)

#define S_ATTENUATOR ((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)

// Adjustable by menu.
#define NORM_PITCH 128
#define NORM_PRIORITY 64
#define NORM_SEP 128
#define S_STEREO_SWING (96<<FRACBITS)

// sf: sound/music hashing
#define SOUND_HASHSLOTS 17
// use sound_hash for music hash too
#define sound_hash(s)                             \
         ( ( tolower((s)[0]) + (s)[0] ?           \
             tolower((s)[1]) + (s)[1] ?           \
             tolower((s)[2]) + (s)[2] ?           \
             tolower((s)[3]) + (s)[3] ?           \
             tolower((s)[4]) : 0 : 0 : 0 : 0 ) % SOUND_HASHSLOTS )

static void S_CreateSoundHashTable();

static int sound_initted = false;       // set to true after running s_init

//jff 1/22/98 make sound enabling variables readable here
extern int snd_card, mus_card;
extern boolean nosfxparm, nomusicparm;
//jff end sound enabling variables readable here

const char * S_music_files[NUMMUSIC]; // cournia - stores music file names

CONSOLE_STRING(mus_e1m1, S_music_files[mus_e1m1], NULL, 20, 0) {}
CONSOLE_STRING(mus_e1m2, S_music_files[mus_e1m2], NULL, 20, 0) {}
CONSOLE_STRING(mus_e1m3, S_music_files[mus_e1m3], NULL, 20, 0) {}
CONSOLE_STRING(mus_e1m4, S_music_files[mus_e1m4], NULL, 20, 0) {}
CONSOLE_STRING(mus_e1m5, S_music_files[mus_e1m5], NULL, 20, 0) {}
CONSOLE_STRING(mus_e1m6, S_music_files[mus_e1m6], NULL, 20, 0) {}
CONSOLE_STRING(mus_e1m7, S_music_files[mus_e1m7], NULL, 20, 0) {}
CONSOLE_STRING(mus_e1m8, S_music_files[mus_e1m8], NULL, 20, 0) {}
CONSOLE_STRING(mus_e1m9, S_music_files[mus_e1m9], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m1, S_music_files[mus_e2m1], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m2, S_music_files[mus_e2m2], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m3, S_music_files[mus_e2m3], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m4, S_music_files[mus_e2m4], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m5, S_music_files[mus_e2m5], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m6, S_music_files[mus_e2m6], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m7, S_music_files[mus_e2m7], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m8, S_music_files[mus_e2m8], NULL, 20, 0) {}
CONSOLE_STRING(mus_e2m9, S_music_files[mus_e2m9], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m1, S_music_files[mus_e3m1], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m2, S_music_files[mus_e3m2], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m3, S_music_files[mus_e3m3], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m4, S_music_files[mus_e3m4], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m5, S_music_files[mus_e3m5], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m6, S_music_files[mus_e3m6], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m7, S_music_files[mus_e3m7], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m8, S_music_files[mus_e3m8], NULL, 20, 0) {}
CONSOLE_STRING(mus_e3m9, S_music_files[mus_e3m9], NULL, 20, 0) {}
CONSOLE_STRING(mus_inter, S_music_files[mus_inter], NULL, 20, 0) {}
CONSOLE_STRING(mus_intro, S_music_files[mus_intro], NULL, 20, 0) {}
CONSOLE_STRING(mus_bunny, S_music_files[mus_bunny], NULL, 20, 0) {}
CONSOLE_STRING(mus_victor, S_music_files[mus_victor], NULL, 20, 0) {}
CONSOLE_STRING(mus_introa, S_music_files[mus_introa], NULL, 20, 0) {}
CONSOLE_STRING(mus_runnin, S_music_files[mus_runnin], NULL, 20, 0) {}
CONSOLE_STRING(mus_stalks, S_music_files[mus_stalks], NULL, 20, 0) {}
CONSOLE_STRING(mus_countd, S_music_files[mus_countd], NULL, 20, 0) {}
CONSOLE_STRING(mus_betwee, S_music_files[mus_betwee], NULL, 20, 0) {}
CONSOLE_STRING(mus_doom, S_music_files[mus_doom], NULL, 20, 0) {}
CONSOLE_STRING(mus_the_da, S_music_files[mus_the_da], NULL, 20, 0) {}
CONSOLE_STRING(mus_shawn, S_music_files[mus_shawn], NULL, 20, 0) {}
CONSOLE_STRING(mus_ddtblu, S_music_files[mus_ddtblu], NULL, 20, 0) {}
CONSOLE_STRING(mus_in_cit, S_music_files[mus_in_cit], NULL, 20, 0) {}
CONSOLE_STRING(mus_dead, S_music_files[mus_dead], NULL, 20, 0) {}
CONSOLE_STRING(mus_stlks2, S_music_files[mus_stlks2], NULL, 20, 0) {}
CONSOLE_STRING(mus_theda2, S_music_files[mus_theda2], NULL, 20, 0) {}
CONSOLE_STRING(mus_doom2, S_music_files[mus_doom2], NULL, 20, 0) {}
CONSOLE_STRING(mus_ddtbl2, S_music_files[mus_ddtbl2], NULL, 20, 0) {}
CONSOLE_STRING(mus_runni2, S_music_files[mus_runni2], NULL, 20, 0) {}
CONSOLE_STRING(mus_dead2, S_music_files[mus_dead2], NULL, 20, 0) {}
CONSOLE_STRING(mus_stlks3, S_music_files[mus_stlks3], NULL, 20, 0) {}
CONSOLE_STRING(mus_romero, S_music_files[mus_romero], NULL, 20, 0) {}
CONSOLE_STRING(mus_shawn2, S_music_files[mus_shawn2], NULL, 20, 0) {}
CONSOLE_STRING(mus_messag, S_music_files[mus_messag], NULL, 20, 0) {}
CONSOLE_STRING(mus_count2, S_music_files[mus_count2], NULL, 20, 0) {}
CONSOLE_STRING(mus_ddtbl3, S_music_files[mus_ddtbl3], NULL, 20, 0) {}
CONSOLE_STRING(mus_ampie, S_music_files[mus_ampie], NULL, 20, 0) {}
CONSOLE_STRING(mus_theda3, S_music_files[mus_theda3], NULL, 20, 0) {}
CONSOLE_STRING(mus_adrian, S_music_files[mus_adrian], NULL, 20, 0) {}
CONSOLE_STRING(mus_messg2, S_music_files[mus_messg2], NULL, 20, 0) {}
CONSOLE_STRING(mus_romer2, S_music_files[mus_romer2], NULL, 20, 0) {}
CONSOLE_STRING(mus_tense, S_music_files[mus_tense], NULL, 20, 0) {}
CONSOLE_STRING(mus_shawn3, S_music_files[mus_shawn3], NULL, 20, 0) {}
CONSOLE_STRING(mus_openin, S_music_files[mus_openin], NULL, 20, 0) {}
CONSOLE_STRING(mus_evil, S_music_files[mus_evil], NULL, 20, 0) {}
CONSOLE_STRING(mus_ultima, S_music_files[mus_ultima], NULL, 20, 0) {}
CONSOLE_STRING(mus_read_m, S_music_files[mus_read_m], NULL, 20, 0) {}
CONSOLE_STRING(mus_dm2ttl, S_music_files[mus_dm2ttl], NULL, 20, 0) {}
CONSOLE_STRING(mus_dm2int, S_music_files[mus_dm2int], NULL, 20, 0) {}

static char *samplerate_str[] =
{
  "11025",
  "22050",
  "44100"
};

static int desired_samplerate;

CONSOLE_INT(snd_samplerate, desired_samplerate, NULL, 0, 2, samplerate_str, cf_buffered)
{
	switch (desired_samplerate) {
	default:
	case 0:
		snd_samplerate = 11025;
		break;
	case 1:
		snd_samplerate = 22050;
		break;
	case 2:
		snd_samplerate = 44100;
		break;
	}
	I_InitSound();
}

typedef struct
{
  sfxinfo_t *sfxinfo;  // sound information (if null, channel avail.)
  const mobj_t *origin;// origin of sound
  int handle;          // handle of the sound being played
} channel_t;

// the set of channels available
static channel_t *channels;

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int snd_SfxVolume = 15;

// Maximum volume of music. Useless so far.
int snd_MusicVolume = 15;

// whether songs are mus_paused
static boolean mus_paused;

// music currently being played
static musicinfo_t *mus_playing;

// following is set
//  by the defaults code in M_misc:
// number of channels available
int default_numChannels = 8;
int numChannels;

//jff 3/17/98 to keep track of last IDMUS specified music num
int idmusnum;

        // sf:
sfxinfo_t *sfxinfos[SOUND_HASHSLOTS];
musicinfo_t *musicinfos[SOUND_HASHSLOTS];

//
// Internals.
//

static void S_StopChannel(int cnum)
{
  int i;
  channel_t *c = &channels[cnum];

  if (c->sfxinfo)
    {
      // stop the sound playing
      if (I_SoundIsPlaying(c->handle))
        I_StopSound(c->handle);

      // check to see
      //  if other channels are playing the sound
      for (i=0 ; i<numChannels ; i++)
        if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
          break;

      // degrade usefulness of sound data
      c->sfxinfo->usefulness--;
      c->sfxinfo = 0;
    }
}

//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//

int S_AdjustSoundParams(const mobj_t *listener, const mobj_t *source,
                        int *vol, int *sep, int *pitch)
{
  fixed_t adx, ady,approx_dist;
  angle_t angle;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return 0;

  // calculate the distance to sound origin
  //  and clip it if necessary
  adx = D_abs(listener->x - source->x);
  ady = D_abs(listener->y - source->y);

  // From _GG1_ p.428. Appox. eucledian distance fast.
  approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);
/*
  if (source)
    {
      // source in a killed-sound sector?
      if (R_PointInSubsector(source->x, source->y)->sector->special
	  & SF_KILLSOUND)
	return 0;
    }
  else
    // are _we_ in a killed-sound sector?
    if(gamestate == GS_LEVEL &&
       R_PointInSubsector(listener->x, listener->y)
       ->sector->special & SF_KILLSOUND)
      return 0;
*/
  if (!approx_dist)  // killough 11/98: handle zero-distance as special case
    {
      *sep = NORM_SEP;
      *vol = snd_SfxVolume * 8;
      return *vol > 0;
    }

  if (approx_dist > S_CLIPPING_DIST)
    return 0;

  // angle of source to listener
  angle = R_PointToAngle2(listener->x, listener->y, source->x, source->y);

  if (angle <= listener->angle)
    angle += 0xffffffff;
  angle -= listener->angle;
  angle >>= ANGLETOFINESHIFT;

  // stereo separation
  *sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

  // volume calculation
  if (approx_dist < S_CLOSE_DIST)
    *vol = snd_SfxVolume*8;
  else
    // distance effect
    *vol = (snd_SfxVolume * ((S_CLIPPING_DIST-approx_dist)>>FRACBITS) * 8)
      / S_ATTENUATOR;

  return (*vol > 0);
}

//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//

static int S_getChannel(const mobj_t *origin, sfxinfo_t *sfxinfo)
{
  // channel number to use
  int cnum;
  channel_t *c;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return -1;

  // Find an open channel
  // killough 12/98: replace is_pickup hack with singularity flag
  for (cnum=0; cnum<numChannels && channels[cnum].sfxinfo; cnum++)
    if (origin && channels[cnum].origin == origin &&
        channels[cnum].sfxinfo->singularity == sfxinfo->singularity)
      {
        S_StopChannel(cnum);
        break;
      }

    // None available
  if (cnum == numChannels)
    {      // Look for lower priority
      for (cnum=0 ; cnum<numChannels ; cnum++)
        if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
          break;
      if (cnum == numChannels)
        return -1;                  // No lower priority.  Sorry, Charlie.
      else
        S_StopChannel(cnum);        // Otherwise, kick out lower priority.
    }

  c = &channels[cnum];              // channel is decided to be cnum.
  c->sfxinfo = sfxinfo;
  c->origin = origin;
  return cnum;
}

void S_StartSfxInfo(const mobj_t *origin, sfxinfo_t *sfx, int sfx_id)
{
  int sep, pitch, priority, cnum;
  int volume = snd_SfxVolume;
//  int sfx_id;

  //sfx_id = sfx - S_sfx;

  // check for bogus sound #
  if (sfx_id < 1 || sfx_id > NUMSFX)
    I_Error("S_StartSoundAtVolume: Bad sfx #: %d", sfx_id);

#ifdef COMPILE_VIDD
  if (VIDD_REC_inProgress()) {
    VIDD_REC_registerSound(sfx_id, origin); // POPE
    return;
  }
#endif

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm || !sound_initted)
    return;

  // Initialize sound parameters
  if (sfx->link)
    {
      pitch = sfx->pitch;
      priority = sfx->priority;
      volume += sfx->volume;

      if (volume < 1)
        return;

      if (volume > snd_SfxVolume)
        volume = snd_SfxVolume;
    }
  else
    {
      pitch = NORM_PITCH;
      priority = NORM_PRIORITY;
    }

  // Check to see if it is audible, modify the params
  // killough 3/7/98, 4/25/98: code rearranged slightly

  if (!origin || origin == players[displayplayer].mo) {
    sep = NORM_SEP;
    volume *= 8;
  } else
    if (!S_AdjustSoundParams(players[displayplayer].mo, origin, &volume,
                             &sep, &pitch))
      return;
    else
      if ( origin->x == players[displayplayer].mo->x &&
           origin->y == players[displayplayer].mo->y)
        sep = NORM_SEP;

  if (pitched_sounds)
    {
  // hacks to vary the sfx pitches
  if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
    pitch += 8 - (M_Random()&15);
  else
    if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
      pitch += 16 - (M_Random()&31);

  if (pitch<0)
    pitch = 0;

  if (pitch>255)
    pitch = 255;
    }

  // kill old sound
  // killough 12/98: replace is_pickup hack with singularity flag
  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo && channels[cnum].origin == origin &&
        (comp[comp_sound] || channels[cnum].sfxinfo->singularity == sfx->singularity))
      {
        S_StopChannel(cnum);
        break;
      }

  // try to find a channel
  cnum = S_getChannel(origin, sfx);

  if (cnum<0)
    return;

  while(sfx->link) sfx = sfx->link;     // sf: skip thru link(s)

  // get lumpnum if necessary
  // killough 2/28/98: make missing sounds non-fatal
  if (sfx->lumpnum < 0 && (sfx->lumpnum = I_GetSfxLumpNum(sfx)) < 0)
    return;

  // increase the usefulness
  if (sfx->usefulness++ < 0)
    sfx->usefulness = 1;

  // Assigns the handle to one of the channels in the mix/output buffer.
  channels[cnum].handle = I_StartSound(sfx, cnum, volume, sep, pitch, priority);
}

void S_StartSound(const mobj_t *origin, int sfx_id)
{
  S_StartSfxInfo(origin, &S_sfx[sfx_id], sfx_id);
}

void S_StartSoundName(const mobj_t *origin, char *name)
{
  sfxinfo_t *sfx;
  
  if(!(sfx = S_SfxInfoForName(name)))
    doom_printf("sound not found: %s\n", name);
  else
    S_StartSfxInfo(origin, sfx, 0);
}

void S_StopSound(const mobj_t *origin)
{
  int cnum;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm || !sound_initted)
    return;

  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
      {
        S_StopChannel(cnum);
        break;
      }
}


//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound(void)
{
  if (sound_initted && mus_playing && !mus_paused)
    {
      I_PauseSong(mus_playing->handle);
      mus_paused = true;
    }
}

void S_ResumeSound(void)
{
  if (sound_initted && mus_playing && mus_paused)
    {
      I_ResumeSong(mus_playing->handle);
      mus_paused = false;
    }
}

//
// Updates music & sounds
//

void S_UpdateSounds(const mobj_t* listener)
{
  int cnum;

  //jff 1/22/98 return if sound is not enabled
  if (!sound_initted || !snd_card || nosfxparm)
    return;

  for (cnum=0 ; cnum<numChannels ; cnum++)
    {
      channel_t *c = &channels[cnum];
      sfxinfo_t *sfx = c->sfxinfo;

      if (sfx)
        {
          if (I_SoundIsPlaying(c->handle))
            {
              // initialize parameters
              int volume = snd_SfxVolume;
              int pitch = NORM_PITCH;
              int sep = NORM_SEP;

              if (sfx->link)
                {
                  pitch = sfx->pitch;
                  volume += sfx->volume;
                  if (volume < 1)
                    {
                      S_StopChannel(cnum);
                      continue;
                    }
                  else
                    if (volume > snd_SfxVolume)
                      volume = snd_SfxVolume;
                }

#ifdef COMPILE_VIDD
              if (VIDD_PLAY_inProgress()) pitch = VIDD_PLAY_getSoundPitch(); // POPE
#endif

              // check non-local sounds for distance clipping
              // or modify their params

	      // sf: removed check for if
	      // listener is source, for silencing sectors
	      // sf again: use external camera if there is one
	      // fix afterglows bug: segv because of NULL listener

              if (c->origin) { // killough 3/20/98
                if (!S_AdjustSoundParams(listener, c->origin,
                                         &volume, &sep, &pitch))
                  S_StopChannel(cnum);
                else
                  I_UpdateSoundParams(c->handle, volume, sep, pitch);
	      }
            }
          else   // if channel is allocated but sound has stopped, free it
            S_StopChannel(cnum);
        }
    }
}

void S_SetMusicVolume(int volume)
{
  //jff 1/22/98 return if music is not enabled
  if (!sound_initted || !mus_card || nomusicparm)
    return;
  if (volume < 0 || volume > 15)
    I_Error("S_SetMusicVolume: Attempt to set music volume at %d", volume);
  I_SetMusicVolume(volume);
  snd_MusicVolume = volume;
}

void S_SetSfxVolume(int volume)
{
  //jff 1/22/98 return if sound is not enabled
  if (!sound_initted || !snd_card || nosfxparm)
    return;
  if (volume < 0 || volume > 127)
    I_Error("S_SetSfxVolume: Attempt to set sfx volume at %d", volume);
  snd_SfxVolume = volume;
}

// sf: created changemusicnum, not limited to original musics
// change by music number
// removed mus_new

void S_ChangeMusicNum(int musnum, int looping)
{
  musicinfo_t *music;
  
  if (musnum <= mus_None || musnum >= NUMMUSIC)
    {
      doom_printf("Bad music number %d\n", musnum);
      return;
    }

  music = &S_music[musnum];
    
  S_ChangeMusic(music, looping);
}

// change by name
void S_ChangeMusicName(char *name, int looping)
{
  musicinfo_t *music;

  music = S_MusicForName(name);

  if(music)
    S_ChangeMusic(music, looping);
  else
    {
      doom_printf("music not found: %s\n", name);    
      S_StopMusic(); // stop music anyway
    }
}

void S_ChangeMusic(musicinfo_t *music, int looping)
{
  int music_file_failed; // cournia - if true load the default MIDI music
  char* music_filename;  // cournia

  //jff 1/22/98 return if music is not enabled
  if (!sound_initted || !mus_card || nomusicparm)
    return;

  // same as the one playing ?

  if (mus_playing == music)
    return;

  // shutdown old music
  S_StopMusic();

  // get lumpnum if neccessary
  if (!music->lumpnum)
    {
      char namebuf[9];
      sprintf(namebuf, "d_%s", music->name);
      music->lumpnum = W_GetNumForName(namebuf);
    }

  music_file_failed = 1;
/*
  // proff_fs - only load when from IWAD
  if (lumpinfo[music->lumpnum].source == source_iwad)
    {
      // cournia - check to see if we can play a higher quality music file
      //           rather than the default MIDI
      music_filename = I_FindFile((const char *)S_music_files[musicnum], "");
      if (music_filename)
        {
          music_file_failed = I_RegisterMusic(music_filename, music);
          free(music_filename);
        }
    }
*/
  if (music_file_failed)
    {
      //cournia - could not load music file, play default MIDI music

      // load & register it
      music->data = W_CacheLumpNum(music->lumpnum);
      music->handle = I_RegisterSong(music->data, W_LumpLength(music->lumpnum));
    }

  // play it
  I_PlaySong(music->handle, looping);

  mus_playing = music;
}

// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(int m_id)
{
  S_ChangeMusicNum(m_id, false);
}

// start a random mus

void S_StartRandomMusic()
{
  int mnum;
  
  if (gamemode == commercial)
    mnum = mus_runnin + M_Random() % 32;
  else
    mnum = mus_e1m1 + M_Random() % 9;

  S_StartMusic(mnum);
}

void S_StopMusic(void)
{
  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  if (mus_playing)
    {
      if (mus_paused)
        I_ResumeSong(mus_playing->handle);

      I_StopSong(mus_playing->handle);
      I_UnRegisterSong(mus_playing->handle);
       if (mus_playing->lumpnum >= 0)
     W_UnlockLumpNum(mus_playing->lumpnum); // cph - release the music data

      mus_playing->data = 0;
      mus_playing = 0;
    }
}


void S_StopSounds()
{
  int cnum;

  if(!sound_initted)
    return;


  // kill all playing sounds at start of level
  //  (trust me - a good idea)

  //jff 1/22/98 skip sound init if sound not enabled
  if (snd_card && !nosfxparm)
  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo)
      S_StopChannel(cnum);
}

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
  int mnum;

  S_StopSounds();

  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  // start new music for the level
  mus_paused = 0;

  if(!*info_music && gamemap==0)
    {
      // dont know what music to play
      // we need a default
      info_music = gamemode == commercial ? "runnin" : "e1m1";
    }
  
  // sf: replacement music
  if(*info_music)
    S_ChangeMusicName(info_music, true);
  else
    {
  if (idmusnum!=-1)
    mnum = idmusnum; //jff 3/17/98 reload IDMUS music if not -1
  else
    if (gamemode == commercial)
      mnum = mus_runnin + gamemap - 1;
    else
      {
        static const int spmus[] =     // Song - Who? - Where?
        {
          mus_e3m4,     // American     e4m1
          mus_e3m2,     // Romero       e4m2
          mus_e3m3,     // Shawn        e4m3
          mus_e1m5,     // American     e4m4
          mus_e2m7,     // Tim  e4m5
          mus_e2m4,     // Romero       e4m6
          mus_e2m6,     // J.Anderson   e4m7 CHIRON.WAD
          mus_e2m5,     // Shawn        e4m8
          mus_e1m9      // Tim          e4m9
        };

        if (gameepisode < 4)
          mnum = mus_e1m1 + (gameepisode-1)*9 + gamemap-1;
        else
          mnum = spmus[gamemap-1];
      }
     
      // start music
  S_ChangeMusicNum(mnum, true);
}
}

void S_InvalidateCache(void)
{
  int i;
  
  // Note that sounds have not been cached (yet).
  for (i=1 ; i<NUMSFX ; i++)
    S_sfx[i].lumpnum = S_sfx[i].usefulness = -1;
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//

void S_Init(int sfxVolume, int musicVolume)
{
  sound_initted = true;  

  S_CreateSoundHashTable();

  //jff 1/22/98 skip sound init if sound not enabled
  numChannels = default_numChannels;
  if (snd_card && !nosfxparm)
  {
      C_Printf("\tdefault sfx volume %d\n", sfxVolume);  // killough 8/8/98

    I_SetChannels();

    S_SetSfxVolume(sfxVolume);

    // Allocating the internal channels for mixing
    // (the maximum numer of sounds rendered
    // simultaneously) within zone memory.
    // CPhipps - calloc
    channels = (channel_t *) calloc(numChannels,sizeof(channel_t));

    S_InvalidateCache();
  }

  // CPhipps - music init reformatted
  if (mus_card && !nomusicparm) {
    S_SetMusicVolume(musicVolume);

    // no sounds are playing, and they are not mus_paused
    mus_paused = 0;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Sound Hashing
//

int soundhash_created = false;  // set to 1 when created

        // store sfxinfo_t in hashchain
void S_StoreSfxInfo(sfxinfo_t *sfxinfo)
{
  int hashnum = sound_hash(sfxinfo->name);
  
  if(!soundhash_created)
    {
      S_CreateSoundHashTable();
      hashnum = sound_hash(sfxinfo->name);
    }
  
  // hook it in

  sfxinfo->next = sfxinfos[hashnum];
  sfxinfos[hashnum] = sfxinfo;
}

static void S_CreateSoundHashTable()
{
  int i;
  
  if(soundhash_created) return;   // already done
  soundhash_created = true;    // set here to prevent recursive calls
  
  // clear hash slots first

  for(i=0; i<SOUND_HASHSLOTS; i++)
    sfxinfos[i] = 0;

  for(i=1; i<NUMSFX; i++)
    {
      S_StoreSfxInfo(&S_sfx[i]);
    }
}

sfxinfo_t *S_SfxInfoForName(char *name)
{
   sfxinfo_t *si;
 
   if(!soundhash_created)
     {
       S_CreateSoundHashTable();
     }
   
   si = sfxinfos[sound_hash(name)];
   
   while(si)
     {
       if(!strcasecmp(name, si->name)) return si;
       si = si->next;
     }
   
   return NULL;
}

void S_Chgun()
{
  memcpy(&S_sfx[sfx_chgun], &chgun, sizeof(sfxinfo_t));
  S_sfx[sfx_chgun].data = NULL;
}

// free sound and reload
// also check to see if a new sound name has been found
// (ie. not one in the original game). If so, we create
// a new sfxinfo_t and hook it into the hashtable for use
// by scripting and skins

// NOTE: LUMPNUM NOT SOUNDNUM
void S_UpdateSound(int lumpnum)
{
  sfxinfo_t *sfx;
  char name[8];
  
  strncpy(name,lumpinfo[lumpnum].name+2,6);
  name[6] = 0;
  
  sfx = S_SfxInfoForName(name);
  
  if(!sfx)
    {
      // create a new one and hook into hashchain
      
      sfx = Z_Malloc(sizeof(sfxinfo_t), PU_STATIC, 0);
      
      sfx->name = strdup(name);
      sfx->singularity = sg_none;
      sfx->priority = 64;
      sfx->link = NULL;
      sfx->pitch = sfx->volume = -1;
      sfx->data = NULL;
      S_StoreSfxInfo(sfx);
    }
  
  if(sfx->data)
    {
      // free it if cached
      Z_Free(sfx->data);      // free
      sfx->data = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Music Hashing
//

static boolean mushash_created = false;

static void S_HookMusic(musicinfo_t *music)
{
  int hashslot;

  if(!music || !music->name) return;
  
  hashslot = sound_hash(music->name);

  music->next = musicinfos[hashslot];
  musicinfos[hashslot] = music;
}

static void S_CreateMusicHashTable()
{
  int i;

  // only build once
  
  if(mushash_created)
    return;
  else
    mushash_created = true;

  for(i=0; i<SOUND_HASHSLOTS; i++)
    musicinfos[i] = NULL;
  
  // hook in all musics
  for(i=0; i<NUMMUSIC; i++)
    {
      S_HookMusic(&S_music[i]);
    }
}

musicinfo_t *S_MusicForName(char *name)
{
  int hashnum = sound_hash(name);
  musicinfo_t *mus;

  if(!mushash_created)
    S_CreateMusicHashTable();

  for(mus=musicinfos[hashnum]; mus; mus = mus->next)
    {
      if(!strcasecmp(name, mus->name))
	return mus;
    }
    
  return NULL;
}

void S_UpdateMusic(int lumpnum)
{
  musicinfo_t *music;
  char sndname[8];

  strncpy(sndname, lumpinfo[lumpnum].name + 2, 6);

  // check if one already in the table first

  music = S_MusicForName(sndname);

  if(!music)         // not found in list
    {
      // build a new musicinfo_t
      music = Z_Malloc(sizeof(*music), PU_STATIC, 0);
      music->name = Z_Strdup(sndname, PU_STATIC, 0);

      // hook into hash list
      S_HookMusic(music);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

void S_ResetVolume()
{
  S_SetMusicVolume(snd_MusicVolume);
  S_SetSfxVolume(snd_SfxVolume);
}

CONSOLE_BOOLEAN(s_pitched, pitched_sounds, NULL,   onoff, 0) {}
CONSOLE_INT(snd_channels, default_numChannels, NULL, 1, 32, NULL, 0)
{
	I_InitSound();
}
CONSOLE_INT(sfx_volume, snd_SfxVolume, NULL,         0, 15, NULL, 0)
{
  S_ResetVolume();
}
CONSOLE_INT(music_volume, snd_MusicVolume, NULL,     0, 15, NULL, 0)
{
  S_ResetVolume();
}

void S_AddCommands()
{
	S_music_files[mus_None] = Z_Strdup("", PU_STATIC, 0);
	S_music_files[mus_e1m1] = Z_Strdup("e1m1.mp3", PU_STATIC, 0);
	S_music_files[mus_e1m2] = Z_Strdup("e1m2.mp3", PU_STATIC, 0);
	S_music_files[mus_e1m3] = Z_Strdup("e1m3.mp3", PU_STATIC, 0);
	S_music_files[mus_e1m4] = Z_Strdup("e1m4.mp3", PU_STATIC, 0);
	S_music_files[mus_e1m5] = Z_Strdup("e1m5.mp3", PU_STATIC, 0);
	S_music_files[mus_e1m6] = Z_Strdup("e1m6.mp3", PU_STATIC, 0);
	S_music_files[mus_e1m7] = Z_Strdup("e1m7.mp3", PU_STATIC, 0);
	S_music_files[mus_e1m8] = Z_Strdup("e1m8.mp3", PU_STATIC, 0);
	S_music_files[mus_e1m9] = Z_Strdup("e1m9.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m1] = Z_Strdup("e2m1.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m2] = Z_Strdup("e2m2.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m3] = Z_Strdup("e2m3.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m4] = Z_Strdup("e2m4.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m5] = Z_Strdup("e1m7.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m6] = Z_Strdup("e2m6.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m7] = Z_Strdup("e2m7.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m8] = Z_Strdup("e2m8.mp3", PU_STATIC, 0);
	S_music_files[mus_e2m9] = Z_Strdup("e3m1.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m1] = Z_Strdup("e3m1.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m2] = Z_Strdup("e3m2.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m3] = Z_Strdup("e3m3.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m4] = Z_Strdup("e1m8.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m5] = Z_Strdup("e1m7.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m6] = Z_Strdup("e1m6.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m7] = Z_Strdup("e2m7.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m8] = Z_Strdup("e3m8.mp3", PU_STATIC, 0);
	S_music_files[mus_e3m9] = Z_Strdup("e1m9.mp3", PU_STATIC, 0);
	S_music_files[mus_inter] = Z_Strdup("e2m3.mp3", PU_STATIC, 0);
	S_music_files[mus_intro] = Z_Strdup("intro.mp3", PU_STATIC, 0);
	S_music_files[mus_bunny] = Z_Strdup("bunny.mp3", PU_STATIC, 0);
	S_music_files[mus_victor] = Z_Strdup("victor.mp3", PU_STATIC, 0);
	S_music_files[mus_introa] = Z_Strdup("intro.mp3", PU_STATIC, 0);
	S_music_files[mus_runnin] = Z_Strdup("runnin.mp3", PU_STATIC, 0);
	S_music_files[mus_stalks] = Z_Strdup("stalks.mp3", PU_STATIC, 0);
	S_music_files[mus_countd] = Z_Strdup("countd.mp3", PU_STATIC, 0);
	S_music_files[mus_betwee] = Z_Strdup("betwee.mp3", PU_STATIC, 0);
	S_music_files[mus_doom] = Z_Strdup("doom.mp3", PU_STATIC, 0);
	S_music_files[mus_the_da] = Z_Strdup("the_da.mp3", PU_STATIC, 0);
	S_music_files[mus_shawn] = Z_Strdup("shawn.mp3", PU_STATIC, 0);
	S_music_files[mus_ddtblu] = Z_Strdup("ddtblu.mp3", PU_STATIC, 0);
	S_music_files[mus_in_cit] = Z_Strdup("in_cit.mp3", PU_STATIC, 0);
	S_music_files[mus_dead] = Z_Strdup("dead.mp3", PU_STATIC, 0);
	S_music_files[mus_stlks2] = Z_Strdup("stalks.mp3", PU_STATIC, 0);
	S_music_files[mus_theda2] = Z_Strdup("the_da.mp3", PU_STATIC, 0);
	S_music_files[mus_doom2] = Z_Strdup("doom.mp3", PU_STATIC, 0);
	S_music_files[mus_ddtbl2] = Z_Strdup("ddtblu.mp3", PU_STATIC, 0);
	S_music_files[mus_runni2] = Z_Strdup("runnin.mp3", PU_STATIC, 0);
	S_music_files[mus_dead2] = Z_Strdup("dead.mp3", PU_STATIC, 0);
	S_music_files[mus_stlks3] = Z_Strdup("stalks.mp3", PU_STATIC, 0);
	S_music_files[mus_romero] = Z_Strdup("romero.mp3", PU_STATIC, 0);
	S_music_files[mus_shawn2] = Z_Strdup("shawn.mp3", PU_STATIC, 0);
	S_music_files[mus_messag] = Z_Strdup("messag.mp3", PU_STATIC, 0);
	S_music_files[mus_count2] = Z_Strdup("countd.mp3", PU_STATIC, 0);
	S_music_files[mus_ddtbl3] = Z_Strdup("ddtblu.mp3", PU_STATIC, 0);
	S_music_files[mus_ampie] = Z_Strdup("ampie.mp3", PU_STATIC, 0);
	S_music_files[mus_theda3] = Z_Strdup("the_da.mp3", PU_STATIC, 0);
	S_music_files[mus_adrian] = Z_Strdup("adrian.mp3", PU_STATIC, 0);
	S_music_files[mus_messg2] = Z_Strdup("messag.mp3", PU_STATIC, 0);
	S_music_files[mus_romer2] = Z_Strdup("romero.mp3", PU_STATIC, 0);
	S_music_files[mus_tense] = Z_Strdup("tense.mp3", PU_STATIC, 0);
	S_music_files[mus_shawn3] = Z_Strdup("shawn.mp3", PU_STATIC, 0);
	S_music_files[mus_openin] = Z_Strdup("openin.mp3", PU_STATIC, 0);
	S_music_files[mus_evil] = Z_Strdup("evil.mp3", PU_STATIC, 0);
	S_music_files[mus_ultima] = Z_Strdup("ultima.mp3", PU_STATIC, 0);
	S_music_files[mus_read_m] = Z_Strdup("read_m.mp3", PU_STATIC, 0);
	S_music_files[mus_dm2ttl] = Z_Strdup("dm2ttl.mp3", PU_STATIC, 0);
	S_music_files[mus_dm2int] = Z_Strdup("dm2int.mp3", PU_STATIC, 0);
	C_AddCommand(s_pitched);
	C_AddCommand(snd_channels);
	C_AddCommand(sfx_volume);
	C_AddCommand(snd_samplerate);
	C_AddCommand(music_volume);
	C_AddCommand(mus_e1m1);
	C_AddCommand(mus_e1m2);
	C_AddCommand(mus_e1m3);
	C_AddCommand(mus_e1m4);
	C_AddCommand(mus_e1m5);
	C_AddCommand(mus_e1m6);
	C_AddCommand(mus_e1m7);
	C_AddCommand(mus_e1m8);
	C_AddCommand(mus_e1m9);
	C_AddCommand(mus_e2m1);
	C_AddCommand(mus_e2m2);
	C_AddCommand(mus_e2m3);
	C_AddCommand(mus_e2m4);
	C_AddCommand(mus_e2m5);
	C_AddCommand(mus_e2m6);
	C_AddCommand(mus_e2m7);
	C_AddCommand(mus_e2m8);
	C_AddCommand(mus_e2m9);
	C_AddCommand(mus_e3m1);
	C_AddCommand(mus_e3m2);
	C_AddCommand(mus_e3m3);
	C_AddCommand(mus_e3m4);
	C_AddCommand(mus_e3m5);
	C_AddCommand(mus_e3m6);
	C_AddCommand(mus_e3m7);
	C_AddCommand(mus_e3m8);
	C_AddCommand(mus_e3m9);
	C_AddCommand(mus_inter);
	C_AddCommand(mus_intro);
	C_AddCommand(mus_bunny);
	C_AddCommand(mus_victor);
	C_AddCommand(mus_introa);
	C_AddCommand(mus_runnin);
	C_AddCommand(mus_stalks);
	C_AddCommand(mus_countd);
	C_AddCommand(mus_betwee);
	C_AddCommand(mus_doom);
	C_AddCommand(mus_the_da);
	C_AddCommand(mus_shawn);
	C_AddCommand(mus_ddtblu);
	C_AddCommand(mus_in_cit);
	C_AddCommand(mus_dead);
	C_AddCommand(mus_stlks2);
	C_AddCommand(mus_theda2);
	C_AddCommand(mus_doom2);
	C_AddCommand(mus_ddtbl2);
	C_AddCommand(mus_runni2);
	C_AddCommand(mus_dead2);
	C_AddCommand(mus_stlks3);
	C_AddCommand(mus_romero);
	C_AddCommand(mus_shawn2);
	C_AddCommand(mus_messag);
	C_AddCommand(mus_count2);
	C_AddCommand(mus_ddtbl3);
	C_AddCommand(mus_ampie);
	C_AddCommand(mus_theda3);
	C_AddCommand(mus_adrian);
	C_AddCommand(mus_messg2);
	C_AddCommand(mus_romer2);
	C_AddCommand(mus_tense);
	C_AddCommand(mus_shawn3);
	C_AddCommand(mus_openin);
	C_AddCommand(mus_evil);
	C_AddCommand(mus_ultima);
	C_AddCommand(mus_read_m);
	C_AddCommand(mus_dm2ttl);
	C_AddCommand(mus_dm2int);
}


