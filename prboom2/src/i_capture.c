/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *
 *  Copyright (C) 2011 by
 *  Nicholai Main
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

#include "SDL.h"
#include "SDL_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include "i_sound.h"
#include "i_video.h"
#include "lprintf.h"
#include "i_capture.h"


int capturing_video = 0;
static const char *vid_fname;


typedef struct
{ // information on a running pipe
  char command[PATH_MAX];
  FILE *f_stdin;
  FILE *f_stdout;
  FILE *f_stderr;
  SDL_Thread *outthread;
  const char *stdoutdumpname;
  SDL_Thread *errthread;
  const char *stderrdumpname;
  void *user;
} pipeinfo_t;

static pipeinfo_t soundpipe;
static pipeinfo_t videopipe;
static pipeinfo_t muxpipe;


const char *cap_soundcommand;
const char *cap_videocommand;
const char *cap_muxcommand;
const char *cap_tempfile1;
const char *cap_tempfile2;
int cap_remove_tempfiles;
int cap_fps;
int cap_frac;

// parses a command with simple printf-style replacements.

// %w video width (px)
// %h video height (px)
// %s sound rate (hz)
// %f filename passed to -viddump
// %% single percent sign
// TODO: add aspect ratio information
static int parsecommand (char *out, const char *in, int len)
{
  int i;

  while (*in && len > 1)
  {
    if (*in == '%')
    {
      switch (in[1])
      {
        case 'w':
          i = doom_snprintf (out, len, "%u", REAL_SCREENWIDTH);
          break;
        case 'h':
          i = doom_snprintf (out, len, "%u", REAL_SCREENHEIGHT);
          break;
        case 's':
          i = doom_snprintf (out, len, "%u", snd_samplerate);
          break;
        case 'f':
          i = doom_snprintf (out, len, "%s", vid_fname);
          break;
        case 'r':
          i = doom_snprintf (out, len, "%u", cap_fps);
          break;
        case '%':
          i = doom_snprintf (out, len, "%%");
          break;
        default:
          return 0;
      }
      out += i;
      len -= i;
      in += 2;
    }
    else
    {
      *out++ = *in++;
      len--;
    }
  }
  if (*in || len < 1)
  { // out of space
    return 0;
  }
  *out = 0;
  return 1;
}




// popen3() implementation -
// starts a child process

// user is a pointer to implementation defined extra data
static int my_popen3 (pipeinfo_t *p); // 1 on success
// close waits on process
static void my_pclose3 (pipeinfo_t *p);


#ifdef _WIN32
// direct winapi implementation
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <io.h>

typedef struct
{
  HANDLE proc;
  HANDLE thread;
} puser_t;

