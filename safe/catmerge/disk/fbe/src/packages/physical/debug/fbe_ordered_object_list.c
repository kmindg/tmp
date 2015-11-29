/******************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *****************************************************************************/

/*!****************************************************************************
 * @file    fbe_ordered_object_list.c
 *
 * @brief   This module implements a circular double linked list to manage the objects
 *          in order. It is based on double linked list definition in csx_ext_utils.h
 *
 * @author
 *          @6/18/2012 Michael Li -- Created.
 *          @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/

#include "csx_ext.h"
#include <malloc.h>
#include "fbe/fbe_platform.h"
#include "fbe_ordered_object_list.h"
#include "pp_dbgext.h"

/*
    Abstraction for portability
*/
typedef unsigned long	fbe_ordered_object_list_magicnum_t;

//Shown in debugger as "DLST"
#define FBE_ORDERED_OBJECT_LIST_HEAD_MAGIC          0x444C5354
#define FBE_ORDERED_OBJECT_LIST_MEMORY_ALLOCATION	malloc	        
#define FBE_ORDERED_OBJECT_LIST_MEMORY_RELEASE		free	        

#define FBE_ORDERED_DLIST_FOR_EACH_ENTRY(_walker, _head) \
    for ((_walker) = (_head)->csx_dlist_field_next; \
         (_walker) != (_head); \
         (_walker) = (_walker)->csx_dlist_field_next)

#define FBE_ORDERED_DLIST_FOR_EACH_ENTRY_REVERSE(_walker, _head) \
    for ((_walker) = (_head)->csx_dlist_field_prev; \
         (_walker) != (_head); \
         (_walker) = (_walker)->csx_dlist_field_prev)

/*
    Circular double linked list node
*/
typedef struct _fbe_ordered_obj_list_node
{
    //MUST be the first to cast
    csx_dlist_entry_t       dl_node;
    fbe_dbgext_ptr          object_ptr;
}fbe_ordered_object_list_node_t;

/*
    Circular double linked list head
*/
typedef struct _fbe_ordered_obj_list_head
{
    csx_dlist_head_t                    dl_head;
    fbe_object_order_type_e             order_type;
    fbe_ordered_object_list_magicnum_t  obj_list_magic;
    fbe_ordered_object_list_node_t*     walk_node_ptr;
}fbe_ordered_object_list_head_t;

static void		fbe_ordered_object_list_insert_before(	fbe_ordered_object_list_head_t* list, 
														fbe_ordered_object_list_node_t* ref_node, 
														fbe_ordered_object_list_node_t* node_to_insert);

static void     fbe_ordered_object_list_insert_after(	fbe_ordered_object_list_head_t* list, 
			  								            fbe_ordered_object_list_node_t* ref_node, 
											            fbe_ordered_object_list_node_t* node_to_insert);

/*!****************************************************************************
 * @fn  fbe_ordered_object_list_create
 *
 * @brief
 *      This function creates a circular double linked empty list with the
 *      pre-defined order specified by obj_order_type argument. Nodes can be inserted after creation.
 *
 * @param obj_order_type - one of the following:
 *        FBE_OBJ_ORDER_TYPE_ASCEND,
 *        FBE_OBJ_ORDER_TYPE_DESCEND,
 *        The order of the list is pre-defined by this parameter. It can NOT be
 *        changed dynamically.
 *               
 * @param ret_list_handle - pointer of pointer to return the head of the created list. 
 *
 * @return
 *      FBE_OBJ_LIST_RET_CODE_ERROR - the list is created successfully.
 *      FBE_OBJ_LIST_RET_CODE_OK   - the list is not created.
 *
 * @author
 *		@6/18/2012 Michael Li -- Created.
 *		@6/29/2012 Armando Vega -- Review actions.
 *
 *****************************************************************************/
