#ifndef FBE_CLASSES_DEBUG_H
#define FBE_CLASSES_DEBUG_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"

fbe_status_t
fbe_class_debug_get_lifecycle_const_data(char * p_module_name,
                                         fbe_class_id_t class_id,
                                         fbe_dbgext_ptr * pp_class_const);

fbe_status_t
fbe_class_debug_get_lifecycle_inst_cond(char * p_module_name,
                                        fbe_dbgext_ptr p_object,
                                        fbe_class_id_t class_id,
                                        fbe_dbgext_ptr * pp_inst_cond);

#endif /* FBE_CLASSES_DEBUG_H */
