// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_sound.c,v 1.1 2000/04/09 18:03:40 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      System interface for sound.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: I_sound.c,v 1.1 2000/04/09 18:03:40 proff_fs Exp $";

// proff 07/04/98: Changed from _MSC_VER to _WIN32 for CYGWIN32 compatibility
#ifdef _WIN32 // proff: Sound-routines using DirectSound
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
// proff 07/04/98: Added for CYGWIN32 compatibility
#if defined (_MSC_VER) || defined (__MINGW32__)
#include <mmsystem.h>
#endif
#include "doomtype.h"
// proff 07/04/98: Added for CYGWIN32 compatibility
#if defined (_MSC_VER) || defined (__MINGW32__)
#define DIRECTX
#define MCI_MIDI    1
#define STREAM_MIDI 2
#endif
#if defined (DIRECTX) && !defined (__MINGW32__)
#define __BYTEBOOL__
#define false 0
#define true !false
#endif
#if defined (DIRECTX)
#include <dsound.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "mmus2mid.h"
#include "i_sound.h"
#include "w_wad.h"
#include "m_misc.h"
// proff 11/21/98: Added DirectSound device selection
#include "m_argv.h"
#include "lprintf.h"

int snd_card;
int snd_freq;
int snd_bits;
int snd_stereo;
int snd_dsounddevice=0;
int snd_mididevice=0;
int mus_card;
extern H_boolean nosfxparm, nomusicparm;
extern HWND ghWnd;
int used_mus_card;

void I_CheckMusic();

// proff 07/02/98: Moved music-varibles down to music-functions

extern  int  numChannels;
extern  int  default_numChannels;

#define DS_VOLRANGE 2000
#define DS_PANRANGE 3000
#define DS_PITCHRANGE 1000
// proff 07/09/98: Added these macros to simplify the functions
#define SEP(x)   ((x-128)*DS_PANRANGE/128)
#define VOL(x)   ((x*DS_VOLRANGE/15)-DS_VOLRANGE)
#define PITCH(x) (pitched_sounds ? ((x-128)*DS_PITCHRANGE/128) : 0)

H_boolean noDSound = true;

// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef DIRECTX
LPDIRECTSOUND lpDS;
LPDIRECTSOUNDBUFFER lpPrimaryDSB;
LPDIRECTSOUNDBUFFER *lpSecondaryDSB;
  // proff 11/21/98: Added DirectSound device selection
LPGUID DSDeviceGUIDs[16];
int DSDeviceCount=0;
int DSoundDevice=0;

typedef struct {
  int id;
  int samplerate;
  int endtime;
  int playing;
} channel_info_t;

channel_info_t *ChannelInfo;

static HRESULT CreateSecondaryBuffer(LPDIRECTSOUNDBUFFER *lplpDsb, int size)
{
    PCMWAVEFORMAT pcmwf;
    DSBUFFERDESC dsbdesc;

    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
    pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.wf.nChannels = 1;
    pcmwf.wf.nSamplesPerSec = 11025;
    pcmwf.wf.nBlockAlign = 1;
    pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec;
    pcmwf.wBitsPerSample = 8;

    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_STATIC;
    dsbdesc.dwBufferBytes = size; 
    dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;

    return IDirectSound_CreateSoundBuffer(lpDS,&dsbdesc, lplpDsb, NULL);
}

static HRESULT CreatePrimaryBuffer(void)
{
  DSBUFFERDESC dsbdesc;
// proff 07/23/98: Added WAVEFORMATEX and HRESULT
  WAVEFORMATEX wf;
  HRESULT result;

  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER; 

  memset(&wf, 0, sizeof(WAVEFORMATEX));
  if (snd_bits!=16)
    snd_bits=8;
// proff 07/23/98: Added wf
  wf.wFormatTag = WAVE_FORMAT_PCM;
  if (snd_stereo!=0)
    wf.nChannels = 2;
  else
    wf.nChannels = 1;
  wf.wBitsPerSample = snd_bits;
  wf.nSamplesPerSec = snd_freq;
  wf.nBlockAlign = wf.nChannels*wf.wBitsPerSample/8;
  wf.nAvgBytesPerSec = wf.nSamplesPerSec*wf.nBlockAlign;
    
  result=IDirectSound_CreateSoundBuffer(lpDS,&dsbdesc, &lpPrimaryDSB, NULL);
// proff 07/23/98: Added wf and result
  if (result == DS_OK)
    result=IDirectSoundBuffer_SetFormat(lpPrimaryDSB,&wf);
  if (result == DS_OK)
    result=IDirectSoundBuffer_Play(lpPrimaryDSB,0,0,DSBPLAY_LOOPING);
  return result;
}
#endif // DIRECTX

void I_SetChannels()
{
}

int I_GetSfxLumpNum (sfxinfo_t* sfx)
{
  char namebuf[9];
  sprintf(namebuf, "ds%s", sfx->name);
  return W_CheckNumForName(namebuf);
}