fbe_ordered_status_e fbe_ordered_object_list_create(fbe_object_order_type_e obj_order_type, fbe_ordered_handle_t *ret_list_handle)
{
    fbe_ordered_status_e retv = FBE_OBJ_LIST_RET_CODE_ERROR; 
	fbe_ordered_handle_t new_head_ptr = NULL;
    fbe_ordered_object_list_head_t *new_head_struct_ptr = NULL;
    
    if((obj_order_type != FBE_OBJ_ORDER_TYPE_ASCEND) &&
       (obj_order_type != FBE_OBJ_ORDER_TYPE_DESCEND))
    {
        *ret_list_handle = NULL; 
        return retv; 
    }
                    
    new_head_ptr = (fbe_ordered_object_list_head_t*)FBE_ORDERED_OBJECT_LIST_MEMORY_ALLOCATION(sizeof(fbe_ordered_object_list_head_t));
    
    if(new_head_ptr)
	{
        new_head_struct_ptr = (fbe_ordered_object_list_head_t*)new_head_ptr;
        new_head_struct_ptr->obj_list_magic	= FBE_ORDERED_OBJECT_LIST_HEAD_MAGIC;
	    new_head_struct_ptr->order_type        = obj_order_type;
        new_head_struct_ptr->walk_node_ptr     = NULL;
        csx_dlist_head_init(&(new_head_struct_ptr->dl_head)); 
        
	    *ret_list_handle = new_head_ptr;
	    retv = FBE_OBJ_LIST_RET_CODE_OK;
    }
	
	return retv; 
}


