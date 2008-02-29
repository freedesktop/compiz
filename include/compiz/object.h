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

#ifndef _COMPIZ_OBJECT_H
#define _COMPIZ_OBJECT_H

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

#include <compiz/types.h>
#include <compiz/macros.h>
#include <compiz/privates.h>

COMPIZ_BEGIN_DECLS

/*
  object names

  - must only contain the ASCII characters "[A-Z][a-z][0-9]_"
  - may not be the empty string
  - may not be exceed the maximum length of 255 characters

  member names

  - must only contain the ASCII characters "[A-Z][a-z][0-9]_"
  - may not begin with a digit
  - may not be the empty string
  - may not be exceed the maximum length of 255 characters

  interface names

  - composed of 1 or more elements separated by a period ('.') character
  - must not begin with a '.' (period) character
  - may not be exceed the maximum length of 255 characters
  - all elements must only contain the ASCII characters "[A-Z][a-z][0-9]_"
  - all elements must not begin with a digit
  - all elements must not be the empty string
*/

typedef struct _CompObject	       CompObject;
typedef struct _CompObjectType	       CompObjectType;
typedef struct _CompObjectType	       CompObjectInterface;
typedef struct _CompObjectFactory      CompObjectFactory;
typedef struct _CompObjectVTable       CompObjectVTable;
typedef struct _CompObjectInstantiator CompObjectInstantiator;

typedef CompBool (*InitObjectProc) (const CompObjectInstantiator *i,
				    CompObject			 *object,
				    const CompObjectFactory	 *factory);

typedef struct _CompObjectPrivates {
    int	len;
    int	*sizes;
    int	totalSize;
} CompObjectPrivates;

struct _CompObjectInstantiator {
    const CompObjectInstantiator *base;
    const CompObjectInterface	 *interface;
    InitObjectProc		 init;
    CompObjectVTable		 *vTable;
};

typedef struct _CompObjectInstantiatorNode {
    CompObjectInstantiator	       base;
    struct _CompObjectInstantiatorNode *next;
    const CompObjectInstantiator       *instantiator;
    CompObjectPrivates		       privates;
} CompObjectInstantiatorNode;

struct _CompObjectFactory {
    const CompObjectFactory    *master;
    CompObjectInstantiatorNode *instantiators;
};

typedef struct _CompObjectPrivatesNode {
    struct _CompObjectPrivatesNode *next;
    const char			   *name;
    CompObjectPrivates		   privates;
} CompObjectPrivatesNode;

typedef struct _CompFactory CompFactory;

typedef int (*AllocatePrivateIndexProc) (CompFactory *factory,
					 const char  *name,
					 int	     size);

typedef void (*FreePrivateIndexProc) (CompFactory *factory,
				      const char  *name,
				      int	  index);

struct _CompFactory {
    CompObjectFactory base;

    AllocatePrivateIndexProc allocatePrivateIndex;
    FreePrivateIndexProc     freePrivateIndex;

    CompObjectPrivatesNode *privates;
};

typedef CompBool (*InstallProc)   (const CompObjectInterface *interface,
				   CompFactory		      *factory,
				   char			      **error);
typedef void     (*UninstallProc) (const CompObjectInterface *interface,
				   CompFactory	             *factory);

typedef CompBool (*InitInterfaceProc) (CompObject	       *object,
				       const CompObjectVTable  *vTable,
				       const CompObjectFactory *factory);
typedef void     (*FiniInterfaceProc) (CompObject *object);

struct _CompObjectType {
    const char *name;
    int	       version;

    struct {
	const char *name;
	int	   version;
    } base;

    struct {
	const CompObjectVTable *impl;
	const CompObjectVTable *noop;
	size_t		       size;
    } vTable;

    struct {
	InitObjectProc init;
	size_t	       size;
    } instance;

    struct {
	InstallProc   install;
	UninstallProc uninstall;
    } factory;

    struct {
	InitInterfaceProc init;
	FiniInterfaceProc fini;
    } interface;
};

typedef unsigned int CompObjectTypeID;