int I_StartSound(int id, int channel, char *snddata,
         int vol, int sep, int pitch, int priority )
{
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef DIRECTX
  HRESULT error;
  char *hand1,*hand2;
  int len1,len2;
  char *sound_data;
  int length;

  if (noDSound == true)
    return channel;
// proff 07/26/98: Added volume check
// proff 10/31/98: Added Stop before updating sound-data
  error=IDirectSoundBuffer_Stop(lpSecondaryDSB[channel]);
  ChannelInfo[channel].playing=false;
  if (vol==0)
    return channel;
  sound_data=&snddata[8];
  length=(int)(((unsigned short *)snddata)[2]);
  ChannelInfo[channel].samplerate = (snddata[3]<<8)+snddata[2];
// proff 10/31/98: Use accurate time for this one
  ChannelInfo[channel].endtime = I_GetTrueTime() + (length * 1000 / ChannelInfo[channel].samplerate);
  error=IDirectSoundBuffer_SetCurrentPosition(lpSecondaryDSB[channel],0);
  // proff 11/09/98: Added for a slight speedup
  if (id!=ChannelInfo[channel].id)
  {
    ChannelInfo[channel].id=id;
    error=IDirectSoundBuffer_Lock(lpSecondaryDSB[channel],0,65535,&hand1,&len1,&hand2,&len2,DSBLOCK_FROMWRITECURSOR);
    memset(hand1,128,len1);
    if (len1<65534)
    {
      memset(hand2,128,len2);
      if (len1>=length) 
        memcpy(hand1,sound_data,length);
      else 
      {
        memcpy(hand1,sound_data,len1);
        memcpy(hand2,&sound_data[len1],length-len1);
      }
    }
    else
      memcpy(hand1,sound_data,length);
    error=IDirectSoundBuffer_Unlock(lpSecondaryDSB[channel],hand1,len1,hand2,len2);
  }
  error=IDirectSoundBuffer_SetVolume(lpSecondaryDSB[channel],VOL(vol));
  error=IDirectSoundBuffer_SetPan(lpSecondaryDSB[channel],SEP(sep));
  error=IDirectSoundBuffer_SetFrequency(lpSecondaryDSB[channel],ChannelInfo[channel].samplerate+PITCH(pitch));
  error=IDirectSoundBuffer_Play(lpSecondaryDSB[channel],0,0,0);
  ChannelInfo[channel].playing=true;
#endif // DIRECTX
  return channel;
}

void I_StopSound(int channel)
{
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef DIRECTX
  IDirectSoundBuffer_Stop(lpSecondaryDSB[channel]);
  ChannelInfo[channel].playing=false;
#endif // DIRECTX
}

void I_UpdateSoundParams(int channel, int vol, int sep, int pitch)
{
  int DSB_Status;
  if (noDSound == true)
    return;
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef DIRECTX
// proff 07/26/98: Added volume check
  if (vol==0)
  {
    IDirectSoundBuffer_Stop(lpSecondaryDSB[channel]);
    return;
  }
  IDirectSoundBuffer_SetVolume(lpSecondaryDSB[channel],VOL(vol));
  IDirectSoundBuffer_SetPan(lpSecondaryDSB[channel],SEP(sep));
  IDirectSoundBuffer_SetFrequency(lpSecondaryDSB[channel],ChannelInfo[channel].samplerate+PITCH(pitch));
  if (ChannelInfo[channel].playing==true)
  {
    IDirectSoundBuffer_GetStatus(lpSecondaryDSB[channel],&DSB_Status);
    if ((DSB_Status & DSBSTATUS_PLAYING) == 0)
      IDirectSoundBuffer_Play(lpSecondaryDSB[channel],0,0,0);
  }
#endif // DIRECTX
}

int I_SoundIsPlaying(int channel)
{
// proff 07/14/98: Added this because ChannelInfo is not initialized when nosound
  if (noDSound == true)
    return false;
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef DIRECTX
// proff 10/31/98: Use accurate time for this one
  ChannelInfo[channel].playing=(I_GetTrueTime()<=ChannelInfo[channel].endtime);
  return ChannelInfo[channel].playing;
#else // DIRECTX
  return true;
#endif // DIRECTX
}

void I_UpdateSound( void )
{
  I_CheckMusic();
}

void I_SubmitSound(void)
{
}

void I_ShutdownSound(void)
{
  lprintf(LO_INFO,"I_ShutdownSound: ");
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef DIRECTX
  if (lpDS)
  {
    IDirectSound_Release(lpDS);
    lprintf(LO_INFO,"released DirectSound\n");
  }
#endif // DIRECTX
}

#ifdef DIRECTX
static HRESULT WINAPI DSEnumCallback(LPGUID lpGUID, LPCSTR lpcstrDescription,
                    LPCSTR lpcstrModule, LPVOID lpContext)
{
  if (DSDeviceCount==16)
    return FALSE;
  lprintf(LO_INFO,"  Device %i: %s\n",DSDeviceCount,lpcstrDescription);
  DSDeviceGUIDs[DSDeviceCount]=lpGUID;
  DSDeviceCount++;
  return TRUE;
}
#endif // DIRECTX

