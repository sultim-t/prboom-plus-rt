/*
	$RCSfile: gl_dyn.c,v $

	This code is part of DynGL, a method of dynamically loading an OpenGL
	library without much pain designed by Joseph Carter and is based
	loosely on previous work done both by Zephaniah E. Hull and Joseph.

	Both contributors have decided to disclaim all Copyright to this work.
	It is released to the Public Domain WITHOUT ANY WARRANTY whatsoever,
	express or implied, in the hopes that others will use it instead of
	other less-evolved hacks which usually don't work right.  ;)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "z_zone.h"

#include "SDL.h"

#include "gl_dyn.h"

/*
 * Alternate functions for DYNGL_WANT
 *
 * Here's as good a place as any to put the alternates for the functions you'd
 * like to have, but which may not exist in older versions of OpenGL.  In
 * Project Twilight, this is used for glDrawRangeElements, which is supported
 * officially by OpenGL 1.2, but may not exist in old/crappy drivers.  ;)
 */

DYNGLCALL void DYNGLENTRY Alt_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices)
{
	if (p_glDrawRangeElementsEXT)
		p_glDrawRangeElementsEXT(mode, start, end, count, type, indices);
	else
		p_glDrawElements(mode, count, type, indices);
}


/*
 * Function Pointers
 *
 * Each OpenGL function we use needs to have a function pointer.  These
 * pointers are the same name as the OpenGL function, with a q prefix.  The
 * reason for the q prefix is that Quake 2/3 use it.  Because of how popular
 * Id Software, Inc's games are, it is essentially guaranteed that the qgl
 * namespace is essentially available for this purpose.  The function pointers
 * are generated from the macros in gl_funcs.h.
 */
#define DYNGL_NEED(ret, name, args) ret (DYNGLENTRY * p_##name) args = NULL;
#define DYNGL_EXT(ret, name, args, extension)	ret (DYNGLENTRY * p_##name) args = NULL;
#define DYNGL_WANT(ret, name, args, alt) ret (DYNGLENTRY * p_##name) args = NULL;
#include "gl_funcs.h"
#undef DYNGL_NEED
#undef DYNGL_EXT
#undef DYNGL_WANT


/* Internal flag for shutting things down properly */
static SDL_bool dyngl_loaded = SDL_FALSE;


/*
 * Extension lists
 *
 * We're breaking up the GL_EXTENSIONS list into a linked list which is easy
 * for us to parse later.  There are two lists used here: A list of good
 * extensions, and a list of bad ones.  An extension reported is considered to
 * be good until we find out that it isn't - namely when we ask for a function
 * which is supposed to be part of the extension, but the driver doesn't
 * actually have it.  The justification for doing this is that we can then
 * print out a list of extensions which were found to be broken.  A user
 * deserves to know when his driver is broken, even if we're able to work
 * around the problem.
 *
 * We're using a list/node system similar to that which the Amiga used here.
 * As used by this code, it is not as generic as that used on the Amiga, but
 * it's good enough for our purpose.  It takes a little getting used to, and
 * I'm not going to take the time to document it fully here.  The primary
 * change over the Amiga node structure is that since we're using this in a
 * one-shot arrangement, we're quite well off including the data right in the
 * node structure.  This is traditionally not done.
 */

typedef struct dynglnode_s {
	struct dynglnode_s *next;
	struct dynglnode_s *prev;
	char *name;
} dynglnode_t;

typedef struct dyngllist_s {
	struct dynglnode_s *head;
	struct dynglnode_s *tail;
	struct dynglnode_s *tailprev;
} dyngllist_t;

dyngllist_t extensions;
dyngllist_t bad_extensions;

static char *gl_extensions_list;


#ifdef LIST_INSERT_BEFORE
#undef LIST_INSERT_BEFORE
#endif
#define LIST_INSERT_BEFORE(listnode, node)									\
	((((dynglnode_t *)(node))->next = (listnode)),							\
	(((dynglnode_t *)(node))->prev = ((dynglnode_t *)(node))->next->prev),	\
	(((dynglnode_t *)(node))->prev->next = ((dynglnode_t *)(node))),		\
	(((dynglnode_t *)(node))->next->prev = ((dynglnode_t *)(node))))

