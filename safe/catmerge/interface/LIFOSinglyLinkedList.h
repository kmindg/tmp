#ifndef __LIFOSinglyLinkedList__
#define __LIFOSinglyLinkedList__

//***************************************************************************
// Copyright (C) EMC Corporation 2000-2002
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      LIFOSinglyLinkedList.h
//
// Contents:
//      Macros to creat and manipulate singly linked lists with only a head.
//      These singly linked lists are typed, and type safety is provided by
//      the compiler.  The cost of type safety is that the link fields used
//      must be specified as macro parameters.
//
//      Note: these lists display well in the debugger.
//
// Usage:
//      0) Select the correct type of list:
//          - DoublyLinked if items are randomly removed from the list
//          - SinglyLinked if items are typically removed from the LIFOhead and
//            queued at the tail.
//          - LIFOSinglyLinked if items are typically added/removed
//            from the LIFOhead (e.g. freelists)
//          - InterlockedSinglyLinked for inter-thread FIFO queuing, 
//              (n producer, single consumer )
//
//      1) Define an embedded link field in each element, eg.
//          typedef struct FOO FOO, *PFOO;
//          struct FOO { PFOO  flink;  ...}
//         There are no restrictions on link field placement.
//          Items that go on two lists at the same time will
//          need more link fields.
//      2) Define a new list type using LIFOSLListDefineListType, e.g.
//          LIFOSLListDefineListType(FOO, ListOfFOO);
//
//      3) Declare an instance of the List.
//
//      4) Initialize the list:
//          C++ : Constructor does this for you.
//          C   : Use LIFOSLListInitialize()
//
//      5) Specify link field when adding/removing elements from the list.
//
//      6) Each list is assumed to be appropriately locked by the caller.
//         Simultaneous calls for the same list on different threads/CPUs
//         will cause list corruption.
//
// Revision History:
//  15-Aug-2000   Harvey    Created.
//--
# include "K10Assert.h"

# ifdef __cplusplus
# define LIFOSLListDefineListType( elementType, listType)	\
	typedef struct listType listType, *P##listType;  	\
	struct listType {							        \
      listType() : LIFOhead(NULL)  {}            		\
	  elementType * LIFOhead;							\
      VOID AddHead(elementType * element);                                \
      elementType * RemoveHead();                                         \
      void Remove(elementType * element);                                 \
      bool IsEmpty();                                                     \
      void Initialize();                                                  \
      listType & operator = (listType & other);                 \
	};
# define LIFOSLListDefineInlineCppListTypeFunctions( elementType, listType, link)	\
      inline VOID listType::AddHead(elementType * element)               \
      {                                                                  \
          LIFOSLListAddHead(*this, element, link);                       \
      }                                                                  \
                                                                         \
      inline elementType * listType::RemoveHead()                        \
      {                                                                  \
        elementType * result;                                            \
                                                                         \
        LIFOSLListRemoveHead(*this, result, link);                       \
        return result;                                                   \
      }                                                                  \
      inline void listType::Remove(elementType * element)                \
      {                                                                  \
        LIFOSLListRemove(elementType, *this, element, link);             \
      }                                                                  \
      inline bool listType::IsEmpty()                                    \
      {                                                                  \
        return LIFOSLListIsEmpty(*this);                                 \
      }                                                                  \
      inline void listType::Initialize()                                 \
      {                                                                  \
        LIFOSLListInitialize(*this);                                     \
      }                                                                  \
      inline listType & listType::operator = (listType & other) {        \
            LIFOSLListMove(other, *this);                                \
            return *this;                                                \
      }
# else 
# define LIFOSLListDefineListType( elementType, listType)	\
	typedef struct listType listType, *P##listType;		\
	struct listType {							        \
		elementType * LIFOhead;								\
	};
# endif
# define LIFOSLListDefineInlineListTypeFunctions(elementType, listType, link) \
static __inline VOID listType##AddHead(listType * list, elementType * element)    \
{                                                                    \
                                                                     \
    LIFOSLListAddHead(*list, element, link);                         \
                                                                     \
}                                                                    \
                                                                     \
static __inline elementType * listType##RemoveHead(listType * list)         \
{                                                                    \
    elementType * result;                                            \
                                                                     \
    LIFOSLListRemoveHead(*list, result, link);                       \
    return result;                                                   \
}                                                                    \
static __inline void listType##Remove(listType * list, elementType * element)   \
{                                                                    \
    LIFOSLListRemove(elementType, *list, element, link);             \
}                                                                    \
static __inline BOOLEAN listType##IsEmpty(listType * list)                  \
{                                                                    \
    return LIFOSLListIsEmpty(*list);                                     \
}                                                                    \
static __inline void listType##Initialize(listType * list)                  \
{                                                                    \
    LIFOSLListInitialize(*list);                                     \
}

// Same effect as C++ constructor, must be used in C
# define LIFOSLListInitialize(list)                         \
    { (list).LIFOhead = NULL; } 

// Contents of "from" are moved to "to".  "from" end up empty                            
# define LIFOSLListMove(from, to)							\
{														    \
	(to).LIFOhead = from.LIFOhead;                      	\
	(from).LIFOhead = NULL;                     			\
}

