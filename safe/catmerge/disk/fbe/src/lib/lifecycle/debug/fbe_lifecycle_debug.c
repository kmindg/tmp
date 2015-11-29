#include "../fbe_lifecycle_private.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_interface.h"
#include "fbe_classes_debug.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_topology_debug.h"

typedef union {
    fbe_u32_t u32;
    fbe_u64_t u64;
} debug_ptr_t;

typedef struct {
    fbe_class_id_t class_id;
    fbe_u32_t rotary_max;
    fbe_u32_t base_cond_max;
    fbe_dbgext_ptr p_class_const;
    fbe_dbgext_ptr p_rotary;
    fbe_dbgext_ptr p_base_cond;
} debug_class_const_t;

#define DEBUG_CLASS_CONST_MAX 10

typedef struct {
    fbe_u32_t class_const_cnt;
    fbe_u32_t rotary_entry_size;
    fbe_u32_t rotary_cond_entry_size;
    fbe_u32_t base_cond_entry_size;
    debug_class_const_t class_const[DEBUG_CLASS_CONST_MAX];
} debug_class_const_array_t;

typedef struct fbe_debug_offsets_s {
    fbe_u32_t object_status_offset;                     /* topology_object_table_entry_t */
    fbe_u32_t control_handle_offset;                    /* topology_object_table_entry_t */
    fbe_u32_t object_id_offset;                         /* fbe_base_object_t */
    fbe_u32_t class_id_offset;                          /* fbe_base_object_t */
    fbe_u32_t lifecycle_offset;                         /* fbe_base_object_t */
    fbe_u32_t canary_offset;                            /* fbe_lifecycle_inst_base_t */
    fbe_u32_t state_offset;                             /* fbe_lifecycle_inst_base_t */
    fbe_u32_t const_t_canary_offset;                    /* fbe_lifecycle_const_t */
    fbe_u32_t p_super_offset;                           /* fbe_lifecycle_const_t */
    fbe_u32_t const_t_class_id_offset;                  /* fbe_lifecycle_const_t */
    fbe_u32_t p_rotary_array_offset;                    /* fbe_lifecycle_const_t */
    fbe_u32_t p_base_cond_array_offset;                 /* fbe_lifecycle_const_t */
    fbe_u32_t const_rotary_array_t_canary_offset;       /* fbe_lifecycle_const_rotary_array_t */
    fbe_u32_t rotary_max_offset;                        /* fbe_lifecycle_const_rotary_array_t */
    fbe_u32_t p_rotary_offset;                          /* fbe_lifecycle_const_rotary_array_t */
    fbe_u32_t const_base_cond_array_t_canary_offset;    /* fbe_lifecycle_const_base_cond_array_t */
    fbe_u32_t base_cond_max_offset;                     /* fbe_lifecycle_const_base_cond_array_t */
    fbe_u32_t pp_base_cond_offset;                      /* fbe_lifecycle_const_base_cond_array_t */
    fbe_u32_t this_state_offset;                        /* fbe_lifecycle_const_rotary_t */
    fbe_u32_t rotary_cond_max_offset;                   /* fbe_lifecycle_const_rotary_t */
    fbe_u32_t p_rotary_cond_offset;                     /* fbe_lifecycle_const_rotary_t */
    fbe_u32_t attr_offset;                              /* fbe_lifecycle_const_rotary_cond_t */
    fbe_u32_t p_const_cond_offset;                      /* fbe_lifecycle_const_rotary_cond_t */
    fbe_u32_t const_cond_t_canary_offset;               /* fbe_lifecycle_const_cond_t */
    fbe_u32_t cond_type_offset;                         /* fbe_lifecycle_const_cond_t */
    fbe_u32_t cond_id_offset;                           /* fbe_lifecycle_const_cond_t */
    fbe_u32_t cond_attr_offset;                         /* fbe_lifecycle_const_cond_t */
    fbe_u32_t const_base_cond_t_canary_offset;          /* fbe_lifecycle_const_base_cond_t */
    fbe_u32_t const_cond_offset;                        /* fbe_lifecycle_const_base_cond_t */
    fbe_u32_t p_cond_name_offset;                       /* fbe_lifecycle_const_base_cond_t */
} fbe_debug_offsets_t;

fbe_debug_offsets_t fbe_debug_offsets;

extern char * pp_ext_module_name;

static void fbe_debug_offsets_set(const fbe_u8_t * module_name);

static fbe_u32_t
lifecycle_debug_get_pointer_size(fbe_lifecycle_trace_context_t * p_trace_context)
{
    fbe_u32_t ptr_size;

    FBE_GET_TYPE_SIZE(p_trace_context->p_module_name, ULONG_PTR, &ptr_size)

    if (ptr_size == 0) {
        p_trace_context->trace_func("\tERROR! zero pointer size\n");
        return 0;
    }
    if (ptr_size > sizeof(fbe_dbgext_ptr)) {
        p_trace_context->trace_func("\tERROR! pointer overflow, got: %d, expected: %llu\n",
                                    ptr_size,(unsigned long long) sizeof(fbe_dbgext_ptr));
        return 0;
    }
    return ptr_size;
#   undef EXPR_SIZE
}


