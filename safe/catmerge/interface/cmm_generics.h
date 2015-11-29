#ifndef CMM_GENERICS_H
#define CMM_GENERICS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2001
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *     This file provides some genric CMM macros and includes common types.
 *   
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *
 ***************************************************************************/

#include "k10ntddk.h"
#include "EmcPAL.h"

#include "generics.h"
#include "EmcPrefast.h"
/*
 * A couple generic macros to be used internally by CMM for making QUEUES
 * Note that the LINKS for a particular type must be defined before a QUEUE
 * for that type.
 */
#define CMM_DEFINE_QUEUE_LINKS(type, name)		\
struct	type##Links								\
{												\
	struct type##Links			*nextPtr;		\
	struct type##Links			*prevPtr;		\
	struct type					*element;		\
} name;											\

#define CMM_DEFINE_QUEUE(type, name)			\
struct	type##Queue								\
{												\
	struct	type##Links	links;					\
												\
	UINT_32			numberOfEntries;			\
} name											\

/*
 * How about a couple of macro's to manipulate queues as defined above.
 * These always are passed sets of links, not the structures in which
 * the links are embedded.
 *
 * The one exception is REMOVE_FROM_TAIL which sets element to a
 * pointer to the actual enqueued structure.
 */

//Initialize a queue
#define CMM_INIT_QUEUE(queue)										\
{																	\
	(queue)->links.nextPtr = &((queue)->links);						\
	(queue)->links.prevPtr = &((queue)->links);						\
	(queue)->links.element = NULL;									\
	(queue)->numberOfEntries = 0;									\
}

//Remove element from queue
#define CMM_REMOVE_FROM_QUEUE(queue, element_links)					\
{																	\
	CMM_ASSERT((element_links)->prevPtr->nextPtr == (element_links)); \
	CMM_ASSERT((element_links)->nextPtr->prevPtr == (element_links)); \
	(element_links)->prevPtr->nextPtr = (element_links)->nextPtr;	\
	(element_links)->nextPtr->prevPtr = (element_links)->prevPtr;	\
	(element_links)->prevPtr = NULL;								\
	(element_links)->nextPtr = NULL;								\
	(queue)->numberOfEntries--;										\
}

//Add an element to a queue after position
#define CMM_ADD_TO_QUEUE(queue, position_links, element_links)		\
{																	\
	(element_links)->prevPtr = (position_links);					\
	(element_links)->nextPtr = (position_links)->nextPtr;			\
	(position_links)->nextPtr->prevPtr = (element_links);			\
	(position_links)->nextPtr = (element_links);					\
	(queue)->numberOfEntries++;										\
}

//Remove an structure from the tail of a queue and return it
#define CMM_GET_FROM_QUEUE_TAIL(queue, return_element)				\
{																	\
	return_element = (queue)->links.prevPtr->element;				\
	(queue)->links.prevPtr->nextPtr = NULL;							\
	(queue)->links.prevPtr = (queue)->links.prevPtr->prevPtr;		\
	(queue)->links.prevPtr->nextPtr->prevPtr = NULL;				\
	(queue)->links.prevPtr->nextPtr = &((queue)->links);			\
	(queue)->numberOfEntries--;										\
}

//Add an element on to the end of a queue
#define CMM_ADD_TO_QUEUE_TAIL(queue, element_links)					\
{																	\
	(element_links)->prevPtr = (queue)->links.prevPtr;				\
	(element_links)->nextPtr = &((queue)->links);					\
	(queue)->links.prevPtr->nextPtr = (element_links);				\
	(queue)->links.prevPtr = (element_links);						\
	(queue)->numberOfEntries++;										\
}
 
/*
 * Calculate the number of units based on number of bytes and
 * the allocation unit's size
 */
#define CMM_CALC_UNITS(bytes, unit_size) \
    ((UINT_32)(((bytes) + (unit_size) - 1) / (unit_size)))

//Useful rounding macros
#define ROUND_UP(value, align)										\
((((value) + (align) - 1)/(align)) * (align))
#define ROUND_DOWN(value, align)									\
(((value)/(align)) * (align))

/*
 * Wrapper around the panic function
 */
#define CMM_PANIC(error, text1, text2) cmmPanic((error), text1, text2, __FILE__, __LINE__)
#define CMM_ASSERT(condition) if (!(condition)) \
	cmmPanic(CMM_STATUS_ASSERT_FAILED, "An ASSERT of the following condition failed:", #condition, __FILE__, __LINE__)
 
/*
 * Wrapper the CMM dump value function.  Use to dump
 * the values of CMM variables to the debugger and KTRACE.
 * Use CMM_DUMP_VALUE_X if you want to specify the format
 * the value will be printed with (default is hex)
 */
#define CMM_DUMP_VALUE(var) cmmUtilDumpValue(#var, NULL, (UINT_PTR)(var))
#define CMM_DUMP_VALUE_X(fmt, var) cmmUtilDumpValue(#var, fmt, (UINT_PTR)(var))

/*
 * This is the CMM locking structure
 */
typedef	struct
{
	EMCPAL_SPINLOCK	lock;
	BOOLEAN		flag;
} CMM_LOCK;

IMPORT CMM_LOCK cmmLock;

#define CMM_ACQUIRE_LOCK()	\
{															\
	EmcpalSpinlockLock(&(cmmLock.lock)); \
	/********** DEBUG CODE **********						\
	if (cmmLock.flag == TRUE) CMM_PANIC(0,NULL,NULL);		\
	cmmLock.flag = TRUE;									\
	*********************************/						\
}

#define CMM_RELEASE_LOCK()	\
{															\
	/********** DEBUG CODE **********						\
	if (cmmLock.flag == FALSE) CMM_PANIC(0,NULL,NULL);		\
	cmmLock.flag = FALSE;									\
	*********************************/						\
	EmcpalSpinlockUnlock(&(cmmLock.lock));	\
}

#endif