/*!****************************************************************************
 * @fn fbe_ordered_object_list_insert_before     
 *
 * @brief
 *      This function insert a node before the reference node. 
 *
 * @param list - the list head pointer
 * @param ref_node = reference node before which the node is going to be inserted.
 * @param node_to_insert - node to be inserted.
 *
 * @author
 *      @6/18/2012 Michael Li -- Created.
 *      @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/
static void fbe_ordered_object_list_insert_before(  fbe_ordered_object_list_head_t* list, 
										            fbe_ordered_object_list_node_t* ref_node, 
										            fbe_ordered_object_list_node_t* node_to_insert)
{
	if( ref_node && node_to_insert)	
	{
		(node_to_insert->dl_node).csx_dlist_field_prev = (ref_node->dl_node).csx_dlist_field_prev;
		(node_to_insert->dl_node).csx_dlist_field_next = &(ref_node->dl_node);
		((node_to_insert->dl_node).csx_dlist_field_prev)->csx_dlist_field_next = &(node_to_insert->dl_node);
		((node_to_insert->dl_node).csx_dlist_field_next)->csx_dlist_field_prev = &(node_to_insert->dl_node);
	}
}

/*!****************************************************************************
 * @fn fbe_ordered_object_list_insert_after     
 *
 * @brief
 *      This function insert a node after the reference node. 
 *
 * @param list - the list head pointer
 * @param ref_node = reference node after which the node is going to be inserted.
 * @param node_to_insert - node to be inserted.
 *
 * @author
 *      @6/18/2012 Michael Li -- Created.
 *      @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/
static void fbe_ordered_object_list_insert_after(   fbe_ordered_object_list_head_t* list, 
											        fbe_ordered_object_list_node_t* ref_node, 
											        fbe_ordered_object_list_node_t* node_to_insert)
{
	if( ref_node && node_to_insert)	
	{
		(node_to_insert->dl_node).csx_dlist_field_next = (ref_node->dl_node).csx_dlist_field_next;
		(node_to_insert->dl_node).csx_dlist_field_prev = &(ref_node->dl_node);
		((ref_node->dl_node).csx_dlist_field_next)->csx_dlist_field_prev = &(node_to_insert->dl_node);
		(ref_node->dl_node).csx_dlist_field_next = &(node_to_insert->dl_node);
	}
 }


/*!****************************************************************************
 * @fn fbe_ordered_object_list_insert     
 *
 * @brief
 *      This function insert a node into a list, whose order was specified
 *      when the list was created. 
 *      Caller must provide the compare function: my_object_cmp_func with the
 *      following return values:
 *                  COMPARE_LESS_THAN - object < inlistobject
 *                  COMPARE_EQUAL - equal 
 *                  COMPARE_HIGHER_THAN - object > inlisobject
 * @param list_handle - the list head pointer
 * @param object - pointer to the object to be inserted into the list.
 * @param my_object_cmp_func - callback function pointer to compare two objects.
 *                             The compare result must follow the criteria above.
 *
 * @return
 *      FBE_DLIST_RET_CODE_ERROR - insert failed.
 *      FBE_OBJ_LIST_RET_CODE_OK - insert suceedded.
 *
 * @author
 *      @6/1/2012 Michael Li -- Created.
 *      @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/

fbe_ordered_status_e fbe_ordered_object_list_insert(fbe_ordered_handle_t list_handle, 
                                                    fbe_dbgext_ptr object, 
                                                    compare_status_t (*my_object_cmp_func)(fbe_dbgext_ptr newobject, fbe_dbgext_ptr inlistobject))
{

    fbe_ordered_status_e                retv;
	csx_dlist_entry_t*                  base_node_ptr;       
    fbe_ordered_object_list_node_t*     ref_node_ptr;
	compare_status_t                    cmp_result;
    fbe_ordered_object_list_head_t*     list_handle_struct_ptr;
    fbe_ordered_object_list_node_t*     new_node;

    retv = FBE_OBJ_LIST_RET_CODE_ERROR;
	base_node_ptr	= NULL;       
    ref_node_ptr	= NULL;
	list_handle_struct_ptr = NULL;

	if(list_handle && object && my_object_cmp_func)
	{
        list_handle_struct_ptr = (fbe_ordered_object_list_head_t*) list_handle;                                    

        new_node = (fbe_ordered_object_list_node_t*)FBE_ORDERED_OBJECT_LIST_MEMORY_ALLOCATION(sizeof(fbe_ordered_object_list_node_t));
        if(new_node == NULL)
        {
            fbe_debug_trace_func(NULL, "Not Enough Memory\n");
            return retv;
        }
        
		new_node->object_ptr = object;
        
        // Insert the first node directly for empty list
        if(csx_dlist_is_empty(&(list_handle_struct_ptr->dl_head)))
        {
            csx_dlist_add_entry_head(&(list_handle_struct_ptr->dl_head), &(new_node->dl_node)); 
            retv = FBE_OBJ_LIST_RET_CODE_OK;
        }
        else
        {
             FBE_ORDERED_DLIST_FOR_EACH_ENTRY(base_node_ptr, &(list_handle_struct_ptr->dl_head))
            {
                ref_node_ptr = (fbe_ordered_object_list_node_t*)base_node_ptr;
				cmp_result = my_object_cmp_func(object, ref_node_ptr->object_ptr);

                if (cmp_result == COMPARE_ERROR)
                {
                    fbe_debug_trace_func(NULL, "Could not insert object\n");
                    break;
                }

                if(((list_handle_struct_ptr->order_type == FBE_OBJ_ORDER_TYPE_DESCEND) && 
                    ((cmp_result == COMPARE_HIGHER_THAN) || (cmp_result == COMPARE_EQUAL))) ||
                    ((list_handle_struct_ptr->order_type == FBE_OBJ_ORDER_TYPE_ASCEND) && 
                    ((cmp_result == COMPARE_LESS_THAN) || (cmp_result == COMPARE_EQUAL))))
                 {
                    //insert before the current node
                    fbe_ordered_object_list_insert_before(list_handle, ref_node_ptr, new_node);
                    retv = FBE_OBJ_LIST_RET_CODE_OK;
                    break;
                 } 
                 else if(CSX_DLIST_HAS_NO_NEXT_ENTRY(&(ref_node_ptr->dl_node), &(list_handle_struct_ptr->dl_head)))
                 {
                    //insert as the last node
                    csx_dlist_add_entry_tail( &(list_handle_struct_ptr->dl_head), &(new_node->dl_node));
                    retv = FBE_OBJ_LIST_RET_CODE_OK;
                    break;
                }
            }
            if(retv != FBE_OBJ_LIST_RET_CODE_OK)
            {
                    FBE_ORDERED_OBJECT_LIST_MEMORY_RELEASE(new_node);
            }// FBE_ORDERED_DLIST_FOR_EACH_ENTRY(base_node_ptr, &(list_handle->dl_head))
        }       // if(csx_dlist_is_empty(&(list->dl_head)))
	}           // if(list_handle && object && my_object_cmp_func)

    return retv;
}

/*!****************************************************************************
 * @fn fbe_ordered_object_list_walk     
 *
 * @brief
 *      This function walks through a list and returns an object pointer via callback, 
 *      each pointer at a time..
 *
 * @param list_handle - the list head pointer
 * @param walk_callback_func - callback to pass the object pointer back to the caller.
 * @param reverse_walk - If set to true, the list will be walked backwards starting with
 *                       the last element.
 *
 * @return
        FBE_OBJ_LIST_RET_CODE_OK - when the callback function has been called. The
                                   caller should check the pointer passed back..
        FBE_OBJ_LIST_RET_CODE_ERROR - callback function has not been called.
 *
 * @author
 *      @6/18/2012 Michael Li -- Created.
 *      @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/
fbe_ordered_status_e fbe_ordered_object_list_walk(  fbe_ordered_handle_t list_handle, 
                                                    void (*walk_callback_func)(fbe_dbgext_ptr object),
                                                    fbe_bool_t reverse_walk)
{

    fbe_ordered_status_e retv = FBE_OBJ_LIST_RET_CODE_ERROR; 
    fbe_ordered_object_list_head_t* list_handle_struct_ptr = NULL;
    fbe_dbgext_ptr object_ptr = NULL;
    csx_dlist_entry_t*  base_node_ptr;

    list_handle_struct_ptr = (fbe_ordered_object_list_head_t*) list_handle;  

    if(list_handle && walk_callback_func)
    {
        if(csx_dlist_is_empty(&(list_handle_struct_ptr->dl_head)))
        {
            list_handle_struct_ptr->walk_node_ptr = NULL;
            retv = FBE_OBJ_LIST_RET_CODE_ERROR;
        }
        else
        {
            /* Note that base_node_ptr is not used on this loop since the walk methods
             * keep track of their position using an internal reference.
             */
            if (reverse_walk)
            {
                FBE_ORDERED_DLIST_FOR_EACH_ENTRY_REVERSE(base_node_ptr, &(list_handle_struct_ptr->dl_head))
                {
                    //check for the last node
                    if(list_handle_struct_ptr->walk_node_ptr == NULL)
                    {
                        object_ptr = fbe_ordered_object_list_walk_get_last(list_handle);
                        walk_callback_func(object_ptr);
                    }
                    else
                    {
                        object_ptr = fbe_ordered_object_list_walk_get_prev(list_handle);
                        walk_callback_func(object_ptr);
                    }
                }
            }
            else
            {
                FBE_ORDERED_DLIST_FOR_EACH_ENTRY(base_node_ptr, &(list_handle_struct_ptr->dl_head))
                {
                    //check for the first node
                    if(list_handle_struct_ptr->walk_node_ptr == NULL)
                    {
                        object_ptr = fbe_ordered_object_list_walk_get_first(list_handle);
                        walk_callback_func(object_ptr);
                    }
                    else
                    {
                        object_ptr = fbe_ordered_object_list_walk_get_next(list_handle);
                        walk_callback_func(object_ptr);
                    }
                }
            }
            retv = FBE_OBJ_LIST_RET_CODE_OK;
        }
        list_handle_struct_ptr->walk_node_ptr = NULL;

    }

    return retv;
}

