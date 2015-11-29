#ifndef __DoublyLinkedList__
#define __DoublyLinkedList__

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
//      DoublyLinkedList.h
//
// Contents:
//      Macros to creat and manipulate Doubly linked lists with head & tail.
//      These Doubly linked lists are typed, and type safety is provided by
//      the compiler.  The cost of type safety is that the link fields used
//      must be specified as macro parameters.  
//      
//      Note: these lists display well in the debugger.
//
// Usage:
//      0) Select the correct type of list:
//          - DoublyLinked if items are randomly removed from the list
//          - SinglyLinked if items are typically removed from the head
//          - InterlockedSinglyLinked for inter-thread FIFO queuing, 
//              (n producer, single consumer )
//      1) Define two embedded link fields in each element, eg.
//          typedef struct FOO FOO, *PFOO;
//          struct FOO { PFOO  flink; PFOO  blink; ...}
//         There are no restrictions on link field placement.
//          Items that go on two lists at the same time will
//          need more link fields.
//      2) Define a new list type using DLListDefineListType, e.g.
//          DLListDefineListType(FOO, ListOfFOO);
//
//      3) Declare an instance of the List.
//
//      4) Initialize the list:
//          C++ : Constructor does this for you.
//          C   : Use DLListInitialize()
//
//      5) Specify link fields when adding/removing elements from the list.
//
//      6) Each list is assumed to be appropriately locked by the caller.
//         Simultaneous calls for the same list on different threads/CPUs
//         will cause list corruption.
//
// Revision History:
//  4-Jan-2001   Harvey    Created.
//--
# include "K10Assert.h"

# ifdef __cplusplus
# define DLListDefineListType( elementType, listType)	\
	typedef struct listType listType;					\
	struct listType {							        \
		listType() : dHead(NULL), dTail(NULL){}  			\
		elementType * dHead;								\
		elementType * dTail;								\
      VOID AddHead(elementType * element);                                \
      VOID AddTail(elementType * element);                                \
      VOID InsertAfter(elementType * element, elementType * listelement); \
      elementType * RemoveHead();                                         \
      void Remove(elementType * element);                                 \
      listType & operator = (listType & other);                 \
      bool IsEmpty() const;                                                     \
      void Initialize();                                                  \
      elementType * ViewNext( elementType * element) const;                     \
      elementType * ViewPrevious ( elementType * element) const;                \
	};
# define DLListDefineInlineCppListTypeFunctions( elementType, listType, flink, blink)	\
      inline VOID listType::AddHead(elementType * element)               \
      {                                                                  \
          DLListAddHead(*this, element, flink, blink);                   \
      }                                                                  \
      inline VOID listType::AddTail(elementType * element)               \
      {                                                                  \
          DLListAddTail(*this, element, flink, blink);                   \
      }                                                                  \
      inline VOID listType::InsertAfter(elementType * element, elementType * listelement)  \
      {                                                                  \
          DLListInsertAfter(elementType, *this, element, listelement, flink, blink); \
      }                                                                  \
      inline elementType * listType::RemoveHead()                        \
      {                                                                  \
        elementType * result;                                            \
                                                                         \
        DLListRemoveHead(*this, result, flink, blink);                   \
        return result;                                                   \
      }                                                                  \
      inline void listType::Remove(elementType * element)                \
      {                                                                  \
        DLListRemove(elementType, *this, element, flink, blink);         \
      }                                                                  \
      inline listType & listType::operator = (listType & other)                 \
      {                                                                  \
          dHead = other.dHead;dTail=other.dTail;                         \
          other.dHead = NULL; other.dTail = NULL;                        \
          return *this;                                                  \
      }                                                                  \
      inline bool listType::IsEmpty() const                                    \
      {                                                                  \
        return DLListIsEmpty(*this);                                     \
      }                                                                  \
      inline void listType::Initialize()                                 \
      {                                                                  \
        DLListInitialize(*this);                                         \
      }                                                                  \
      inline elementType * listType::ViewNext(elementType * element) const     \
      {                                                                  \
        return element->flink;                                           \
      }                                                                  \
      inline elementType * listType::ViewPrevious(elementType * element)  const \
      {                                                                  \
        return element->blink;                                           \
      }

