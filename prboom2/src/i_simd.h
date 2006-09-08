// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __I_SIMD_H__
#define __I_SIMD_H__

/*
===============================================================================

	3DNow! implementation of idSIMDProcessor

===============================================================================
*/

#ifdef _WIN32
 #ifdef SIMD_INSTRUCTIONS

  typedef void *(*memcpy_fast_f)(void *, const void *, size_t);
  typedef void *(*memset_fast_f)(void *, int, size_t);

  extern memcpy_fast_f memcpy_fast;
  extern memset_fast_f memset_fast;

  void I_InitSIMD(void);

 #else // SIMD_INSTRUCTIONS

  #define memcpy_fast memcpy
  #define memset_fast memset

 #endif // SIMD_INSTRUCTIONS

#else

  #define memcpy_fast memcpy
  #define memset_fast memset

#endif // _WIN32

#endif // !__I_SIMD_H__
