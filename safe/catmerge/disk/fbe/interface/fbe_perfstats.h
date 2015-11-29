#ifndef fbe_perfstats_H
#define fbe_perfstats_H

#include "fbe/fbe_perfstats_interface.h"

/*this function is used by SEP and the Physical Package to 
  get the segment of the perfstats memory for a LUN or PDO object*/
fbe_status_t fbe_perfstats_get_system_memory_for_object_id(fbe_object_id_t object_id,
                                                           fbe_package_id_t package_id,
                                                           void **ptr);
fbe_status_t fbe_perfstats_delete_system_memory_for_object_id(fbe_object_id_t object_id,
                                                              fbe_package_id_t package_id); 
fbe_status_t fbe_perfstats_is_collection_enabled_for_package(fbe_package_id_t package_id,
                                                             fbe_bool_t *is_enabled);
fbe_status_t fbe_pefstats_populate_lun_number(fbe_object_id_t object_id,
                                              fbe_lun_number_t lun_number);

#endif /*FBE_PERFSTATS_H */

