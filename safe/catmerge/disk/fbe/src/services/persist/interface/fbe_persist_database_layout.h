#ifndef FBE_PERSIST_DATBASE_LAYOUT_H
#define FBE_PERSIST_DATBASE_LAYOUT_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_persist_interface.h"

fbe_status_t persist_init_entry_id_map(fbe_u64_t ** entry_id_map, fbe_u32_t *map_size);
fbe_status_t fbe_persist_database_layout_get_sector_total_entries(fbe_persist_sector_type_t sector_type, fbe_u32_t *entry_count);
fbe_status_t fbe_persist_database_layout_get_sector_offset(fbe_persist_sector_type_t sector_type, fbe_u32_t *offset);
fbe_status_t fbe_persist_database_layout_get_max_total_entries(fbe_u32_t *total);

#endif /*FBE_PERSIST_DATBASE_LAYOUT_H*/