// Insert element at LIFOhead of list
# define LIFOSLListAddHead(list, element, link)				\
{                                                       \
    LIFOSLAssertNotOnList(list, element, link);             \
	(element)->link = (list).LIFOhead;						\
	(list).LIFOhead = element;								\
}

#if DBG
# define LIFOSLClearLink_DEBUG(element, link)             \
    (element)->link = NULL;

// Validates that the item is not on the list.  Optimized
// if element->link is NULL when not on list.
// WARNING: Destroys element->link (to avoid needing to know type)
#define LIFOSLAssertNotOnList(list, element, link)      \
    {                                                   \
        if ((element)->link) {                          \
            for ((element)->link = (list).LIFOhead;         \
              (element)->link;                          \
              (element)->link = (element)->link->link) {\
                DBG_ASSERT((element)->link != (element));\
            }                                           \
        }                                               \
        else {                                          \
            DBG_ASSERT((list).LIFOhead != (element) );      \
        }                                               \
    }
    
# else 
#define LIFOSLAssertNotOnList(list, element, link)
# define LIFOSLClearLink_DEBUG(element, link) 
# endif


// Insert element at tail of list.
# define LIFOSLListAddTail(list, element, link)			\
{														\
    LIFOSLAssertNotOnList(list, element, link);         \
	if ((list).LIFOhead == NULL) {							\
	    (element)->link = NULL;							\
		(list).LIFOhead = element;							\
	}													\
	else {	/* 	(element)->link is temp of right type*/ \
        (element)->link = (list).LIFOhead;	                \
        while((element)->link->link) {                  \
            (element)->link = (element)->link->link;    \
        }                                               \
        (element)->link->link = (element);              \
	    (element)->link = NULL;							\
	}													\
}

// Remove element from LIFOhead of list.  Parameter
// "element" points to the item removed.  This is
// returned as NULL if the list is empty. 
# define LIFOSLListRemoveHead(list, element, link)		\
{														\
	(element) = (list).LIFOhead;							\
	if (element) {										\
		(list).LIFOhead = (element)->link;					\
        LIFOSLClearLink_DEBUG(element, link);           \
	}													\
}

// Remove element from the list.  Parameter
// "element" points to the item to be removed. 
// WARNING: the element is required to be on the list.
// NOTE: Use doubly linked lists for better performance
// on removal from middle of large lists.
# define LIFOSLListRemove(type, list, element, link)	    \
{														\
    if((element) == (list).LIFOhead) {						\
		(list).LIFOhead = (element)->link;					\
	}													\
    else {                                              \
       type * XX = (list).LIFOhead;                         \
       while (XX) {                                     \
           if (XX->link == (element)) {                 \
               XX->link = (element)->link;              \
               break;                                   \
           }                                            \
           XX = XX->link;                               \
       }                                                \
       FF_ASSERT(XX);                                   \
    }                                                   \
    LIFOSLClearLink_DEBUG(element, link);                   \
}

// Iterate through the list.  Items must not be removed
// within the loop.
# define for_each_LIFOSLListElement(list, element, link)    \
for(element = (list).LIFOhead; element; element = element->link) 

// Search the list until the boolean "expression" is true for
// an element, then extract that element.
#define LIFOSLListExtractMatch(type, list, element, link, expression)			\
{																	\
	type * XXprev;													\
	for(element = (list).LIFOhead,XXprev = NULL; element; XXprev= element, element = element->link) { \
		if (expression) {											\
			if (XXprev) { XXprev->link = XXprev->link->link; }	    \
			else { (list).LIFOhead = (list).LIFOhead->link; }				\
            LIFOSLClearLink_DEBUG(element, link);                   \
			break;													\
		}															\
	}																\
}

// Add a element to a sorted list.  "field" is the name of the field
// the list is sorted on.
#define LIFOSLListInsertAscending(type, list, element, link, field)	\
{									\
    if (!(list).head || (element)->field < (list).head->field) {        \
        (element)->field = (list).head;                                 \
        (list).head = (element);                                        \
    }                                                                   \
    else {                                                              \
        type * XXnext;		                                        \
        XXnext = (list).head;                                           \
        for(;;) {                                                       \
            DBG_ASSERT(element != XXnext);                              \
            if (!XXnext->Link || (element)->field < XXnext->Link->field) {    \
                break;							\
            }                                                           \
            XXnext = XXnext->link;                                      \
        }                                                               \
        (element)->Link = XXnext->Link;                                 \
        XXnext->Link = (element);                                       \
    }                                                                   \
}	

// Returns first element
#define LIFOSLListGetFirst(list, element)		\
{													\
	(element) = (list).LIFOhead;		            \
}                                                   \

// Returns next element 
#define LIFOSLListGetNext(type, element, link) \
{                                       \
	if (element != NULL) {              \
	    type *XXnext = (element)->link; \
		element = XXnext;        \
	}                                   \
}                                       \
		
# define LIFOSLListIsEmpty(list) ((list).LIFOhead == NULL)


# endif