#define COMP_OBJECT_TYPE_CORE    0
#define COMP_OBJECT_TYPE_DISPLAY 1
#define COMP_OBJECT_TYPE_SCREEN  2
#define COMP_OBJECT_TYPE_WINDOW  3

/* compiz uses a sub-set of the type-codes in the dbus specification

   BOOLEAN	98  (ASCII 'b')	Boolean value, 0 is FALSE and 1 is TRUE.
   INT32	105 (ASCII 'i')	32-bit signed integer.
   DOUBLE	100 (ASCII 'd')	IEEE 754 double.
   STRING	115 (ASCII 's')	Nul terminated UTF-8 string.
   OBJECT	111 (ASCII 'o')	Object path
*/

#define COMP_TYPE_INVALID ((int) '\0')

#define COMP_TYPE_BOOLEAN ((int) 'b')
#define COMP_TYPE_INT32   ((int) 'i')
#define COMP_TYPE_DOUBLE  ((int) 'd')
#define COMP_TYPE_STRING  ((int) 's')
#define COMP_TYPE_OBJECT  ((int) 'o')

typedef union {
    CompBool b;
    int32_t  i;
    double   d;
    char     *s;
} CompAnyValue;

typedef struct _CompArgs CompArgs;

typedef void (*ArgsLoadProc) (CompArgs *args,
			      int      type,
			      void     *value);

typedef void (*ArgsStoreProc) (CompArgs	*args,
			       int      type,
			       void	*value);

typedef void (*ArgsErrorProc) (CompArgs	*args,
			       char	*error);

struct _CompArgs {
    ArgsLoadProc  load;
    ArgsStoreProc store;
    ArgsErrorProc error;
};

typedef void (*FinalizeObjectProc) (CompObject *object);

#define COMP_PROP_BASE_VTABLE 0
#define COMP_PROP_PRIVATES    1

typedef void (*GetPropProc) (CompObject   *object,
			     unsigned int what,
			     void	  *value);

typedef void (*SetPropProc) (CompObject   *object,
			     unsigned int what,
			     void	  *value);

typedef void (*InsertObjectProc) (CompObject *object,
				  CompObject *parent,
				  const char *name);

typedef void (*RemoveObjectProc) (CompObject *object);

typedef void (*InsertedProc) (CompObject *object);

typedef void (*RemovedProc)  (CompObject *object);

typedef CompBool (*InterfaceCallBackProc) (CompObject		     *object,
					   const CompObjectInterface *iface,
					   void			     *closure);

typedef CompBool (*ForEachInterfaceProc) (CompObject		*object,
					  InterfaceCallBackProc proc,
					  void		        *closure);

typedef void (*MethodMarshalProc) (CompObject *object,
				   void	      (*method) (void),
				   CompArgs   *args);

typedef CompBool (*MethodCallBackProc) (CompObject	  *object,
					const char	  *name,
					const char	  *in,
					const char	  *out,
					size_t		  offset,
					MethodMarshalProc marshal,
					void		  *closure);

typedef CompBool (*ForEachMethodProc) (CompObject		 *object,
				       const CompObjectInterface *interface,
				       MethodCallBackProc	 proc,
				       void			 *closure);

typedef CompBool (*SignalCallBackProc) (CompObject *object,
					const char *name,
					const char *out,
					size_t	   offset,
					void	   *closure);

typedef CompBool (*ForEachSignalProc) (CompObject		 *object,
				       const CompObjectInterface *interface,
				       SignalCallBackProc	 proc,
				       void			 *closure);

typedef CompBool (*PropCallBackProc) (CompObject *object,
				      const char *name,
				      int	 type,
				      void	 *closure);

typedef CompBool (*ForEachPropProc) (CompObject		       *object,
				     const CompObjectInterface *interface,
				     PropCallBackProc	       proc,
				     void		       *closure);

typedef void (*InterfaceAddedProc) (CompObject *object,
				    const char *interface);

typedef void (*InterfaceRemovedProc) (CompObject *object,
				      const char *interface);

typedef CompBool (*AddChildObjectProc) (CompObject *object,
					CompObject *child,
					const char *name);