void I_InitSound(void)
{
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef DIRECTX
  HRESULT error;
  int c;
  int i;
#endif // DIRECTX

// proff 07/01/98: Added I_InitMusic
  if (!nomusicparm)
    I_InitMusic();
  if (nosfxparm)
    return;
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef DIRECTX
  // proff 11/21/98: Added DirectSound device selection
  i = M_CheckParm ("-dsounddevice");
  if ((i) || (snd_dsounddevice))
  {
    lprintf(LO_INFO,"I_InitSound: Sound Devices\n");
    DirectSoundEnumerate(&DSEnumCallback,NULL);
    DSoundDevice=snd_dsounddevice;
    if (i)
      DSoundDevice=atoi(myargv[i+1]);
    DSoundDevice=(DSoundDevice<0) ? 0 : ((DSoundDevice>=DSDeviceCount) ? 0 : DSoundDevice);
  }
  lprintf(LO_INFO,"I_InitSound: ");
  error = DirectSoundCreate(DSDeviceGUIDs[DSoundDevice],&lpDS,NULL);
  // proff 11/21/98: End of additions and changes
  if (error == DSERR_NODRIVER)
  {
    lpDS = NULL;
    lprintf(LO_WARN,"no sounddevice found\n");
    noDSound = true;
    return;
  }
  noDSound = false;
  if (error != DS_OK)
  {
    noDSound = true;
    return;
  }
  lprintf(LO_INFO,"created DirectSound. Selected Device: %i\n",DSoundDevice);
    atexit(I_ShutdownSound); 
  error = IDirectSound_SetCooperativeLevel(lpDS,ghWnd,DSSCL_EXCLUSIVE);
  if (error != DS_OK)
  {
    noDSound = true;
    return;
  }
  lprintf(LO_INFO,"I_InitSound: ");
  lprintf(LO_INFO,"CooperativeLevel set\n");
  lprintf(LO_INFO,"I_InitSound: ");
  error = CreatePrimaryBuffer();
  if (error != DS_OK)
  {
    noDSound = true;
    return;
  }
  numChannels = default_numChannels;
  lpSecondaryDSB=malloc(sizeof(LPDIRECTSOUNDBUFFER)*numChannels);
  if (lpSecondaryDSB)
  {
    memset(lpSecondaryDSB,0,sizeof(LPDIRECTSOUNDBUFFER)*numChannels);
    lprintf (LO_INFO, "Channels : %i\n", numChannels);
  }
  else
  {
    noDSound = true;
    return;
  }
  ChannelInfo=malloc(sizeof(channel_info_t)*numChannels);
  if (ChannelInfo)
  // proff 11/09/98: Added for security
  {
    memset(ChannelInfo,0,sizeof(channel_info_t)*numChannels);
  }
  else
  {
    noDSound = true;
    return;
  }
  for (c=0; c<numChannels; c++)
  {
    error = CreateSecondaryBuffer(&lpSecondaryDSB[c],65535);
    if (error != DS_OK)
    {
      noDSound = true;
      return;
    }
  }
#endif // DIRECTX
}

static MIDI mididata;
// proff: 07/26/98: changed to static
static int MusicLoaded=0;
static int MusicLoop=0;

#ifdef STREAM_MIDI
static UINT MidiDevice;
static HMIDISTRM hMidiStream;
static MIDIEVENT *MidiEvents[MIDI_TRACKS];
static MIDIHDR MidiStreamHdr;
static MIDIEVENT *NewEvents;
static int NewSize;
static int NewPos;
static int BytesRecorded[MIDI_TRACKS];
static int BufferSize[MIDI_TRACKS];
static int CurrentTrack;
static int CurrentPos;

static int getvl(void)
{
  int l=0;
  byte c;
  for (;;)
  {
    c=mididata.track[CurrentTrack].data[CurrentPos];
    CurrentPos++;
    l += (c & 0x7f);
    if (!(c & 0x80)) 
      return l;
    l<<=7;
  }
}

static void AddEvent(DWORD at, DWORD type, byte event, byte a, byte b)
{
  MIDIEVENT *CurEvent;

  if ((BytesRecorded[CurrentTrack]+(int)sizeof(MIDIEVENT))>=BufferSize[CurrentTrack])
  {
    BufferSize[CurrentTrack]+=100*sizeof(MIDIEVENT);
    MidiEvents[CurrentTrack]=realloc(MidiEvents[CurrentTrack],BufferSize[CurrentTrack]);
  }
  CurEvent=(MIDIEVENT *)((byte *)MidiEvents[CurrentTrack]+BytesRecorded[CurrentTrack]);
  memset(CurEvent,0,sizeof(MIDIEVENT));
  CurEvent->dwDeltaTime=at;
  CurEvent->dwEvent=event+(a<<8)+(b<<16)+(type<<24);
  BytesRecorded[CurrentTrack]+=3*sizeof(DWORD);
}

