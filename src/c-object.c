/*
 * Copyright © 2008 Novell, Inc.
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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libxml/parser.h>

#include <compiz/error.h>
#include <compiz/c-object.h>

#define CHILD(data, child)				   \
    ((CompObject *) (((char *) (data)) + (child)->offset))

static void
cInsertObjectInterface (CompObject *object,
			CompObject *parent)
{
    CompObject *child;
    CMetadata  m;
    char       *data;
    int        i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);
    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    if (m.insert)
	(*m.insert) (object, parent);

    for (i = 0; i < m.nInterface; i++)
    {
	for (j = 0; j < m.interface[i].nChild; j++)
	{
	    if (m.interface[i].child[j].type)
	    {
		child = CHILD (data, &m.interface[i].child[j]);
		(*child->vTable->insertObject) (child, object,
						m.interface[i].child[j].name);
	    }
	}
    }
}

void
cInsertObject (CompObject *object,
	       CompObject *parent,
	       const char *name)
{
    FOR_BASE (object, (*object->vTable->insertObject) (object, parent, name));

    cInsertObjectInterface (object, parent);
}

static void
cRemoveObjectInterface (CompObject *object)
{
    CompObject *child;
    CMetadata  m;
    char       *data;
    int        i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);
    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    for (i = 0; i < m.nInterface; i++)
    {
	for (j = 0; j < m.interface[i].nChild; j++)
	{
	    if (m.interface[i].child[j].type)
	    {
		child = CHILD (data, &m.interface[i].child[j]);
		(*child->vTable->removeObject) (child);
	    }
	}
    }

    if (m.remove)
	(*m.remove) (object);
}

void
cRemoveObject (CompObject *object)
{
    cRemoveObjectInterface (object);

    FOR_BASE (object, (*object->vTable->removeObject) (object));
}

#define PROP_VALUE(data, prop, type)			  \
    (*((type *) (((char *) data) + (prop)->base.offset)))

void
cInserted (CompObject *object)
{
    CompObject *child;
    CMetadata  m;
    char       *data;
    int        i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);
    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    FOR_BASE (object, (*object->vTable->inserted) (object));

    for (i = 0; i < m.nInterface; i++)
    {
	(*object->vTable->interfaceAdded) (object, m.interface[i].name);

	for (j = 0; j < m.interface[i].nBoolProp; j++)
	{
	    CBoolProp *prop = &m.interface[i].boolProp[j];

	    (*object->vTable->properties.boolChanged) (object,
						       m.interface[i].name,
						       prop->base.name,
						       PROP_VALUE (data, prop,
								   CompBool));
	}

	for (j = 0; j < m.interface[i].nIntProp; j++)
	{
	    CIntProp *prop = &m.interface[i].intProp[j];

	    (*object->vTable->properties.intChanged) (object,
						      m.interface[i].name,
						      prop->base.name,
						      PROP_VALUE (data, prop,
								  int32_t));
	}

	for (j = 0; j < m.interface[i].nDoubleProp; j++)
	{
	    CDoubleProp *prop = &m.interface[i].doubleProp[j];

	    (*object->vTable->properties.doubleChanged) (object,
							 m.interface[i].name,
							 prop->base.name,
							 PROP_VALUE (data,
								     prop,
								     double));
	}

	for (j = 0; j < m.interface[i].nStringProp; j++)
	{
	    CStringProp *prop = &m.interface[i].stringProp[j];

	    (*object->vTable->properties.stringChanged) (object,
							 m.interface[i].name,
							 prop->base.name,
							 PROP_VALUE (data,
								     prop,
								     char *));
	}

	for (j = 0; j < m.interface[i].nChild; j++)
	{
	    if (m.interface[i].child[j].type)
	    {
		child = CHILD (data, &m.interface[i].child[j]);
		(*child->vTable->inserted) (child);
	    }
	}
    }
}

void
cRemoved (CompObject *object)
{
    CompObject *child;
    CMetadata  m;
    char       *data;
    int        i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);
    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    for (i = 0; i < m.nInterface; i++)
    {
	for (j = 0; j < m.interface[i].nChild; j++)
	{
	    if (m.interface[i].child[j].type)
	    {
		child = CHILD (data, &m.interface[i].child[j]);
		(*child->vTable->removed) (child);
	    }
	}

	(*object->vTable->interfaceRemoved) (object, m.interface[i].name);
    }

    FOR_BASE (object, (*object->vTable->removed) (object));
}

CompBool
cForEachInterface (CompObject	         *object,
		   InterfaceCallBackProc proc,
		   void		         *closure)
{
    CompBool  status;
    CMetadata m;
    int       i;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!(*proc) (object,
		      m.interface[i].name,
		      m.interface[i].offset,
		      m.interface[i].type,
		      closure))
	    return FALSE;

    FOR_BASE (object,
	      status = (*object->vTable->forEachInterface) (object,
							    proc,
							    closure));

    return status;
}

CompBool
cForEachMethod (CompObject	   *object,
		const char	   *interface,
		MethodCallBackProc proc,
		void	           *closure)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
    {
	if (interface)
	    if (*interface && strcmp (interface, m.interface[i].name))
		continue;

	for (j = 0; j < m.interface[i].nMethod; j++)
	    if (!(*proc) (object,
			  m.interface[i].method[j].name,
			  m.interface[i].method[j].in,
			  m.interface[i].method[j].out,
			  m.interface[i].method[j].offset,
			  m.interface[i].method[j].marshal,
			  closure))
		return FALSE;
    }

    FOR_BASE (object,
	      status = (*object->vTable->forEachMethod) (object,
							 interface,
							 proc,
							 closure));

    return status;
}

CompBool
cForEachSignal (CompObject	   *object,
		const char	   *interface,
		SignalCallBackProc proc,
		void		   *closure)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
    {
	if (interface)
	    if (*interface && strcmp (interface, m.interface[i].name))
		continue;

	for (j = 0; j < m.interface[i].nSignal; j++)
	    if (m.interface[i].signal[j]->out)
		if (!(*proc) (object,
			      m.interface[i].signal[j]->name,
			      m.interface[i].signal[j]->out,
			      m.interface[i].signal[j]->offset,
			      closure))
		    return FALSE;
    }

    FOR_BASE (object,
	      status = (*object->vTable->forEachSignal) (object,
							 interface,
							 proc,
							 closure));

    return status;
}

CompBool
cForEachProp (CompObject       *object,
	      const char       *interface,
	      PropCallBackProc proc,
	      void	       *closure)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
    {
	if (interface)
	    if (*interface && strcmp (interface, m.interface[i].name))
		continue;

	for (j = 0; j < m.interface[i].nBoolProp; j++)
	    if (!(*proc) (object, m.interface[i].boolProp[j].base.name,
			  COMP_TYPE_BOOLEAN, closure))
		return FALSE;

	for (j = 0; j < m.interface[i].nIntProp; j++)
	    if (!(*proc) (object, m.interface[i].intProp[j].base.name,
			  COMP_TYPE_INT32, closure))
		return FALSE;

	for (j = 0; j < m.interface[i].nDoubleProp; j++)
	    if (!(*proc) (object, m.interface[i].doubleProp[j].base.name,
			  COMP_TYPE_DOUBLE, closure))
		return FALSE;

	for (j = 0; j < m.interface[i].nStringProp; j++)
	    if (!(*proc) (object, m.interface[i].stringProp[j].base.name,
			  COMP_TYPE_STRING, closure))
		return FALSE;
    }

    FOR_BASE (object,
	      status = (*object->vTable->forEachProp) (object,
						       interface,
						       proc,
						       closure));

    return status;
}

CompBool
cForEachChildObject (CompObject		     *object,
		     ChildObjectCallBackProc proc,
		     void		     *closure)
{
    CompBool   status;
    CMetadata  m;
    char       *data;
    int        i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);
    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    for (i = 0; i < m.nInterface; i++)
	for (j = 0; j < m.interface[i].nChild; j++)
	    if (!(*proc) (CHILD (data, &m.interface[i].child[j]), closure))
		return FALSE;

    FOR_BASE (object,
	      status = (*object->vTable->forEachChildObject) (object,
							      proc,
							      closure));

    return status;
}

typedef struct _HandleConnectContext {
    const char		   *interface;
    size_t		   offset;
    char		   *in;
    CompBool		   out;
    const CompObjectVTable *vTable;
    MethodMarshalProc	   marshal;
} HandleConnectContext;

static CompBool
connectMethod (CompObject	 *object,
	       const char	 *name,
	       const char	 *in,
	       const char	 *out,
	       size_t		 offset,
	       MethodMarshalProc marshal,
	       void		 *closure)
{
    HandleConnectContext *pCtx = (HandleConnectContext *) closure;

    if (offset != pCtx->offset)
	return TRUE;

    if (out[0] != '\0')
    {
	pCtx->out = TRUE;
	return FALSE;
    }

    pCtx->out     = FALSE;
    pCtx->in      = strdup (in);
    pCtx->offset  = offset;
    pCtx->marshal = marshal;

    return FALSE;
}

static CompBool
connectInterface (CompObject	       *object,
		  const char	       *name,
		  size_t	       offset,
		  const CompObjectType *type,
		  void		       *closure)
{
    HandleConnectContext *pCtx = (HandleConnectContext *) closure;

    if (strcmp (name, pCtx->interface) == 0)
    {
	if (!(*object->vTable->forEachMethod) (object, name, connectMethod,
					       closure))
	{
	    /* method must not have any output arguments */
	    if (pCtx->out)
		return TRUE;
	}

	/* need vTable if interface is not part of object type */
	if (!type)
	    pCtx->vTable = object->vTable;

	/* add interface vTable offset to method offset set by
	   connectMethod function */
	pCtx->offset += offset;

	return FALSE;
    }

    return TRUE;
}

