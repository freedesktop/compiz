/*
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <string.h>

#include <compiz-core.h>

CompCore core;

#define INTERFACE_VERSION_coreType CORE_ABIVERSION

static const CommonInterface coreInterface[] = {
    C_INTERFACE (core, Type, CompObjectVTable, _, _, _, _)
};

static CompBool
coreForBaseObject (CompObject		  *object,
		   BaseObjectCallBackProc proc,
		   void			  *closure)
{
    CompObjectVTableVec v = { object->vTable };
    CompBool		status;

    CORE (object);

    UNWRAP (&c->object, object, vTable);
    status = (*proc) (object, closure);
    WRAP (&c->object, object, vTable, v.vTable);

    return status;
}

static CompBool
coreForEachInterface (CompObject	    *object,
		      InterfaceCallBackProc proc,
		      void		    *closure)
{
    return handleForEachInterface (object,
				   coreInterface,
				   N_ELEMENTS (coreInterface),
				   proc, closure);
}

static const CompObjectType *
coreGetType (CompObject *object)
{
    return getCoreObjectType ();
}

static char *
coreQueryName (CompObject *object)
{
    return NULL;
}

static CompBool
coreForEachChildObject (CompObject		*object,
			ChildObjectCallBackProc proc,
			void			*closure)
{
    CompObjectVTableVec v = { object->vTable };
    CompDisplay		*d;
    CompBool		status;

    CORE (object);

    for (d = c->displays; d; d = d->next)
	if (!(*proc) (&d->base, closure))
	    return FALSE;

    UNWRAP (&c->object, object, vTable);
    status = (*object->vTable->forEachChildObject) (object, proc, closure);
    WRAP (&c->object, object, vTable, v.vTable);

    return status;
}

static CompObject *
coreLookupChildObject (CompObject *object,
		       const char *type,
		       const char *name)
{
    CompObjectVTableVec v = { object->vTable };
    CompObject		*result;

    CORE (object);

    if (strcmp (type, getDisplayObjectType ()->name) == 0)
	if (c->displays && (!name || !name[0]))
	    return &c->displays->base.base;

    UNWRAP (&c->object, object, vTable);
    result = (*object->vTable->lookupChildObject) (object, type, name);
    WRAP (&c->object, object, vTable, v.vTable);

    return result;
}

static CompBool
coreGetMetadata (CompObject    *object,
		 const char    *interface,
		 CompListValue *list,
		 char	       **error)
{
    CompObjectVTableVec v = { object->vTable };
    CompBool		status;

    CORE (object);

    if (strcmp (interface, "core") == 0)
    {
	xmlBufferPtr buffer;

	buffer = xmlBufferCreate ();
	if (buffer)
	{
	    int i;

	    list->type   = CompOptionTypeString;
	    list->nValue = 0;
	    list->value  = NULL;

	    for (i = 0; i < coreMetadata.nDoc; i++)
	    {
		/* FIXME: we should only dump the interface specific portion
		   of the metadata documents */
		if (xmlNodeDump (buffer,
				 coreMetadata.doc[i],
				 coreMetadata.doc[i]->children,
				 0, 1) > 0)
		{
		    char *str;

		    str = strdup ((char *) xmlBufferContent (buffer));
		    if (str)
		    {
			CompOptionValue *value;

			value = realloc (list->value,
					 sizeof (CompOptionValue) *
					 (list->nValue + 1));
			if (value)
			{
			    value[list->nValue++].s = str;
			    list->value = value;
			}
			else
			{
			    free (str);
			}
		    }
		}

		xmlBufferEmpty (buffer);
	    }

	    xmlBufferFree (buffer);

	    return TRUE;
	}
    }

    UNWRAP (&c->object, object, vTable);
    status = (*object->vTable->metadata.get) (object, interface, list, error);
    WRAP (&c->object, object, vTable, v.vTable);

    return status;
}

static CompBool
initCorePluginForObject (CompPlugin *p,
			 CompObject *o)
{
    return TRUE;
}

static void
finiCorePluginForObject (CompPlugin *p,
			 CompObject *o)
{
}

static CompBool
setOptionForPlugin (CompObject      *object,
		    const char	    *plugin,
		    const char	    *name,
		    CompOptionValue *value)
{
    CompPlugin *p;

    p = findActivePlugin (plugin);
    if (p && p->vTable->setObjectOption)
	return (*p->vTable->setObjectOption) (p, object, name, value);

    return FALSE;
}

static void
coreObjectAdd (CompObject      *parent,
	       CompChildObject *object)
{
    object->parent = parent;
}

static void
coreObjectRemove (CompObject	  *parent,
		  CompChildObject *object)
{
    object->parent = NULL;
}

static void
fileWatchAdded (CompCore      *core,
		CompFileWatch *fileWatch)
{
}

static void
fileWatchRemoved (CompCore      *core,
		  CompFileWatch *fileWatch)
{
}

static CompBool
coreForEachObjectType (ObjectTypeCallBackProc proc,
		       void		      *closure)
{
    if (!(*proc) (getObjectType (), closure))
	return FALSE;

    if (!(*proc) (getCoreObjectType (), closure))
	return FALSE;

    if (!(*proc) (getDisplayObjectType (), closure))
	return FALSE;

    if (!(*proc) (getScreenObjectType (), closure))
	return FALSE;

    if (!(*proc) (getWindowObjectType (), closure))
	return FALSE;

    return TRUE;
}

static CompObjectVTable coreObjectVTable = {
    .forBaseObject	= coreForBaseObject,
    .forEachInterface   = coreForEachInterface,
    .getType		= coreGetType,
    .queryName		= coreQueryName,
    .forEachChildObject = coreForEachChildObject,
    .lookupChildObject	= coreLookupChildObject,
    .version.get	= commonGetVersion,
    .metadata.get	= coreGetMetadata
};

