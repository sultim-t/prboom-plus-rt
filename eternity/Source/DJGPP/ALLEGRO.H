#ifndef _ETERNITY_ALLEGRO_INCLUSION_
#define _ETERNITY_ALLEGRO_INCLUSION_

/* Julian: 6/6/2001

   This file includes <allegro.h> and defines the ALLEGRO_ET_VERSION (Allegro Eternity
   version) to the right level (see end of file for levels).

   Allegro Macros are not directly tested in the code, the ALLEGRO_ET_VERSION
   is tested instead. Gives more control and flexibility.
*/

#include <stdio.h>
#include <allegro.h>

// Julian: That way, you can force allegro version from the makefile
// See the end of file for compatibility notes
#ifndef ALLEGRO_ET_VERSION

#if(ALLEGRO_VERSION<3)

 #define ALLEGRO_ET_VERSION -1

#else

 #if(ALLEGRO_VERSION==3)

        #if(ALLEGRO_SUB_VERSION==0)

          #define ALLEGRO_ET_VERSION 0

        #else

          #if(ALLEGRO_SUB_VERSION==1 || (ALLEGRO_SUB_VERSION>=10 && ALLEGRO_SUB_VERSION<20))

            #define ALLEGRO_ET_VERSION 1

          #else

            #define ALLEGRO_ET_VERSION 2

          #endif

        #endif

 #else

        #define ALLEGRO_ET_VERSION 2

 #endif

#endif

// Control if version is supported
// Controls even if ALLEGRO_ET_VERSION has been set externally
#endif

#if(ALLEGRO_ET_VERSION<0 || ALLEGRO_ET_VERSION>1)
#error Allegro version not supported (Eternity support 3.0, 3.1x)
#endif

#endif

/** Julian: 6/6/2001: NOTE

  ALLEGRO_ET_VERSION can take 3 different values :
        - -1 for Allegro 3.0 excluded and below *
        - +0 for Allegro 3.0
        - +1 for Allegro 3.1x
        - +2 for Allegro 3.9 and above *

  * : not supported
*/

// EOF

