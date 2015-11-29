#ifndef FBE_TERMINATOR_MINIPORT_SAS_DEVICE_TABLE_H
#define FBE_TERMINATOR_MINIPORT_SAS_DEVICE_TABLE_H
#include "fbe/fbe_types.h"
//#include "fbe_terminator_miniport_interface.h"
#include "terminator_miniport_api_private.h"
#define INDEX_BIT_MASK  0xFFFF

fbe_status_t terminator_miniport_sas_device_table_init(fbe_u32_t record_count, terminator_miniport_sas_device_table_t *table);
fbe_status_t terminator_miniport_sas_device_table_destroy(terminator_miniport_sas_device_table_t *table);

fbe_status_t terminator_miniport_sas_device_table_add_record(const terminator_miniport_sas_device_table_t *table, 
                                                             const fbe_terminator_device_ptr_t device_handle,
                                                             fbe_u32_t *index);
fbe_status_t terminator_miniport_sas_device_table_remove_record(
    const terminator_miniport_sas_device_table_t *table, 
    const fbe_u32_t index);
fbe_status_t terminator_miniport_sas_device_table_reserve_record(
    const terminator_miniport_sas_device_table_t *table, 
    const fbe_u32_t index);
fbe_status_t terminator_miniport_sas_device_table_get_device_id(
    const terminator_miniport_sas_device_table_t *table, 
    const fbe_u32_t index, 
    fbe_miniport_device_id_t *cpd_device_id);
fbe_status_t terminator_miniport_sas_device_table_get_device_handle(
    const terminator_miniport_sas_device_table_t *table,
    const fbe_u32_t index, 
    fbe_terminator_device_ptr_t *device_handle);


#endif /*FBE_TERMINATOR_MINIPORT_SAS_DEVICE_TABLE_H*/