typedef CompObject *(*RemoveChildObjectProc) (CompObject *object,
					      const char *name);

typedef CompBool (*ChildObjectCallBackProc) (CompObject *object,
					     void	*closure);

typedef CompBool (*ForEachChildObjectProc) (CompObject		    *object,
					    ChildObjectCallBackProc proc,
					    void		    *closure);

typedef CompObject *(*LookupChildObjectProc) (CompObject *object,
					      const char *name);

typedef int (*ConnectProc) (CompObject		      *object,
			    const CompObjectInterface *interface,
			    size_t		      offset,
			    CompObject		      *descendant,
			    const CompObjectInterface *descendantInterface,
			    size_t		      descendantOffset,
			    const char		      *details,
			    va_list		      args);

typedef int (*ConnectAsyncProc) (CompObject		   *object,
				 CompObject		   *descendant,
				 const CompObjectInterface *descendantIface,
				 size_t			   descendantOffset,
				 CompObject		   *target,
				 const CompObjectInterface *targetInterface,
				 size_t			   targetOffset);

typedef void (*DisconnectProc) (CompObject		  *object,
				const CompObjectInterface *interface,
				size_t			  offset,
				int			  index);

typedef void (*SignalProc) (CompObject   *object,
			    const char   *path,
			    const char   *interface,
			    const char   *name,
			    const char   *signature,
			    CompAnyValue *value,
			    int	         nValue);

typedef CompBool (*GetBoolPropProc) (CompObject *object,
				     const char *interface,
				     const char *name,
				     CompBool   *value,
				     char	**error);

typedef CompBool (*SetBoolPropProc) (CompObject *object,
				     const char *interface,
				     const char *name,
				     CompBool   value,
				     char	**error);

typedef void (*BoolPropChangedProc) (CompObject *object,
				     const char *interface,
				     const char *name,
				     CompBool   value);

typedef CompBool (*GetIntPropProc) (CompObject *object,
				    const char *interface,
				    const char *name,
				    int32_t    *value,
				    char       **error);

typedef CompBool (*SetIntPropProc) (CompObject *object,
				    const char *interface,
				    const char *name,
				    int32_t    value,
				    char       **error);

typedef void (*IntPropChangedProc) (CompObject *object,
				    const char *interface,
				    const char *name,
				    int32_t    value);

typedef CompBool (*GetDoublePropProc) (CompObject *object,
				       const char *interface,
				       const char *name,
				       double	  *value,
				       char	  **error);

typedef CompBool (*SetDoublePropProc) (CompObject *object,
				       const char *interface,
				       const char *name,
				       double	  value,
				       char	  **error);

typedef void (*DoublePropChangedProc) (CompObject *object,
				       const char *interface,
				       const char *name,
				       double     value);

typedef CompBool (*GetStringPropProc) (CompObject *object,
				       const char *interface,
				       const char *name,
				       char	  **value,
				       char	  **error);

typedef CompBool (*SetStringPropProc) (CompObject *object,
				       const char *interface,
				       const char *name,
				       const char *value,
				       char	  **error);

typedef void (*StringPropChangedProc) (CompObject *object,
				       const char *interface,
				       const char *name,
				       const char *value);

typedef void (*LogProc) (CompObject *object,
			 const char *interface,
			 const char *member,
			 const char *message);

struct _CompObjectVTable {

    /* finalize function*/
    FinalizeObjectProc finalize;

    /* abstract property functions */
    GetPropProc getProp;
    SetPropProc setProp;

    /* object tree functions

       used when inserting and removing objects from an object tree
    */
    InsertObjectProc insertObject;
    RemoveObjectProc removeObject;

    /* interface functions

       object interfaces are provided by implementing some of these
    */
    ForEachInterfaceProc forEachInterface;
    ForEachMethodProc    forEachMethod;
    ForEachSignalProc    forEachSignal;
    ForEachPropProc      forEachProp;