static void MidiTracktoStream(void)
{
  DWORD atime,len;
  byte event,type,a,b,c;
  byte laststatus,lastchan;

  CurrentPos=0;
  laststatus=0;
  lastchan=0;
  atime=0;
  for (;;)
  {
    if (CurrentPos>=mididata.track[CurrentTrack].len)
      return;
    atime+=getvl();
    event=mididata.track[CurrentTrack].data[CurrentPos];
    CurrentPos++;
    if(event==0xF0 || event == 0xF7) /* SysEx event */
    {
      len=getvl();
      CurrentPos+=len;
    }
    else if(event==0xFF) /* Meta event */
    {
      type=mididata.track[CurrentTrack].data[CurrentPos];
      CurrentPos++;
      len=getvl();
      switch(type)
        {
        case 0x2f:
          return;
        case 0x51: /* Tempo */
          a=mididata.track[CurrentTrack].data[CurrentPos];
          CurrentPos++;
          b=mididata.track[CurrentTrack].data[CurrentPos];
          CurrentPos++;
          c=mididata.track[CurrentTrack].data[CurrentPos];
          CurrentPos++;
          AddEvent(atime, MEVT_TEMPO, c, b, a);
          break;
        default:
          CurrentPos+=len;
          break;
        }
    }
    else
    {
      a=event;
      if (a & 0x80) /* status byte */
      {
        lastchan=a & 0x0F;
        laststatus=(a>>4) & 0x07;
        a=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        a &= 0x7F;
      }
      switch(laststatus)
      {
      case 0: /* Note off */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      case 1: /* Note on */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      case 2: /* Key Pressure */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      case 3: /* Control change */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      case 4: /* Program change */
        a &= 0x7f;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, 0);
        break;

      case 5: /* Channel pressure */
        a &= 0x7f;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, 0);
        break;

      case 6: /* Pitch wheel */
        b=mididata.track[CurrentTrack].data[CurrentPos];
        CurrentPos++;
        b &= 0x7F;
        AddEvent(atime, MEVT_SHORTMSG, (byte)((laststatus<<4)+lastchan+0x80), a, b);
        break;

      default: 
        break;
      }
    }
  }
}

static void BlockOut(void)
{
  MMRESULT err;
  int BlockSize;

  if ((MusicLoaded) && (NewEvents))
  {
    // proff 12/8/98: Added for savety
    midiOutUnprepareHeader(hMidiStream,&MidiStreamHdr,sizeof(MIDIHDR));
    if (NewPos>=NewSize)
      if (MusicLoop)
        NewPos=0;
      else 
        return;
    BlockSize=(NewSize-NewPos);
    if (BlockSize<=0)
      return;
    if (BlockSize>36000)
      BlockSize=36000;
    MidiStreamHdr.lpData=(void *)((byte *)NewEvents+NewPos);
    NewPos+=BlockSize;
    MidiStreamHdr.dwBufferLength=BlockSize;
    MidiStreamHdr.dwBytesRecorded=BlockSize;
    MidiStreamHdr.dwFlags=0;
//    lprintf(LO_DEBUG,"Data: %p, Size: %i\n",MidiStreamHdr.lpData,BlockSize);
    err=midiOutPrepareHeader(hMidiStream,&MidiStreamHdr,sizeof(MIDIHDR));
    if (err!=MMSYSERR_NOERROR)
      return;
    err=midiStreamOut(hMidiStream,&MidiStreamHdr,sizeof(MIDIHDR));
      return;
  }
}

static void MIDItoStream(void)
{
  int BufferPos[MIDI_TRACKS];
  MIDIEVENT *CurEvent;
  MIDIEVENT *NewEvent;
  int lTime;
  int Dummy;
  int Track;

  if (!hMidiStream)
    return;
  NewSize=0;
  for (CurrentTrack=0;CurrentTrack<MIDI_TRACKS;CurrentTrack++)
  {
    MidiEvents[CurrentTrack]=NULL;
    BytesRecorded[CurrentTrack]=0;
    BufferSize[CurrentTrack]=0;
    MidiTracktoStream();
    NewSize+=BytesRecorded[CurrentTrack];
    BufferPos[CurrentTrack]=0;
  }
  NewEvents=realloc(NewEvents,NewSize);
  if (NewEvents)
  {
    NewPos=0;
    while (1)
    {
      lTime=INT_MAX;
      Track=-1;
      for (CurrentTrack=MIDI_TRACKS-1;CurrentTrack>=0;CurrentTrack--)
      {
        if ((BytesRecorded[CurrentTrack]>0) && (BufferPos[CurrentTrack]<BytesRecorded[CurrentTrack]))
          CurEvent=(MIDIEVENT *)((byte *)MidiEvents[CurrentTrack]+BufferPos[CurrentTrack]);
        else 
          continue;
        if ((int)CurEvent->dwDeltaTime<=lTime)
        {
          lTime=CurEvent->dwDeltaTime;
          Track=CurrentTrack;
        }
      }
      if (Track==-1)
        break;
      else
      {
        CurEvent=(MIDIEVENT *)((byte *)MidiEvents[Track]+BufferPos[Track]);
        BufferPos[Track]+=12;
        NewEvent=(MIDIEVENT *)((byte *)NewEvents+NewPos);
        memcpy(NewEvent,CurEvent,12);
        NewPos+=12;
      }
    }
    NewPos=0;
    lTime=0;
    while (NewPos<NewSize)
    {
      NewEvent=(MIDIEVENT *)((byte *)NewEvents+NewPos);
      Dummy=NewEvent->dwDeltaTime;
      NewEvent->dwDeltaTime-=lTime;
      lTime=Dummy;
      NewPos+=12;
    }
    NewPos=0;
    MusicLoaded=1;
    BlockOut();
  }
  for (CurrentTrack=0;CurrentTrack<MIDI_TRACKS;CurrentTrack++)
  {
    if (MidiEvents[CurrentTrack])
      free(MidiEvents[CurrentTrack]);
  }
}