static CompObjectPrivates coreObjectPrivates = {
    0,
    NULL,
    0,
    offsetof (CompCore, privates),
    NULL,
    0
};

static CompBool
coreInitObject (CompObject *object)
{
    CORE (object);

    if (!compObjectInit (object, getObjectType ()))
	return FALSE;

    c->base.id = COMP_OBJECT_TYPE_CORE; /* XXX: remove id asap */

    if (!allocateObjectPrivates (object, &coreObjectPrivates))
    {
	compObjectFini (&c->base, getObjectType ());
	return FALSE;
    }

    c->tmpRegion = XCreateRegion ();
    if (!c->tmpRegion)
    {
	compObjectFini (&c->base, getObjectType ());
	return FALSE;
    }

    c->outputRegion = XCreateRegion ();
    if (!c->outputRegion)
    {
	XDestroyRegion (c->tmpRegion);
	compObjectFini (&c->base, getObjectType ());
	return FALSE;
    }

    WRAP (&c->object, &c->base, vTable, &coreObjectVTable);

    c->displays = NULL;

    c->fileWatch	   = NULL;
    c->lastFileWatchHandle = 1;

    c->timeouts		 = NULL;
    c->lastTimeoutHandle = 1;

    c->watchFds	         = NULL;
    c->lastWatchFdHandle = 1;
    c->watchPollFds	 = NULL;
    c->nWatchFds	 = 0;

    gettimeofday (&c->lastTimeout, 0);

    c->forEachObjectType = coreForEachObjectType;

    c->initPluginForObject = initCorePluginForObject;
    c->finiPluginForObject = finiCorePluginForObject;

    c->setOptionForPlugin = setOptionForPlugin;

    c->objectAdd    = coreObjectAdd;
    c->objectRemove = coreObjectRemove;

    c->fileWatchAdded   = fileWatchAdded;
    c->fileWatchRemoved = fileWatchRemoved;

    core.sessionInit  = sessionInit;
    core.sessionFini  = sessionFini;
    core.sessionEvent = sessionEvent;

    return TRUE;
}

static void
coreFiniObject (CompObject *object)
{
    CORE (object);

    XDestroyRegion (c->outputRegion);
    XDestroyRegion (c->tmpRegion);

    UNWRAP (&c->object, &c->base, vTable);

    if (c->privates)
	free (c->privates);

    compObjectFini (&c->base, getObjectType ());
}

static void
coreInitVTable (void *vTable)
{
    (*getObjectType ()->initVTable) (vTable);
}

static CompObjectType coreObjectType = {
    "core",
    {
	coreInitObject,
	coreFiniObject
    },
    &coreObjectPrivates,
    coreInitVTable
};

CompObjectType *
getCoreObjectType (void)
{
    static CompBool init = FALSE;

    if (!init)
    {
	coreInitVTable (&coreObjectVTable);
	init = TRUE;
    }

    return &coreObjectType;
}

int
allocateCorePrivateIndex (void)
{
    return compObjectAllocatePrivateIndex (&coreObjectType, 0);
}

void
freeCorePrivateIndex (int index)
{
    compObjectFreePrivateIndex (&coreObjectType, index);
}

CompBool
initCore (void)
{
    CompPlugin *corePlugin;

    if (!compObjectInit (&core.base, getCoreObjectType ()))
	return FALSE;

    corePlugin = loadPlugin ("core");
    if (!corePlugin)
    {
	compLogMessage (0, "core", CompLogLevelFatal,
			"Couldn't load core plugin");
	compObjectFini (&core.base, getCoreObjectType ());
	return FALSE;
    }

    if (!pushPlugin (corePlugin))
    {
	compLogMessage (0, "core", CompLogLevelFatal,
			"Couldn't activate core plugin");
	unloadPlugin (corePlugin);
	compObjectFini (&core.base, getCoreObjectType ());
	return FALSE;
    }

    return TRUE;
}

void
finiCore (void)
{
    while (core.displays)
	removeDisplay (core.displays);

    while (popPlugin ());

    compObjectFini (&core.base, getCoreObjectType ());
}

void
addDisplayToCore (CompDisplay *d)
{
    CompDisplay *prev;

    for (prev = core.displays; prev && prev->next; prev = prev->next);

    if (prev)
	prev->next = d;
    else
	core.displays = d;
}

CompFileWatchHandle
addFileWatch (const char	    *path,
	      int		    mask,
	      FileWatchCallBackProc callBack,
	      void		    *closure)
{
    CompFileWatch *fileWatch;

    fileWatch = malloc (sizeof (CompFileWatch));
    if (!fileWatch)
	return 0;

    fileWatch->path	= strdup (path);
    fileWatch->mask	= mask;
    fileWatch->callBack = callBack;
    fileWatch->closure  = closure;
    fileWatch->handle   = core.lastFileWatchHandle++;

    if (core.lastFileWatchHandle == MAXSHORT)
	core.lastFileWatchHandle = 1;

    fileWatch->next = core.fileWatch;
    core.fileWatch = fileWatch;

    (*core.fileWatchAdded) (&core, fileWatch);

    return fileWatch->handle;
}

void
removeFileWatch (CompFileWatchHandle handle)
{
    CompFileWatch *p = 0, *w;

    for (w = core.fileWatch; w; w = w->next)
    {
	if (w->handle == handle)
	    break;

	p = w;
    }

    if (w)
    {
	if (p)
	    p->next = w->next;
	else
	    core.fileWatch = w->next;

	(*core.fileWatchRemoved) (&core, w);

	if (w->path)
	    free (w->path);

	free (w);
    }
}