#ifdef LIST_ADD_TAIL
#undef LIST_ADD_TAIL
#endif
#define LIST_ADD_TAIL(list, node)											\
	(LIST_INSERT_BEFORE ((dynglnode_t *)(&(list).tail), (node)))

#ifdef LIST_NODE_IS_TAIL
#undef LIST_NODE_IS_TAIL
#endif
#define LIST_NODE_IS_TAIL(node)												\
	(((dynglnode_t *)(node))->next == NULL)

#ifdef LIST_NODE_REMOVE
#undef LIST_NODE_REMOVE
#endif
#define LIST_NODE_REMOVE(node)												\
	((((dynglnode_t *)(node))->prev->next = ((dynglnode_t *)(node))->next),	\
	(((dynglnode_t *)(node))->next->prev = ((dynglnode_t *)(node))->prev),	\
	(((dynglnode_t *)(node))->prev = NULL),									\
	(((dynglnode_t *)(node))->next = NULL))

#ifdef LIST_INIT
#undef LIST_INIT
#endif
#define LIST_INIT(list)														\
	(((list).head = (dynglnode_t *)(&((list).tail))),						\
	((list).tail = NULL),													\
	((list).tailprev = (dynglnode_t *)(&((list).head))))

#ifdef LIST_HEAD
#undef LIST_HEAD
#endif
#define LIST_HEAD(list)														\
	((dynglnode_t *)&(list))


/*
 * DynGL_LoadLibrary
 *
 * Opens your OpenGL library and makes sure we can get OpenGL functions from
 * it.  This is normally platform-specific, but SDL handles it for us here.
 * Name should be set to something like libGL.so.1 or OpenGL32.DLL or whatever
 * is appropriate for the platform - put it in config.h or make it a config
 * option (or both!)
 */
SDL_bool DynGL_LoadLibrary(char *name)
{
	if (dyngl_loaded) {
		SDL_SetError("DynGL_LoadLibrary: Library already loaded");
		return SDL_FALSE;
	}

	/* Make sure the lists are ready to use */
	LIST_INIT(extensions);
	LIST_INIT(bad_extensions);

	/* Load the library, if possible */
	if (SDL_GL_LoadLibrary(name) == -1) {
		SDL_SetError("DynGL_LoadLibrary: Can't load %s", name);
		dyngl_loaded = SDL_FALSE;
		return SDL_FALSE;
	}

	dyngl_loaded = SDL_TRUE;
	return SDL_TRUE;
}


/*
 * DynGL_CloseLibrary
 *
 * Call this to "close" the library.  Actually, SDL does not export a function
 * for closing the OpenGL library currently, but will do so when you attempt
 * to load a new library.  This function just nullifies all pointers and
 * clears the extensions lists for now.
 */
void DynGL_CloseLibrary(void)
{
	dynglnode_t *p;

	if (!dyngl_loaded)
		return;

	dyngl_loaded = SDL_FALSE;

	free(gl_extensions_list);

	p = LIST_HEAD(extensions)->next;
	while (!LIST_NODE_IS_TAIL(p)) {
		LIST_NODE_REMOVE(p);
		free(p);
		p = LIST_HEAD(extensions)->next;
	}

	p = LIST_HEAD(bad_extensions)->next;
	while (!LIST_NODE_IS_TAIL(p)) {
		LIST_NODE_REMOVE(p);
		free(p);
		p = LIST_HEAD(bad_extensions)->next;
	}

#define DYNGL_NEED(ret, name, args) p_##name = NULL;
#define DYNGL_EXT(ret, name, args, extension) p_##name = NULL;
#define DYNGL_WANT(ret, name, args, alt) p_##name = NULL;
#include "gl_funcs.h"
#undef DYNGL_NEED
#undef DYNGL_EXT
#undef DYNGL_WANT

	return;
}


/*
 * DynGL_BadExtension
 *
 * If you find an extension to not actually work (how you might do this is
 * beyond the scope of this discussion), feed it to this function and the
 * extension will be disabled.
 */
