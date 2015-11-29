#ifndef CPPETL_SLIST_H
#define CPPETL_SLIST_H

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
//       The SList class is a templated list class. This class implements
//       all of the necessary methods to manage a singly linked list.
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
#include "cppetlSlistElem.h"

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

template <class T> class SList 
{
public:
    SList();
    SList(const SListElem <T> &);
    ~SList();
    void enqueue(SListElem <T> *);
    const T* dequeue(void);
    SListElem <T> * dequeueElem(void);
    inline void firstElem(void) {iterator = firstPtr; return; };
    inline const T* current(void) {return iterator->object; };
    inline SListElem <T> * currentElem(void) {return iterator; };
    const T* next(void);
    SListElem <T> * nextElem(void);
    inline BOOL isEmpty() const {return (count == 0);};
    inline int queueLength() const {return count;};

protected:
    void enqueueTail(SListElem <T> *);

private:
    int count;                 // Number of elements in list
    SListElem <T> *firstPtr;   // Pointer to 1st Element in list
    SListElem <T> *lastPtr;    // Pointer to last Element in list
    SListElem <T> *iterator;   // Current Location of iterator
};


/************************************************************************
 *                	          SList
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
SList < T > :: SList() : firstPtr(NULL), lastPtr(NULL), iterator(NULL), count(0) {};

/************************************************************************
 *                	          SList
 ************************************************************************
 *
 * Description:
 *   This is the  constructor for the Singly Linked List Class. 
 *   This is used to contruct new elements in the list.
 *
 * Input:
 *   SList Element
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
SList < T > :: SList(const SListElem <T> &elem) : 
    firstPtr(&elem), lastPtr(&elem), iterator(&elem), count(0) {};

/************************************************************************
 *                	          ~SList
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
SList < T > :: ~SList()
{
    // Nothing to be done here since the elements of the list are
    // part of the objects.
}

/************************************************************************
 *                	          enqueue()
 ************************************************************************
 *
 * Description:
 *   This is the enqueue function for the list class. This will add a new
 *   object to the list.
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
void SList < T > :: enqueue( SListElem <T> *obj)
{
    
    if (isEmpty())
    {
        firstPtr = obj;
        lastPtr = obj;
    }
    else
    {
        obj->next = firstPtr;
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
 *   object to the list at the end of the list.
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
void SList < T > :: enqueueTail( SListElem <T> *obj)
{
    
    if (isEmpty())
    {
        firstPtr = obj;
        lastPtr = obj;
    }
    else
    {
        lastPtr->next = obj;
        lastPtr = obj;
        obj->next = NULL;
    }

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
const T * SList < T > :: dequeue( void )
{
    SListElem <T> *tmp;

    if (isEmpty())
    {
        return NULL;
    }

    tmp = firstPtr;
    firstPtr = tmp->next;
    tmp->next = 0;           // clear pointer to indicate not on list

    if (firstPtr == NULL)
    {
        lastPtr = NULL;
    }

    count--;

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
SListElem <T> * SList < T > :: dequeueElem( void )
{
    SListElem <T> *tmp;

    if (isEmpty())
    {
        return NULL;
    }

    tmp = firstPtr;
    firstPtr = tmp->next;
    tmp->next = 0;           // clear pointer to indicate not on list

    if (firstPtr == NULL)
    {
        lastPtr = NULL;
    }

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
const T * SList < T > :: next( void )
{

    if (iterator != NULL)
    {
        iterator = iterator->next;
        if (iterator != NULL)
        {
            return iterator->object;
        }
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
SListElem <T> * SList < T > :: nextElem( void )
{

    if (iterator != NULL)
    {
        iterator = iterator->next;
    }

    return iterator;
}
 
} // END OF asetl namespace

#endif // CPPETL_SLIST_H
