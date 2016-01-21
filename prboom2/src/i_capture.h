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

#ifndef __I_CAPTURE__
#define __I_CAPTURE__

// commandlines passed to popen()
// this one recieves raw PCM sound on stdin
extern const char *cap_soundcommand;
// this one recieves raw RGB video on stdin
extern const char *cap_videocommand;
// this one recieves nothing on stdin and is called after the other two finish
extern const char *cap_muxcommand;
// names of two files to remove after muxcommand finishes
extern const char *cap_tempfile1;
extern const char *cap_tempfile2;
extern int cap_remove_tempfiles;
extern int cap_fps;
extern int cap_frac;

// true if we're capturing video
extern int capturing_video;

// init and open sound, video pipes
// fn is filename passed from command line, typically final output file
void I_CapturePrep (const char *fn);

// capture a single frame of video (and corresponding audio length)
// and send it to pipes
void I_CaptureFrame (void);

// close pipes, call muxcommand, finalize
void I_CaptureFinish (void);

#endif