# else 
# define DLListDefineListType( elementType, listType)	\
	typedef struct listType listType;					\
	struct listType {							        \
		elementType * dHead;								\
		elementType * dTail;								\
	};

# endif

// Same effect as C++ constructor, must be used in C
# define DLListInitialize(list)                         \
    { (list).dHead = (list).dTail = NULL; } 

# define DLListIsEmpty(list) ((list).dHead == NULL)

// Contents of "from" are moved to "to".  "from" end up empty                            
# define DLListMove(from, to)							\
{														\
	(to).dHead = (from).dHead; (to).dTail = (from).dTail;		\
	(from).dHead = NULL; (from).dTail = NULL;				\
}

// Insert element at dHead of list
# define DLListAddHead(list, element, flink, blink)		\
{                                                       \
    DLAssertNotOnList(list, element, flink, blink);     \
	(element)->flink = (list).dHead;						\
	(element)->blink = NULL;						    \
	if ((list).dHead == NULL) {							\
		(list).dTail = (element);						\
	}                                                   \
    else {                                              \
        (list).dHead->blink = (element);                 \
    }                                                   \
	(list).dHead = (element);							\
}

#if DBG
# define DLClearLink_DEBUG(element, flink, blink)       \
  {  (element)->flink = NULL; (element)->blink = NULL; }


// Validates that the item is not on the list.  Optimized
// if element->link is NULL when not on list.
// WARNING: Destroys element->link (to avoid needing to know type)
#define DLAssertNotOnList(list, element, flink, blink)   \
    {                                                   \
        if ((element)->flink) {                          \
            for ((element)->flink = (list).dHead;         \
              (element)->flink;                          \
              (element)->flink = (element)->flink->flink) {\
                DBG_ASSERT((element)->flink != (element));\
            }                                           \
        }                                               \
        else {                                          \
            DBG_ASSERT((list).dHead != (element));      \
        }                                               \
    }
    
#define DLAssertOnList(type, list, element, flink, blink)    \
    {                                                   \
         type * XXnext;		                                            \
         XXnext = (list).dHead;                                         \
         while(XXnext) {                                                \
             if(XXnext == element) {                                    \
                 break;                                                 \
             }                                                          \
             XXnext = XXnext->flink;                                    \
         }                                                              \
         DBG_ASSERT(XXnext);                                            \
    }
    
# else 
#define DLAssertNotOnList(list, element, flink, blink)
#define DLAssertOnList(type, list, element, flink, blink)
# define DLClearLink_DEBUG(element, flink, blink) 
# endif


// Insert element at dTail of list.
# define DLListAddTail(list, element, flink, blink)	    \
{														\
    DLAssertNotOnList(list, element, flink, blink);     \
	(element)->flink = NULL;						    \
	if ((list).dHead == NULL) {							\
		(list).dHead = element;							\
	    (element)->blink = NULL;						\
	}													\
	else {												\
	    (element)->blink = (list).dTail;					\
		(list).dTail->flink = element;					\
	}													\
	(list).dTail = element;								\
}

// Remove element from dHead of list.  Parameter
// "element" points to the item removed.  This is
// returned as NULL if the list is empty. 

# define DLListRemoveHead(list, element, flink, blink)	\
{														\
	(element) = (list).dHead;							\
	if (element) {										\
        DBG_ASSERT((element)->blink == NULL);           \
		(list).dHead = (element)->flink;					\
        if ((list).dHead) {                              \
            DBG_ASSERT((list).dHead->blink == (element)); \
            (list).dHead->blink = NULL;                  \
        }                                               \
        else {   (list).dTail = NULL; }                  \
        DLClearLink_DEBUG(element, flink, blink);       \
	}													\
}

