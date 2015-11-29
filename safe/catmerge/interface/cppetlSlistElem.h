#ifndef CPPETL_SLISTELEM_H
#define CPPETL_SLISTELEM_H

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
//       The SListElement class is a templated list element class. This 
//       class implements all of the necessary methods to for an element
//       of a singly linked list.
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

template <class T> class SList;

template <class T> class SListElem
{
    friend class SList<T>; 
public:
    SListElem(const T *);
    SListElem();
    ~SListElem();
    const T* getObject() const;
    void setObject(const T *);

private:
    const T *object;           // Pointer to Object being linked together
                               // So we don't have to copy the object.
	SListElem <T> *next;       // Pointer to next object
};



/************************************************************************
 *                	          SListElem
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
SListElem< T >::SListElem(const T *data) : next(0)
{
    object = data;             // Save a pointer to the object containing
                                // this object
}

/************************************************************************
 *                	          SListElem
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
SListElem< T >::SListElem(void ) : next(0), object(0)
{

}

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
SListElem < T > :: ~SListElem()
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
const T * SListElem < T > :: getObject() const
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
void SListElem < T > :: setObject(const T *obj)
{
    object = obj;
    return ;
}
 
} // END OF asetl namespace

#endif // CPPETL_SLISTELEM_H
