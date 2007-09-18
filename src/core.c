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

static char *
coreNameObject (CompObject *object)
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

    CORE_CORE (object);

    for (d = c->displays; d; d = d->next)
	if (!(*proc) (&d->base, closure))
	    return FALSE;

    UNWRAP (&c->object, object, vTable);
    status = (*object->vTable->forEachChildObject) (object, proc, closure);
    WRAP (&c->object, object, vTable, v.vTable);

    return status;
}

static CompObject *
coreFindChildObject (CompObject *object,
		     const char *type,
		     const char *name)
{
    CompObjectVTableVec v = { object->vTable };
    CompObject		*result;

    CORE_CORE (object);

    if (strcmp (type, getDisplayObjectType ()->name) == 0)
	if (c->displays && (!name || !name[0]))
	    return &c->displays->base.base;

    UNWRAP (&c->object, object, vTable);
    result = (*object->vTable->findChildObject) (object, type, name);
    WRAP (&c->object, object, vTable, v.vTable);

    return result;
}

static CompMetadata *
coreGetObjectMetadata (CompObject *object,
		       const char *interface)
{
    CompObjectVTableVec v = { object->vTable };
    CompMetadata	*result;

    CORE_CORE (object);

    if (strcmp (interface, CORE_INTERFACE_NAME) == 0)
	return &coreMetadata;

    UNWRAP (&c->object, object, vTable);
    result = (*object->vTable->getMetadata) (object, interface);
    WRAP (&c->object, object, vTable, v.vTable);

    return result;
}

static CompOption *
coreGetObjectProps (CompObject *object,
		    const char *interface,
		    int	       *n)
{
    *n = 0;
    return NULL;
}

static CompBool
coreSetObjectProp (CompObject		 *object,
		   const char		 *interface,
		   const char		 *name,
		   const CompOptionValue *value)
{
    return FALSE;
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
    if (!(*proc) (getDisplayObjectType (), closure))
	return FALSE;

    if (!(*proc) (getScreenObjectType (), closure))
	return FALSE;

    if (!(*proc) (getWindowObjectType (), closure))
	return FALSE;

    return (*proc) (getCoreObjectType (), closure);
}

static CompObjectVTable coreObjectVTable = {
    coreNameObject,
    coreForEachChildObject,
    coreFindChildObject,
    coreGetObjectMetadata,
    coreGetObjectProps,
    coreSetObjectProp
};

static CompBool
coreInitObject (CompObject     *object,
		CompObjectType *type)
{
    CORE_CORE (object);

    if (!compObjectInit (object, type, COMP_OBJECT_TYPE_CORE))
	return FALSE;

    WRAP (&c->object, &c->base, vTable, &coreObjectVTable);

    c->displays = NULL;

    c->tmpRegion = XCreateRegion ();
    if (!c->tmpRegion)
	return FALSE;

    c->outputRegion = XCreateRegion ();
    if (!c->outputRegion)
    {
	XDestroyRegion (c->tmpRegion);
	return FALSE;
    }

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
    CORE_CORE (object);

    XDestroyRegion (c->outputRegion);
    XDestroyRegion (c->tmpRegion);

    UNWRAP (&c->object, &c->base, vTable);

    compObjectFini (&c->base);
}

static CompObjectFuncs coreObjectFuncs = {
    coreInitObject,
    coreFiniObject
};

static CompObjectType coreObjectType = {
    "core",
    NULL,
    0,
    NULL,
    &coreObjectFuncs
};

CompObjectType *
getCoreObjectType (void)
{
    return &coreObjectType;
}

int
allocateCorePrivateIndex (void)
{
    return compObjectAllocatePrivateIndex (&coreObjectType);
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

    if (!(*coreObjectType.funcs->init) (&core.base, &coreObjectType))
	return FALSE;

    corePlugin = loadPlugin ("core");
    if (!corePlugin)
    {
	compLogMessage (0, "core", CompLogLevelFatal,
			"Couldn't load core plugin");
	(*core.base.type->funcs->fini) (&core.base);
	return FALSE;
    }

    if (!pushPlugin (corePlugin))
    {
	compLogMessage (0, "core", CompLogLevelFatal,
			"Couldn't activate core plugin");
	unloadPlugin (corePlugin);
	(*core.base.type->funcs->fini) (&core.base);
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

    (*coreObjectType.funcs->fini) (&core.base);
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