static CompBool
signalIndex (const char	      *name,
	     const CInterface *interface,
	     int	      nInterface,
	     size_t	      offset,
	     int	      *index,
	     const char	      **signature)
{
    int i, j;

    for (i = 0; i < nInterface; i++)
    {
	if (strcmp (name, interface[i].name) == 0)
	{
	    for (j = 0; j < interface[i].nSignal; j++)
	    {
		if (offset == interface[i].signal[j]->offset)
		{
		    if (signature)
			*signature = interface[i].signal[j]->out;

		    *index = interface[i].signal[j]->index;

		    return TRUE;
		}
	    }

	    *index = -1;

	    return TRUE;
	}
    }

    return FALSE;
}

static int
getSignalVecSize (const CInterface *interface,
		  int		   nInterface)
{
    int	i, size = 0;

    for (i = 0; i < nInterface; i++)
	size += interface[i].nSignal;

    return size;
}

CompBool
handleConnect (CompObject	 *object,
	       const CInterface  *interface,
	       int		 nInterface,
	       int		 *signalVecOffset,
	       const char	 *name,
	       size_t		 offset,
	       CompObject	 *descendant,
	       const char	 *descendantInterface,
	       size_t		 descendantOffset,
	       const char	 *details,
	       va_list		 args,
	       int		 *id)
{
    const char *in;
    int	       index;

    if (signalIndex (name, interface, nInterface, offset, &index, &in))
    {
	HandleConnectContext ctx;
	CompSignalHandler    *handler;
	int		     size;
	CompSignalHandler    **vec;

	if (index < 0)
	    return -1;

	/* make sure details match if signal got a signature */
	if (in && details && strncmp (in, details, strlen (details)))
	    return -1;

	if (!details)
	    details = "";

	ctx.interface = descendantInterface;
	ctx.in	      = NULL;
	ctx.vTable    = NULL;
	ctx.offset    = descendantOffset;
	ctx.marshal   = NULL;

	if ((*descendant->vTable->forEachInterface) (descendant,
						     connectInterface,
						     (void *) &ctx))
	    return -1;

	/* make sure signatures match */
	if (ctx.in && in && strcmp (ctx.in, in))
	{
	    free (ctx.in);
	    return -1;
	}

	if (ctx.in)
	    free (ctx.in);

	vec = compGetSignalVecRange (object,
				     getSignalVecSize (interface, nInterface),
				     signalVecOffset);
	if (!vec)
	    return -1;

	size = compSerializeMethodCall (object,
					descendant,
					descendantInterface,
					name,
					details,
					args,
					NULL,
					0);

	handler = malloc (sizeof (CompSignalHandler) + size);
	if (!handler)
	    return -1;

	handler->next    = NULL;
	handler->object  = descendant;
	handler->vTable  = ctx.vTable;
	handler->offset  = ctx.offset;
	handler->marshal = ctx.marshal;
	handler->header  = (CompSerializedMethodCallHeader *) (handler + 1);

	compSerializeMethodCall (object,
				 descendant,
				 descendantInterface,
				 name,
				 details,
				 args,
				 handler->header,
				 size);

	if (vec[index])
	{
	    CompSignalHandler *last;

	    for (last = vec[index]; last->next; last = last->next);

	    handler->id = last->id + 1;
	    last->next  = handler;
	}
	else
	{
	    handler->id = 1;
	    vec[index]  = handler;
	}

	*id = handler->id;
	return TRUE;
    }

    return FALSE;
}

CompBool
handleDisconnect (CompObject	   *object,
		  const CInterface *interface,
		  int		   nInterface,
		  int		   *signalVecOffset,
		  const char	   *name,
		  size_t	   offset,
		  int		   id)
{
    int	index;

    if (signalIndex (name, interface, nInterface, offset, &index, NULL))
    {
	CompSignalHandler *handler, *prev = NULL;
	CompSignalHandler **vec = object->signalVec;

	if (index < 0)
	    return TRUE;

	if (signalVecOffset)
	{
	    if (!*signalVecOffset)
		return TRUE;

	    object->signalVec += *signalVecOffset;
	}

	for (handler = vec[index]; handler; handler = handler->next)
	{
	    if (handler->id == id)
	    {
		if (prev)
		    prev->next = handler->next;
		else
		    vec[index] = handler->next;

		break;
	    }

	    prev = handler;
	}

	if (handler)
	    free (handler);

	return TRUE;
    }

    return FALSE;
}

int
cConnect (CompObject *object,
	  const char *interface,
	  size_t     offset,
	  CompObject *descendant,
	  const char *descendantInterface,
	  size_t     descendantOffset,
	  const char *details,
	  va_list    args)
{
    CompInterfaceData *data;
    CMetadata	      m;
    int		      id;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);
    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    if (handleConnect (object,
		       m.interface, m.nInterface,
		       &data->signalVecOffset,
		       interface,
		       offset,
		       descendant,
		       descendantInterface,
		       descendantOffset,
		       details,
		       args,
		       &id))
	return id;

    FOR_BASE (object,
	      id = (*object->vTable->signal.connect) (object,
						      interface,
						      offset,
						      descendant,
						      descendantInterface,
						      descendantOffset,
						      details,
						      args));

    return id;
}

