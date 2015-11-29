#ifndef FBE_DATABASE_HOOK_H
#define FBE_DATABASE_HOOK_H

#include "fbe_database.h"
#include "fbe_database_private.h"

/***************************************************************************/
/** @file fbe_database_hook.h
***************************************************************************
*
* @brief
*  This file contains definitions of structures and functions that are exported
*  by database service for setting hook
* 
***************************************************************************/


typedef void (* database_hook_func)(struct fbe_packet_s * packet, void* hook_elem, void *context);


typedef struct fbe_database_hook_s
{
    fbe_database_hook_type_t hook_type;
    database_hook_func hook_function;  /*hook function for this hook*/
    fbe_u32_t hook_set; /*whether tester set this hook*/
    fbe_u32_t hook_triggered;  /*whether this hook has been triggered*/
}fbe_database_hook_t;

void database_hook_init(void);
fbe_status_t  database_set_hook(fbe_database_hook_type_t hook_type);
fbe_status_t  database_remove_hook(fbe_database_hook_type_t hook_type);
fbe_status_t  database_get_hook_state(fbe_database_hook_type_t hook_type, fbe_bool_t *is_set, fbe_bool_t *is_triggered);
fbe_status_t  database_check_and_run_hook(fbe_packet_t* packet, fbe_database_hook_type_t hook_type, void* context);


#endif /* FBE_DATABASE_HOOK_H */
