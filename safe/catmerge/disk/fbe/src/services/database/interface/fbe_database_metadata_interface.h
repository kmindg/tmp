#ifndef FBE_DATABASE_METADATA_INTERFACE_H
#define FBE_DATABASE_METADATA_INTERFACE_H

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_database.h"

fbe_status_t fbe_database_metadata_nonpaged_system_clear(void);
fbe_status_t fbe_database_metadata_nonpaged_system_load(void);
fbe_status_t fbe_database_metadata_nonpaged_load(void);
fbe_status_t fbe_database_metadata_nonpaged_system_zero_and_persist(fbe_object_id_t object_id, fbe_block_count_t block_count);

fbe_status_t fbe_database_metadata_nonpaged_system_load_with_drivemask(fbe_u32_t valid_drive_mask);

fbe_status_t fbe_database_metadata_set_ndu_in_progress(fbe_bool_t is_set);

fbe_status_t fbe_database_metadata_nonpaged_system_read_persist_with_drivemask(fbe_u32_t valid_drive_mask, fbe_object_id_t object_id, fbe_u32_t object_count);

fbe_status_t fbe_database_metadata_nonpaged_get_data_ptr(fbe_database_get_nonpaged_metadata_data_ptr_t *get_nonpaged_metadata);


#endif /*  FBE_DATABASE_METADATA_INTERFACE_H */