void
cDisconnect (CompObject *object,
	     const char *interface,
	     size_t     offset,
	     int	id)
{
    CompInterfaceData *data;
    CMetadata	      m;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);
    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    if (handleDisconnect (object,
			  m.interface, m.nInterface,
			  &data->signalVecOffset,
			  interface,
			  offset,
			  id))
	return;

    FOR_BASE (object, (*object->vTable->signal.disconnect) (object,
							    interface,
							    offset,
							    id));
}

int
cGetVersion (CompObject *object,
	     const char *interface)
{
    CMetadata  m;
    int        version, i;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (strcmp (interface, m.interface[i].name) == 0)
	    return m.version;

    FOR_BASE (object,
	      version = (*object->vTable->version.get) (object, interface));

    return version;
}

static CompBool
handleGetBoolProp (CompObject	   *object,
		   const CBoolProp *prop,
		   CompBool	   *value)
{
    char *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    *value = *((CompBool *) (data + prop->base.offset));

    return TRUE;
}

CompBool
cGetBoolProp (CompObject *object,
	      const char *interface,
	      const char *name,
	      CompBool   *value,
	      char	 **error)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nBoolProp; j++)
		if (strcmp (name, m.interface[i].boolProp[j].base.name) == 0)
		    return handleGetBoolProp (object,
					      &m.interface[i].boolProp[j],
					      value);

    FOR_BASE (object,
	      status = (*object->vTable->properties.getBool) (object,
							      interface,
							      name,
							      value,
							      error));

    return status;
}

static CompBool
handleSetBoolProp (CompObject	   *object,
		   const CBoolProp *prop,
		   const char	   *interface,
		   const char	   *name,
		   CompBool	   value,
		   char		   **error)
{
    CompBool *ptr, oldValue;
    char     *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    ptr = (CompBool *) (data + prop->base.offset);
    oldValue = *ptr;

    if (prop->set)
    {
	if (!(*prop->set) (object, interface, name, value, error))
	    return FALSE;
    }
    else
    {
	*ptr = value;
    }

    if (!*ptr != !oldValue)
	(*object->vTable->properties.boolChanged) (object,
						   interface, name,
						   *ptr);

    return TRUE;
}

CompBool
cSetBoolProp (CompObject *object,
	      const char *interface,
	      const char *name,
	      CompBool   value,
	      char	 **error)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nBoolProp; j++)
		if (strcmp (name, m.interface[i].boolProp[j].base.name) == 0)
		    return handleSetBoolProp (object,
					      &m.interface[i].boolProp[j],
					      interface, name,
					      value, error);

    FOR_BASE (object,
	      status = (*object->vTable->properties.setBool) (object,
							      interface,
							      name,
							      value,
							      error));

    return status;
}

void
cBoolPropChanged (CompObject *object,
		  const char *interface,
		  const char *name,
		  CompBool   value)
{
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nBoolProp; j++)
		if (strcmp (name, m.interface[i].boolProp[j].base.name) == 0)
		    if (m.interface[i].boolProp[j].changed)
			(*m.interface[i].boolProp[j].changed) (object,
							       interface,
							       name,
							       value);

    FOR_BASE (object, (*object->vTable->properties.boolChanged) (object,
								 interface,
								 name,
								 value));
}

static CompBool
handleGetIntProp (CompObject	 *object,
		  const CIntProp *prop,
		  int32_t	 *value)
{
    char *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    *value = *((int32_t *) (data + prop->base.offset));

    return TRUE;
}

CompBool
cGetIntProp (CompObject *object,
	     const char *interface,
	     const char *name,
	     int32_t    *value,
	     char	**error)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nIntProp; j++)
		if (strcmp (name, m.interface[i].intProp[j].base.name) == 0)
		    return handleGetIntProp (object,
					     &m.interface[i].intProp[j],
					     value);

    FOR_BASE (object,
	      status = (*object->vTable->properties.getInt) (object,
							      interface,
							      name,
							      value,
							      error));

    return status;
}

static CompBool
handleSetIntProp (CompObject	 *object,
		  const CIntProp *prop,
		  const char	 *interface,
		  const char	 *name,
		  int32_t	 value,
		  char		 **error)
{
    int32_t *ptr, oldValue;
    char    *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    ptr = (int32_t *) (data + prop->base.offset);
    oldValue = *ptr;

    if (prop->set)
    {
	if (!(*prop->set) (object, interface, name, value, error))
	    return FALSE;
    }
    else
    {
	if (prop->restriction)
	{
	    if (value > prop->max)
	    {
		if (error)
		    *error = strdup ("Value is greater than maximium "
				     "allowed value");

		return FALSE;
	    }
	    else if (value < prop->min)
	    {
		if (error)
		    *error = strdup ("Value is less than minimuim "
				     "allowed value");

		return FALSE;
	    }
	}

	*ptr = value;
    }

    if (*ptr != oldValue)
	(*object->vTable->properties.intChanged) (object,
						  interface, name,
						  *ptr);

    return TRUE;
}

CompBool
cSetIntProp (CompObject *object,
	     const char *interface,
	     const char *name,
	     int32_t    value,
	     char	**error)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nIntProp; j++)
		if (strcmp (name, m.interface[i].intProp[j].base.name) == 0)
		    return handleSetIntProp (object,
					     &m.interface[i].intProp[j],
					     interface, name,
					     value, error);

    FOR_BASE (object,
	      status = (*object->vTable->properties.setInt) (object,
							     interface,
							     name,
							     value,
							     error));

    return status;
}

void
cIntPropChanged (CompObject *object,
		 const char *interface,
		 const char *name,
		 int32_t    value)
{
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nIntProp; j++)
		if (strcmp (name, m.interface[i].intProp[j].base.name) == 0)
		    if (m.interface[i].intProp[j].changed)
			(*m.interface[i].intProp[j].changed) (object,
							      interface,
							      name,
							      value);

    FOR_BASE (object, (*object->vTable->properties.intChanged) (object,
								interface,
								name,
								value));
}

static CompBool
handleGetDoubleProp (CompObject	       *object,
		     const CDoubleProp *prop,
		     double	       *value)
{
    char *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    *value = *((double *) (data + prop->base.offset));

    return TRUE;
}

CompBool
cGetDoubleProp (CompObject *object,
		const char *interface,
		const char *name,
		double	   *value,
		char	   **error)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nDoubleProp; j++)
		if (strcmp (name, m.interface[i].doubleProp[j].base.name) == 0)
		    return handleGetDoubleProp (object,
						&m.interface[i].doubleProp[j],
						value);

    FOR_BASE (object,
	      status = (*object->vTable->properties.getDouble) (object,
								interface,
								name,
								value,
								error));

    return status;
}

static CompBool
handleSetDoubleProp (CompObject	       *object,
		     const CDoubleProp *prop,
		     const char	       *interface,
		     const char	       *name,
		     double	       value,
		     char	       **error)
{
    double *ptr, oldValue;
    char   *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    ptr = (double *) (data + prop->base.offset);
    oldValue = *ptr;

    if (prop->set)
    {
	if (!(*prop->set) (object, interface, name, value, error))
	    return FALSE;
    }
    else
    {
	if (prop->restriction)
	{
	    if (value > prop->max)
	    {
		if (error)
		    *error = strdup ("Value is greater than maximium "
				     "allowed value");

		return FALSE;
	    }
	    else if (value < prop->min)
	    {
		if (error)
		    *error = strdup ("Value is less than minimuim "
				     "allowed value");

		return FALSE;
	    }
	}

	*ptr = value;
    }

    if (*ptr != oldValue)
	(*object->vTable->properties.doubleChanged) (object,
						     interface, name,
						     *ptr);

    return TRUE;
}