void CALLBACK MidiProc( HMIDIIN hMidi, UINT uMsg, DWORD dwInstance,
                        DWORD dwParam1, DWORD dwParam2 )
{
    switch( uMsg )
    {
    case MOM_DONE:
      if ((MusicLoaded) && ((DWORD)dwParam1 == (DWORD)&MidiStreamHdr))
      {
        BlockOut();
      }
      break;
    default:
      break;
    }
}
#endif // STREAM_MIDI

#ifdef MCI_MIDI
char *SafemciSendString(char *cmd)
{
  static char errora[256];
  int err;

  err = mciSendString(cmd,errora,255,NULL);
  if (err)
  {
    mciGetErrorString(err,errora,256);
        lprintf (LO_DEBUG, "%s\n",errora);
    }
    return errora;
}
#endif // MCI_MIDI

void I_CheckMusic()
{
#ifdef MCI_MIDI
  static int nexttic=0;

  if (used_mus_card!=MCI_MIDI)
    return;
  if (snd_MusicVolume==0)
    return;
  if ((gametic>nexttic) & (MusicLoaded))
  {
    nexttic=gametic+150;
    if(MusicLoop)
      if (_stricmp(SafemciSendString("status doommusic mode"),"stopped")==0)
        SafemciSendString("play doommusic from 0");
  }
#endif // MCI_MIDI
}

void I_PlaySong(int handle, int looping)
{
  if (snd_MusicVolume==0)
    return;
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if ((handle>=0) & (MusicLoaded))
    {
      if (_stricmp(SafemciSendString("status doommusic mode"),"playing")==0)
        return;
      SafemciSendString("play doommusic from 0");
      MusicLoop=looping;
    }
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (hMidiStream)
    {
      MusicLoop=looping;
      midiStreamRestart(hMidiStream);
    }
    break;
#endif
  default:
    break;
  }
}

void I_SetMusicVolume(int volume)
{
  snd_MusicVolume = volume;
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if (snd_MusicVolume == 0)
      I_StopSong(1);
    else
    {
      I_PlaySong(1,MusicLoop);
      if (paused)
        I_PauseSong(1);
    }
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (snd_MusicVolume == 0)
      I_StopSong(1);
    else
    {
      I_PlaySong(1,MusicLoop);
      if (paused)
        I_PauseSong(1);
    }
    break;
#endif
  default:
    break;
  }
}

void I_PauseSong(int handle)
{
  if (paused == 0)
    return;
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if ((handle>=0) & (MusicLoaded))
      SafemciSendString("pause doommusic");
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (hMidiStream)
      midiStreamPause(hMidiStream);
    break;
#endif
  default:
    break;
  }
}

void I_ResumeSong(int handle)
{
  if (paused == 1)
    return;
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if ((handle>=0) & (MusicLoaded))
      SafemciSendString("resume doommusic");
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (hMidiStream)
      midiStreamRestart(hMidiStream);
    break;
#endif
  default:
    break;
  }
}

void I_StopSong(int handle)
{
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    if ((handle>=0) & (MusicLoaded))
      SafemciSendString("stop doommusic");
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (!hMidiStream)
      return;
//    midiStreamPause(hMidiStream);
    midiStreamStop(hMidiStream);
    midiOutReset(hMidiStream);
    break;
#endif
  default:
    break;
  }
}

void I_UnRegisterSong(int handle)
{
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    MusicLoaded=0;
    SafemciSendString("close doommusic");
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    if (hMidiStream)
    {
      MusicLoaded=0;
      midiStreamStop(hMidiStream);
      midiOutReset(hMidiStream);
      midiStreamClose(hMidiStream);
    }
    break;
#endif
  default:
    break;
  }
}

int I_RegisterSong(void *data)
{
#ifdef MCI_MIDI
  UBYTE *mid;
  int midlen;
  char fname[PATH_MAX+1];
  char mcistring[PATH_MAX+1];
  char *D_DoomExeDir(void);
#endif
#ifdef STREAM_MIDI
  MMRESULT merr;
  MIDIPROPTIMEDIV mptd;
#endif
  int err;

//  SafemciSendString("close doommusic");
  if    //jff 02/08/98 add native midi support
    (
     (err=MidiToMIDI(data, &mididata)) &&       // try midi first
     (err=mmus2mid(data, &mididata, 89, 0))     // now try mus
     )
    {
    dprintf("Error loading midi: %d",err);
    return -1;
    }
  switch (used_mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    MIDIToMidi(&mididata,&mid,&midlen);
// proff 07/01/98: Changed filename to prboom.mid
    M_WriteFile(strcat(strcpy(fname,D_DoomExeDir()),"prboom.mid"),mid,midlen);
    sprintf(mcistring,"open %s alias doommusic",fname);
    SafemciSendString(mcistring);
    free(mid);
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    memset(&MidiStreamHdr,0,sizeof(MIDIHDR));
    merr=midiStreamOpen(&hMidiStream,&MidiDevice,1,(DWORD)&MidiProc,0,CALLBACK_FUNCTION);
    if (merr!=MMSYSERR_NOERROR)
      hMidiStream=0;
    if (!hMidiStream)
      return 0;
    mptd.cbStruct=sizeof(MIDIPROPTIMEDIV);
    mptd.dwTimeDiv=mididata.divisions;
    merr=midiStreamProperty(hMidiStream,(LPBYTE)&mptd,MIDIPROP_SET | MIDIPROP_TIMEDIV);
    MIDItoStream();
    break;
#endif
  default:
    break;
  }
  MusicLoaded=1;

  return 0;
}

