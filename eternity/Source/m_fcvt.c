// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details 
// Copyright (C) 2003 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//    fcvt from DJGPP libc and supporting functions, to solve
//    problem with psnprintf. ::SIGH::
//
//-----------------------------------------------------------------------------

#ifdef NO_FCVT

// NOTE: Do not include z_zone.h into this file.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include "m_fcvt.h"

#ifndef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP 308
#endif

char *M_Fcvt(double value, int ndigits, int *decpt, int *sign)
{
   static char fcvt_buf[2 * DBL_MAX_10_EXP + 10];

   return M_Fcvtbuf(value, ndigits, decpt, sign, fcvt_buf);
}

//
// haleyjd: changes from DJGPP:
//
// * Changed alloca to malloc and added calls to free
//
char *M_Fcvtbuf(double value, int ndigits, int *decpt, int *sign, char *buf)
{
   static char INFINITY[] = "Infinity";
   char decimal = '.';
   int digits = ndigits >= 0 ? ndigits : 0;
   char *cvtbuf = (char *)(malloc)(2*DBL_MAX_10_EXP + 16);
   char *s = cvtbuf;
   char *dot;

   sprintf(cvtbuf, "%-+#.*f", DBL_MAX_10_EXP + digits + 1, value);
   
   /* The sign.  */
   if(*s++ == '-')
      *sign = 1;
   else
      *sign = 0;

   /* Where's the decimal point?  */
   dot = strchr(s, decimal);
   if(dot)
      *decpt = dot - s;
   else
      *decpt = strlen(s);
  
   /* SunOS docs says if NDIGITS is 8 or more, produce "Infinity"
      instead of "Inf".  */
   if(strncmp(s, "Inf", 3) == 0)
   {
      memcpy(buf, INFINITY, ndigits >= 8 ? 9 : 3);
      if(ndigits < 8)
         buf[3] = '\0';
      (free)(cvtbuf); /* haleyjd */
      return buf;
   }
   else if(ndigits < 0)
   {
      char *ret = M_Ecvtbuf(value, *decpt + ndigits, decpt, sign, buf);
      (free)(cvtbuf); /* haleyjd */
      return ret;
   }
   else if(*s == '0' && value != 0.0)
   {
      char *ret = M_Ecvtbuf(value, ndigits, decpt, sign, buf);
      (free)(cvtbuf); /* haleyjd */
      return ret;
   }
   else
   {
      memcpy(buf, s, *decpt);
      if(s[*decpt] == decimal)
      {
         memcpy(buf + *decpt, s + *decpt + 1, ndigits);
         buf[*decpt + ndigits] = '\0';
      }
      else
	buf[*decpt] = '\0';
      M_Ecvround(buf, buf + *decpt + ndigits - 1,
		 s + *decpt + ndigits + 1, decpt);
      (free)(cvtbuf); /* haleyjd */
      return buf;
    }
}

void
M_Ecvround(char *numbuf, char *last_digit, const char *after_last, int *decpt)
{
   char *p;
   int carry = 0;
   
   /* Do we have at all to round the last digit?  */
   if(*after_last > '4')
   {
      p = last_digit;
      carry = 1;

      /* Propagate the rounding through trailing '9' digits.  */
      do
      {
         int sum = *p + carry;
         carry = sum > '9';
         *p-- = sum - carry * 10;
      } while(carry && p >= numbuf);

      /* We have 9999999... which needs to be rounded to 100000..  */
      if(carry && p == numbuf)
      {
         *p = '1';
         *decpt += 1;
      }
   }
}

//
// haleyjd: changes from DJGPP:
//
// * Changed alloca to malloc and added calls to free
//
char *
M_Ecvtbuf(double value, int ndigits, int *decpt, int *sign, char *buf)
{
   static char INFINITY[] = "Infinity";
   char decimal = '.';
   char *cvtbuf = (char *)(malloc)(ndigits + 20); /* +3 for sign, dot, null; */
                                                  /* two extra for rounding */
                                                  /* 15 extra for alignment */
   char *s = cvtbuf, *d = buf;

   /* Produce two extra digits, so we could round properly.  */
   sprintf(cvtbuf, "%-+.*E", ndigits + 2, value);
   *decpt = 0;

   /* The sign.  */
   if(*s++ == '-')
      *sign = 1;
   else
      *sign = 0;

   /* Special values get special treatment.  */
   if(strncmp (s, "Inf", 3) == 0)
   {
      /* SunOS docs says we have return "Infinity" for NDIGITS >= 8.  */
      memcpy(buf, INFINITY, ndigits >= 8 ? 9 : 3);
      if(ndigits < 8)
         buf[3] = '\0';
   }
   else if(strcmp (s, "NaN") == 0)
      memcpy(buf, s, 4);
   else
   {
      char *last_digit, *digit_after_last;
      
      /* Copy (the single) digit before the decimal.  */
      while(*s && *s != decimal && d - buf < ndigits)
         *d++ = *s++;
      
      /* If we don't see any exponent, here's our decimal point.  */
      *decpt = d - buf;
      if(*s)
         s++;

      /* Copy the fraction digits.  */
      while(*s && *s != 'E' && d - buf < ndigits)
         *d++ = *s++;

      /* Remember the last digit copied and the one after it.  */
      last_digit = d > buf ? d - 1 : d;
      digit_after_last = s;

      /* Get past the E in exponent field.  */
      while(*s && *s++ != 'E')
         ;

      /* Adjust the decimal point by the exponent value.  */
      *decpt += atoi(s);
      
      /* Pad with zeroes if needed.  */
      while(d - buf < ndigits)
         *d++ = '0';

      /* Zero-terminate.  */
      *d = '\0';
      
      /* Round if necessary.  */
      M_Ecvround(buf, last_digit, digit_after_last, decpt);
   }
   /* haleyjd */
   (free)(cvtbuf);
   return buf;
}

#endif

// EOF