CompBool
cSetDoubleProp (CompObject *object,
		const char *interface,
		const char *name,
		double	   value,
		char	   **error)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nDoubleProp; j++)
		if (strcmp (name, m.interface[i].doubleProp[j].base.name) == 0)
		    return handleSetDoubleProp (object,
						&m.interface[i].doubleProp[j],
						interface, name,
						value, error);

    FOR_BASE (object,
	      status = (*object->vTable->properties.setDouble) (object,
								interface,
								name,
								value,
								error));

    return status;
}

void
cDoublePropChanged (CompObject *object,
		    const char *interface,
		    const char *name,
		    double     value)
{
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nDoubleProp; j++)
		if (strcmp (name, m.interface[i].doubleProp[j].base.name) == 0)
		    if (m.interface[i].doubleProp[j].changed)
			(*m.interface[i].doubleProp[j].changed) (object,
								 interface,
								 name,
								 value);

    FOR_BASE (object, (*object->vTable->properties.doubleChanged) (object,
								   interface,
								   name,
								   value));
}

static CompBool
handleGetStringProp (CompObject	       *object,
		     const CStringProp *prop,
		     char	       **value,
		     char	       **error)
{
    char *data, *s;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    s = strdup (*((char **) (data + prop->base.offset)));
    if (!s)
    {
	if (error)
	    *error = strdup ("Failed to copy string value");

	return FALSE;
    }

    *value = s;

    return TRUE;
}

CompBool
cGetStringProp (CompObject *object,
		const char *interface,
		const char *name,
		char	   **value,
		char	   **error)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nStringProp; j++)
		if (strcmp (name, m.interface[i].stringProp[j].base.name) == 0)
		    return handleGetStringProp (object,
						&m.interface[i].stringProp[j],
						value, error);

    FOR_BASE (object,
	      status = (*object->vTable->properties.getString) (object,
								interface,
								name,
								value,
								error));

    return status;
}

static CompBool
handleSetStringProp (CompObject	       *object,
		     const CStringProp *prop,
		     const char	       *interface,
		     const char	       *name,
		     const char	       *value,
		     char	       **error)
{
    char **ptr, *oldValue;
    char *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    ptr = (char **) (data + prop->base.offset);
    oldValue = *ptr;

    if (prop->set)
    {
	if (!(*prop->set) (object, interface, name, value, error))
	    return FALSE;
    }
    else
    {
	if (strcmp (*ptr, value))
	{
	    char *s;

	    s = strdup (value);
	    if (!s)
	    {
		if (error)
		    *error = strdup ("Failed to copy string value");

		return FALSE;
	    }

	    free (*ptr);
	    *ptr = s;
	}
    }

    if (*ptr != oldValue)
	(*object->vTable->properties.stringChanged) (object,
						     interface, name,
						     *ptr);

    return TRUE;
}

CompBool
cSetStringProp (CompObject *object,
		const char *interface,
		const char *name,
		const char *value,
		char	   **error)
{
    CompBool  status;
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nStringProp; j++)
		if (strcmp (name, m.interface[i].stringProp[j].base.name) == 0)
		    return handleSetStringProp (object,
						&m.interface[i].stringProp[j],
						interface, name,
						value, error);

    FOR_BASE (object,
	      status = (*object->vTable->properties.setString) (object,
								interface,
								name,
								value,
								error));

    return status;
}

void
cStringPropChanged (CompObject *object,
		    const char *interface,
		    const char *name,
		    const char *value)
{
    CMetadata m;
    int       i, j;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (!interface || strcmp (interface, m.interface[i].name) == 0)
	    for (j = 0; j < m.interface[i].nStringProp; j++)
		if (strcmp (name, m.interface[i].stringProp[j].base.name) == 0)
		    if (m.interface[i].stringProp[j].changed)
			(*m.interface[i].stringProp[j].changed) (object,
								 interface,
								 name,
								 value);

    FOR_BASE (object, (*object->vTable->properties.stringChanged) (object,
								   interface,
								   name,
								   value));
}

#define HOME_DATADIR   ".compiz-0/data"
#define XML_EXTENSION ".xml"

typedef CompBool (*ForMetadataFileProc) (FILE	     *fp,
					 const char  *path,
					 struct stat *buf,
					 void	     *closure);

static CompBool
forMetadataFile (const char	     *path,
		 ForMetadataFileProc proc,
		 void		     *closure)
{
    FILE	*fp;
    struct stat buf;
    CompBool	status = TRUE;

    if (stat (path, &buf) == 0)
    {
	fp = fopen (path, "r");
	if (fp)
	{
	    status = (*proc) (fp, path, &buf, closure);
	    fclose (fp);
	}
    }

    return status;
}

static CompBool
forEachMetadataFile (const char	         *file,
		     ForMetadataFileProc proc,
		     void		 *closure)
{
    CompBool status = TRUE;
    char     *home;
    char     *path;

    home = getenv ("HOME");
    if (home)
    {
	path = malloc (strlen (home) + strlen (HOME_DATADIR) +
		       strlen (file) + strlen (XML_EXTENSION) + 2);
	if (path)
	{
	    sprintf (path, "%s/%s%s%s", home, HOME_DATADIR, file,
		     XML_EXTENSION);

	    status = forMetadataFile (path, proc, closure);

	    free (path);
	}
    }

    if (status)
    {
	path = malloc (strlen (METADATADIR) + strlen (file) +
		       strlen (XML_EXTENSION) + 2);
	if (path)
	{
	    sprintf (path, "%s/%s%s", METADATADIR, file, XML_EXTENSION);

	    status = forMetadataFile (path, proc, closure);

	    free (path);
	}
    }

    return status;
}

static CompBool
readMetadata (FILE	  *fp,
	      const char  *path,
	      struct stat *buf,
	      void	  *closure)
{
    char *data;

    data = malloc (buf->st_size + 1);
    if (data)
    {
	data[fread (data, 1, buf->st_size, fp)] = '\0';
	*((char **) closure) = data;

	return FALSE;
    }

    return TRUE;
}

CompBool
handleGetMetadata (CompObject *object,
		   const char *interface,
		   char       **data,
		   char       **error)
{
    if (forEachMetadataFile (interface, readMetadata, data))
    {
	char *str;

	str = strdup ("<compiz/>");
	if (str)
	{
	    *data = str;
	}
	else
	{
	    esprintf (error, NO_MEMORY_ERROR_STRING);
	    return FALSE;
	}
    }

    return TRUE;
}

CompBool
cGetMetadata (CompObject *object,
	      const char *interface,
	      char	 **data,
	      char	 **error)
{
    CompBool  status;
    CMetadata m;
    int       i;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	if (strcmp (interface, m.interface[i].name) == 0)
	    return handleGetMetadata (object, interface, data, error);


    FOR_BASE (object, status = (*object->vTable->metadata.get) (object,
								interface,
								data,
								error));

    return status;
}