/*!****************************************************************************
 * @fn fbe_ordered_object_list_walk_get_last     
 *
 * @brief
 *      This function gets the object pointer from the last node of the list.
 *
 * @param list_handle - the list head pointer
 *
 * @return
 *      Pointer to the object that the last node points to.       
 *      NULL - if the list is empty.
 *
 * Note: Whenever this function is called, the internal walk_node_ptr is reset
 *       to point to the last node or NULL if the list is empty. 
 *
 * @author
 *      @6/29/2012 Armando Vega -- Created.
 *
 *****************************************************************************/
fbe_dbgext_ptr fbe_ordered_object_list_walk_get_last(fbe_ordered_handle_t list_handle)
{
    fbe_dbgext_ptr object_ptr = NULL;
    fbe_ordered_object_list_head_t*     list_handle_struct_ptr = NULL;
    
    if(list_handle)
    {
        list_handle_struct_ptr = (fbe_ordered_object_list_head_t*) list_handle;  
        if(csx_dlist_is_empty(&(list_handle_struct_ptr->dl_head)))
        {
            list_handle_struct_ptr->walk_node_ptr = NULL;
        }
        else
        {
            /* Since this is a circular list, last element can be reached by seeking previous to first.
             */
            list_handle_struct_ptr->walk_node_ptr = (fbe_ordered_object_list_node_t*)((((fbe_ordered_object_list_head_t*)(list_handle))->dl_head).csx_dlist_field_prev);
            object_ptr = (list_handle_struct_ptr->walk_node_ptr)->object_ptr;
        }
    }
    
    return object_ptr;
}

