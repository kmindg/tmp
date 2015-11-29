#ifndef CPPETL_DLISTELEM_H
#define CPPETL_DLISTELEM_H

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
//       The DListElement class is a templated list element class. This 
//       class implements all of the necessary methods to for an element
//       of a doubly linked list.
//
// NOTES:
//
// HISTORY:
//    28-Jun-01  Created by Austin Spang
//    $Log:$
//
//**********************************************************************

//**********************************************************************
// INCLUDE FILES
//**********************************************************************

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

template <class T> class DList;

template <class T> class DListElem
{
    friend class DList<T>; 
public:
    DListElem(T *);
    DListElem();
    ~DListElem();
    T* getObject() const;
    void setObject(T *);

	DListElem <T> *get_next() {return next;};  // Normally not used.  But useful for WinDbg Scripts
	DListElem <T> *get_prev() {return prev;};  // Normally not used.  But useful for WinDbg Scripts

#ifdef __SFD_WINDBG_SUPPORT__
	void display();
#endif

private:
    T *object;           // Pointer to Object being linked together
                               // So we don't have to copy the object.
	DListElem <T> *next;       // Pointer to next object
	DListElem <T> *prev;       // Pointer to previous object
};

#ifdef __SFD_WINDBG_SUPPORT__
template <class T> 
void DListElem < T > :: display()
{
	nt_dprintf(" object=0x%08x;", (PVOID) object);
	nt_dprintf(" next=0x%08x;", (PVOID) next);
	nt_dprintf(" prev=0x%08x\n", (PVOID) prev);
}
#endif


/************************************************************************
 *                	          DListElem
 ************************************************************************
 *
 * Description:
 *   This is the constructor for the Singly Linked List Element
 *   Class. This is used to contruct new elements in the list.
 *
 * Input:
 *   Contant Reference to the object which contains this list element
 *
 * Returns:
 *   void
 *
 * HISTORY:
 *   26-Jun-01 Created by Austin Spang
 *
 ************************************************************************/
template <class T> 
DListElem< T >::DListElem(T *data) : next(0), prev(0)
{
    object = data;             // Save a pointer to the object containing
                                // this object
}

/************************************************************************
 *                	          DListElem
 ************************************************************************
 *
 * Description:
 *   This is the default constructor for the Singly Linked List Element
 *   Class. This is used to contruct new elements in the list.
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
DListElem< T >::DListElem(void ) : next(0), object(0), prev(0)
{

}

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
DListElem < T > :: ~DListElem()
{
}

/************************************************************************
 *                	          getObject
 ************************************************************************
 *
 * Description:
 *   This method will return a pointer to the object that contains this
 *   list element.
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
T * DListElem < T > :: getObject() const
{
    return object;
}

/************************************************************************
 *                	          setObject
 ************************************************************************
 *
 * Description:
 *   This method will the pointer to the object that contains this
 *   list element.
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
void DListElem < T > :: setObject(T *obj)
{
    object = obj;
    return ;
}

} // END OF asetl namespace

#endif // CPPETL_DLISTELEM_H