static const CompObjectVTable cVTable = {
    .setProp = cSetObjectProp,

    .insertObject = cInsertObject,
    .removeObject = cRemoveObject,
    .inserted     = cInserted,
    .removed      = cRemoved,

    .forEachInterface = cForEachInterface,
    .forEachMethod    = cForEachMethod,
    .forEachSignal    = cForEachSignal,
    .forEachProp      = cForEachProp,

    .forEachChildObject = cForEachChildObject,

    .signal.connect    = cConnect,
    .signal.disconnect = cDisconnect,

    .version.get = cGetVersion,

    .properties.getBool     = cGetBoolProp,
    .properties.setBool     = cSetBoolProp,
    .properties.boolChanged = cBoolPropChanged,

    .properties.getInt     = cGetIntProp,
    .properties.setInt     = cSetIntProp,
    .properties.intChanged = cIntPropChanged,

    .properties.getDouble     = cGetDoubleProp,
    .properties.setDouble     = cSetDoubleProp,
    .properties.doubleChanged = cDoublePropChanged,

    .properties.getString     = cGetStringProp,
    .properties.setString     = cSetStringProp,
    .properties.stringChanged = cStringPropChanged,

    .metadata.get = cGetMetadata
};

static CompBool
cObjectAllocPrivateIndex (CompFactory    *factory,
			  CObjectPrivate *private)
{
    int	index;

    index = (*factory->allocatePrivateIndex) (factory,
					      private->name,
					      private->size);
    if (index < 0)
	return FALSE;

    *(private->pIndex) = index;

    return TRUE;
}

static void
cObjectFreePrivateIndex (CompFactory    *factory,
			 CObjectPrivate *private)
{
    (*factory->freePrivateIndex) (factory, private->name, *(private->pIndex));
}

CompBool
cObjectAllocPrivateIndices (CompFactory    *factory,
			    CObjectPrivate *private,
			    int	           nPrivate)
{
    int	i;

    for (i = 0; i < nPrivate; i++)
	if (!cObjectAllocPrivateIndex (factory, &private[i]))
	    break;

    if (i < nPrivate)
    {
	if (i)
	    cObjectFreePrivateIndices (factory, private, i - 1);

	return FALSE;
    }

    return TRUE;
}

void
cObjectFreePrivateIndices (CompFactory	  *factory,
			   CObjectPrivate *private,
			   int		  nPrivate)
{
    int	n = nPrivate;

    while (n--)
	cObjectFreePrivateIndex (factory, &private[n]);
}

static CompBool
setIndex (int *index,
	  int value)
{
    *index = value;
    return TRUE;
}

static CompBool
propertyIndex (CInterface *interface,
	       const char *name,
	       int	  type,
	       int	  *index)
{
    int i;

    switch (type) {
    case COMP_TYPE_BOOLEAN:
	for (i = 0; i < interface->nBoolProp; i++)
	    if (strcmp (name, interface->boolProp[i].base.name) == 0)
		return setIndex (index, i);
	break;
    case COMP_TYPE_INT32:
	for (i = 0; i < interface->nIntProp; i++)
	    if (strcmp (name, interface->intProp[i].base.name) == 0)
		return setIndex (index, i);
	break;
    case COMP_TYPE_DOUBLE:
	for (i = 0; i < interface->nDoubleProp; i++)
	    if (strcmp (name, interface->doubleProp[i].base.name) == 0)
		return setIndex (index, i);
	break;
    case COMP_TYPE_STRING:
	for (i = 0; i < interface->nStringProp; i++)
	    if (strcmp (name, interface->stringProp[i].base.name) == 0)
		return setIndex (index, i);
	break;
    }

    return FALSE;
}

static const char *
attrValue (const xmlChar **atts,
	   const char    *name)
{
    int i;

    for (i = 0; atts[i]; i += 2)
	if (strcmp ((const char *) atts[i], name) == 0)
	    return (const char *) atts[i + 1];

    return NULL;
}

typedef struct _DefaultValuesContext {
    CInterface *interface;
    int	       nInterface;
    int	       depth;
    int	       validDepth;
    CInterface *currentInterface;
    int	       propType;
    int	       propIndex;
} DefaultValuesContext;

static void
handleStartElement (void	  *data,
		    const xmlChar *name,
		    const xmlChar **atts)
{
    DefaultValuesContext *pCtx = (DefaultValuesContext *) data;

    if (pCtx->validDepth == pCtx->depth++)
    {
	switch (pCtx->validDepth) {
	case 0:
	    if (strcmp ((const char *) name, "compiz") == 0)
		pCtx->validDepth = pCtx->depth;
	    break;
	case 1:
	    if (strcmp ((const char *) name, "interface") == 0)
	    {
		const char *nameAttr = attrValue (atts, "name");

		if (nameAttr)
		{
		    int i;

		    for (i = 0; i < pCtx->nInterface; i++)
		    {
			if (strcmp (nameAttr, pCtx->interface[i].name) == 0)
			{
			    pCtx->validDepth       = pCtx->depth;
			    pCtx->currentInterface = &pCtx->interface[i];
			    break;
			}
		    }
		}
	    }
	    break;
	case 2:
	    if (strcmp ((const char *) name, "property") == 0)
	    {
		const char *typeAttr = attrValue (atts, "type");

		if (typeAttr && typeAttr[1] == '\0')
		{
		    static int types[] = {
			COMP_TYPE_BOOLEAN,
			COMP_TYPE_INT32,
			COMP_TYPE_DOUBLE,
			COMP_TYPE_STRING
		    };
		    int i;

		    for (i = 0; i < N_ELEMENTS (types); i++)
			if (typeAttr[0] == types[i])
			    break;

		    if (i < N_ELEMENTS (types))
		    {
			const char *nameAttr = attrValue (atts, "name");

			if (nameAttr)
			{
			    if (propertyIndex (pCtx->currentInterface,
					       nameAttr, types[i],
					       &pCtx->propIndex))
			    {
				pCtx->propType	 = types[i];
				pCtx->validDepth = pCtx->depth;
			    }
			}
		    }
		}
	    }
	    break;
	case 3:
	    if (strcmp ((const char *) name, "default") == 0)
		pCtx->validDepth = pCtx->depth;
	    break;
	}
    }
}

static void
handleEndElement (void		*data,
		  const xmlChar *name)
{
    DefaultValuesContext *pCtx = (DefaultValuesContext *) data;

    if (pCtx->validDepth == pCtx->depth--)
	pCtx->validDepth = pCtx->depth;
}

static void
handleCharacters (void		*data,
		  const xmlChar *ch,
		  int		len)
{
    DefaultValuesContext *pCtx = (DefaultValuesContext *) data;

    if (pCtx->validDepth == 4)
    {
	switch (pCtx->propType) {
	case COMP_TYPE_BOOLEAN: {
	    CBoolProp *prop =
		&pCtx->currentInterface->boolProp[pCtx->propIndex];

	    if (len == 4 && strncmp ((const char *) ch, "true", 4) == 0)
		prop->defaultValue = TRUE;
	    else
		prop->defaultValue = FALSE;
	} break;
	case COMP_TYPE_INT32: {
	    char tmp[256];

	    if (len < sizeof (tmp))
	    {
		CIntProp *prop =
		    &pCtx->currentInterface->intProp[pCtx->propIndex];

		strncpy (tmp, (const char *) ch, len);
		tmp[len] = '\0';

		prop->defaultValue = strtol (tmp, NULL, 0);
	    }
	} break;
	case COMP_TYPE_DOUBLE: {
	    char tmp[256];

	    if (len < sizeof (tmp))
	    {
		CDoubleProp *prop =
		    &pCtx->currentInterface->doubleProp[pCtx->propIndex];

		strncpy (tmp, (const char *) ch, len);
		tmp[len] = '\0';

		prop->defaultValue = strtod (tmp, NULL);
	    }
	} break;
	case COMP_TYPE_STRING: {
	    char *value;

	    value = malloc (len + 1);
	    if (value)
	    {
		CStringProp *prop =
		    &pCtx->currentInterface->stringProp[pCtx->propIndex];

		strncpy (value, (const char *) ch, len);
		value[len] = '\0';

		if (prop->data)
		    free (prop->data);

		prop->defaultValue = prop->data	= value;
	    }
	} break;
	}
    }
}

