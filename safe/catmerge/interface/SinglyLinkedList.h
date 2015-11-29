#ifndef __SinglyLinkedList__
#define __SinglyLinkedList__

//***************************************************************************
// Copyright (C) EMC Corporation 2000-2002
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************
# include "K10Assert.h"
# include "ktrace.h"
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
# include "EmcKeInline.h"
#endif
# include "EmcPAL_Misc.h"

//++
// File Name:
//      SinglyLinkedList.h
//
// Contents:
//      Macros to create and manipulate singly linked lists with head & tail.
//      These singly linked lists are typed, and type safety is provided by
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
//
//      1) Define an embedded link field in each element, eg.
//          typedef struct FOO FOO, *PFOO;
//          struct FOO { PFOO  flink;  ...}
//         There are no restrictions on link field placement.
//          Items that go on two lists at the same time will
//          need more link fields.
//         Note that a given element may appear at most once on a given
//          list.  This restriction is checked `#if DBG`.
//
//      2) Define a new list type using SLListDefineListType, e.g.
//          SLListDefineListType(FOO, ListOfFOO);
//
//      3) Declare an instance of the List.
//
//      4) Initialize the list:
//          C++ : Constructor does this for you.
//          C   : Use SLListInitialize()
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

# ifdef __cplusplus
# define SLListDefineListType( elementType, listType)	\
    typedef struct listType listType, *P##listType;  	\
    struct listType##NoConstructor {					\
        elementType * head;								\
        elementType * tail;								\
        VOID AddHead(elementType * element);                                \
        VOID AddTail(elementType * element);                                \
        elementType * RemoveHead();                                         \
        void Remove(elementType * element);                                 \
        bool RemoveIfOnList(elementType * element);						  \
         bool IsEmpty();                                                     \
        void Initialize();                                                  \
    };					  				    \
    struct listType : public listType##NoConstructor {				      \
        listType()  { head = NULL; tail = NULL;	}  					\
        listType(listType##NoConstructor & other);                                      \
        listType & operator = (listType##NoConstructor & other);                  \
         listType(listType & other)                                  \
        {                                                           \
            SLListMove(other, *this);                               \
        }                                                           \
        listType & operator = (listType & other)                    \
        {                                                           \
            SLListMove(other, *this);                               \
            return (*this);                                         \
        }                                                           \
    };

// FIX: improve other list types...
# define SLListDefineInlineCppListTypeFunctions( elementType, listType, link)	\
      inline VOID listType##NoConstructor::AddHead(elementType * element)       \
      {                                                                  \
          SLListAddHead(*this, element, link);                           \
      }                                                                  \
      inline VOID listType##NoConstructor::AddTail(elementType * element)       \
      {                                                                  \
          SLListAddTail(*this, element, link);                           \
      }                                                                  \
                                                                         \
      inline elementType * listType##NoConstructor::RemoveHead()                \
      {                                                                  \
        elementType * result;                                            \
                                                                         \
        SLListRemoveHead(*this, result, link);                           \
        return result;                                                   \
      }                                                                  \
      inline void listType##NoConstructor::Remove(elementType * element)        \
      {                                                                  \
        SLListRemove(elementType, *this, element, link);                 \
      }                                                                  \
      inline  listType::listType(listType##NoConstructor & other) \
      {                                                                                 \
          SLListMove(other, *this);                                                     \
      }                                                                                 \
      inline listType & listType::operator = (listType##NoConstructor & other) \
      {                                                                                 \
          SLListMove(other, *this);                                                     \
          return (*this);                                                               \
      }                                                                                 \
	  inline bool listType##NoConstructor::RemoveIfOnList(elementType * element)                \
      {																		\
			elementType * e;												\
			SLListExtractMatch(elementType, *this, e, link, e == element);  \
			return e != NULL;												\
			}																\
	  inline bool listType##NoConstructor::IsEmpty()                        \
      {                                                                  \
        return SLListIsEmpty(*this);                                 \
      }                                                                  \
      inline void listType##NoConstructor::Initialize()                                 \
      {                                                                  \
        SLListInitialize(*this);                                     \
      }                                                                  \

# else 
# define SLListDefineListType( elementType, listType)	\
	typedef struct listType listType, *P##listType;		\
	typedef listType listType##NoConstructor;			\
	struct listType {							        \
		elementType * head;								\
		elementType * tail;								\
	};
# endif

# define SLListDefineInlineListTypeFunctions(elementType, listType, link) \
static __inline VOID listType##AddTail(listType * list, elementType * element)    \
{                                                                    \
                                                                     \
    SLListAddTail(*list, element, link);                             \
                                                                     \
}                                                                    \
static __inline VOID listType##AddHead(listType * list, elementType * element)    \
{                                                                    \
                                                                     \
    SLListAddHead(*list, element, link);                             \
                                                                     \
}                                                                    \
                                                                     \
static __inline elementType * listType##RemoveHead(listType * list)         \
{                                                                    \
    elementType * result;                                            \
                                                                     \
    SLListRemoveHead(*list, result, link);                           \
    return result;                                                   \
}                                                                    \
static __inline void listType##Remove(listType * list, elementType * element)   \
{                                                                    \
    SLListRemove(elementType, *list, element, link);                 \
}                                                                    \
static __inline BOOL listType##IsEmpty(listType * list)                  \
{                                                                    \
    return SLListIsEmpty(*list);                                     \
}                                                                    \
static __inline void listType##Initialize(listType * list)                  \
{                                                                    \
    SLListInitialize(*list);                                         \
}

// Same effect as C++ constructor, must be used in C
# define SLListInitialize(list)                         \
    { (list).head = (list).tail = NULL; } 

// Contents of "from" are moved to "to".  "from" end up empty                            
# define SLListMove(from, to)							\
{														\
	(to).head = from.head; (to).tail = (from).tail;		\
	(from).head = NULL; (from).tail = NULL;				\
}

// Insert element at head of list
# define SLListAddHead(list, element, link)				\
{                                                       \
    SLAssertNotOnList(list, element, link);             \
	(element)->link = (list).head;						\
	if ((list).head == NULL) {							\
		(list).tail = element;							\
	}													\
	(list).head = element;								\
}

#if DBG
# define SLClearLink_DEBUG(element, link)               \
    (element)->link = NULL;

// Validates that the item is not on the list.  Optimized
// if element->link is NULL when not on list.
// WARNING: Destroys element->link (to avoid needing to know type)
#define SLAssertNotOnList(list, element, link)          \
    {                                                   \
        if ((element)->link) {                          \
            for ((element)->link = (list).head;         \
              (element)->link;                          \
              (element)->link = (element)->link->link) {\
                DBG_ASSERT((element)->link != (element));\
            }                                           \
        }                                               \
        else {                                          \
            DBG_ASSERT((list).tail != (element) || (list).head == NULL);\
        }                                               \
    }
    
# else 
#define SLAssertNotOnList(list, element, link)
# define SLClearLink_DEBUG(element, link) 
# endif


// Insert element at tail of list.
# define SLListAddTail(list, element, link)			\
{														\
    SLAssertNotOnList(list, element, link);             \
	(element)->link = NULL;								\
	if ((list).head == NULL) {							\
		(list).head = element;							\
	}													\
	else {												\
		(list).tail->link = element;					\
	}													\
	(list).tail = element;								\
}

// Remove element from head of list.  Parameter
// "element" points to the item removed.  This is
// returned as NULL if the list is empty. 
# define SLListRemoveHead(list, element, link)			\
{														\
	(element) = (list).head;							\
	if (element) {										\
		(list).head = (element)->link;					\
        SLClearLink_DEBUG(element, link);               \
	}													\
}

// Remove element from tail of list.  Parameter
// "element" points to the item removed.  This is
// returned as NULL if the list is empty. 
// NOTE: This is expensive.
# define SLListRemoveTail(list, element, link)			\
{														\
    for((element) = (list).head; element; (element) = (element)->link) { \
        if ((element)->link == (list).tail) {				\
           (list).tail = (element);                       \
           (element) = (element)->link;                     \
           (list).tail->link = NULL;                    \
           break;                                       \
        }                                               \
	}													\
}

// Remove element from the list.  Parameter
// "element" points to the item to be removed. 
// WARNING: the element is required to be on the list.
// NOTE: Use doubly linked lists for better performance
// on removal from middle of large lists.
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
# define SLListRemove(type, list, element, link)	    \
{														\
    if((element) == (list).head) {						\
		(list).head = (element)->link;					\
	}													\
    else {                                              \
       type * XX = (list).head;                         \
       while (XX) {                                     \
           if (XX->link == (element)) {                 \
               XX->link = (element)->link;              \
               if ((element) == (list).tail) {          \
                   (list).tail = XX;                    \
               }                                        \
               break;                                   \
           }                                            \
           XX = XX->link;                               \
       if (!XX) {                                       \
		EmcKePANIC("List element not found");	        \
       }                                                \
       }                                                \
	}                                                   \
    SLClearLink_DEBUG(element, link);                   \
}
#else
# define SLListRemove(type, list, element, link)	    \
{														\
    if((element) == (list).head) {						\
		(list).head = (element)->link;					\
	}													\
    else {                                              \
       type * XX = (list).head;                         \
       while (XX) {                                     \
           if (XX->link == (element)) {                 \
               XX->link = (element)->link;              \
               if ((element) == (list).tail) {          \
                   (list).tail = XX;                    \
               }                                        \
               break;                                   \
           }                                            \
           XX = XX->link;                               \
       }                                                \
	}                                                   \
    SLClearLink_DEBUG(element, link);                   \
}
#endif
// Iterate through the list.  Items must not be removed
// within the loop.
# define for_each_SLListElement(list, element, link)    \
for(element = (list).head; element; element = element->link) 

// Search the list until the boolean "expression" is true for
// an element, then extract that element.
#define SLListExtractMatch(type, list, element, link, expression)			\
{																	\
	type * XXprev;													\
	for(element = (list).head,XXprev = NULL; element; XXprev= element, element = element->link) { \
		if (expression) {											\
			if (XXprev) { XXprev->link = XXprev->link->link; }	    \
			else { (list).head = (list).head->link; }				\
            if ((element) == (list).tail) {                         \
                (list).tail = XXprev;                               \
            }                                                       \
            SLClearLink_DEBUG(element, link);                       \
			break;													\
		}															\
	}																\
}

// Add a element to a sorted list.  "field" is the name of the field
// the list is sorted on.
#define SLListInsertAscending(type, list, element, link, field)	        \
{																        \
    if (!(list).head || (element)->field < (list).head->field) {        \
        SLListAddHead(list, element, link);                             \
    }                                                                   \
    else {                                                              \
        type * XXnext;		                                            \
        XXnext = (list).tail;                                           \
        if ((element)->field < XXnext->field) {                         \
                                                                        \
            XXnext = (list).head;                                       \
            for(;;) {                                                   \
                DBG_ASSERT(element != XXnext);                          \
                if (!XXnext->link || (element)->field < XXnext->link->field) {    \
                    break;									            \
                }                                                       \
                XXnext = XXnext->link;                                  \
            }                                                           \
        }                                                               \
        (element)->link = XXnext->link;                                 \
        if (!XXnext->link) {                                            \
            (list).tail = (element);                                    \
        }                                                               \
        XXnext->link = (element);                                       \
    }                                                                   \
}	

// Returns first element
#define SLListGetFirst(list, element)		\
{													\
	(element) = (list).head;		            \
}                                                   \

// Returns next element 
#define SLListGetNext(type, element, link) \
{                                       \
	if (element != NULL) {              \
	    type *XXnext = (element)->link; \
		element = XXnext;        \
	}                                   \
}                                       \

# define SLListIsEmpty(list) ((list).head == NULL)


# endif
