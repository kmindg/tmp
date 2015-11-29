#if !defined(GENERICS_H)
#define	GENERICS_H 0x00000001	/* from common dir */

/***********************************************************************
 *  Copyright (C)  EMC Corporation 1989-1999,2001
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ***********************************************************************/

/***************************************************************************
 * $Id: generics.h,v 1.5 2000/02/17 18:42:45 fladm Exp $
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file defines the standard types to be in this operating
 *   system, and software written for it.
 *
 * NOTES:
 *
 * HISTORY:
 *
 * User: H. Weiner
 * 11-1-01
 * Reason: ifdef IMPORT to allow C++ compilation
 *
 * $Log: generics.h,v $
 * Revision 1.5  2000/02/17 18:42:45  fladm
 * User: jdidier
 * Reason: g++_compile
 * First pass g++ compile to correct errors in flare for eventual merge with K10
 *
 * Revision 1.4  1999/12/30 15:15:25  fladm
 * User: browlands
 * Reason: Trunk_EPR_Fixes
 * This does not fix Trunk EPR 3385 (dealing with NULL ptr
 * warnings on Linux builds), but it does pave the way by
 * defining new constants NULL_ID & NULL_INT in generics.h.
 * The redefinition of NULL itself will occur in another
 * update when and if everyone agrees it's a good thing to do.
 *
 * Revision 1.3  1999/12/16 21:07:14  fladm
 * User: jdidier
 * Reason: gcc_pedantic_compile
 * changed definition of TEXT and BM_SG_ELEMENT.address
 *
 * Revision 1.2  1999/07/27 22:00:18  fladm
 * User: lathrop
 * Reason: Crossbow_Development
 * remove sections.h, INDENT-OFF/ON
 *
 * Revision 1.1  1999/01/05 22:26:19  fladm
 * User: dibb
 * Reason: initial
 * Initial tree population
 *
 * Revision 1.5  1998/09/28 17:46:30  aldred
 *  Defined SECS_PER_HOUR.
 *
 * Revision 1.4  1997/08/25 16:29:53  carll
 *  Changed typedef of VOID from INT_32 to void
 *
 * Revision 1.3  1997/06/18 17:56:32  lathrop
 *  add OK/NOT_OK
 *
 * Revision 1.2  1997/04/04 20:38:42  lathrop
 * ADDED to PRODUCT k5ph2 from PRODUCT k5 on Thu Jun  5 15:27:38 EDT 1997
 *  add CALLBACK_FCN typedef
 *
 * Revision 1.1  1996/07/23 23:02:17  lathrop
 * Initial revision
 *
 ***************************************************************************/

/*
 * LOCAL TEXT unique_file_id = "$Header: /fdescm/project/CVS/flare/src/include/generics.h,v 1.5 2000/02/17 18:42:45 fladm Exp $"
 */

/*************************
 * INCLUDE FILES
 *************************/
/*
 * the FLARE__STAR__H symbol should be defined in each other flare
 * include file.
 */
#if defined(FLARE__STAR__H)
#error "generics.h must be the first included file"
#endif

/*
 * NOTE: environment.h is included using <> to insure we get the right
 *       one from the proper directory.  For this to work the following
 *       must be true:
 *           1) generics.h is in .../common/
 *           2) no .../common/environment.h exists
 *           3) for every directory that is in the include paths
 *              specified to the compiler the directory containing the
 *              proper environment.h is before all other directories
 *              containing environment.h
 */
/*
 *	NOTE:	Since generics.h is the first included file, no wrappers are
 *			ever necessary here.
 */
#include "environment.h"
#include "EmcPAL_Misc.h"
#include "flare_sys_types.h"
#include "ktrace.h"

//Environment independent types were seperated
//out into the following new header
#include "generic_types.h"

/* K10 NT generics */
#if defined(K10_ENV)

// When generics.h is included in the mgmt subtree, declarations come
// from ntdefs.h (from SDK), not ntddk.h
#ifndef USER_GENERICS_H
#	define	NTDDK_H
#	include "k10ntddk.h"
#endif

# ifdef HEMI
# define HEMI_PRIVATE(field) field
#else
# define HEMI_PRIVATE(field) HEMIPRIVATEFIELD##field
#endif

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)

#if 		DBG
#define 	DBGPRINT(args)			EmcpalDbgPrint args
#else		//DBG
#define 	DBGPRINT(args)
#endif		//DBG

#else	//UMODE_ENV

#define 	DBGPRINT(args)

#endif	//UMODE_ENV

#define	PANIC_TRAP()				EmcpalDebugBreakPoint()

#else

typedef void VOID;		/* moved from below */

#endif /* K10_ENV */

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) CSX_UNREFERENCED_PARAMETER(P)
#endif

/* define some convenient synonyms  */
/*
 * IMPORT        extern -- indicates that the item is to be imported
 * EXPORT        <nada> -- indicates that the item is exported
 * LOCAL         <nada> -- local func or data (for !K10_ENV)
 * LOCAL         static -- local func or data (for K10_ENV)
 * LOCAL_DATA    <nada> -- local data (for !K10_ENV)
 * LOCAL_DATA    static -- local data (for K10_ENV)
 * LOCAL_FUNC    static -- local func (for both)
 * LOCAL_FUNC    <nada> -- local func (for debugging)
 */