void I_ShutdownMusic(void)
{
  I_StopSong(1);
}

void I_InitMusic(void)
{
#ifdef STREAM_MIDI
  int i;
  int MidiDeviceCount;
  MIDIOUTCAPS MidiOutCaps;
#endif

  switch (mus_card)
  {
#ifdef MCI_MIDI
  case MCI_MIDI:
    lprintf (LO_INFO, "I_InitMusic: Using MCI-MIDI Device\n");
    used_mus_card=MCI_MIDI;
    break;
#endif
#ifdef STREAM_MIDI
  case STREAM_MIDI:
    lprintf (LO_INFO, "I_InitMusic: Using Stream-MIDI Device\n");
    MidiDeviceCount=midiOutGetNumDevs();
    if (MidiDeviceCount<=0)
    {
      used_mus_card=-1;
      break;
    }
    i = M_CheckParm ("-mididevice");
    if ((i) || (snd_mididevice>0))
    {
      lprintf(LO_INFO, "I_InitMusic: Available MidiDevices\n");
      for (i=-1; i<MidiDeviceCount; i++)
        if (midiOutGetDevCaps(i,&MidiOutCaps,sizeof(MIDIOUTCAPS))==MMSYSERR_NOERROR)
          lprintf(LO_INFO, "  Device %i: %s\n",i,MidiOutCaps.szPname);
        else
          lprintf(LO_INFO, "  Device %i: Error\n",i);
      MidiDevice=MIDI_MAPPER;
      if (snd_mididevice>0)
        MidiDevice=snd_mididevice-1;
      if (i)
        MidiDevice=atoi(myargv[i+1]);
      MidiDevice=(MidiDevice<-1) ? MIDI_MAPPER : ((MidiDevice>=(UINT)MidiDeviceCount) ? MIDI_MAPPER : MidiDevice);
    }
    used_mus_card=STREAM_MIDI;
    break;
#endif
  default:
    lprintf (LO_INFO, "I_InitMusic: Using No MIDI Device\n");
    used_mus_card=-1;
    break;
  }
  atexit(I_ShutdownMusic);
}

#else //_WIN32

#include <stdio.h>
#include <allegro.h>

#include "doomstat.h"
#include "mmus2mid.h"   //jff 1/16/98 declarations for MUS->MIDI converter
#include "i_sound.h"
#include "w_wad.h"
#include "g_game.h"     //jff 1/21/98 added to use dprintf in I_RegisterSong
#include "d_main.h"
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf

// Needed for calling the actual sound output.
#define SAMPLECOUNT             512

// Factor volume is increased before sending to allegro
#define VOLSCALE                16

int snd_card;   // default.cfg variables for digi and midi drives
int mus_card;   // jff 1/18/98
int detect_voices; //jff 3/4/98 enables voice detection prior to install_sound
//jff 1/22/98 make these visible here to disable sound/music on install err

static SAMPLE *raw2SAMPLE(unsigned char *rawdata, int len)
{
  SAMPLE *spl = malloc(sizeof(SAMPLE));
  spl->bits = 8;
  // killough 1/22/98: Get correct frequency
  spl->freq = (rawdata[3]<<8)+rawdata[2];
  spl->len = len;
  spl->priority = 255;
  spl->loop_start = 0;
  spl->loop_end = len;
  spl->param = -1;
  spl->data = rawdata + 8;
  _go32_dpmi_lock_data(rawdata+8, len);   // killough 3/8/98: lock sound data
  return spl;
}

//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
static void *getsfx(char *sfxname, int *len)
{
  unsigned char *sfx, *paddedsfx;
  int  i;
  int  size;
  int  paddedsize;
  char name[20];
  int  sfxlump;

  // Get the sound data from the WAD, allocate lump
  //  in zone memory.
  sprintf(name, "ds%s", sfxname);

  // Now, there is a severe problem with the
  //  sound handling, in it is not (yet/anymore)
  //  gamemode aware. That means, sounds from
  //  DOOM II will be requested even with DOOM
  //  shareware.
  // The sound list is wired into sounds.c,
  //  which sets the external variable.
  // I do not do runtime patches to that
  //  variable. Instead, we will use a
  //  default sound for replacement.

  if ( W_CheckNumForName(name) == -1 )
    sfxlump = W_GetNumForName("dspistol");
  else
    sfxlump = W_GetNumForName(name);

  size = W_LumpLength(sfxlump);

  sfx = W_CacheLumpNum(sfxlump, PU_STATIC);

  // Pads the sound effect out to the mixing buffer size.
  // The original realloc would interfere with zone memory.
  paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

  // Allocate from zone memory.
  paddedsfx = (unsigned char*) Z_Malloc(paddedsize+8, PU_STATIC, 0);

  // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
  // This should interfere with zone memory handling,
  //  which does not kick in in the soundserver.

  // Now copy and pad.
  memcpy(paddedsfx, sfx, size);
  for (i=size; i<paddedsize+8; i++)
    paddedsfx[i] = 128;

  // Remove the cached lump.
  Z_Free(sfx);

  // Preserve padded length.
  *len = paddedsize;

  // Return allocated padded data.
  return raw2SAMPLE(paddedsfx,paddedsize);  // killough 1/22/98: pass all data
}

