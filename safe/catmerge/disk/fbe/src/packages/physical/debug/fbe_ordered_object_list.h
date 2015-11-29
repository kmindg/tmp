#ifndef FBE_ORDERED_OBJECT_LIST_H
#define FBE_ORDERED_OBJECT_LIST_H

#include "fbe/fbe_types.h"
/*
    An object can be in ascend or descend order in the list
*/
typedef enum
{
	FBE_OBJ_ORDER_TYPE_ASCEND,
	FBE_OBJ_ORDER_TYPE_DESCEND,
	//max
	FBE_OBJ_ORDER_TYPE_MAX,
}fbe_object_order_type_e;

/*
    Function return code
*/
typedef enum
{
	FBE_OBJ_LIST_RET_CODE_ERROR = -1,
	FBE_OBJ_LIST_RET_CODE_OK,
	//max
	FBE_OBJ_LIST_RET_CODE_MAX,
}fbe_ordered_status_e;

/* 
    Compare function return code
*/
typedef enum compare_status_e
{
    COMPARE_EQUAL,
    COMPARE_LESS_THAN,
    COMPARE_HIGHER_THAN,
    COMPARE_ERROR
}compare_status_t;


//handle to the head of the list
typedef   void*  fbe_ordered_handle_t;

fbe_ordered_status_e fbe_ordered_object_list_create(fbe_object_order_type_e obj_order_type, fbe_ordered_handle_t *ret_list_handle);

fbe_ordered_status_e fbe_ordered_object_list_insert(fbe_ordered_handle_t list_handle, 
                                                    fbe_dbgext_ptr object, 
                                                    compare_status_t (*my_object_cmp_func)(fbe_dbgext_ptr newobject, fbe_dbgext_ptr inlistobject));

fbe_ordered_status_e fbe_ordered_object_list_walk(  fbe_ordered_handle_t list_handle, 
                                                    void (*walk_callback_func)(fbe_dbgext_ptr object),
                                                    fbe_bool_t reverse_walk);

fbe_dbgext_ptr fbe_ordered_object_list_walk_get_first(fbe_ordered_handle_t list_handle);
fbe_dbgext_ptr fbe_ordered_object_list_walk_get_last(fbe_ordered_handle_t list_handle);
fbe_dbgext_ptr fbe_ordered_object_list_walk_get_next (fbe_ordered_handle_t list_handle);
fbe_dbgext_ptr fbe_ordered_object_list_walk_get_prev (fbe_ordered_handle_t list_handle);
void fbe_ordered_object_list_destroy(fbe_ordered_handle_t list_handle);
#endif /* FBE_ORDERED_OBJECT_LIST_H */
                                                            