/*!****************************************************************************
 * @fn fbe_ordered_object_list_walk_get_first     
 *
 * @brief
 *      This function gets the object pointer from the first node of the list.
 *
 * @param list_handle - the list head pointer
 *
 * @return
 *      Pointer to the object that the first node points to.       
 *      NULL - if the list is empty.
 *
 *    Note: Whenever this function is called, the internal walk_node_ptr is reset
 *          to point to the first node or NULL if the list is empty. 
 *
 * @author
 *      @6/18/2012 Michael Li -- Created.
 *      @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/
fbe_dbgext_ptr fbe_ordered_object_list_walk_get_first(fbe_ordered_handle_t list_handle)
{
    fbe_dbgext_ptr object_ptr = NULL;
    fbe_ordered_object_list_head_t*     list_handle_struct_ptr = NULL;
    
    if(list_handle)
    {
        list_handle_struct_ptr = (fbe_ordered_object_list_head_t*) list_handle;  
        if(csx_dlist_is_empty(&(list_handle_struct_ptr->dl_head)))
        {
            list_handle_struct_ptr->walk_node_ptr = NULL;
        }
        else
        {
            list_handle_struct_ptr->walk_node_ptr = (fbe_ordered_object_list_node_t*)((((fbe_ordered_object_list_head_t*)(list_handle))->dl_head).csx_dlist_field_next);
            object_ptr = (list_handle_struct_ptr->walk_node_ptr)->object_ptr;
        }
    }
    
    return object_ptr;
}


/*!****************************************************************************
 * @fn fbe_ordered_object_list_walk_get_next      
 *
 * @brief
 *      This function gets the next object pointer from the list.
 *
 * @param list_handle - the list head pointer
 *
 * @return
 *      pointer to the next object
 *      NULL if next object does not exist.
 *
 * @author
 *      @6/18/2012 Michael Li -- Created.
 *      @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/
fbe_dbgext_ptr fbe_ordered_object_list_walk_get_next(fbe_ordered_handle_t list_handle)
{
    fbe_dbgext_ptr object_ptr = NULL;
    fbe_ordered_object_list_node_t* next_node = NULL;
    fbe_ordered_object_list_head_t* list_handle_struct_ptr = NULL;

    if(list_handle)
    {
        list_handle_struct_ptr = (fbe_ordered_object_list_head_t*) list_handle;  

        if(list_handle_struct_ptr->walk_node_ptr)
        {
            if(CSX_DLIST_HAS_NEXT_ENTRY(&((list_handle_struct_ptr->walk_node_ptr)->dl_node), &(((fbe_ordered_object_list_head_t*)(list_handle))->dl_head)))
            {    
                next_node = (fbe_ordered_object_list_node_t*)(((list_handle_struct_ptr->walk_node_ptr)->dl_node).csx_dlist_field_next);
                //Update walk_node_ptr to the next node 
                list_handle_struct_ptr->walk_node_ptr = next_node;
                object_ptr = next_node->object_ptr;
            }
        }
    }
    
    return object_ptr;
}


/*!****************************************************************************
 * @fn fbe_ordered_list_walk_get_prev     
 *
 * @brief
 *      This function gets the previous object pointer from the list.
 *
 * @param list_handle - the list head pointer
 *
 * @return
 *      pointer to the previous object
 *      NULL if the previous object does not exist.
 *
 * @author
 *      @6/18/2012 Michael Li -- Created.
 *      @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/
fbe_dbgext_ptr fbe_ordered_object_list_walk_get_prev(fbe_ordered_handle_t list_handle)
{
    fbe_dbgext_ptr object_ptr = NULL;
    fbe_ordered_object_list_node_t* prev_node = NULL;
    fbe_ordered_object_list_head_t* list_handle_struct_ptr = NULL;

    if(list_handle)
    {
        list_handle_struct_ptr = (fbe_ordered_object_list_head_t*) list_handle;  

        if (list_handle_struct_ptr->walk_node_ptr)
        {
            if(CSX_DLIST_HAS_PREV_ENTRY(&((list_handle_struct_ptr->walk_node_ptr)->dl_node), &(((fbe_ordered_object_list_head_t*)(list_handle))->dl_head)))
            {    
                prev_node = (fbe_ordered_object_list_node_t*)(((list_handle_struct_ptr->walk_node_ptr)->dl_node).csx_dlist_field_prev);
                //Update walk_node_ptr to the previous node 
                list_handle_struct_ptr->walk_node_ptr = prev_node;
                object_ptr = prev_node->object_ptr;
            }
        }
    }
    
    return object_ptr;
}


//destroy the double linked list. The resources of the list are freed, but the object keep intact.
/*!****************************************************************************
 * @fn fbe_ordered_object_list_destroy     
 *
 * @brief
 *      This function destroys the double linked list and release the resources. The objects
 *      it managed keep intact.
 *
 * @param list_handle - the list head pointer
 *
 * @author
 *      @6/18/2012 Michael Li -- Created.
 *      @6/29/2012 Armando Vega -- Review Actions.
 *
 *****************************************************************************/