// Remove element from the list.  Parameter
// "element" points to the item to be removed. 
// WARNING: the element is required to be on the list.
# define DLListRemove(type, list, element, flink, blink)	\
{														    \
    if((element)->flink) {		        			        \
        DBG_ASSERT((element)->flink->blink == (element));   \
        if((element)->blink) {	/* middle of list */ 		\
            DBG_ASSERT((element)->blink->flink == (element)); \
            (element)->blink->flink = element->flink;       \
            (element)->flink->blink = element->blink;       \
            (element)->blink = NULL;                        \
        }                                                   \
        else {   /* At Head */                              \
            FF_ASSERT((list).dHead == (element));           \
		    (list).dHead = (element)->flink;			    \
            (list).dHead->blink = NULL;                     \
        }                                                   \
        (element)->flink = NULL;                            \
	}													    \
    else {   /* At dTail */                                 \
       FF_ASSERT((list).dTail == (element));                \
       if((element)->blink) {		        				\
            DBG_ASSERT((element)->blink->flink == (element)); \
            (element)->blink->flink = NULL;                 \
            (list).dTail = element->blink;                  \
            (element)->blink = NULL;                        \
        }                                                   \
        else {   /* Only element */                         \
            DBG_ASSERT((list).dHead == (element));          \
		    (list).dHead = NULL;					        \
            (list).dTail = NULL;                            \
        }                                                   \
    }                                                       \
}

// Iterate through the list.  Items must not be removed
// within the loop.

# define for_each_DLListElement(list, element, flink, blink)    \
for(element = (list).dHead; element; element = element->flink) 

// Iterate from the back.
# define for_each_DLListElementReverse(list, element, flink, blink)    \
for(element = (list).dTail; element; element = element->blink) 

// Search the list until the boolean "expression" is true for
// an element, then extract that element.
#define DLListExtractMatch(type, list, element, flink, blink, expression)			\
{																	\
	type * CSX_MAYBE_UNUSED XXprev;													\
	for(element = (list).dHead,XXprev = NULL; element; XXprev= element, element = element->flink) { \
		if (expression) {											\
			DLListRemove(type, list, element, flink, blink)         \
			break;													\
		}															\
	}																\
}
	
// Add a element to a sorted list.  "field" is the name of the field
// the list is sorted on.
#define DLListInsertAscending(type, list, element, flink, blink, field)	  \
{                                                                         \
    if (!(list).dHead || (element)->field < (list).dHead->field) {        \
        DLListAddHead(list, element, flink, blink);                               \
	}																	  \
	else {                                                                \
        type * XXnext;		                                            \
        type * XXtemp;		                                            \
        XXnext = (list).dHead;                                          \
        for(;;) {                                                       \
		    DBG_ASSERT(element != XXnext);                              \
		    if (!XXnext->flink || (element)->field < XXnext->flink->field) {    \
                break;									                \
            }                                                           \
		    XXnext = XXnext->flink;                                     \
	    }                                                               \
	    XXtemp = XXnext->flink;                                         \
        (element)->flink = XXtemp;                                      \
        (element)->blink = XXnext;                                      \
		if (XXtemp) {                                                   \
            XXtemp->blink = (element);                                  \
		}                                                               \
		else {                                                          \
            (list).dTail = (element);                                    \
		}                                                               \
	    XXnext->flink = (element);     		                            \
    }                                                                   \
}

// Add element after listelemnt.
#define DLListInsertAfter(type, list, element, listelement, flink, blink) \
{                                                                    \
    DLAssertNotOnList(list, element, flink, blink);                  \
    DLAssertOnList(type, list, listelement, flink, blink);            \
    (element)->flink = (listelement)->flink;                         \
    (element)->blink = (listelement);                                \
    if((listelement)->flink) {                                       \
        (listelement)->flink->blink = (element);                     \
    }                                                                \
    else {                                                           \
        (list).dTail = (element);                                    \
    }                                                                \
    (listelement)->flink = (element);                                \
}

//
// Inspect, but do not remove the Head or the Tail of the list
//
// 08/02/2004 Evil Twin
// 
#define DLListViewHead(list)                 ((list).dHead)
#define DLListViewTail(list)                 ((list).dTail)

# endif
