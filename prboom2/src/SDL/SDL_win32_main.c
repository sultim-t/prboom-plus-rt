/*
    SDL_main.c, placed in the public domain by Sam Lantinga  4/13/98

    The WinMain function -- calls your program's main() function
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <windows.h>
#include <malloc.h>			/* For _alloca() */

#ifdef _WIN32_WCE
# define DIR_SEPERATOR TEXT("\\")
# undef _getcwd
# define _getcwd(str,len)	wcscpy(str,TEXT(""))
# define setbuf(f,b)
# define setvbuf(w,x,y,z)
# define fopen		_wfopen
# define freopen	_wfreopen
# define remove(x)	DeleteFile(x)
# define strcat		wcscat
#else
# define DIR_SEPERATOR TEXT("/")
# include <direct.h>
#endif

/* Include the SDL main definition header */
#include "SDL.h"
#include "SDL_main.h"

#ifdef main
# ifndef _WIN32_WCE_EMULATION
#  undef main
# endif /* _WIN32_WCE_EMULATION */
#endif /* main */

/* The standard output files */
#define STDOUT_FILE	TEXT("stdout.txt")
#define STDERR_FILE	TEXT("stderr.txt")

#ifndef NO_STDIO_REDIRECT
# ifdef _WIN32_WCE
  static wchar_t stdoutPath[MAX_PATH];
  static wchar_t stderrPath[MAX_PATH];
# else
  static char stdoutPath[MAX_PATH];
  static char stderrPath[MAX_PATH];
# endif
#endif

#if defined(_WIN32_WCE) && _WIN32_WCE < 300
/* seems to be undefined in Win CE although in online help */
#define isspace(a) (((CHAR)a == ' ') || ((CHAR)a == '\t'))

/* seems to be undefined in Win CE although in online help */
char *strrchr(char *str, int c)
{
	char *p;

	/* Skip to the end of the string */
	p=str;
	while (*p)
		p++;

	/* Look for the given character */
	while ( (p >= str) && (*p != (CHAR)c) )
		p--;

	/* Return NULL if character not found */
	if ( p < str ) {
		p = NULL;
	}
	return p;
}
#endif /* _WIN32_WCE < 300 */

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	int argc;

	argc = 0;
	for ( bufp = cmdline; *bufp; ) {
		/* Skip leading whitespace */
		while ( isspace(*bufp) ) {
			++bufp;
		}
		/* Skip over argument */
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && (*bufp != '"') ) {
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
}

/* Show an error message */
static void ShowError(const char *title, const char *message)
{
/* If USE_MESSAGEBOX is defined, you need to link with user32.lib */
#ifdef USE_MESSAGEBOX
	MessageBox(NULL, message, title, MB_ICONEXCLAMATION|MB_OK);
#else
	fprintf(stderr, "%s: %s\n", title, message);
#endif
}

/* Pop up an out of memory message, returns to Windows */
static BOOL OutOfMemory(void)
{
	ShowError("Fatal Error", "Out of memory - aborting");
	return FALSE;
}

/* Remove the output files if there was no output written */
static void __cdecl cleanup_output(void)
{
#ifndef NO_STDIO_REDIRECT
	FILE *file;
	int empty;
#endif

	/* Flush the output in case anything is queued */
	fclose(stdout);
	fclose(stderr);

#ifndef NO_STDIO_REDIRECT
	/* See if the files have any output in them */
	if ( stdoutPath[0] ) {
		file = fopen(stdoutPath, TEXT("rb"));
		if ( file ) {
			empty = (fgetc(file) == EOF) ? 1 : 0;
			fclose(file);
			if ( empty ) {
				remove(stdoutPath);
			}
		}
	}
	if ( stderrPath[0] ) {
		file = fopen(stderrPath, TEXT("rb"));
		if ( file ) {
			empty = (fgetc(file) == EOF) ? 1 : 0;
			fclose(file);
			if ( empty ) {
				remove(stderrPath);
			}
		}
	}
#endif
}

#if defined(_MSC_VER) && !defined(_WIN32_WCE)
/* The VC++ compiler needs main defined */
#define console_main main
#endif