    /* child object functions

       child objects are provided by implementing these
     */
    AddChildObjectProc     addChild;
    RemoveChildObjectProc  removeChild;
    ForEachChildObjectProc forEachChildObject;
    LookupChildObjectProc  lookupChildObject;

    ConnectProc      connect;
    ConnectAsyncProc connectAsync;
    DisconnectProc   disconnect;

    GetBoolPropProc getBool;
    SetBoolPropProc setBool;

    GetIntPropProc getInt;
    SetIntPropProc setInt;

    GetDoublePropProc getDouble;
    SetDoublePropProc setDouble;

    GetStringPropProc getString;
    SetStringPropProc setString;


    /* signals */

    SignalProc signal;

    InsertedProc inserted;
    RemovedProc  removed;

    InterfaceAddedProc   interfaceAdded;
    InterfaceRemovedProc interfaceRemoved;

    BoolPropChangedProc   boolChanged;
    IntPropChangedProc    intChanged;
    DoublePropChangedProc doubleChanged;
    StringPropChangedProc stringChanged;

    LogProc log;
};

typedef struct _CompObjectVTableVec {
    const CompObjectVTable *vTable;
} CompObjectVTableVec;

CompBool
compObjectInit (const CompObjectFactory	     *factory,
		CompObject		     *object,
		const CompObjectInstantiator *instantiator);

CompBool
compObjectInitByType (const CompObjectFactory *factory,
		      CompObject	      *object,
		      const CompObjectType    *type);

CompBool
compObjectInitByTypeName (const CompObjectFactory *factory,
			  CompObject		  *object,
			  const char		  *name);

const CompObjectType *
compLookupObjectType (const CompObjectFactory *factory,
		      const char	      *name);

const CompObjectInterface *
compLookupObjectInterface (const CompObjectFactory *factory,
			   const char		   *name);

const CompObjectInstantiatorNode *
compGetObjectInstantiatorNode (const CompObjectFactory *factory,
			       const CompObjectType    *type);

void
compVTableInit (CompObjectVTable       *vTable,
		const CompObjectVTable *noopVTable,
		int		       size);

CompBool
compFactoryInstallType (CompObjectFactory    *factory,
			const CompObjectType *type,
			char		     **error);

const CompObjectType *
compFactoryUninstallType (CompObjectFactory *factory);

CompBool
compFactoryInstallInterface (CompObjectFactory	       *factory,
			     CompObject		       *root,
			     const CompObjectType      *type,
			     const CompObjectInterface *interface,
			     char		       **error);

const CompObjectInterface *
compFactoryUninstallInterface (CompObjectFactory    *factory,
			       CompObject	    *root,
			       const CompObjectType *type);

typedef struct _CompSerializedMethodCallHeader {
    char	 *path;
    char	 *interface;
    char	 *name;
    char	 *signature;
    CompAnyValue *value;
    int		 nValue;
} CompSerializedMethodCallHeader;

typedef struct _CompSignalHandler {
    struct _CompSignalHandler	   *next;
    int				   id;
    CompObject			   *object;
    const CompObjectVTable	   *vTable;
    size_t			   offset;
    MethodMarshalProc		   marshal;
    CompSerializedMethodCallHeader *header;
} CompSignalHandler;

typedef struct _CompChild {
    CompObject *ref;
    char       *name;
} CompChild;

struct _CompObject {
    const CompObjectVTable *vTable;
    const char		   *name;
    CompObject		   *parent;
    CompSignalHandler      **signalVec;
    CompChild		   *child;
    int			   nChild;
    CompPrivate		   *privates;

    CompObjectTypeID id;
};

#define GET_OBJECT(object) ((CompObject *) (object))
#define OBJECT(object) CompObject *o = GET_OBJECT (object)

#define COMPIZ_OBJECT_VERSION   20080221
#define COMPIZ_OBJECT_TYPE_NAME "org.compiz.object"

/* useful structures for object and interface implementations */

typedef struct _CompInterfaceData {
    const CompObjectVTable *vTable;
    int		           signalVecOffset;
} CompInterfaceData;

typedef struct _CompObjectData {
    CompInterfaceData base;
    CompPrivate       *privates;
} CompObjectData;