static void
cVerfiyDefaultValues (CInterface *interface,
		      int	 nInterface)
{
    int i, j;

    for (i = 0; i < nInterface; i++)
    {
	for (j = 0; j < interface[i].nIntProp; j++)
	    interface[i].intProp[j].defaultValue =
		MAX (MIN (interface[i].intProp[j].defaultValue,
			  interface[i].intProp[j].max),
		     interface[i].intProp[j].min);
	for (j = 0; j < interface[i].nDoubleProp; j++)
	    interface[i].doubleProp[j].defaultValue =
		MAX (MIN (interface[i].doubleProp[j].defaultValue,
			  interface[i].doubleProp[j].max),
		     interface[i].doubleProp[j].min);
    }
}

static CompBool
parseDefaultValues (FILE	*fp,
		    const char  *path,
		    struct stat *buf,
		    void	*closure)
{
    xmlSAXHandler saxHandler = {
	.initialized  = XML_SAX2_MAGIC,
	.startElement = handleStartElement,
	.endElement   = handleEndElement,
	.characters   = handleCharacters
    };

    if (!xmlSAXUserParseFile (&saxHandler, closure, path))
	return TRUE;

    return FALSE;
}

static void
cDefaultValuesFromFile (CInterface *interface,
			int	   nInterface,
			const char *name)
{
    DefaultValuesContext ctx = {
	.interface  = interface,
	.nInterface = nInterface
    };

    forEachMetadataFile (name, parseDefaultValues, (void *) &ctx);

    cVerfiyDefaultValues (interface, nInterface);
}

#define SET_DEFAULT_VALUE(data, prop, type)		 \
    PROP_VALUE (data, prop, type) = (prop)->defaultValue

CompBool
cObjectPropertiesInit (CompObject	*object,
		       char		*data,
		       const CInterface *interface,
		       int		nInterface)
{
    int i, j;

    for (i = 0; i < nInterface; i++)
    {
	for (j = 0; j < interface[i].nBoolProp; j++)
	    SET_DEFAULT_VALUE (data, &interface[i].boolProp[j], CompBool);

	for (j = 0; j < interface[i].nIntProp; j++)
	    SET_DEFAULT_VALUE (data, &interface[i].intProp[j], int32_t);

	for (j = 0; j < interface[i].nDoubleProp; j++)
	    SET_DEFAULT_VALUE (data, &interface[i].doubleProp[j], double);

	for (j = 0; j < interface[i].nStringProp; j++)
	{
	    if (interface[i].stringProp[j].defaultValue)
	    {
		char *str;

		str = strdup (interface[i].stringProp[j].defaultValue);
		if (!str)
		{
		    while (j--)
		    {
			str = PROP_VALUE (data,
					  &interface[i].stringProp[j],
					  char *);
			if (str)
			    free (str);
		    }

		    while (i--)
			cObjectPropertiesFini (object, data, &interface[i], 1);

		    return FALSE;
		}

		PROP_VALUE (data, &interface[i].stringProp[j], char *) = str;
	    }
	    else
	    {
		PROP_VALUE (data, &interface[i].stringProp[j], char *) = NULL;
	    }
	}
    }

    return TRUE;
}

void
cObjectPropertiesFini (CompObject	*object,
		       char		*data,
		       const CInterface *interface,
		       int		nInterface)
{
    int i, j;

    for (i = 0; i < nInterface; i++)
    {
	for (j = 0; j < interface[i].nStringProp; j++)
	{
	    char *str;

	    str = PROP_VALUE (data, &interface[i].stringProp[j], char *);
	    if (str)
		free (str);
	}
    }
}

CompBool
cObjectChildrenInit (CompObject		     *object,
		     char		     *data,
		     const CInterface	     *interface,
		     int		     nInterface,
		     const CompObjectFactory *factory)
{
    CompObject *child;
    int	       i, j;

    for (i = 0; i < nInterface; i++)
    {
	for (j = 0; j < interface[i].nChild; j++)
	{
	    CChildObject *cChild = &interface[i].child[j];

	    if (cChild->type)
	    {
		child = CHILD (data, cChild);

		if (!compObjectInitByTypeName (factory, child, cChild->type))
		{
		    while (j--)
		    {
			cChild = &interface[i].child[j];
			if (cChild->type)
			{
			    child = CHILD (data, cChild);
			    (*child->vTable->finalize) (child);
			}
		    }

		    while (i--)
			cObjectChildrenFini (object, data, &interface[i], 1);

		    return FALSE;
		}
	    }
	}
    }

    if (object->parent)
    {
	for (i = 0; i < nInterface; i++)
	{
	    for (j = 0; j < interface[i].nChild; j++)
	    {
		CChildObject *cChild = &interface[i].child[j];

		if (cChild->type)
		{
		    child = CHILD (data, cChild);

		    (*child->vTable->insertObject) (child, object,
						    cChild->name);
		    (*child->vTable->inserted) (child);
		}
	    }
	}
    }

    return TRUE;
}

void
cObjectChildrenFini (CompObject	      *object,
		     char	      *data,
		     const CInterface *interface,
		     int	      nInterface)
{
    CompObject *child;
    int	       i, j;

    if (object->parent)
    {
	for (i = 0; i < nInterface; i++)
	{
	    for (j = 0; j < interface[i].nChild; j++)
	    {
		CChildObject *cChild = &interface[i].child[j];

		if (cChild->type)
		{
		    child = CHILD (data, cChild);

		    (*child->vTable->removed) (child);
		    (*child->vTable->removeObject) (child);
		}
	    }
	}
    }

    for (i = 0; i < nInterface; i++)
    {
	for (j = 0; j < interface[i].nChild; j++)
	{
	    CChildObject *cChild = &interface[i].child[j];

	    if (cChild->type)
	    {
		child = CHILD (data, &interface[i].child[j]);
		(*child->vTable->finalize) (child);
	    }
	}
    }
}

static CompBool
cInitObjectInterface (CompObject	      *object,
		      const CompObjectFactory *factory,
		      CompInterfaceData	      *data)
{
    CMetadata m;
    int	      i, j, signalIndex = 0;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
    {
	if (!m.interface[i].initialized)
	{
	    CInterface *interface = (CInterface *) &m.interface[i];

	    for (j = 0; j < interface->nSignal; j++)
	    {
		interface->signal[j]->index     = signalIndex + j;
		interface->signal[j]->interface = interface->name;
	    }

	    cDefaultValuesFromFile (interface, 1, interface->name);

	    interface->initialized = TRUE;
	}

	signalIndex += m.interface[i].nSignal;
    }

    data->signalVecOffset = 0;

    if (!cObjectPropertiesInit (object, (char *) data,
				m.interface, m.nInterface))
	return FALSE;

    if (!cObjectChildrenInit (object, (char *) data,
			      m.interface, m.nInterface,
			      factory))
    {
	cObjectPropertiesFini (object, (char *) data,
			       m.interface, m.nInterface);
	return FALSE;
    }

    if (m.init && !(*m.init) (object))
    {
	cObjectChildrenFini (object, (char *) data, m.interface, m.nInterface);
	cObjectPropertiesFini (object, (char *) data,
			       m.interface, m.nInterface);
	return FALSE;
    }

    return TRUE;
}