fbe_status_t
fbe_lifecycle_cond_type_str(fbe_lifecycle_func_cond_type_t cond_type,
                            const char ** pp_str)
{
    switch(cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE:
            *pp_str = "BASE";
            break;
        case FBE_LIFECYCLE_COND_TYPE_DERIVED:
            *pp_str = "DERIVED";
            break;
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
            *pp_str = "TIMER";
            break;
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
            *pp_str = "DERIVED_TIMER";
            break;
        default:
            *pp_str = "<error>";
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_lifecycle_cond_attr_str(fbe_lifecycle_const_cond_attr_t cond_attr,
                            char * p_str_buf,
                            fbe_u32_t str_buf_size)
{
    if ((p_str_buf == NULL) || (str_buf_size < 5)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (cond_attr == FBE_LIFECYCLE_CONST_COND_ATTR_NULL) {
        strncpy(p_str_buf, "NULL", str_buf_size);
        return FBE_STATUS_OK;
    }
    if (((cond_attr & FBE_LIFECYCLE_CONST_COND_ATTR_NOSET) != 0) && (str_buf_size > 6)) {
        strncat(p_str_buf, "+NOSET", str_buf_size);
        p_str_buf += 6;
        str_buf_size -= 6;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_lifecycle_rotary_cond_attr_str(fbe_lifecycle_rotary_cond_attr_t rotary_attr,
                                   char * p_str_buf,
                                   fbe_u32_t str_buf_size)
{
    if ((p_str_buf == NULL) || (str_buf_size < 5)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (rotary_attr == FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL) {
        strncpy(p_str_buf, "NULL", str_buf_size);
        return FBE_STATUS_OK;
    }
    if (((rotary_attr & FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET) != 0) && (str_buf_size > 7)) {
        strncat(p_str_buf, "+PRESET", str_buf_size);
        p_str_buf += 7;
        str_buf_size -= 7;
    }
    if (((rotary_attr & FBE_LIFECYCLE_ROTARY_COND_ATTR_REDO_PRESETS) != 0) && (str_buf_size > 12)) {
        strncat(p_str_buf, "+REDO_PRESET", str_buf_size);
        p_str_buf += 12;
        str_buf_size -= 12;
    }
    if (((rotary_attr & FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL_FUNC_OK) != 0) && (str_buf_size > 13)) {
        strncat(p_str_buf, "+NULL_FUNC_OK", str_buf_size);
        p_str_buf += 13;
        str_buf_size -= 13;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_lifecycle_debug_get_state_name(fbe_lifecycle_state_t state, const char ** pp_state_name)
{
    *pp_state_name = NULL;
    switch (state) {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            *pp_state_name = "SPECIALIZE";
            break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            *pp_state_name = "ACTIVATE";
            break;
        case FBE_LIFECYCLE_STATE_READY:
            *pp_state_name = "READY";
            break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            *pp_state_name = "HIBERNATE";
            break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            *pp_state_name = "OFFLINE";
            break;
        case FBE_LIFECYCLE_STATE_FAIL:
            *pp_state_name = "FAIL";
            break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            *pp_state_name = "DESTROY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            *pp_state_name = "PENDING_READY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            *pp_state_name = "PENDING_ACTIVATE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            *pp_state_name = "PENDING_HIBERNATE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            *pp_state_name = "PENDING_OFFLINE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            *pp_state_name = "PENDING_FAIL";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            *pp_state_name = "PENDING_DESTROY";
            break;
        case FBE_LIFECYCLE_STATE_NOT_EXIST:
            *pp_state_name = "STATE_NOT_EXIST";
            break;
        case FBE_LIFECYCLE_STATE_INVALID:
            *pp_state_name = "INVALID";
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t
lifecycle_debug_get_class_const_hierarchy(fbe_lifecycle_trace_context_t * p_trace_context,
                                          fbe_dbgext_ptr p_leaf_class_const,
                                          debug_class_const_array_t * class_const_array)
{
    fbe_dbgext_ptr p_this_const;
    fbe_dbgext_ptr p_super_const;
    fbe_dbgext_ptr class_const_ptr_array[DEBUG_CLASS_CONST_MAX];
    fbe_lifecycle_canary_t lifecycle_canary;
    debug_class_const_t * p_class_const;
    fbe_u32_t ptr_size;
    debug_ptr_t ptr;
    fbe_u32_t class_const_cnt;
    fbe_u32_t ii, jj;
    fbe_dbgext_ptr addr;

    /* Determine the pointer size. */
    ptr_size = lifecycle_debug_get_pointer_size(p_trace_context);
    if (ptr_size == 0) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if p_leaf_class_const is null, second loop below will overwrite memory */
    if (p_leaf_class_const == NULL) {
    	return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Walk up the hierarchy. */
    p_this_const = p_leaf_class_const;
    for (ii = 0; ii < DEBUG_CLASS_CONST_MAX; ii++) {
        if (p_this_const == NULL) {
            break;
        }
        class_const_ptr_array[ii] = p_this_const;
        lifecycle_canary = 0;

        addr = p_this_const + fbe_debug_offsets.const_t_canary_offset;
        FBE_READ_MEMORY(addr, &lifecycle_canary, sizeof(fbe_lifecycle_canary_t));

        if (lifecycle_canary != FBE_LIFECYCLE_CANARY_CONST) {
            p_trace_context->trace_func("\tERROR! Invalid canary in lifecycle const data, found: 0x%X, expected: 0x%X\n",
                                        lifecycle_canary, FBE_LIFECYCLE_CANARY_CONST);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        ptr.u64 = 0;

        addr = p_this_const + fbe_debug_offsets.p_super_offset;
        FBE_READ_MEMORY(addr, &ptr, sizeof(ptr));
        p_super_const = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);
        p_this_const = p_super_const;
    }
    class_const_cnt = ii;

    /* Re-order the array from the base class to the derrived class. */
    ii = 0;
    jj = class_const_cnt - 1;
    while (jj > ii) {
        p_this_const = class_const_ptr_array[jj];
        class_const_ptr_array[jj] = class_const_ptr_array[ii];
        class_const_ptr_array[ii] = p_this_const;
        ii++;
        jj--;
    }

    /* Populate the the hierarchy array from the class const data. */
    for (ii = 0; ii < class_const_cnt; ii++) {
        p_class_const = &class_const_array->class_const[ii];
        p_class_const->p_class_const = class_const_ptr_array[ii];

        addr = (ULONG64)class_const_ptr_array[ii] + fbe_debug_offsets.const_t_class_id_offset;
        FBE_READ_MEMORY(addr, &p_class_const->class_id, sizeof(fbe_class_id_t));

        addr = (ULONG64)class_const_ptr_array[ii] + fbe_debug_offsets.p_rotary_array_offset;
        FBE_READ_MEMORY(addr, &ptr, sizeof(ptr));

        p_class_const->p_rotary = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);

        addr = (ULONG64)class_const_ptr_array[ii] + fbe_debug_offsets.p_base_cond_array_offset;
        FBE_READ_MEMORY(addr, &ptr, sizeof(ptr));

        p_class_const->p_base_cond = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);
    }

    class_const_array->class_const_cnt = class_const_cnt;

    FBE_GET_TYPE_SIZE(p_trace_context->p_module_name, fbe_lifecycle_const_rotary_t, &class_const_array->rotary_entry_size);
    FBE_GET_TYPE_SIZE(p_trace_context->p_module_name, fbe_lifecycle_const_rotary_cond_t, &class_const_array->rotary_cond_entry_size);
    FBE_GET_TYPE_SIZE(p_trace_context->p_module_name, fbe_lifecycle_const_base_cond_t, &class_const_array->base_cond_entry_size);


    /* Populate the const rotary array data. */
    for (ii = 0; ii < class_const_cnt; ii++) {
        p_class_const = &class_const_array->class_const[ii];
        lifecycle_canary = 0;

        addr = p_class_const->p_rotary + fbe_debug_offsets.const_rotary_array_t_canary_offset;
        FBE_READ_MEMORY(addr, &lifecycle_canary, sizeof(fbe_lifecycle_canary_t));

        if (lifecycle_canary != FBE_LIFECYCLE_CANARY_CONST_ROTARY_ARRAY ) {
            p_trace_context->trace_func("\tERROR! Invalid canary in const rotary array, class id: 0x%x, found: 0x%X, expected: 0x%X\n",
                                        p_class_const->class_id, lifecycle_canary, FBE_LIFECYCLE_CANARY_CONST_ROTARY_ARRAY);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        addr = p_class_const->p_rotary + fbe_debug_offsets.rotary_max_offset;
        FBE_READ_MEMORY(addr, &p_class_const->rotary_max, sizeof(fbe_u32_t));

        addr = p_class_const->p_rotary + fbe_debug_offsets.p_rotary_offset;
        FBE_READ_MEMORY(addr, &ptr, sizeof(ptr));

        p_class_const->p_rotary = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);
    }

    /* Populate the const base condition array data. */
    for (ii = 0; ii < class_const_cnt; ii++) {
        p_class_const = &class_const_array->class_const[ii];
        lifecycle_canary = 0;

        addr = p_class_const->p_base_cond + fbe_debug_offsets.const_base_cond_array_t_canary_offset;
        FBE_READ_MEMORY(addr, &lifecycle_canary, sizeof(fbe_lifecycle_canary_t));

        if (lifecycle_canary != FBE_LIFECYCLE_CANARY_CONST_BASE_COND_ARRAY) {
            p_trace_context->trace_func("\tERROR! Invalid canary in base cond array, class id: 0x%x, found: 0x%X, expected: 0x%X\n",
                                        p_class_const->class_id, lifecycle_canary, FBE_LIFECYCLE_CANARY_CONST_BASE_COND_ARRAY);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        addr = p_class_const->p_base_cond + fbe_debug_offsets.base_cond_max_offset;
        FBE_READ_MEMORY(addr, &p_class_const->base_cond_max, sizeof(fbe_u32_t));

        addr = p_class_const->p_base_cond + fbe_debug_offsets.pp_base_cond_offset;
        FBE_READ_MEMORY(addr, &ptr, sizeof(ptr));


        p_class_const->p_base_cond = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);
    }

    return FBE_STATUS_OK;
}

static fbe_status_t
lifecycle_debug_trace_base_condition(fbe_lifecycle_trace_context_t * p_trace_context,
                                     debug_class_const_array_t * p_class_const_array,
                                     fbe_lifecycle_cond_id_t cond_id)
{
#   define COND_NAME_SIZE 127
    fbe_class_id_t class_id;
    fbe_dbgext_ptr pp_base_cond;
    fbe_u32_t base_cond_max;
    fbe_dbgext_ptr p_base_cond;
    fbe_dbgext_ptr p_base_cond_name;
    fbe_lifecycle_canary_t lifecycle_canary;
    fbe_lifecycle_cond_id_t this_cond_id;
    char cond_name[COND_NAME_SIZE+1];
    fbe_u32_t name_size;
    fbe_u32_t ptr_size;
    debug_ptr_t ptr;
    fbe_u32_t ii;
    fbe_dbgext_ptr addr;

    ptr_size = lifecycle_debug_get_pointer_size(p_trace_context);
    if (ptr_size == 0) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the class that has this base condition. */
    pp_base_cond = 0;
    class_id = fbe_lifecycle_get_cond_class_id(cond_id);
    for (ii = 0; ii < p_class_const_array->class_const_cnt; ii++) {
        if (class_id == p_class_const_array->class_const[ii].class_id) {
            pp_base_cond = p_class_const_array->class_const[ii].p_base_cond;
            base_cond_max = p_class_const_array->class_const[ii].base_cond_max;
            break;
        }
    }
    if (pp_base_cond == 0) {
        p_trace_context->trace_func("\t\tERROR! can't find class in the hierarchy, class id: 0x%X\n", class_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the the base condition const data and get a pointer to its cond name string. */
    p_base_cond_name = 0;
    for (ii = 0; ii < base_cond_max; ii++) {
        ptr.u64 = 0;

        FBE_READ_MEMORY(pp_base_cond, &ptr, ptr_size);

        p_base_cond = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);
        if ((p_base_cond == 0))  {
            p_trace_context->trace_func("\t\tERROR! can't base cond pointer, ptr_size: %d, ptr %llX",
                                        (int)ptr_size, p_base_cond);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (p_base_cond != 0) {
            lifecycle_canary = 0;

            addr = p_base_cond + fbe_debug_offsets.const_base_cond_t_canary_offset;
            FBE_READ_MEMORY(addr, &lifecycle_canary, sizeof(fbe_lifecycle_canary_t));

            if (lifecycle_canary != FBE_LIFECYCLE_CANARY_CONST_BASE_COND) {
                p_trace_context->trace_func("\tERROR! Invalid canary in base cond, found: 0x%X, expected: 0x%X\n",
                                            lifecycle_canary, FBE_LIFECYCLE_CANARY_CONST_BASE_COND);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            this_cond_id = 0;

            addr = p_base_cond + fbe_debug_offsets.const_cond_offset + fbe_debug_offsets.cond_id_offset;
            FBE_READ_MEMORY(addr, &this_cond_id, sizeof(fbe_lifecycle_cond_id_t));

            if (this_cond_id == cond_id) {
                ptr.u64 = 0;

                addr = p_base_cond + fbe_debug_offsets.p_cond_name_offset;
                FBE_READ_MEMORY(addr, &ptr, sizeof(ptr));
                p_base_cond_name = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);
                break;
            }
        }
        pp_base_cond += ptr_size;
    }
    if (p_base_cond_name == 0) {
        p_trace_context->trace_func("\tERROR! can't find base condition, cond id: 0x%X\n", cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the cond name string. */
    FBE_READ_MEMORY(p_base_cond_name, cond_name, COND_NAME_SIZE);
    name_size = COND_NAME_SIZE;
    cond_name[name_size] = 0;
    for (ii = 0; ii < name_size; ii++) {
        if ((cond_name[ii] < 0x20) || (cond_name[ii] > 0x7E)) {
            cond_name[ii] = 0;
            break;
        }
    }
    if (ii > 0) {
        p_trace_context->trace_func("\t\t\tcond name: \"%s\"\n", cond_name);
    }
    
    return FBE_STATUS_OK;
#   undef COND_NAME_SIZE
}

static fbe_status_t
lifecycle_debug_trace_inst_condition(fbe_lifecycle_trace_context_t * p_trace_context,
                                     fbe_dbgext_ptr p_object,
                                     fbe_lifecycle_cond_id_t cond_id,
                                     fbe_lifecycle_func_cond_type_t cond_type)
{
    fbe_class_id_t class_id;
    fbe_status_t status;
    fbe_u32_t cond_ii;
    fbe_dbgext_ptr p_inst_cond;
    fbe_lifecycle_inst_cond_t inst_cond;

    class_id = fbe_lifecycle_get_cond_class_id(cond_id);
    status = fbe_class_debug_get_lifecycle_inst_cond(p_trace_context->p_module_name, p_object, class_id, &p_inst_cond);
    if (status != FBE_STATUS_OK) {
        p_trace_context->trace_func("\tERROR! can't get condition instance pointer, cond id: 0x%X\n", cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    cond_ii = cond_id & FBE_LIFECYCLE_COND_MASK;
    p_inst_cond += cond_ii * sizeof(fbe_lifecycle_inst_cond_t);

    FBE_READ_MEMORY(p_inst_cond, &inst_cond, sizeof(fbe_lifecycle_inst_cond_t));

    switch (cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED:
            p_trace_context->trace_func("\t\t\tcond set count: %d, call set count: %d\n",
                                        inst_cond.u.simple.set_count, inst_cond.u.simple.call_set_count);
            break;
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
            p_trace_context->trace_func("\t\t\tcond timer interval: %d\n",
                                        inst_cond.u.timer.interval);
            break;
    }

    return FBE_STATUS_OK;
}

static void
lifecycle_debug_trace_condition(fbe_lifecycle_trace_context_t * p_trace_context,
                                debug_class_const_array_t * p_class_const_array,
                                fbe_dbgext_ptr p_object,
                                fbe_dbgext_ptr p_const_cond,
                                fbe_lifecycle_rotary_cond_attr_t rotary_attr)
{
#   define ATTR_SIZE 64
    fbe_lifecycle_func_cond_type_t cond_type;
    fbe_lifecycle_cond_id_t cond_id;
    fbe_lifecycle_const_cond_attr_t cond_attr;
    fbe_lifecycle_canary_t lifecycle_canary;
    const char * cond_type_str;
    char cond_attr_str[ATTR_SIZE] = {0};
    char rotary_attr_str[ATTR_SIZE] = {0};
    fbe_status_t status;
    fbe_dbgext_ptr addr;

    /* Verify the condition canary. */

    addr = p_const_cond + fbe_debug_offsets.const_cond_t_canary_offset;
    FBE_READ_MEMORY(addr, &lifecycle_canary, sizeof(fbe_lifecycle_canary_t));


    if (lifecycle_canary != FBE_LIFECYCLE_CANARY_CONST_COND) {
        p_trace_context->trace_func("\tERROR! Invalid canary in lifecycle const condition data, found: 0x%X, expected: 0x%X\n",
                                    lifecycle_canary, FBE_LIFECYCLE_CANARY_CONST_COND);
        return;
    }

    /* Dig out the condition fields. */
    cond_type = 0;
    addr = p_const_cond + fbe_debug_offsets.cond_type_offset;
    FBE_READ_MEMORY(addr, &cond_type, sizeof(fbe_lifecycle_func_cond_type_t));

    cond_id = 0;
    addr = p_const_cond + fbe_debug_offsets.cond_id_offset;
    FBE_READ_MEMORY(addr, &cond_id, sizeof(fbe_lifecycle_cond_id_t));

    cond_attr = 0;
    addr = p_const_cond + fbe_debug_offsets.cond_attr_offset;
    FBE_READ_MEMORY(addr, &cond_attr, sizeof(fbe_lifecycle_const_cond_attr_t));

    /* Get strings for the condition fields. */
    status = fbe_lifecycle_cond_type_str(cond_type, &cond_type_str);
    if (status != FBE_STATUS_OK) {
        cond_type_str = "<error>";
    }
    status = fbe_lifecycle_cond_attr_str(cond_attr, cond_attr_str, ATTR_SIZE);
    if (status != FBE_STATUS_OK) {
        strncpy(cond_attr_str, "<error>", ATTR_SIZE);
    }
    status = fbe_lifecycle_rotary_cond_attr_str(rotary_attr, rotary_attr_str, ATTR_SIZE);
    if (status != FBE_STATUS_OK) {
        strncpy(rotary_attr_str, "<error>", ATTR_SIZE);
    }

    p_trace_context->trace_func("\t\tcond id: 0x%08X, type: %d=%s, cond_attr: 0x%X=%s, rotary_attr: 0x%X=%s\n",
                                cond_id, cond_type, cond_type_str, cond_attr, cond_attr_str, rotary_attr, rotary_attr_str);

    status = lifecycle_debug_trace_base_condition(p_trace_context, p_class_const_array, cond_id);
    if (status != FBE_STATUS_OK) {
        return;
    }

    lifecycle_debug_trace_inst_condition(p_trace_context, p_object, cond_id, cond_type);
#   undef ATTR_SIZE
}

static void
lifecycle_debug_trace_rotary_conditions(fbe_lifecycle_trace_context_t * p_trace_context,
                                        debug_class_const_array_t * p_class_const_array,
                                        fbe_dbgext_ptr p_object,
                                        fbe_dbgext_ptr p_rotary)
{
    fbe_dbgext_ptr p_rotary_cond;
    fbe_dbgext_ptr p_const_cond;
    fbe_u32_t rotary_cond_max;
    fbe_u32_t ptr_size;
    debug_ptr_t ptr;
    fbe_lifecycle_rotary_cond_attr_t attr;
    fbe_u32_t ii;
    fbe_dbgext_ptr addr;

    /* Dig out the count of conditions. */
    rotary_cond_max = 0;

    addr = p_rotary + fbe_debug_offsets.rotary_cond_max_offset;
    FBE_READ_MEMORY(addr, &rotary_cond_max, sizeof(fbe_u32_t));

    if (rotary_cond_max == 0) {
        return;
    }

    /* Determine the pointer size. */
    ptr_size = lifecycle_debug_get_pointer_size(p_trace_context);
    if (ptr_size == 0) {
        return;
    }

    /* Dig out the rotary condition pointer. */
    ptr.u64 = 0;
    addr = p_rotary + fbe_debug_offsets.p_rotary_cond_offset;
    FBE_READ_MEMORY(addr, &ptr, sizeof(ptr));

    p_rotary_cond = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);
    if (p_rotary_cond == 0) {
        return;
    }

    p_trace_context->trace_func("\t\tfbe_lifecycle_const_rotary_t at: 0x%llX, cond_max: %d\n",
                                (unsigned long long)p_rotary, rotary_cond_max);

    /* Loop over the rotary conditions. */
    for (ii = 0; ii < rotary_cond_max; ii++) {
        /* Dig out the rotary condition attributes. */
        attr = 0;
        addr = p_rotary_cond + fbe_debug_offsets.attr_offset;
        FBE_READ_MEMORY(addr, &attr, sizeof(fbe_lifecycle_rotary_cond_attr_t));
        /* Dig out the const condition pointer. */
        ptr.u64 = 0;
        addr = p_rotary_cond + fbe_debug_offsets.p_const_cond_offset;
        FBE_READ_MEMORY(addr, &ptr, sizeof(ptr));
        p_const_cond = (fbe_dbgext_ptr)((ptr_size == 4) ? ptr.u32 : ptr.u64);
        if (p_const_cond != 0) {
            lifecycle_debug_trace_condition(p_trace_context, p_class_const_array, p_object, p_const_cond, attr);
        }
        p_rotary_cond += p_class_const_array->rotary_cond_entry_size;
    }
}

static void
lifecycle_debug_trace_class_rotary(fbe_lifecycle_trace_context_t * p_trace_context,
                                   debug_class_const_array_t * p_class_const_array,
                                   debug_class_const_t * p_class_const,
                                   fbe_dbgext_ptr p_object,
                                   fbe_lifecycle_state_t lifecycle_state)
{
    const char * p_class_id_str;
    fbe_dbgext_ptr p_rotary;
    fbe_lifecycle_state_t this_state;
    fbe_u32_t ii;
    fbe_status_t status;
    fbe_dbgext_ptr addr;

    /* Get the class id string. */
    status = fbe_get_class_name(p_class_const->class_id, &p_class_id_str);
    if (status != FBE_STATUS_OK) {
        p_class_id_str = "<error>";
    }

    p_trace_context->trace_func("\tclass id: 0x%X=%s, fbe_lifecycle_const_t at: 0x%llX\n",
                                p_class_const->class_id, p_class_id_str, (unsigned long long)p_class_const->p_class_const);

    /* Search for a rotary that matches the lifecycle state. */
    this_state = FBE_LIFECYCLE_STATE_INVALID;
    p_rotary = p_class_const->p_rotary;
    for (ii = 0; ii < p_class_const->rotary_max; ii++) {
        /* Dig out the lifecycle state for this rotary. */
        this_state = FBE_LIFECYCLE_STATE_INVALID;

        addr = p_rotary + fbe_debug_offsets.this_state_offset;
        FBE_READ_MEMORY(addr, &this_state, sizeof(fbe_lifecycle_state_t));

        if (this_state == lifecycle_state) {
            break;
        }
        p_rotary += p_class_const_array->rotary_entry_size;
    }

    /* Did we find a rotary? */
    if (this_state == lifecycle_state) {
        lifecycle_debug_trace_rotary_conditions(p_trace_context, p_class_const_array, p_object, p_rotary);
    }
    else {
        p_trace_context->trace_func("\t\t<no rotary>\n");
    }
}

fbe_dbgext_ptr lifecycle_get_object_ptr(fbe_object_id_t object_id,
                                        fbe_dbgext_ptr topology_object_tbl_ptr,
                                        unsigned long entry_size)
{
    fbe_dbgext_ptr p_table_entry = 0;
	fbe_topology_object_status_t  object_status;
    fbe_dbgext_ptr p_object = 0;
    fbe_dbgext_ptr addr = NULL;

    p_table_entry = topology_object_tbl_ptr + (entry_size * object_id);

    addr = p_table_entry + fbe_debug_offsets.object_status_offset;
    FBE_READ_MEMORY(addr, &object_status, sizeof(fbe_topology_object_status_t));

	if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
    {
        addr = p_table_entry + fbe_debug_offsets.control_handle_offset;
        FBE_READ_MEMORY(addr, &p_object, sizeof(fbe_dbgext_ptr));
	}
    return p_object;
}


void
fbe_lifecycle_debug_trace_rotary(fbe_lifecycle_trace_context_t * p_trace_context,
                                 fbe_object_id_t expected_object_id,
                                 fbe_dbgext_ptr p_object,
                                 fbe_lifecycle_state_t lifecycle_state)
{
    fbe_object_id_t object_id;
    fbe_class_id_t class_id;
    const char * p_class_id_str;
    fbe_lifecycle_canary_t lifecycle_canary;
    const char * p_lifecycle_state_str;
    fbe_dbgext_ptr p_class_const;
    debug_class_const_array_t class_const_array;
    fbe_u32_t ii;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dbgext_ptr addr;
	
	p_trace_context->trace_func("\tobject id: 0x%X, fbe_base_object_t at: 0x%llX\n",
								expected_object_id, p_object);

	/* Dig out the object id. */
	addr = p_object + fbe_debug_offsets.object_id_offset;
	FBE_READ_MEMORY(addr, &object_id, sizeof(fbe_object_id_t));

	if (expected_object_id != object_id) {
		p_trace_context->trace_func("\tERROR! object id mismatch, found: 0x%X, expected: 0x%X\n",
									object_id, expected_object_id);
		return;
	}

	/* Dig out the class id. */
	addr = p_object + fbe_debug_offsets.class_id_offset;
	FBE_READ_MEMORY(addr, &class_id, sizeof(fbe_class_id_t));

	status = fbe_get_class_name(class_id, &p_class_id_str);
	if (status != FBE_STATUS_OK) {
		p_class_id_str = "<error>";
	}

	/* Dig out the lifecycle base inst canary. */
	addr = p_object + fbe_debug_offsets.lifecycle_offset + fbe_debug_offsets.canary_offset;
	FBE_READ_MEMORY(addr, &lifecycle_canary, sizeof(fbe_lifecycle_canary_t));

	if (lifecycle_canary != FBE_LIFECYCLE_CANARY_INST_BASE) {
		p_trace_context->trace_func("\tERROR! Invalid canary in lifecycle base inst data, found: 0x%X, expected: 0x%X\n",
									lifecycle_canary, FBE_LIFECYCLE_CANARY_INST_BASE);
		return;
	}

	/* What lifecycle state are we interested in? */
	if (lifecycle_state >= FBE_LIFECYCLE_STATE_NON_PENDING_MAX) {
		/* Dig out the lifecycle state. */
		addr = p_object + fbe_debug_offsets.lifecycle_offset + fbe_debug_offsets.state_offset;
		FBE_READ_MEMORY(addr, &lifecycle_state, sizeof(fbe_lifecycle_state_t));
	}
	status = fbe_lifecycle_debug_get_state_name(lifecycle_state, &p_lifecycle_state_str);
	if (status != FBE_STATUS_OK) {
		p_lifecycle_state_str = "<error>";
	}

	p_trace_context->trace_func("\tlifecycle state: 0x%X=%s\n", lifecycle_state, p_lifecycle_state_str);

	/* Find the constant lifecycle data for this class. */
	status = fbe_class_debug_get_lifecycle_const_data(pp_ext_module_name, class_id, &p_class_const);
	if (status != FBE_STATUS_OK) {
		p_trace_context->trace_func("\tERROR! can't get lifecycle const data for class, id: 0x%X\n", class_id);
		return;
	}
	
	if (!p_class_const) return;
    
	/* Get the hiearchy of constant lifecycle data for this class. */
	status = lifecycle_debug_get_class_const_hierarchy(p_trace_context,
													   p_class_const,
													   &class_const_array);
	if (status != FBE_STATUS_OK) {
		p_trace_context->trace_func("\tERROR! can't get lifecycle const hierarchy for class, id: 0x%X\n", class_id);
		return;
	}

	/* Iterate over the class hierarchy. */
	for (ii = 0; ii < class_const_array.class_const_cnt; ii++) {
		lifecycle_debug_trace_class_rotary(p_trace_context,
										   &class_const_array,
										   &class_const_array.class_const[ii],
										   p_object,
										   lifecycle_state);
	}
}

fbe_status_t fbe_lifecycle_debug_process_rotary(fbe_lifecycle_trace_context_t * p_trace_context,
                                                fbe_object_id_t expected_object_id,
                                                fbe_lifecycle_state_t lifecycle_state)
{
    fbe_status_t status = FBE_STATUS_OK;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr p_object;
	fbe_u32_t object_id = 0;
    fbe_u32_t start_object_id = 0;
    fbe_u32_t ptr_size;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_dbgext_ptr p_object_table = 0;
    unsigned long entry_size = 0;

    fbe_debug_offsets_set(pp_ext_module_name);

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return status; 
    }

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &p_object_table);
	FBE_READ_MEMORY(p_object_table, &topology_object_tbl_ptr, ptr_size);
    if (topology_object_tbl_ptr == 0) {
        return status;
    }

    FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &entry_size);
    if (entry_size == 0) {
        return status;
    }

	/* If default display then display all the object's specified rotary */
	if(expected_object_id == FBE_OBJECT_ID_INVALID)
	{

        if(strncmp(pp_ext_module_name, "sep", 3) == 0) 
        {
            start_object_id = 1;
        }
        else
        {
            start_object_id = 0;
        }

		for(object_id = start_object_id; object_id < max_entries ; object_id++)
		{
			if (FBE_CHECK_CONTROL_C())
			{
				return status;
			}
			p_object = lifecycle_get_object_ptr(object_id, topology_object_tbl_ptr, entry_size);
			if(p_object != 0)
			{
				fbe_lifecycle_debug_trace_rotary(p_trace_context, object_id,p_object, lifecycle_state);
			}
		}
	}
	else
	{
        if (expected_object_id >= max_entries) {
            csx_dbg_ext_print("%s object_id exceeded max entries %d\n", __FUNCTION__, max_entries);
            return FBE_STATUS_INVALID;
        }
		p_object = lifecycle_get_object_ptr(expected_object_id, topology_object_tbl_ptr, entry_size);
		if(p_object != 0)
		{
			fbe_lifecycle_debug_trace_rotary(p_trace_context, expected_object_id,p_object, lifecycle_state);
		}
	}

	return status;
}

/*************************************************************
 * fbe_debug_struct_offsets_set
 *************************************************************
 * @brief
 *  Set the offsets for frequently used fields in a global for later use.
 *
 * @return void
 *
 * @author
 *  July 10th 2013 - Created. Q. Zhang
 *
 *************************************************************/
static void
fbe_debug_offsets_set(const fbe_u8_t * module_name)
{
    FBE_GET_FIELD_OFFSET(module_name, topology_object_table_entry_t, "object_status",
                         &fbe_debug_offsets.object_status_offset);
    FBE_GET_FIELD_OFFSET(module_name, topology_object_table_entry_t, "control_handle",
                         &fbe_debug_offsets.control_handle_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "object_id",
                         &fbe_debug_offsets.object_id_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "class_id",
                         &fbe_debug_offsets.class_id_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "lifecycle",
                         &fbe_debug_offsets.lifecycle_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_inst_base_t, "canary",
                         &fbe_debug_offsets.canary_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_inst_base_t, "state",
                         &fbe_debug_offsets.state_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_t, "canary",
                         &fbe_debug_offsets.const_t_canary_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_t, "p_super",
                         &fbe_debug_offsets.p_super_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_t, "class_id",
                         &fbe_debug_offsets.const_t_class_id_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_t, "p_rotary_array",
                         &fbe_debug_offsets.p_rotary_array_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_t, "p_base_cond_array",
                         &fbe_debug_offsets.p_base_cond_array_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_rotary_array_t, "canary",
                         &fbe_debug_offsets.const_rotary_array_t_canary_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_rotary_array_t, "rotary_max",
                         &fbe_debug_offsets.rotary_max_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_rotary_array_t, "p_rotary",
                         &fbe_debug_offsets.p_rotary_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_base_cond_array_t, "canary",
                         &fbe_debug_offsets.const_base_cond_array_t_canary_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_base_cond_array_t, "base_cond_max",
                         &fbe_debug_offsets.base_cond_max_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_base_cond_array_t, "pp_base_cond",
                         &fbe_debug_offsets.pp_base_cond_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_rotary_t, "this_state",
                         &fbe_debug_offsets.this_state_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_rotary_t, "rotary_cond_max",
                         &fbe_debug_offsets.rotary_cond_max_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_rotary_t, "p_rotary_cond",
                         &fbe_debug_offsets.p_rotary_cond_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_rotary_cond_t, "attr",
                         &fbe_debug_offsets.attr_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_rotary_cond_t, "p_const_cond",
                         &fbe_debug_offsets.p_const_cond_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_cond_t, "canary",
                         &fbe_debug_offsets.const_cond_t_canary_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_cond_t, "cond_type",
                         &fbe_debug_offsets.cond_type_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_cond_t, "cond_id",
                         &fbe_debug_offsets.cond_id_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_cond_t, "cond_attr",
                         &fbe_debug_offsets.cond_attr_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_base_cond_t, "canary",
                         &fbe_debug_offsets.const_base_cond_t_canary_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_base_cond_t, "const_cond",
                         &fbe_debug_offsets.const_cond_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_lifecycle_const_base_cond_t, "p_cond_name",
                         &fbe_debug_offsets.p_cond_name_offset);
}