// SFX API
// Note: this was called by S_Init.
// However, whatever they did in the
// old DPMS based DOS version, this
// were simply dummies in the Linux
// version.
// See soundserver initdata().
//

void I_SetChannels()
{
  // no-op.
}


void I_SetSfxVolume(int volume)
{
  // Identical to DOS.
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  snd_SfxVolume = volume;
}

// jff 1/21/98 moved music volume down into MUSIC API with the rest

//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
  char namebuf[9];
  sprintf(namebuf, "ds%s", sfx->name);
  return W_CheckNumForName(namebuf);
}

// Almost all of the sound code from this point on was
// rewritten by Lee Killough, based on Chi's rough initial
// version.

// killough 2/21/98: optionally use varying pitched sounds

#define PITCH(x) (pitched_sounds ? ((x)*1000)/128 : 1000)

// This is the number of active sounds that these routines
// can handle at once, regardless of the mixer's ability
// (which we don't care about since allegro does the mixing)
// We set it to some ridiculously large number, to avoid
// any chances that these routines will stop the sounds.
// killough

#define NUM_CHANNELS 256

// "Channels" used to buffer requests. Distinct SAMPLEs
// must be used for each active sound, or else clipping
// will occur.

static SAMPLE channel[NUM_CHANNELS];

// This function adds a sound to the list of currently
// active sounds, which is maintained as a given number
// of internal channels. Returns a handle.

int I_StartSound(int sfx, int   vol, int sep, int pitch, int pri)
{
  static int handle;

  // move up one slot, with wraparound
  if (++handle >= NUM_CHANNELS)
    handle = 0;

  // destroy anything still in the slot
  stop_sample(&channel[handle]);

  // Copy the sound's data into the sound sample slot
  memcpy(&channel[handle], S_sfx[sfx].data, sizeof(SAMPLE));

  // Start the sound
  play_sample(&channel[handle], vol*VOLSCALE+VOLSCALE-1, 256-sep, PITCH(pitch), 0);

  // Reference for s_sound.c to use when calling functions below
  return handle;
}

// Stop the sound. Necessary to prevent runaway chainsaw,
// and to stop rocket launches when an explosion occurs.

void I_StopSound (int handle)
{
  stop_sample(channel+handle);
}

// Update the sound parameters. Used to control volume,
// pan, and pitch changes such as when a player turns.

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
  adjust_sample(
                &channel[handle],
                vol*VOLSCALE+VOLSCALE-1,
                256-sep,
                PITCH(pitch),
                0
               );
}

// We can pretend that any sound that we've associated a handle
// with is always playing.

int I_SoundIsPlaying(int handle)
{
  return 1;
}

// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
//  allegro does this now

void I_UpdateSound( void )
{
}

// This would be used to write out the mixbuffer
//  during each game loop update.
// Updates sound buffer and audio device at runtime.
// It is called during Timer interrupt with SNDINTR.

void I_SubmitSound(void)
{
  //this should no longer be necessary because
  //allegro is doing all the sound mixing now
}

void I_ShutdownSound(void)
{
  remove_sound();
}

void I_InitSound(void)
{
  int lengths[NUMSFX];  // The actual lengths of all sound effects. -- killough
  int i, snd_c = snd_card;

  // Secure and configure sound device first.
  //jff 8/3/98 use logical output routine
  lprintf(LO_INFO,"I_InitSound: ");

  if (detect_voices && snd_card>=0 && mus_card>=0)
    {
      int mv;                          //jff 3/3/98 try it according to Allegro
      int dv = detect_digi_driver(snd_card); // detect the digital sound driver
      if (dv==0)
        snd_c=0;
      mv = detect_midi_driver(mus_card);     // detect the midi driver
      if (mv==-1)
        dv=mv=dv/2;          //note stealing driver, uses digital voices
      if (mv==0xffff)
        mv=-1;               //extern MPU-401 - unknown use default voices
      reserve_voices(dv,mv); // reserve the number of voices detected
    }                                  //jff 3/3/98 end of sound init changes


  if (install_sound(snd_c, mus_card, "none")==-1) //jff 1/18/98 autodect MIDI
    {
      //jff 8/3/98 use logical output routine
      lprintf(LO_ERROR, "ALLEGRO SOUND INIT ERROR!!!!\n%s\n", allegro_error);
      //jff 1/22/98 on error, disable sound this invocation
      //in future - nice to detect if either sound or music might be ok
      nosfxparm = true;
      nomusicparm = true;
      //jff end disable sound this invocation
    }
  else //jff 1/22/98 don't register I_ShutdownSound if errored
    {
      //jff 8/3/98 use logical output routine
      lprintf(LO_CONFIRM," configured audio device\n");
      LOCK_VARIABLE(channel);  // killough 2/7/98: prevent VM swapping of sfx
      atexit(I_ShutdownSound); // killough
    }

  // Initialize external data (all sounds) at start, keep static.
  //jff 8/3/98 use logical output routine
  lprintf(LO_INFO,"I_InitSound: ");

  for (i=1; i<NUMSFX; i++)
    if (!S_sfx[i].link)   // Load data from WAD file.
      S_sfx[i].data = getsfx(S_sfx[i].name, &lengths[i]);
    else
      { // Alias? Example is the chaingun sound linked to pistol.
        // Previously loaded already?
        S_sfx[i].data = S_sfx[i].link->data;
        lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
      }

  // Finished initialization.
  //jff 8/3/98 use logical output routine
  lprintf(LO_CONFIRM,"I_InitSound: sound module ready\n");
}

