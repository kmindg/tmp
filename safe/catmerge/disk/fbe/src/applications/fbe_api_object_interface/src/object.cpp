/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011                    
 * All Rights Reserved.                                          
 * Licensed Material-Property of EMC Corporation.                
 * This software is made available solely pursuant to the terms  
 * of a EMC license agreement which governs its use.             
 *********************************************************************/

/*********************************************************************
 *
 *  Description: 
 *      This file defines the methods of the Object, Nil, and list 
 *      classes. 
 *  
 *  Notes:
 *      Object class : Base (ADT for container and packages)
 *      Nil class    : Derived (place holder)
 *      list class   : Derived (container class methods)
 *
 *  History:
 *      07-March-2011 : inital version
 *
 *********************************************************************/

#ifndef T_OBJECT_H
#include "object.h"
#endif

/*********************************************************************
 * Object class - Constructor methods (Base class)
 *********************************************************************/
        
Object::Object()
{
    idnumber=OBJECT; 
    if (Debug) {
        sprintf(temp, "Object class constructor (%d)\n", idnumber);
        vWriteFile(dkey, temp);
    }
    objCount = ++gObjCount;
    next = prev = NULL;
    head = tail = NULL;
}

Object::Object(FLAG f ) 
{
    idnumber=OBJECT;
    if (Debug) {
        sprintf(temp, "Initial Object class constructor (%d)\n", idnumber);
        vWriteFile(dkey, temp);
    }
    objCount = ++gObjCount;
    next = prev = NULL;
    head = tail = NULL;
}

/*********************************************************************
 * Object class - Accessor methods (Get class data)
 *********************************************************************/

unsigned Object::MyObjNumIs(void)
{ 
    return objCount;
}

unsigned Object::MyIdNumIs(void)
{ 
    return idnumber;
}

void Object::dumpme(void) 
{   
    strcpy (key, "Object::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
        idnumber, objCount);
    vWriteFile(key, temp);
}

void Object::setHelp(unsigned dhelp) 
{   
    help = dhelp;
}

void Object::setDebug(int ddebug) 
{   
    Debug= ddebug;
}

/*********************************************************************
 * Object class : Get object name of non FBE API type objects. 
 *********************************************************************/  

char * Object::ObjTypeName(unsigned id )
{
    char * buffer = new char [MAX_STRING_LENGTH];
  
    switch (id) 
    {
        case OBJECT : 
            strcpy(buffer, "OBJECT"); 
            break;
        case DOBJECT : 
            strcpy(buffer, "DOBJECT"); 
            break;
        case FILEUTILCLASS : 
            strcpy(buffer, "FILE UTILITY CLASS"); 
            break;
        case ARGUMENTS : 
            strcpy(buffer, "ARGUMENTS"); 
            break;
        case NILOBJECT : 
            strcpy(buffer, "Nil"); 
            break;
        case COLLECTIONOFOBJECTS : 
            strcpy(buffer, "List"); 
            break;
        default :
           strcpy(buffer, "Not Found"); 
    }

    return buffer;
}

/*********************************************************************
 * list class : Object class - base class of the list class
 *********************************************************************/

list::list(void) : Object(FIRSTTIME)
{
    idnumber=COLLECTIONOFOBJECTS; 
    if (Debug) {
        sprintf(temp, "Collector class constructor (%d)", idnumber);
        vWriteFile(dkey, temp);
    }
    itemcount  = 0;
    deadend = new Nil();
    for (int j=0; j<ARRAYSIZE; j++){array[j] = new Nil();}
}

list::~list()
{
    if (Debug) {
        sprintf(temp, "exiting Collect class (%d)", idnumber);
        vWriteFile(dkey, temp);
    }
}

/*********************************************************************
 * list class : Methods that keeps count of objects that are put in 
 *              list and /or returns the last count.
 *********************************************************************/

int list::NumItems(int x)
{
    itemcount = x;
    return itemcount;
}

int list::NumItems()
{
    return itemcount;
}

/*********************************************************************
 * list class : Method that returns current list pointer.
 *********************************************************************/
 
Object * list::getLast()
{
    return last;
}

/*********************************************************************
 * list class : Method that sets current list pointer.
 *********************************************************************/

void list::setLast( Object * p)
{
    last = p; 
    
    if (Debug) {
        sprintf(temp, "list::setLast");
        vWriteFile(dkey, temp);
        last->dumpme();
    }
}

/*********************************************************************
 * list class : Link object
 *********************************************************************/

int list::LinkObj(Object * p)
{
    if (Debug) {
        sprintf(temp, "list::LinkObj");
        vWriteFile(dkey, temp);
        p->dumpme();
    }
   
    if (Insert(p)) {
        sprintf(key, "list::LinkObj");
        sprintf(temp, "<ERROR> Can't link object. %s\n",
            "No room in array table.");
        vWriteFile(key, temp);
        return (FAIL);
    }
    
    p->prev = last->tail;
    p->next = NULL;
    
    last->tail->next = p;
    last->tail = p;
    return 0;
}