// extra pointer is used to hold process id to wait on to close
// NB: stdin is opened as "wb", stdout, stderr as "r"
static int my_popen3 (pipeinfo_t *p)
{
  FILE *fin = NULL;
  FILE *fout = NULL;
  FILE *ferr = NULL;
  HANDLE child_hin = INVALID_HANDLE_VALUE;
  HANDLE child_hout = INVALID_HANDLE_VALUE;
  HANDLE child_herr = INVALID_HANDLE_VALUE;
  HANDLE parent_hin = INVALID_HANDLE_VALUE;
  HANDLE parent_hout = INVALID_HANDLE_VALUE;
  HANDLE parent_herr = INVALID_HANDLE_VALUE;


  puser_t *puser = NULL;


  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  SECURITY_ATTRIBUTES sa;

  puser = malloc (sizeof (puser_t));
  if (!puser)
    return 0;

  puser->proc = INVALID_HANDLE_VALUE;
  puser->thread = INVALID_HANDLE_VALUE;


  // make the pipes

  sa.nLength = sizeof (sa);
  sa.bInheritHandle = 1;
  sa.lpSecurityDescriptor = NULL;
  if (!CreatePipe (&child_hin, &parent_hin, &sa, 0))
    goto fail;
  if (!CreatePipe (&parent_hout, &child_hout, &sa, 0))
    goto fail;
  if (!CreatePipe (&parent_herr, &child_herr, &sa, 0))
    goto fail;


  // very important
  if (!SetHandleInformation (parent_hin, HANDLE_FLAG_INHERIT, 0))
    goto fail;
  if (!SetHandleInformation (parent_hout, HANDLE_FLAG_INHERIT, 0))
    goto fail;
  if (!SetHandleInformation (parent_herr, HANDLE_FLAG_INHERIT, 0))
    goto fail;




  // start the child process

  ZeroMemory (&siStartInfo, sizeof (STARTUPINFO));
  siStartInfo.cb         = sizeof (STARTUPINFO);
  siStartInfo.hStdInput  = child_hin;
  siStartInfo.hStdOutput = child_hout;
  siStartInfo.hStdError  = child_herr;
  siStartInfo.dwFlags    = STARTF_USESTDHANDLES;

  if (!CreateProcess(NULL,// application name
       (LPTSTR)p->command,// command line
       NULL,              // process security attributes
       NULL,              // primary thread security attributes
       TRUE,              // handles are inherited
       DETACHED_PROCESS,  // creation flags
       NULL,              // use parent's environment
       NULL,              // use parent's current directory
       &siStartInfo,      // STARTUPINFO pointer
       &piProcInfo))      // receives PROCESS_INFORMATION
  {
    goto fail;
  }



  puser->proc = piProcInfo.hProcess;
  puser->thread = piProcInfo.hThread;


                                // what the hell is this cast for
  if (NULL == (fin = _fdopen (_open_osfhandle ((int) parent_hin, 0), "wb")))
    goto fail;
  if (NULL == (fout = _fdopen (_open_osfhandle ((int) parent_hout, 0), "r")))
    goto fail;
  if (NULL == (ferr = _fdopen (_open_osfhandle ((int) parent_herr, 0), "r")))
    goto fail;
  // after fdopen(osf()), we don't need to keep track of parent handles anymore
  // fclose on the FILE struct will automatically free them


  p->user = puser;
  p->f_stdin = fin;
  p->f_stdout = fout;
  p->f_stderr = ferr;

  CloseHandle (child_hin);
  CloseHandle (child_hout);
  CloseHandle (child_herr);

  return 1;

  fail:
  if (fin)
    fclose (fin);
  if (fout)
    fclose (fout);
  if (ferr)
    fclose (ferr);

  if (puser->proc)
    CloseHandle (puser->proc);
  if (puser->thread)
    CloseHandle (puser->thread);

  if (child_hin != INVALID_HANDLE_VALUE)
    CloseHandle (child_hin);
  if (child_hout != INVALID_HANDLE_VALUE)
    CloseHandle (child_hout);
  if (child_herr != INVALID_HANDLE_VALUE)
    CloseHandle (child_herr);
  if (parent_hin != INVALID_HANDLE_VALUE)
    CloseHandle (parent_hin);
  if (parent_hout != INVALID_HANDLE_VALUE)
    CloseHandle (parent_hout);
  if (parent_herr != INVALID_HANDLE_VALUE)
    CloseHandle (parent_herr);

  free (puser);

  return 0;


}

static void my_pclose3 (pipeinfo_t *p)
{
  puser_t *puser = (puser_t *) p->user;

  if (!p->f_stdin || !p->f_stdout || !p->f_stderr || !puser)
    return;

  fclose (p->f_stdin);
  //fclose (p->f_stdout); // these are closed elsewhere
  //fclose (p->f_stderr);

  WaitForSingleObject (puser->proc, INFINITE);

  CloseHandle (puser->proc);
  CloseHandle (puser->thread);
  free (puser);
}

#else // _WIN32
// posix implementation
// not tested
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


typedef struct
{
  int pid;
} puser_t;


