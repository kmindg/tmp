#ifndef FBE_PERSIST_DEBUG_H
#define FBE_PERSIST_DEBUG_H

/***************************************************************************/
/** @file fbe_persist_debug.h
***************************************************************************
*
* @brief
*  This file contains definitions of structures and functions that are exported
*  by persist service for setting hook
* 
***************************************************************************/

#include "fbe/fbe_persist_interface.h"

typedef void (* persist_hook_func)(struct fbe_packet_s * packet, void* hook_elem, void *context);


typedef struct fbe_persist_hook_s
{
    fbe_persist_hook_type_t hook_type;
    persist_hook_func hook_function;  /*hook function for this hook*/
    fbe_u32_t esp_triggered;    /*whether this hook has been triggered in esp*/
    fbe_u32_t sep_triggered;    /*whether this hook has been triggered in sep*/
    fbe_u32_t esp_hook;         /*hooks for esp package*/
    fbe_u32_t sep_hook;         /*hooks for sep package*/
}fbe_persist_hook_t;

void persist_hook_init(void);
fbe_status_t  persist_set_hook(fbe_persist_hook_type_t hook_type, fbe_bool_t is_set_esp,
                               fbe_bool_t is_set_sep);
fbe_status_t  persist_remove_hook(fbe_persist_hook_type_t hook_type, fbe_bool_t is_set_esp,
                                  fbe_bool_t is_set_sep);
fbe_status_t  persist_get_hook_state(fbe_persist_hook_type_t hook_type,
                                     fbe_bool_t *is_triggered, fbe_bool_t *is_set_esp,
                                     fbe_bool_t *is_set_sep);
fbe_status_t persist_check_and_run_hook(fbe_packet_t* packet, fbe_persist_hook_type_t hook_type, void* context,
                                        fbe_bool_t *esp_hook_is_set, fbe_bool_t *sep_hook_is_set);


#endif /* FBE_PERSIST_DEBUG_H */