void DynGL_BadExtension(char *ext)
{
	dynglnode_t *p;

	if (!dyngl_loaded)
		return;

	p = LIST_HEAD(extensions)->next;
	while (!LIST_NODE_IS_TAIL(p)) {
		if (strcmp(p->name, ext) == 0) {
			LIST_NODE_REMOVE(p);
			LIST_ADD_TAIL(bad_extensions, p);
			return;
		}
		p = p->next;
	}

	/* Didn't find it! */
	return;
}


/*
 * DynGL_HasExtension
 *
 * Checks to see if an extension exists - properly!  This function will tell
 * you something different than the GL_EXTENSIONS string would we found an
 * extension to be bad in DynGL_GetFunctions () above.
 */
SDL_bool DynGL_HasExtension(char *ext)
{
	dynglnode_t *p;

	if (!dyngl_loaded)
		return SDL_FALSE;

	p = LIST_HEAD(extensions)->next;
	while (!LIST_NODE_IS_TAIL(p)) {
		if (strcmp(p->name, ext) == 0)
			return SDL_TRUE;
		p = p->next;
	}

	/* Didn't find it, sorry! */
	return SDL_FALSE;
}


/*
 * DynGL_GetFunctions
 *
 * Goes through the list of requested functions in gl_funcs.h and tries to
 * allocate pointers for each of them.  This must be called after you get a
 * valid OpenGL context with SDL_InitVideoMode (), it's not guaranteed to work
 * if you try it before.
 *
 * Remember, the pointer for glFoo is named qglFoo to avoid polluting any
 * namespaces not already widely used for this purpose.
 */
SDL_bool DynGL_GetFunctions(void (*errfunc) (const char *fmt, ...))
{
	char *p;
	const char *last;
	int i;
	dynglnode_t *node;

	if (!dyngl_loaded) {
		SDL_SetError("DynGL_GetFuncs: no OpenGL library loaded");
		return SDL_FALSE;
	}

	/* Assign all of the functions, except extensions */
#define DYNGL_NEED(ret, name, args)											\
	if (!(p_##name = SDL_GL_GetProcAddress (#name)))							\
	{																		\
		SDL_SetError ("DynGL_GetFunctions: can't find %s", #name);			\
		return SDL_FALSE;													\
	}
#define DYNGL_EXT(ret, name, args, extension)
#define DYNGL_WANT(ret, name, args, alt)									\
	if (!(p_##name = SDL_GL_GetProcAddress (#name)))							\
		p_##name = alt;
#include "gl_funcs.h"
#undef DYNGL_NEED
#undef DYNGL_EXT
#undef DYNGL_WANT

	/* Now we need the extensions list */
	gl_extensions_list = strdup(p_glGetString(GL_EXTENSIONS));
	p = gl_extensions_list;
	last = p + strlen(p);

	while (p < last) {
		i = strcspn(p, " ");
		*(p + i) = '\0';
		node = malloc(sizeof(dynglnode_t));
		node->name = p;
		LIST_ADD_TAIL(extensions, node);

		p += (i + 1);
	}

	/* Second pass to get the extensions */
#define DYNGL_NEED(ret, name, args)
#define DYNGL_EXT(ret, name, args, extension)								\
	if (DynGL_HasExtension (extension))										\
		if (!(p_##name = SDL_GL_GetProcAddress (#name)))						\
		{																	\
			DynGL_BadExtension (extension);									\
				if (errfunc != NULL)										\
					errfunc ("DynGL_GetFunctions: Missing %s, "				\
							"needed for %s\n", #name, extension);			\
		}
#define DYNGL_WANT(ret, name, args, alt)
#include "gl_funcs.h"
#undef DYNGL_EXT

	/*
	 * Third pass to clear out any extension functions we assigned above for
	 * extensions which don't actually exist
	 */
#define DYNGL_EXT(ret, name, args, extension)								\
	if (!DynGL_HasExtension (extension))									\
		p_##name = NULL;
#include "gl_funcs.h"
#undef DYNGL_NEED
#undef DYNGL_EXT
#undef DYNGL_WANT

	return SDL_TRUE;
}
