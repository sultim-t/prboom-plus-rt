// ===================================================================
// AdvXMLParser
//
// See AdvXMLParser.h
// ===================================================================

// ===================================================================
// Some usefull macros
// ===================================================================

// Smalltalk-like self. It's name advself to avoid any problems is self
// is already defined (g++-2 uses a typedef of 'self')
#ifndef advself
#define advself (*this)
#endif

// Define NULL if not already the case
#ifndef NULL
#define NULL 0
#endif

// Macro to get the number of element in a array
#ifndef elemof
#define elemof(array) (sizeof(array) / sizeof((array)[0]))
#endif

// ===================================================================
// Some debugging stuff

#ifndef ASSERT
#if (defined(DEBUG) || defined(_DEBUG)) && defined(_MSC_VER)
void BreakInDebugger();
#define ASSERT(expr) do { if(!(expr)) BreakInDebugger(); } while(0)
#else
#define ASSERT(expr) ((void)0)
#endif
#endif

#ifndef VERIFY
#if defined(DEBUG) || defined(_DEBUG)
#define VERIFY(expr) ASSERT(expr)
#else
#define VERIFY(expr) ((void)(expr))
#endif
#endif