/*********************************************************************
 * list class : Find object type in table or next slot for new object 
 *              type.
 *********************************************************************/

int list::Insert(Object * p)
{
    id = p->MyIdNumIs();
    strcpy (key, "list::Insert");

    if (Debug) {
        sprintf(temp, "list::Insert into object type table");
        vWriteFile(dkey, temp);
        p->dumpme();
    }

    for (int j=0; j < ARRAYSIZE; j++){
        
        // Find exiting object type 
        
        if (array[j]->head != NULL) { 
            if (array[j]->next->MyIdNumIs() == id) {
                setLast(array[j]);
                
                if (Debug) {
                    sprintf(temp, "list::Insert %s ((%d)) of table",
                        "slot", j);
                    vWriteFile(dkey, temp);
                }
                return 0;
            } 
        
        // Setup for new object type on array. Replace head/tail
        // with Nil pointers to self

        } else {
            array[j]->head = p;
            array[j]->tail = array[j];
            setLast(array[j]);
            NumItems(j);
            
            if (Debug) {
                sprintf(temp, "list::Insert %s ((%d)) of table", 
                    "new slot", j);
                vWriteFile(dkey, temp);
            }
            return 0;
        } 
    }
    
    return 1;   
}

int list::Find (Object * p)
{
    id = p->MyIdNumIs();
    return (Find(id));
}

int list::Find (unsigned id)
{  
    strcpy (key, "list::Find");
    
    if (Debug) {
        sprintf(temp, "list::Find %s ((%d))\n",
            "Search table for object type", id);
        vWriteFile(dkey, temp);
    }
                
    for (int j=0; j < ARRAYSIZE; j++){
        if (array[j]->head != NULL) { 
            if (array[j]->next->MyIdNumIs() == id) {
                setLast(array[j]);
                return 0;
            } 
        }
    }
    
    sprintf(temp, "<ERROR> %s ((%d))", 
        "Did not find head for object type\n", id);
    vWriteFile(key, temp);            
    
    return 1;
}

void list::getHead()
{
    id = last->head->MyIdNumIs();
    
    if (Debug) {
        sprintf(temp, "list::getHead Object->id ((%d))\n", id);
        vWriteFile(dkey, temp);
        last->head->dumpme();
    }    
    
    if (Debug) {
        sprintf(temp, "list::getHead->next\n");
        vWriteFile(dkey, temp);
        last->next->dumpme();
    }
    
    last->next = last->head;
    if (Debug) {
        sprintf(temp, "list::getHead->next(after next=head)\n");
        vWriteFile(dkey, temp);
        last->next->dumpme();
    }
}

void list::getTail()
{
    id = last->tail->MyIdNumIs();
    
    if (Debug) {
        sprintf(temp, "list::getTail Object->id ((%d))\n", id);
        vWriteFile(dkey, temp);
        last->tail->dumpme();
    }
    //return last->tail;
}

int list::getNext()
{
    if (last->next != NULL) {
        id = last->next->MyIdNumIs();
    
        if (id != NILOBJECT) {
            
            if (Debug) {
                sprintf(temp, 
                    "list::getNext Object->id ((%d))\n", id);
                vWriteFile(dkey, temp);
                last->next->dumpme();
            }

            last->next = last->next;
            return 0;
        }
    }

    if (Debug) {
        sprintf(temp, "list::getNext (reached end of list)\n"); 
        vWriteFile(dkey, temp);
    }
    return 1;
}

int list::getPrev()
{
    if (last->prev != NULL) {
        id = last->prev->MyIdNumIs();
    
        if (id != NILOBJECT) {
            
            if (Debug) {
                sprintf(temp, 
                    "list::getPrev Object->id ((%d))\n", id);
                vWriteFile(dkey, temp);
                last->prev->dumpme();
            }
            
            last->prev = last->prev;
            return 0;
        }
    }

    if (Debug) {
        sprintf(temp, "list::getPrev (reached start of list)\n");
        vWriteFile(dkey, temp);
    }
    return 1;
}

void list::ListLinks(Object * p)
{ 
    unsigned id = p->MyIdNumIs();
    char * name = p->ObjTypeName(id);
    
    if (Debug) {
        sprintf(temp, 
            "list::ListLinks (object name: %s id ((%d)))\n", name, id);
        vWriteFile(dkey, temp);
        p->dumpme();
  
        sprintf(temp, "list::ListLinks (next object in list)\n");
        vWriteFile(dkey, temp);
        if (p->next != NULL) {p->next->dumpme();}

        sprintf(temp, "list::ListLinks (previous object in list)\n");
        vWriteFile(dkey, temp);
        if (p->prev != NULL) {p->prev->dumpme();}

        sprintf(temp, "list::ListLinks (head object in list)\n");
        vWriteFile(dkey, temp);
        if (p->head != NULL) {p->head->dumpme();}

        sprintf(temp, "list::ListLinks (tailobject in list)\n");
        vWriteFile(dkey, temp);
        if (p->tail != NULL) {p->tail->dumpme();}
    }
    delete name;
}