#if defined CPD_DM_POST_BUILD
#define CALL_TYPE
#define IMPORT extern
#elif defined(__cplusplus)
#define CALL_TYPE	__stdcall
#define IMPORT		extern "C"
#else
#define CALL_TYPE
#define IMPORT extern
#endif

#define EXPORT

#ifndef	LOCAL	  /* allow for compiler switch to make static */
#if 0 //Need good stack traces// defined(K10_ENV) && (DBG+0==0)
#define	LOCAL static /* change to #error when only LOCAL_{DATA|FUNC} used */
#else	/* def K10_ENV */
#define	LOCAL
#endif	/* def K10_ENV */
#endif	/* ndef LOCAL */

#ifndef	LOCAL_DATA
#ifdef	K10_ENV
#define	LOCAL_DATA static
#else	/* def K10_ENV */
#define	LOCAL_DATA
#endif	/* def K10_ENV */
#endif	/* ndef LOCAL_DATA */

#ifndef	LOCAL_FUNC
#ifdef	K10_ENV
#define	LOCAL_FUNC static
#else	/* def K10_ENV */
#define	LOCAL_FUNC static
#endif	/* def K10_ENV */
#endif	/* ndef LOCAL_FUNC */

/* define to inhibit warnings for C++ language unreferenced function */
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) CSX_UNREFERENCED_PARAMETER(P)
#endif

#ifndef USER_GENERICS_H
typedef VOID (*CALLBACK_FCN)(VOID*, VOID*);
#endif

#ifndef USER_GENERICS_H
extern BOOLEAN TestWithDisks;
#endif

/************************
 * QUEUE CREATION AND MANIPULATION
 ************************/

/*
 * A couple generic macros to be used by HEMI for making QUEUES
 * Note that the LINKS for a particular type must be defined before a QUEUE
 * for that type.
 */
#define HEMI_DEFINE_QUEUE_LINKS(type, name)		\
struct	type##Links								\
{												\
	struct type##Links			*nextPtr;		\
	struct type##Links			*prevPtr;		\
	struct type					*element;		\
} name;											\

#define HEMI_DEFINE_QUEUE(type, name)			\
struct	type##Queue								\
{												\
	struct	type##Links	links;					\
												\
	UINT_32			numberOfEntries;			\
} name											\

/*
 * A couple of macro's to manipulate queues as defined above.
 * These always are passed sets of links, not the structures in which
 * the links are embedded (elements).
 *
 * The one exception is HEMI_GET_FROM_QUEUE_TAIL which sets element to a
 * pointer to the actual enqueued structure.
 */

//Initialize a queue
#define HEMI_INIT_QUEUE(queue)										\
do{																	\
	(queue)->links.nextPtr = &((queue)->links);						\
	(queue)->links.prevPtr = &((queue)->links);						\
	(queue)->links.element = NULL;									\
	(queue)->numberOfEntries = 0;									\
} while(FALSE)

//Remove element from queue
#define HEMI_REMOVE_FROM_QUEUE(queue, element_links)				\
do{																	\
	(element_links)->prevPtr->nextPtr = (element_links)->nextPtr;	\
	(element_links)->nextPtr->prevPtr = (element_links)->prevPtr;	\
	(element_links)->prevPtr = NULL;								\
	(element_links)->nextPtr = NULL;								\
	(queue)->numberOfEntries--;										\
} while(FALSE)

//Add an element to a queue after position
#define HEMI_ADD_TO_QUEUE(queue, position_links, element_links)		\
do{																	\
	(element_links)->prevPtr = (position_links);					\
	(element_links)->nextPtr = (position_links)->nextPtr;			\
	(position_links)->nextPtr->prevPtr = (element_links);			\
	(position_links)->nextPtr = (element_links);					\
	(queue)->numberOfEntries++;										\
} while(FALSE)

//Remove an structure from the tail of a queue and return it
#define HEMI_GET_FROM_QUEUE_TAIL(queue, return_element)				\
do{																	\
	return_element = (queue)->links.prevPtr->element;				\
	(queue)->links.prevPtr->nextPtr = NULL;							\
	(queue)->links.prevPtr = (queue)->links.prevPtr->prevPtr;		\
	(queue)->links.prevPtr->nextPtr->prevPtr = NULL;				\
	(queue)->links.prevPtr->nextPtr = &((queue)->links);			\
	(queue)->numberOfEntries--;										\
} while(FALSE)

//Add an element on to the end of a queue
#define HEMI_ADD_TO_QUEUE_TAIL(queue, element_links)				\
do{																	\
	(element_links)->prevPtr = (queue)->links.prevPtr;				\
	(element_links)->nextPtr = &((queue)->links);					\
	(queue)->links.prevPtr->nextPtr = (element_links);				\
	(queue)->links.prevPtr = (element_links);						\
	(queue)->numberOfEntries++;										\
} while(FALSE)

/************************
 * PROTOTYPE DEFINITIONS
 ************************/

/*
 * End $Id: generics.h,v 1.5 2000/02/17 18:42:45 fladm Exp $
 */
#endif	/* ndf GENERICS_H */ /* MUST be last line in file */
