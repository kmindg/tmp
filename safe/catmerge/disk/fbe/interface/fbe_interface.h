#ifndef FBE_INTERFACE_H
#define FBE_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe_lifecycle.h"

fbe_status_t fbe_get_class_lifecycle_const_data(fbe_class_id_t class_id, fbe_lifecycle_const_t ** pp_class_const);
fbe_status_t fbe_get_class_name(fbe_class_id_t class_id, const char ** pp_class_name);

#endif /* FBE_INTERFACE_H */