static void
cFiniObjectInterface (CompObject	*object,
		      CompInterfaceData	*data)
{
    CMetadata m;

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    if (m.fini)
	(*m.fini) (object);

    cObjectChildrenFini (object, (char *) data, m.interface, m.nInterface);
    cObjectPropertiesFini (object, (char *) data, m.interface, m.nInterface);

    if (data->signalVecOffset)
	compFreeSignalVecRange (object,
				getSignalVecSize (m.interface, m.nInterface),
				data->signalVecOffset);
}

CompBool
cObjectInterfaceInit (const CompObjectInstantiator *instantiator,
		      CompObject		   *object,
		      const CompObjectFactory      *factory)
{
    const CompObjectInstantiator *base = instantiator->base;
    const CompObjectVTable	 *vTable;
    CompInterfaceData	         *data;

    if (!(*base->init) (base, object, factory))
	return FALSE;

    vTable = object->vTable;
    object->vTable = instantiator->vTable;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    data->vTable = vTable;

    if (!cInitObjectInterface (object, factory, data))
    {
	object->vTable = vTable;
	(*object->vTable->finalize) (object);
	return FALSE;
    }

    return TRUE;
}

void
cObjectInterfaceFini (CompObject *object)
{
    CompInterfaceData *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    cFiniObjectInterface (object, data);

    object->vTable = data->vTable;

    (*object->vTable->finalize) (object);
}

CompBool
cObjectInit (const CompObjectInstantiator *instantiator,
	     CompObject			  *object,
	     const CompObjectFactory      *factory)
{
    CompObjectData		     *data;
    CMetadata			     m;
    int				     i;
    const CompObjectInstantiatorNode *node =
	(const CompObjectInstantiatorNode *) instantiator;
    const CompObjectInstantiatorNode *baseNode =
	(const CompObjectInstantiatorNode *) node->base.base;
    const CompObjectInstantiator     *base = baseNode->instantiator;
    const CompObjectVTable	     *vTable;

    if (!(*base->init) (base, object, factory))
	return FALSE;

    vTable = object->vTable;
    object->vTable = instantiator->vTable;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    data->base.vTable = vTable;

    if (!cInitObjectInterface (object, factory, &data->base))
    {
	object->vTable = vTable;
	(*object->vTable->finalize) (object);
	return FALSE;
    }

    (*object->vTable->getProp) (object, COMP_PROP_C_METADATA, (void *) &m);

    for (i = 0; i < m.nInterface; i++)
	((CInterface *) m.interface)[i].type = node->type;

    data->privates = allocatePrivates (node->privates.len,
				       node->privates.sizes,
				       node->privates.totalSize);
    if (!data->privates)
    {
	cFiniObjectInterface (object, &data->base);
	object->vTable = vTable;
	(*object->vTable->finalize) (object);
	return FALSE;
    }

    return TRUE;
}

void
cObjectFini (CompObject *object)
{
    CompObjectData *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    if (data->privates)
	free (data->privates);

    cFiniObjectInterface (object, &data->base);

    object->vTable = data->base.vTable;

    (*object->vTable->finalize) (object);
}

void
cGetInterfaceProp (CompInterfaceData *data,
		   const CMetadata   *template,
		   unsigned int	     what,
		   void		     *value)
{
    switch (what) {
    case COMP_PROP_BASE_VTABLE:
	*((const CompObjectVTable **) value) = data->vTable;
	break;
    case COMP_PROP_C_DATA:
	*((CompInterfaceData **) value) = data;
	break;
    case COMP_PROP_C_METADATA:
	*((CMetadata *) value) = *template;
	break;
    }
}

void
cGetObjectProp (CompObjectData	*data,
		const CMetadata *template,
		unsigned int	what,
		void		*value)
{
    switch (what) {
    case COMP_PROP_PRIVATES:
	*((CompPrivate **) value) = data->privates;
	break;
    default:
	cGetInterfaceProp (&data->base, template, what, value);
	break;
    }
}

void
cSetInterfaceProp (CompObject   *object,
		   unsigned int what,
		   void	        *value)
{
    CompInterfaceData *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    switch (what) {
    case COMP_PROP_BASE_VTABLE:
	data->vTable = *((const CompObjectVTable **) value);
	break;
    }
}