static int my_popen3 (pipeinfo_t *p)
{
  FILE *fin = NULL;
  FILE *fout = NULL;
  FILE *ferr = NULL;
  int child_hin = -1;
  int child_hout = -1;
  int child_herr = -1;
  int parent_hin = -1;
  int parent_hout = -1;
  int parent_herr = -1;

  int scratch[2];

  int pid;

  puser_t *puser = NULL;

  puser = malloc (sizeof (puser_t));
  if (!puser)
    return 0;



  // make the pipes
  if (pipe (scratch))
    goto fail;
  child_hin = scratch[0];
  parent_hin = scratch[1];
  if (pipe (scratch))
    goto fail;
  parent_hout = scratch[0];
  child_hout = scratch[1];
  if (pipe (scratch))
    goto fail;
  parent_herr = scratch[0];
  child_herr = scratch[1];

  pid = fork ();

  if (pid == -1)
    goto fail;
  if (pid == 0)
  {
    dup2 (child_hin, STDIN_FILENO);
    dup2 (child_hout, STDOUT_FILENO);
    dup2 (child_herr, STDERR_FILENO);

    close (parent_hin);
    close (parent_hout);
    close (parent_herr);

    // does this work? otherwise we have to parse cmd into an **argv style array
    execl ("/bin/sh", "sh", "-c", p->command, NULL);
    // exit forked process if command failed
    _exit (0);
  }

  if (NULL == (fin = fdopen (parent_hin, "wb")))
    goto fail;
  if (NULL == (fout = fdopen (parent_hout, "r")))
    goto fail;
  if (NULL == (ferr = fdopen (parent_herr, "r")))
    goto fail;

  close (child_hin);
  close (child_hout);
  close (child_herr);

  puser->pid = pid;

  p->user = puser;
  p->f_stdin = fin;
  p->f_stdout = fout;
  p->f_stderr = ferr;
  return 1;

  fail:
  if (fin)
    fclose (fin);
  if (fout)
    fclose (fout);
  if (ferr)
    fclose (ferr);

  close (parent_hin);
  close (parent_hout);
  close (parent_herr);
  close (child_hin);
  close (child_hout);
  close (child_herr);

  free (puser);
  return 0;

}


static void my_pclose3 (pipeinfo_t *p)
{
  puser_t *puser = (puser_t *) p->user;

  int s;

  if (!p->f_stdin || !p->f_stdout || !p->f_stderr || !puser)
    return;

  fclose (p->f_stdin);
  //fclose (p->f_stdout); // these are closed elsewhere
  //fclose (p->f_stderr);

  waitpid (puser->pid, &s, 0);

  free (puser);
}


#endif // _WIN32

typedef struct
{
  FILE *fin;
  const char *fn;
} threaddata_t;


static int threadstdoutproc (void *data)
{ // simple thread proc dumps stdout
  // not terribly fast
  int c;

  pipeinfo_t *p = (pipeinfo_t *) data;

  FILE *f = fopen (p->stdoutdumpname, "w");

  if (!f || !p->f_stdout)
    return 0;

  while ((c = fgetc (p->f_stdout)) != EOF)
    fputc (c, f);

  fclose (f);
  fclose (p->f_stdout);
  return 1;
}

static int threadstderrproc (void *data)
{ // simple thread proc dumps stderr
  // not terribly fast
  int c;

  pipeinfo_t *p = (pipeinfo_t *) data;

  FILE *f = fopen (p->stderrdumpname, "w");

  if (!f || !p->f_stderr)
    return 0;

  while ((c = fgetc (p->f_stderr)) != EOF)
    fputc (c, f);

  fclose (f);
  fclose (p->f_stderr);
  return 1;
}