void fbe_ordered_object_list_destroy(fbe_ordered_handle_t list_handle)
{
    fbe_ordered_object_list_node_t*     ref_node_ptr;
    fbe_ordered_object_list_node_t*     prev_node_ptr;
	csx_dlist_entry_t*                  base_node_ptr;
    fbe_ordered_object_list_head_t*     list_handle_struct_ptr;

    ref_node_ptr = NULL;
    prev_node_ptr = NULL;
	base_node_ptr = NULL;
    list_handle_struct_ptr = (fbe_ordered_object_list_head_t*) list_handle;

    FBE_ORDERED_DLIST_FOR_EACH_ENTRY(base_node_ptr, &(list_handle_struct_ptr->dl_head))
    {
        ref_node_ptr = (fbe_ordered_object_list_node_t*)base_node_ptr;
		if(prev_node_ptr)
		{
			FBE_ORDERED_OBJECT_LIST_MEMORY_RELEASE(prev_node_ptr);
			prev_node_ptr = NULL;
		}
		if(ref_node_ptr)
		{
			prev_node_ptr = ref_node_ptr;
		}
	}
	//delete the last node
	if(prev_node_ptr)
	{
		FBE_ORDERED_OBJECT_LIST_MEMORY_RELEASE(prev_node_ptr);
		prev_node_ptr = NULL;
	}

	//delete the head
	FBE_ORDERED_OBJECT_LIST_MEMORY_RELEASE((fbe_ordered_object_list_head_t*)list_handle);
}