///
// MUSIC API.
//

// This is the number of active musics that these routines
// can handle at once, regardless of the mixer's ability
// (which we don't care about since allegro does the mixing)
// We set it to 1 to allow just one music at a time for now.

#define NUM_MIDICHAN 1

// mididata is used to buffer the current music.

static MIDI mididata;

void I_ShutdownMusic(void)
{
  stop_midi();          //jff 1/16/98 shut down midi
}

void I_InitMusic(void)
{
  atexit(I_ShutdownMusic); //jff 1/16/98 enable atexit routine for shutdown
}

// jff 1/18/98 changed interface to make mididata destroyable

void I_PlaySong(int handle, int looping)
{
  if (handle>=0)
    play_midi(&mididata,looping);       // start registered midi playing
}

void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  snd_MusicVolume = volume;
  // Now set volume on output device.

  //jff 01/17/98 - add VOLSCALE-1 to get most out of volume
  set_volume(-1,snd_MusicVolume*VOLSCALE+VOLSCALE-1);   // jff 1/18/98
}

void I_PauseSong (int handle)
{
  if (handle>=0)
    midi_pause();       // jff 1/16/98 pause midi playing
}

void I_ResumeSong (int handle)
{
  if (handle>=0)
    midi_resume();      // jff 1/16/98 resume midi playing
}

void I_StopSong(int handle)
{
  if (handle>=0)
    stop_midi();        // jff 1/16/98 stop midi playing
}

void I_UnRegisterSong(int handle)
{
}

// jff 1/16/98 created to convert data to MIDI ala Allegro

int I_RegisterSong(void *data)
{
  int handle, err;

  //jff 1/21/98 just stop any midi currently playing
  stop_midi();

  // convert the MUS lump data to a MIDI structure
  //jff 1/17/98 make divisions 89, compression allowed

  if    //jff 02/08/98 add native midi support
    (
     (err=MidiToMIDI(data, &mididata)) &&       // try midi first
     (err=mmus2mid(data, &mididata, 89, 0))     // now try mus
     )
    {
      handle=-1;
      dprintf("Error loading midi: %d",err);
    }
  else
    {
      handle=0;
      lock_midi(&mididata);     // data must be locked for Allegro
    }
  //jff 02/08/98 add native midi support:
  return handle;                        // 0 if successful, -1 otherwise
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
  return 0;
}

#endif //_WIN32

//----------------------------------------------------------------------------
//
// $Log: I_sound.c,v $
// Revision 1.1  2000/04/09 18:03:40  proff_fs
// Initial revision
//
// Revision 1.16  1998/09/07  20:06:36  jim
// Added logical output routine
//
// Revision 1.15  1998/05/03  22:32:33  killough
// beautification, use new headers/decls
//
// Revision 1.14  1998/03/09  07:11:29  killough
// Lock sound sample data
//
// Revision 1.13  1998/03/05  00:58:46  jim
// fixed autodetect not allowed in allegro detect routines
//
// Revision 1.12  1998/03/04  11:51:37  jim
// Detect voices in sound init
//
// Revision 1.11  1998/03/02  11:30:09  killough
// Make missing sound lumps non-fatal
//
// Revision 1.10  1998/02/23  04:26:44  killough
// Add variable pitched sound support
//
// Revision 1.9  1998/02/09  02:59:51  killough
// Add sound sample locks
//
// Revision 1.8  1998/02/08  15:15:51  jim
// Added native midi support
//
// Revision 1.7  1998/01/26  19:23:27  phares
// First rev with no ^Ms
//
// Revision 1.6  1998/01/23  02:43:07  jim
// Fixed failure to not register I_ShutdownSound with atexit on install_sound error
//
// Revision 1.4  1998/01/23  00:29:12  killough
// Fix SSG reload by using frequency stored in lump
//
// Revision 1.3  1998/01/22  05:55:12  killough
// Removed dead past changes, changed destroy_sample to stop_sample
//
// Revision 1.2  1998/01/21  16:56:18  jim
// Music fixed, defaults for cards added
//
// Revision 1.1.1.1  1998/01/19  14:02:57  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