/* This is where execution begins [console apps] */
int console_main(int argc, char *argv[])
{
	int n;
	char *bufp, *appname;

	/* Get the class name from argv[0] */
	appname = argv[0];
	if ( (bufp=strrchr(argv[0], '\\')) != NULL ) {
		appname = bufp+1;
	} else
	if ( (bufp=strrchr(argv[0], '/')) != NULL ) {
		appname = bufp+1;
	}

	if ( (bufp=strrchr(appname, '.')) == NULL )
		n = strlen(appname);
	else
		n = (bufp-appname);

	bufp = (char *)alloca(n+1);
	if ( bufp == NULL ) {
		return OutOfMemory();
	}
	strncpy(bufp, appname, n);
	bufp[n] = '\0';
	appname = bufp;

	/* Load SDL dynamic link library */
	if ( SDL_Init(SDL_INIT_NOPARACHUTE) < 0 ) {
		ShowError("WinMain() error", SDL_GetError());
		return(FALSE);
	}
	atexit(cleanup_output);
	atexit(SDL_Quit);

#ifndef DISABLE_VIDEO
#if 0
	/* Create and register our class *
	   DJM: If we do this here, the user nevers gets a chance to
	   putenv(SDL_WINDOWID).  This is already called later by
	   the (DIB|DX5)_CreateWindow function, so it should be
	   safe to comment it out here.
	if ( SDL_RegisterApp(appname, CS_BYTEALIGNCLIENT, 
	                     GetModuleHandle(NULL)) < 0 ) {
		ShowError("WinMain() error", SDL_GetError());
		exit(1);
	}*/
#else
	/* Sam:
	   We still need to pass in the application handle so that
	   DirectInput will initialize properly when SDL_RegisterApp()
	   is called later in the video initialization.
	 */
	SDL_SetModuleHandle(GetModuleHandle(NULL));
#endif /* 0 */
#endif /* !DISABLE_VIDEO */

	/* Run the application main() code */
	SDL_main(argc, argv);

	/* Exit cleanly, calling atexit() functions */
	exit(0);

	/* Hush little compiler, don't you cry... */
	return(0);
}

/* This is where execution begins [windowed apps] */
#ifdef _WIN32_WCE
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR szCmdLine, int sw)
#else
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
#endif
{
	HINSTANCE handle;
	char **argv;
	int argc;
	char *cmdline;
#ifdef _WIN32_WCE
	wchar_t *bufp;
	int nLen;
#else
	char *bufp;
#endif
#ifndef NO_STDIO_REDIRECT
	FILE *newfp;
#endif

	/* Start up DDHELP.EXE before opening any files, so DDHELP doesn't
	   keep them open.  This is a hack.. hopefully it will be fixed 
	   someday.  DDHELP.EXE starts up the first time DDRAW.DLL is loaded.
	 */
	handle = LoadLibrary(TEXT("DDRAW.DLL"));
	if ( handle != NULL ) {
		FreeLibrary(handle);
	}

#ifndef NO_STDIO_REDIRECT
	_getcwd( stdoutPath, sizeof( stdoutPath ) );
	strcat( stdoutPath, DIR_SEPERATOR STDOUT_FILE );
    
	/* Redirect standard input and standard output */
	newfp = freopen(stdoutPath, TEXT("w"), stdout);

#ifndef _WIN32_WCE
	if ( newfp == NULL ) {	/* This happens on NT */
#if !defined(stdout)
		stdout = fopen(stdoutPath, TEXT("w"));
#else
		newfp = fopen(stdoutPath, TEXT("w"));
		if ( newfp ) {
			*stdout = *newfp;
		}
#endif
	}
#endif /* _WIN32_WCE */

	_getcwd( stderrPath, sizeof( stderrPath ) );
	strcat( stderrPath, DIR_SEPERATOR STDERR_FILE );

	newfp = freopen(stderrPath, TEXT("w"), stderr);
#ifndef _WIN32_WCE
	if ( newfp == NULL ) {	/* This happens on NT */
#if !defined(stderr)
		stderr = fopen(stderrPath, TEXT("w"));
#else
		newfp = fopen(stderrPath, TEXT("w"));
		if ( newfp ) {
			*stderr = *newfp;
		}
#endif
	}
#endif /* _WIN32_WCE */

	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);	/* Line buffered */
	setbuf(stderr, NULL);			/* No buffering */
#endif /* !NO_STDIO_REDIRECT */

#ifdef _WIN32_WCE
	nLen = wcslen(szCmdLine)+128+1;
	bufp = (wchar_t *)alloca(nLen*2);
	wcscpy (bufp, TEXT("\""));
	GetModuleFileName(NULL, bufp+1, 128-3);
	wcscpy (bufp+wcslen(bufp), TEXT("\" "));
	wcsncpy(bufp+wcslen(bufp), szCmdLine,nLen-wcslen(bufp));
	nLen = wcslen(bufp)+1;
	cmdline = (char *)alloca(nLen);
	if ( cmdline == NULL ) {
		return OutOfMemory();
	}
	WideCharToMultiByte(CP_ACP, 0, bufp, -1, cmdline, nLen, NULL, NULL);
#else
	/* Grab the command line (use alloca() on Windows) */
	bufp = GetCommandLine();
	cmdline = (char *)alloca(strlen(bufp)+1);
	if ( cmdline == NULL ) {
		return OutOfMemory();
	}
	strcpy(cmdline, bufp);
#endif

	/* Parse it into argv and argc */
	argc = ParseCommandLine(cmdline, NULL);
	argv = (char **)alloca((argc+1)*(sizeof *argv));
	if ( argv == NULL ) {
		return OutOfMemory();
	}
	ParseCommandLine(cmdline, argv);

	/* Run the main program (after a little SDL initialization) */
	return(console_main(argc, argv));
}
