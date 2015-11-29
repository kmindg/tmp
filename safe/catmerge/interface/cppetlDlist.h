#ifndef CPPETL_DLIST_H
#define CPPETL_DLIST_H

//**********************************************************************
// Copyright  (C) EMC Corporation  2001
// All Rights Reserved
// Licensed material - Property of EMC Corporation
//**********************************************************************

//**********************************************************************
// $id:$
//**********************************************************************
//
// DESCRIPTION:
//       The Dlist class is a templated list class. This class implements
//       all of the necessary methods to manage a doubly linked list.
//
// NOTES:
//
// HISTORY:
//    26-Jun-01  Created by Austin Spang
//    $Log:$
//
//**********************************************************************

//**********************************************************************
// INCLUDE FILES
//**********************************************************************
#include "cppetlDlistElem.h"
#include "generics.h"

namespace asetl 
{
	
//**********************************************************************
// MACRO DEFINITIONS
//**********************************************************************

//**********************************************************************
// STRING LITERALS
//**********************************************************************

//**********************************************************************
// TYPE DEFINITIONS
//**********************************************************************

//**********************************************************************
// IMPORTED VARIABLE DEFINITIONS
//**********************************************************************

//**********************************************************************
// IMPORTED FUNCTION DEFINITIONS
//**********************************************************************

//**********************************************************************
// IMPORTED CLASS DEFINITIONS
//**********************************************************************

//**********************************************************************
// FUNCTION DEFINITIONS
//**********************************************************************

//**********************************************************************
// IMPORTED VARIABLE DEFINITIONS
//**********************************************************************

//**********************************************************************
// CLASS DEFINITIONS
//**********************************************************************

template <class T> class DList 
{
public:
    DList();
    DList(DListElem <T> &);
    ~DList();
    void enqueue(DListElem <T> *elem) { enqueueHead(elem); return;};
    void enqueueHead(DListElem <T> *);
    void enqueueTail(DListElem <T> *);
    bool insertAfter(DListElem <T> *, DListElem <T> *);
    bool insertBefore(DListElem <T> *, DListElem <T> *);
    T* dequeue(void);
    DListElem <T> * dequeueElem(void);
    inline void firstElem(void) {iterator = firstPtr; return; };
    inline void lastElem(void) {iterator = lastPtr; return; };
    inline void setIterator(DListElem <T> * elem) {iterator = elem; return;};
    T* next(void);
    DListElem <T> * nextElem(void);
    T* prev(void);
    DListElem <T> * prevElem(void);
    bool isMember(T*) const;
    inline bool isEmpty() const {return (count == 0);};
    inline int queueLength() const {return count;};
    inline void advance(void) {iterator = iterator->next;}
    inline T* current(void) const {return (iterator? iterator->object: 0);}
    inline DListElem <T> * currentElem(void) const {return iterator;}
    T* remove(void);
    DListElem <T> * removeElem(void);
 
#ifdef __SFD_WINDBG_SUPPORT__
	void display(char*);
//	displayListName();
//	static winDbgDisplayObject(T* object) { nt_dprintf("    %d=0x%08x\n", (PVOID) object)};

#endif
    
private:
    int count;                 // Number of elements in list
    DListElem <T> *firstPtr;   // Pointer to 1st Element in list
    DListElem <T> *lastPtr;    // Pointer to last Element in list
    DListElem <T> *iterator;   // Current Location of iterator
};

#ifdef __SFD_WINDBG_SUPPORT__
template <class T> 
void DList < T > :: display(char* listName="")
{
    DListElem<T> listElem;
    DListElem<T> *pListElem;
	
	nt_dprintf(" %s DList\n", listName);
    nt_dprintf("  queueLength=%d\n", queueLength());

	nt_dprintf("    firstPtr");
    ReadMemory((ULONG) firstPtr, &listElem, sizeof(DListElem<T>), NULL);
    listElem.display();

	nt_dprintf("    lastPtr");
    ReadMemory((ULONG) lastPtr, &listElem, sizeof(DListElem<T>), NULL);
	listElem.display();

	nt_dprintf("    iterator");
    ReadMemory((ULONG) iterator, &listElem, sizeof(DListElem<T>), NULL);
	listElem.display();

    if (queueLength()>0)
    {
        firstElem();
        pListElem = currentElem();
        for (int i=0; i<queueLength() && pListElem!=NULL; i++)
        {
			nt_dprintf("    Element %d:", i);
			ReadMemory((ULONG) pListElem, &listElem, sizeof(DListElem<T>), NULL);
            setIterator(listElem.get_next());
            listElem.display();
            pListElem = currentElem();
        }
    }

}
#endif

/************************************************************************
 *                	          DList
 ************************************************************************
 *
 * Description:
 *   This is the default constructor for the Singly Linked List Class. 
 *   This is used to contruct new elements in the list.
 *
 * Input:
 *   None
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
DList < T > :: DList() : firstPtr(NULL), lastPtr(NULL), iterator(NULL), count(0) {};

/************************************************************************
 *                	          DList
 ************************************************************************
 *
 * Description:
 *   This is the  constructor for the Singly Linked List Class. 
 *   This is used to contruct new elements in the list.
 *
 * Input:
 *   DList Element
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
DList < T > :: DList(DListElem <T> &elem) : 
    firstPtr(&elem), lastPtr(&elem), iterator(&elem), count(0) {};

/************************************************************************
 *                	          ~DList
 ************************************************************************
 *
 * Description:
 *   This is the default destructor for the Singly Linked List Class. 
 *   This is used to destruct the list.
 *
 * Input:
 *   None.
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
DList < T > :: ~DList()
{
    // Nothing to be done here since the elements of the list are
    // part of the objects.
}

/************************************************************************
 *                	          enqueueHead()
 ************************************************************************
 *
 * Description:
 *   This is the enqueue function for the list class. This will add a new
 *   object to the head of the list.
 *
 * Input:
 *   obj - A reference to the object to added to the list.
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
void DList < T > :: enqueueHead( DListElem <T> *obj)
{
    
    if (isEmpty())
    {
        firstPtr = lastPtr = obj;
    }
    else
    {
        obj->next = firstPtr;
        obj->prev = 0;
        firstPtr->prev = obj;
        firstPtr = obj;
    }

    count++;

    return;
}

/************************************************************************
 *                	          enqueueTail()
 ************************************************************************
 *
 * Description:
 *   This is the enqueue function for the list class. This will add a new
 *   object to the head of the list.
 *
 * Input:
 *   obj - A reference to the object to added to the list.
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
void DList < T > :: enqueueTail( DListElem <T> *obj)
{
    
    if (isEmpty())
    {
        firstPtr = lastPtr = obj;
    }
    else
    {
        ASSERT(lastPtr != NULL);
        ASSERT(firstPtr != NULL);
        obj->next = 0;
        obj->prev = lastPtr;
        lastPtr->next = obj;
        lastPtr = obj;
    }

    ASSERT(lastPtr != NULL);
    ASSERT(firstPtr != NULL);

    count++;

    return;
}

/************************************************************************
 *                	          dequeue()
 ************************************************************************
 *
 * Description:
 *   This is the dequeue function for the list class. This will remove
 *   an object from the list.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
T * DList < T > :: dequeue( void )
{
    DListElem <T> *tmp;

    if (isEmpty())
    {
        return 0;
    }

    tmp = firstPtr;
    firstPtr = tmp->next;
    // As long as we have another element on the list, then clear its prev 
    // pointer. Otherwise, the list is empty and then clear the lastPtr.
    if (firstPtr != 0)
    {
        firstPtr->prev = 0;
    }
    else
    {
        lastPtr = 0;
    }
    tmp->next = 0;           // clear pointer to indicate not on list

    count--;

    if (!isEmpty())
    {
        ASSERT(lastPtr != NULL);
        ASSERT(firstPtr != NULL);
    }


    if (!isEmpty())
    {
        ASSERT(lastPtr != NULL);
        ASSERT(firstPtr != NULL);
    }


    return tmp->object;
}

/************************************************************************
 *                	          dequeueElem()
 ************************************************************************
 *
 * Description:
 *   This is the dequeue function for the list class. This will remove
 *   an object from the list.
 *
 * Input:
 *   none
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
DListElem <T> * DList < T > :: dequeueElem( void )
{
    DListElem <T> *tmp;

    if (isEmpty())
    {
        return NULL;
    }

    tmp = firstPtr;
    firstPtr = tmp->next;

    // As long as we have another element on the list, then clear its prev 
    // pointer. Otherwise, the list is empty and then clear the lastPtr.
    if (firstPtr != 0)
    {
        firstPtr->prev = 0;
    }
    else
    {
        lastPtr = 0;
    }

    tmp->next = 0;           // clear pointer to indicate not on list

    count--;

    return tmp;
}

/************************************************************************
 *                	          next()
 ************************************************************************
 *
 * Description:
 *   This is the next element iterator for the list class. This function
 *   will return the next object from the list without dequeueing the
 *   element from the list.  The iterator is then set to the next element.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   Pointer to the object
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
T * DList < T > :: next( void )
{
    DListElem <T> *tmp;

    tmp = iterator;
    if (tmp != NULL)
    {
        iterator = tmp->next;
        return tmp->object;
    }
    
    return 0;

}

/************************************************************************
 *                	          nextElem()
 ************************************************************************
 *
 * Description:
 *   This is the next element iterator for the list class. This function
 *   will return the next object from the list without dequeueing the
 *   element from the list.  The interator is then set to the next element.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   Pointer to the next list element
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
DListElem <T> * DList < T > :: nextElem( void )
{
    DListElem <T> *tmp;

    tmp = iterator;
    if (tmp != NULL)
    {
        iterator = tmp->next;
    }

    return tmp;
}

/************************************************************************
 *                	          prev()
 ************************************************************************
 *
 * Description:
 *   This is the previous element iterator for the list class. This function
 *   will return the previous object from the list without dequeueing the
 *   element from the list.  The iterator is then set to the previous element.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   Pointer to the object
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
T * DList < T > :: prev( void )
{
    DListElem <T> *tmp;

    tmp = iterator;
    if (tmp != NULL)
    {
        iterator = tmp->prev;
        return tmp->object;
    }
    
    return 0;

}

/************************************************************************
 *                	          prevElem()
 ************************************************************************
 *
 * Description:
 *   This is the previous element iterator for the list class. This function
 *   will return the previous object from the list without dequeueing the
 *   element from the list.  The interator is then set to the previous element.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   Pointer to the next list element
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
DListElem <T> * DList < T > :: prevElem( void )
{
    DListElem <T> *tmp;

    tmp = iterator;
    if (tmp != NULL)
    {
        iterator = tmp->prev;
    }

    return tmp;
}

/************************************************************************
 *                	          isMember()
 ************************************************************************
 *
 * Description:
 *   This function will search the list to see if the object is a member 
 *   of this list.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   Boolean where true = Is a member.
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
bool DList < T > :: isMember(T* obj ) const
{
    DListElem <T> *tmp = firstPtr;
    bool found = false;

    while (tmp != NULL && !found)
    {
        if (!(found = (tmp->object == obj)))
        {
            tmp = tmp->next;
        }
    }
    return found;
}

/************************************************************************
 *                	          insertAfter()
 ************************************************************************
 *
 * Description:
 *   This function will insert a new list element into the list after 
 *   the element specified.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   Boolean where true = Is a member.
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
bool DList < T > :: insertAfter( DListElem <T> *newElem, 
                                 DListElem <T> *elem)
{
    bool found = false;

    found = isMember(elem->object);
    if (found)
    {
        if (elem == lastPtr)
        {
            // Special case if insert after the last elem.
            enqueueTail(newElem);
        }
        else
        {
            // count is incremented by 1.
        newElem->next = elem->next;
        elem->next = newElem;
        newElem->prev = elem;
        newElem->next->prev = newElem;
            count++;
        }
    }
    return found;
}

/************************************************************************
 *                	          insertBefore()
 ************************************************************************
 *
 * Description:
 *   This function will insert a new list element into the list before 
 *   the element specified.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   Boolean where true = Is a member.
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
bool DList < T > :: insertBefore( DListElem <T> *newElem, 
                                  DListElem <T> *elem)
{
    bool found = false;

    found = isMember(elem->object);
    if (found)
    {
        if (elem == firstPtr)
        {
            // Special case if insert before the first elem.
            enqueueHead(newElem);
        }
        else
        {
            // count is incremented by 1.
        newElem->prev = elem->prev;
        elem->prev = newElem;
        newElem->next = elem;
        newElem->prev->next = newElem;
            count++;
        }
    }
    return found;
}
 
/************************************************************************
 *                	          remove()
 ************************************************************************
 *
 * Description:
 *   This is the remove function for the list class. This will remove
 *   the object iterator pointed to from the list, and move iterator to
 *   the next element. Iterator can be firstPtr or lastPtr.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   pointer to the object just removed from the list.
 *
 * HISTORY:
 *   04-Jun-03 Created by RG.
 *
 ************************************************************************/
template <class T> 
T * DList < T > :: remove( void )
{
    DListElem <T> *tmp;

    if (isEmpty())
    {
        return 0;
    }

    tmp = iterator;
    
    if (firstPtr == iterator)
    {
        firstPtr = iterator->next;
    }
    else
    {
        iterator->prev->next = iterator->next;
    }

    if (lastPtr == iterator)
    {
        lastPtr = iterator->prev;
    }
    else
    {
        iterator->next->prev = iterator->prev;
    }

    iterator = iterator->next;

    count--;
    tmp->prev = tmp->next = NULL;

    if (!isEmpty())
    {
        ASSERT(lastPtr != NULL);
        ASSERT(firstPtr != NULL);
    }

    return tmp->object;
}
 
/************************************************************************
 *                	          removeElem()
 ************************************************************************
 *
 * Description:
 *   This is the remove function for the list class. This will remove
 *   the iterator from the list, and move iterator to
 *   the next element. Iterator can be firstPtr or lastPtr.
 *
 * Input:
 *   none.
 *
 * Returns:
 *   pointer to the object just removed from the list.
 *
 * HISTORY:
 *   04-Jun-03 Created by RG.
 *
 ************************************************************************/
template <class T> 
DListElem <T> * DList < T > :: removeElem( void )
{
    DListElem <T> *tmp;

    if (isEmpty())
    {
        return NULL;
    }

    tmp = iterator;
    
    if (firstPtr == iterator)
    {
        firstPtr = iterator->next;
    }
    else
    {
        iterator->prev->next = iterator->next;
    }

    if (lastPtr == iterator)
    {
        lastPtr = iterator->prev;
    }
    else
    {
        iterator->next->prev = iterator->prev;
    }

    iterator = iterator->next;

    count--;
    tmp->prev = tmp->next = NULL;

    return tmp;
}
 
} // END OF asetl namespace

#endif // CPPETL_DLIST_H
