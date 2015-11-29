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
 *      This file defines the Object, Nil, and list classes. These 
 *      classes comprise the linked list. 
 
 *      The Object class is the base class of the linked list and 
 *      other data classes that are put into the linked list.
 *
 *      The Nil class provides the anchor point of the linked
 *      list of each type of object.
 *
 *      The list class performs the following operations:
 *          inserts new objects
 *          gets next object in list
 *          gets previous object in list 
 *          sets to walk list from tail or head of list.
 *
 *  History:
 *      07-March-2011 : initial version
 *
 *********************************************************************/

#ifndef T_OBJECT_H
#define T_OBJECT_H

#ifndef T_FILEUTILCLASS_H
#include "file_util_class.h"
#endif

/*********************************************************************
 * Object class - base class 
 *********************************************************************/

class Object : public fileUtilClass
{
    protected:
        // Every object has an idnumber
        unsigned idnumber;
        unsigned objCount;
        
    public:
        Object * next;
        Object * prev;
        Object * head;
        Object * tail;

        // Constructor
        Object(FLAG); 
        Object();
        
        // Destructor
        ~Object(){}

        // Accessor methods 
        char * ObjTypeName(unsigned);
        virtual unsigned MyObjNumIs(void);
        virtual unsigned MyIdNumIs(void);
        virtual char * MyIdNameIs(void) {return 0;}
        virtual void dumpme(void);
        virtual void setHelp(unsigned);
        virtual void setDebug(int);
        
        // FBE API abstract class methods 
        virtual fbe_status_t select(int i, int c, char *a[]) {
            return FBE_STATUS_INVALID;
        }
};

/*********************************************************************
 * Nil class : Object class 
 *********************************************************************/

class Nil: public Object
{
    protected:
       
        // Output data and name  
        char temp[1000]; 
        char key[30];
    
    public:
        
        // Constructor
        Nil() {
            idnumber = NILOBJECT;
            if (Debug) {
                strcpy (key, "<DEBUG!>"); 
                sprintf(temp, "Nil class constructor %d\n", idnumber);
                vWriteFile(key, temp);
            }
        }
        
        // Destructor
        virtual ~Nil(){
            if (Debug) {
                strcpy (key, "<DEBUG!>"); 
                sprintf(temp, "Nil class destructor %d\n", idnumber);
                vWriteFile(key, temp);
            }
        };
        
        // Display object data 
        void dumpme(void) {
            strcpy (key, "<Nil::dumpme>"); 
            sprintf(temp, "id %d count %d\n", idnumber, objCount);
            vWriteFile(key, temp);
        }
};

/*********************************************************************
 * list class : Object class (derived base class)
 *********************************************************************/

class list : public Object
{   
    protected:
        int itemcount;
        Object * deadend;  // pointer to a Nil object
        Object * array[ARRAYSIZE];
        Object * last;

    public:
        list(void); 
        ~list();
        
        Object * getLast(void);
        void setLast(Object *);
        
        int NumItems(void);
        int NumItems(int);
        int Insert(Object *);
        int Find(Object * );
        int Find(unsigned );
        int LinkObj(Object *);
        int getNext(void);
        int getPrev(void);
        
        void getHead(void);
        void getTail(void);
        void ListLinks(Object *);
};

#endif