// init and open sound, video pipes
// fn is filename passed from command line, typically final output file
void I_CapturePrep (const char *fn)
{
  vid_fname = fn;

  if (!parsecommand (soundpipe.command, cap_soundcommand, sizeof(soundpipe.command)))
  {
    lprintf (LO_ERROR, "I_CapturePrep: malformed command %s\n", cap_soundcommand);
    capturing_video = 0;
    return;
  }
  if (!parsecommand (videopipe.command, cap_videocommand, sizeof(videopipe.command)))
  {
    lprintf (LO_ERROR, "I_CapturePrep: malformed command %s\n", cap_videocommand);
    capturing_video = 0;
    return;
  }
  if (!parsecommand (muxpipe.command, cap_muxcommand, sizeof(muxpipe.command)))
  {
    lprintf (LO_ERROR, "I_CapturePrep: malformed command %s\n", cap_muxcommand);
    capturing_video = 0;
    return;
  }

  lprintf (LO_INFO, "I_CapturePrep: opening pipe \"%s\"\n", soundpipe.command);
  if (!my_popen3 (&soundpipe))
  {
    lprintf (LO_ERROR, "I_CapturePrep: sound pipe failed\n");
    capturing_video = 0;
    return;
  }
  lprintf (LO_INFO, "I_CapturePrep: opening pipe \"%s\"\n", videopipe.command);
  if (!my_popen3 (&videopipe))
  {
    lprintf (LO_ERROR, "I_CapturePrep: video pipe failed\n");
    my_pclose3 (&soundpipe);
    capturing_video = 0;
    return;
  }
  I_SetSoundCap ();
  lprintf (LO_INFO, "I_CapturePrep: video capture started\n");
  capturing_video = 1;

  // start reader threads
  soundpipe.stdoutdumpname = "sound_stdout.txt";
  soundpipe.stderrdumpname = "sound_stderr.txt";
  soundpipe.outthread = SDL_CreateThread (threadstdoutproc, "soundpipe.outthread", &soundpipe);
  soundpipe.errthread = SDL_CreateThread (threadstderrproc, "soundpipe.errthread", &soundpipe);
  videopipe.stdoutdumpname = "video_stdout.txt";
  videopipe.stderrdumpname = "video_stderr.txt";
  videopipe.outthread = SDL_CreateThread (threadstdoutproc, "videopipe.outthread", &videopipe);
  videopipe.errthread = SDL_CreateThread (threadstderrproc, "videopipe.errthread", &videopipe);

  atexit (I_CaptureFinish);
}



// capture a single frame of video (and corresponding audio length)
// and send it to pipes
void I_CaptureFrame (void)
{
  unsigned char *snd;
  unsigned char *vid;
  static int partsof35 = 0; // correct for sync when samplerate % 35 != 0
  int nsampreq;

  if (!capturing_video)
    return;

  nsampreq = snd_samplerate / cap_fps;
  partsof35 += snd_samplerate % cap_fps;
  if (partsof35 >= cap_fps)
  {
    partsof35 -= cap_fps;
    nsampreq++;
  }

  snd = I_GrabSound (nsampreq);
  if (snd)
  {
    if (fwrite (snd, nsampreq * 4, 1, soundpipe.f_stdin) != 1)
      lprintf(LO_WARN, "I_CaptureFrame: error writing soundpipe.\n");
    //free (snd); // static buffer
  }
  vid = I_GrabScreen ();
  if (vid)
  {
    if (fwrite (vid, REAL_SCREENWIDTH * REAL_SCREENHEIGHT * 3, 1, videopipe.f_stdin) != 1)
      lprintf(LO_WARN, "I_CaptureFrame: error writing videopipe.\n");
    //free (vid); // static buffer
  }

}


// close pipes, call muxcommand, finalize
void I_CaptureFinish (void)
{
  int s;

  if (!capturing_video)
    return;
  capturing_video = 0;

  // on linux, we have to close videopipe first, because it has a copy of the write
  // end of soundpipe_stdin (so that stream will never see EOF).
  // is there a better way to do this?
  
  // (on windows, it doesn't matter what order we do it in)
  my_pclose3 (&videopipe);
  SDL_WaitThread (videopipe.outthread, &s);
  SDL_WaitThread (videopipe.errthread, &s);

  my_pclose3 (&soundpipe);
  SDL_WaitThread (soundpipe.outthread, &s);
  SDL_WaitThread (soundpipe.errthread, &s);

  // muxing and temp file cleanup

  lprintf (LO_INFO, "I_CaptureFinish: opening pipe \"%s\"\n", muxpipe.command);

  if (!my_popen3 (&muxpipe))
  {
    lprintf (LO_ERROR, "I_CaptureFinish: finalize pipe failed\n");
    return;
  }

  muxpipe.stdoutdumpname = "mux_stdout.txt";
  muxpipe.stderrdumpname = "mux_stderr.txt";
  muxpipe.outthread = SDL_CreateThread (threadstdoutproc, "muxpipe.outthread", &muxpipe);
  muxpipe.errthread = SDL_CreateThread (threadstderrproc, "muxpipe.errthread", &muxpipe);

  my_pclose3 (&muxpipe);
  SDL_WaitThread (muxpipe.outthread, &s);
  SDL_WaitThread (muxpipe.errthread, &s);


  // unlink any files user wants gone
  if (cap_remove_tempfiles)
  {
    remove (cap_tempfile1);
    remove (cap_tempfile2);
  }
}