#define INVOKE_HANDLER_PROC(object, offset, prototype, ...)		\
    (*(*((prototype *)							\
	 (((char *) (object)->vTable) +					\
	  (offset))))) ((void *) object, ##__VA_ARGS__)

#define MATCHING_DETAILS(handler, ...)					\
    ((!(handler)->header->signature) ||					\
     compCheckEqualityOfValuesAndArgs ((handler)->header->signature,	\
				       (handler)->header->value,	\
				       ##__VA_ARGS__))

#define EMIT_SIGNAL(source, type, vOffset, ...)				\
    do									\
    {									\
	CompSignalHandler   *handler = (source)->signalVec[(vOffset)];	\
	CompObjectVTableVec save;					\
									\
	while (handler)							\
	{								\
	    if (MATCHING_DETAILS (handler, ##__VA_ARGS__))		\
	    {								\
		if (handler->vTable)					\
		{							\
		    save.vTable = handler->object->vTable;		\
									\
		    UNWRAP (handler, handler->object, vTable);		\
		    INVOKE_HANDLER_PROC (handler->object,		\
					 handler->offset,		\
					 type, ##__VA_ARGS__);		\
		    WRAP (handler, handler->object, vTable,		\
			  save.vTable);					\
		}							\
		else							\
		{							\
		    INVOKE_HANDLER_PROC (handler->object,		\
					 handler->offset,		\
					 type, ##__VA_ARGS__);		\
		}							\
	    }								\
									\
	    handler = handler->next;					\
	}								\
    } while (0)

#define FOR_BASE(object, ...)						\
    do {								\
	const CompObjectVTable *__selfVTable = (object)->vTable;	\
									\
	(*(object)->vTable->getProp) (object,				\
				      COMP_PROP_BASE_VTABLE,		\
				      (void *) &(object)->vTable);	\
									\
	__VA_ARGS__;							\
									\
	(object)->vTable = __selfVTable;				\
    } while (0)


const CompObjectType *
getObjectType (void);

CompBool
compForInterface (CompObject		*object,
		  const char		*interface,
		  InterfaceCallBackProc proc,
		  void			*closure);

CompObject *
compLookupDescendant (CompObject *object,
		      const char *path);

CompObject *
compLookupDescendantVa (CompObject *object,
			const char *name,
			...);

CompBool
compInvokeMethodWithArgs (CompObject *object,
			  const char *interface,
			  const char *name,
			  const char *in,
			  const char *out,
			  CompArgs   *args);

CompBool
compInvokeMethod (CompObject *object,
		  const char *interface,
		  const char *name,
		  const char *in,
		  const char *out,
		  ...);

int
compGetObjectPath (CompObject *ancestor,
		   CompObject *descendant,
		   char	      *path,
		   int	      size);

int
compSerializeMethodCall (CompObject *observer,
			 CompObject *subject,
			 const char *interface,
			 const char *name,
			 const char *signature,
			 va_list    args,
			 void	    *data,
			 int	    size);

CompSignalHandler **
compGetSignalVecRange (CompObject *object,
		       int	  size,
		       int	  *offset);

void
compFreeSignalVecRange (CompObject *object,
			int	   size,
			int	   offset);

CompBool
compCheckEqualityOfValuesAndArgs (const char   *signature,
				  CompAnyValue *value,
				  ...);

int
compConnect (CompObject		       *object,
	     const CompObjectInterface *interface,
	     size_t		       offset,
	     CompObject		       *descendant,
	     const CompObjectInterface *descendantInterface,
	     size_t		       descendantOffset,
	     const char		       *details,
	     ...);

void
compDisconnect (CompObject		  *object,
		const CompObjectInterface *interface,
		size_t			  offset,
		int			  index);

const char *
compTranslateObjectPath (CompObject *ancestor,
			 CompObject *descendant,
			 const char *path);

void
compLog (CompObject		   *object,
	 const CompObjectInterface *interface,
	 size_t			   offset,
	 const char		   *fmt,
	 ...);

COMPIZ_END_DECLS

#endif