void
cSetObjectProp (CompObject   *object,
		unsigned int what,
		void	     *value)
{
    CompObjectData *data;

    (*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

    switch (what) {
    case COMP_PROP_PRIVATES:
	data->privates = *((CompPrivate **) value);
	break;
    default:
	cSetInterfaceProp (object, what, value);
	break;
    }
}

CompObjectType *
cObjectTypeFromTemplate (const CompObjectType *template)
{
    CompObjectType   *type;
    CompObjectVTable *vTable;
    int		     nameSize = strlen (template->name.name) + 1;
    int		     baseNameSize = 0;
    int		     vTableSize = template->vTable.size;
    int		     noopVTableSize = 0;

    if (template->name.base)
	baseNameSize = strlen (template->name.base) + 1;
    else
	baseNameSize = strlen (OBJECT_TYPE_NAME) + 1;

    if (!vTableSize)
	vTableSize = sizeof (CompObjectVTable);

    if (template->vTable.noop)
	noopVTableSize = vTableSize;

    type = malloc (sizeof (CompObjectType) +
		   nameSize +
		   baseNameSize +
		   vTableSize +
		   noopVTableSize);
    if (!type)
	return NULL;

    type->name.name = strcpy ((char *) (type + 1), template->name.name);
    type->name.base = NULL;

    type->vTable.size = vTableSize;
    type->vTable.impl = NULL;
    type->vTable.noop = NULL;

    type->instance.init = cObjectInit;
    type->instance.size = template->instance.size;

    if (template->name.base)
	type->name.base = strcpy ((char *) (type + 1) + nameSize,
				  template->name.base);
    else
	type->name.base = strcpy ((char *) (type + 1) + nameSize,
				  OBJECT_TYPE_NAME);

    vTable = (CompObjectVTable *) ((char *) (type + 1) + nameSize +
				   baseNameSize);

    if (template->vTable.impl)
	type->vTable.impl = memcpy (vTable, template->vTable.impl, vTableSize);
    else
	type->vTable.impl = memset (vTable, 0, vTableSize);

    if (!vTable->finalize)
	vTable->finalize = cObjectFini;

    compVTableInit (vTable, &cVTable, sizeof (CompObjectVTable));

    if (template->vTable.noop)
	type->vTable.noop = memcpy ((char *) vTable + vTableSize,
				    template->vTable.noop, vTableSize);

    if (template->instance.init)
	type->instance.init = template->instance.init;

    return type;
}

static CompBool
topObjectType (CompObject	    *object,
	       const char	    *name,
	       size_t		    offset,
	       const CompObjectType *type,
	       void		    *closure)
{
    if (type)
    {
	*((const CompObjectType **) closure) = type;
	return FALSE;
    }

    return TRUE;
}

typedef struct _InitObjectContext {
    const CompObjectFactory *factory;
    const CompObjectType    *type;
    const CompObjectVTable  *vTable;
    CompObject		    *object;
} InitObjectContext;

static CompBool
initBaseObject (CompObject *object,
		void	   *closure)
{
    InitObjectContext    *pCtx = (InitObjectContext *) closure;
    const CompObjectType *type;

    (*object->vTable->forEachInterface) (object,
					 topObjectType,
					 (void *) &type);

    if (type == pCtx->type)
    {
	const CompObjectVTable *vTable = object->vTable;
	CompInterfaceData      *data;

	object->vTable = pCtx->vTable;

	(*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

	data->vTable = vTable;

	if (!cInitObjectInterface (object, pCtx->factory, data))
	{
	    object->vTable = vTable;
	    return FALSE;
	}

	return TRUE;
    }
    else
    {
	const CompObjectVTable *baseVTable, *vTable = object->vTable;
	CompBool	       status = TRUE;

	(*object->vTable->getProp) (object,
				    COMP_PROP_BASE_VTABLE,
				    (void *) &baseVTable);

	if (baseVTable)
	{
	    object->vTable = baseVTable;

	    status = initBaseObject (object, closure);

	    baseVTable = object->vTable;
	    object->vTable = vTable;

	    (*object->vTable->setProp) (object,
					COMP_PROP_BASE_VTABLE,
					(void *) &baseVTable);
	}

	return status;
    }
}

static CompBool
finiBaseObject (CompObject *object,
		void	   *closure)
{
    InitObjectContext    *pCtx = (InitObjectContext *) closure;
    const CompObjectType *type;

    (*object->vTable->forEachInterface) (object,
					 topObjectType,
					 (void *) &type);

    if (type == pCtx->type)
    {
	CompInterfaceData *data;

	(*object->vTable->getProp) (object, COMP_PROP_C_DATA, (void *) &data);

	cFiniObjectInterface (object, data);

	object->vTable = data->vTable;
    }
    else
    {
	const CompObjectVTable *baseVTable, *vTable = object->vTable;

	(*object->vTable->getProp) (object,
				    COMP_PROP_BASE_VTABLE,
				    (void *) &baseVTable);

	if (baseVTable)
	{
	    object->vTable = baseVTable;

	    finiBaseObject (object, closure);

	    baseVTable = object->vTable;
	    object->vTable = vTable;

	    (*object->vTable->setProp) (object,
					COMP_PROP_BASE_VTABLE,
					(void *) &baseVTable);
	}
    }

    return TRUE;
}

static CompBool
initTypedObjects (const CompObjectFactory *factory,
		  CompObject              *object,
		  const CompObjectType    *type,
		  const CompObjectVTable  *vTable);

static CompBool
finiTypedObjects (const CompObjectFactory *factory,
		  CompObject              *object,
		  const CompObjectType    *type);

static CompBool
initObjectTree (CompObject *object,
		void	   *closure)
{
    InitObjectContext *pCtx = (InitObjectContext *) closure;

    pCtx->object = object;

    return initTypedObjects (pCtx->factory, object, pCtx->type,
			     pCtx->vTable);
}

static CompBool
finiObjectTree (CompObject *object,
		void	   *closure)
{
    InitObjectContext *pCtx = (InitObjectContext *) closure;

    /* pCtx->object is set to the object that failed to be initialized */
    if (pCtx->object == object)
	return FALSE;

    return finiTypedObjects (pCtx->factory, object, pCtx->type);
}

static CompBool
initTypedObjects (const CompObjectFactory *factory,
		  CompObject              *object,
		  const CompObjectType    *type,
		  const CompObjectVTable  *vTable)
{
    InitObjectContext ctx;

    ctx.factory = factory;
    ctx.type    = type;
    ctx.vTable  = vTable;
    ctx.object  = NULL;

    if (!initBaseObject (object, (void *) &ctx))
	return FALSE;

    if (!(*object->vTable->forEachChildObject) (object,
						initObjectTree,
						(void *) &ctx))
    {
	(*object->vTable->forEachChildObject) (object,
					       finiObjectTree,
					       (void *) &ctx);

	finiBaseObject (object, (void *) &ctx);

	return FALSE;
    }

    return TRUE;
}

static CompBool
finiTypedObjects (const CompObjectFactory *factory,
		  CompObject              *object,
		  const CompObjectType    *type)
{
    InitObjectContext ctx;

    ctx.factory = factory;
    ctx.type    = type;
    ctx.object  = NULL;

    (*object->vTable->forEachChildObject) (object,
					   finiObjectTree,
					   (void *) &ctx);

    finiBaseObject (object, (void *) &ctx);

    return TRUE;
}

static CompBool
cObjectInitPrivate (CompBranch	   *branch,
		    CObjectPrivate *private)
{
    CompObjectInstantiatorNode *node;
    CompObjectInstantiator     *instantiator;

    node = compObjectInstantiatorNode (&branch->factory, private->name);
    if (!node)
	return FALSE;

    instantiator = malloc (sizeof (CompObjectInstantiator) +
			   private->vTableSize);
    if (!instantiator)
	return FALSE;

    instantiator->base   = NULL;
    instantiator->init   = cObjectInterfaceInit;
    instantiator->vTable = (CompObjectVTable *) (instantiator + 1);

    if (private->vTableSize)
    {
	const CompObjectInstantiatorNode *n;
	const CompObjectInstantiator     *p;

	memcpy (instantiator->vTable, private->vTable, private->vTableSize);

	if (!instantiator->vTable->finalize)
	    instantiator->vTable->finalize = cObjectInterfaceFini;

	compVTableInit (instantiator->vTable, &cVTable,
			sizeof (CompObjectVTable));

	for (p = &node->base; p; p = p->base)
	{
	    n = (const CompObjectInstantiatorNode *) p;

	    if (n->type->vTable.noop)
		compVTableInit (instantiator->vTable, n->type->vTable.noop,
				n->type->vTable.size);
	}
    }
    else
    {
	instantiator->vTable = NULL;
    }

    /* initialize all objects of this type */
    if (!initTypedObjects (&branch->factory, &branch->u.base, node->type,
			   instantiator->vTable))
	return FALSE;

    instantiator->base = node->instantiator;
    node->instantiator = instantiator;

    return TRUE;
}

static void
cObjectFiniPrivate (CompBranch	   *branch,
		    CObjectPrivate *private)
{
    CompObjectInstantiatorNode *node;
    CompObjectInstantiator     *instantiator;

    node = compObjectInstantiatorNode (&branch->factory, private->name);

    instantiator = (CompObjectInstantiator *) node->instantiator;
    node->instantiator = instantiator->base;

    /* finalize all objects of this type */
    finiTypedObjects (&branch->factory, &branch->u.base, node->type);

    free (instantiator);
}

CompBool
cObjectInitPrivates (CompBranch	    *branch,
		     CObjectPrivate *private,
		     int	    nPrivate)
{
    int	i;

    for (i = 0; i < nPrivate; i++)
	if (!cObjectInitPrivate (branch, &private[i]))
	    break;

    if (i < nPrivate)
    {
	if (i)
	    cObjectFiniPrivates (branch, private, i - 1);

	return FALSE;
    }

    return TRUE;
}

void
cObjectFiniPrivates (CompBranch	    *branch,
		     CObjectPrivate *private,
		     int	    nPrivate)
{
    int	n = nPrivate;

    while (n--)
	cObjectFiniPrivate (branch, &private[n]);
}
