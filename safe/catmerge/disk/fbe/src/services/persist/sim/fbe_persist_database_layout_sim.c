#include "fbe_persist_database_layout.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_persist_private.h"

/*this is the simulation implementation of the persistence layout*/
/*each entry in the array holds a bitmap for 64 entries*/
#define ENTRY_ID_MAP_ENTRY_SIZE 64

/****************************************************************************************
*                                 H O W   TO  ADD  NEW  SECTORS                         *
*****************************************************************************************
* 1) update enum fbe_persist_db_layout_sim_t with the new sector and how			    *
*    many entries it  will have															*
* 2) update table sector_order_sim with the new sector and what is the max              *
*    entry enum for it                  												*
* 3) Number of entries must be devisible by 64 !!!!!!   								* 															
*****************************************************************************************/

typedef enum fbe_persist_db_layout_sim_e{
    FBE_PERSIST_MAX_SEP_OBJECTS_SIM = 640, /*If there is any change in the layout, 
										   you also need to change the sailor_moon_test which uses this layout.*/
	FBE_PERSIST_MAX_SEP_EDGES_SIM = 2560,
	FBE_PERSIST_MAX_SEP_ADMIN_CONVERSION_SIM = 320,
	FBE_PERSIST_MAX_ESP_OBJECTS_SIM  = 256,
	FBE_PERSIST_MAX_SYS_GLOBAL_DATA_SIM  = 256,
	FBE_PERSIST_MAX_SCRATCH_PAD_SIM = 256,
	FBE_PERSIST_MAX_DIEH_RECORD_SIM = 64

	/*ADD NEW SECTORS ABOVE THIS LINE*/

}fbe_persist_db_layout_sim_t;

/*******************/
/*local declerations*/
/*******************/
fbe_persist_sector_order_t	sector_order_sim [] = 
{
    {FBE_PERSIST_SECTOR_TYPE_INVALID, 0},
	{FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS, FBE_PERSIST_MAX_SEP_OBJECTS_SIM},
	{FBE_PERSIST_SECTOR_TYPE_SEP_EDGES, FBE_PERSIST_MAX_SEP_EDGES_SIM},
	{FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION, FBE_PERSIST_MAX_SEP_ADMIN_CONVERSION_SIM},
	{FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS, FBE_PERSIST_MAX_ESP_OBJECTS_SIM},
    {FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA, FBE_PERSIST_MAX_SYS_GLOBAL_DATA_SIM},
	{FBE_PERSIST_SECTOR_TYPE_SCRATCH_PAD, FBE_PERSIST_MAX_SCRATCH_PAD_SIM},
	{FBE_PERSIST_SECTOR_TYPE_DIEH_RECORD, FBE_PERSIST_MAX_DIEH_RECORD_SIM},

	/*ADD NEW SECTORS ABOVE THIS LINE*/

	{FBE_PERSIST_SECTOR_TYPE_LAST, 0},/*end of table*/

};

/*allocate the bit map in which each bit represents an entry. 0 means we can use it and 1 means it's used*/
fbe_status_t persist_init_entry_id_map(fbe_u64_t ** entry_id_map, fbe_u32_t *map_size)
{
	fbe_persist_sector_type_t	sector_type;
    
	*map_size = 0;

    for (sector_type = FBE_PERSIST_SECTOR_TYPE_INVALID + 1; sector_type < FBE_PERSIST_SECTOR_TYPE_LAST; sector_type++) {
        (*map_size) += ((sector_order_sim[sector_type].entries / ENTRY_ID_MAP_ENTRY_SIZE) * sizeof(fbe_u64_t));
	}

	*entry_id_map = fbe_memory_ex_allocate(*map_size);
    
	fbe_zero_memory(*entry_id_map ,*map_size);
    
	return FBE_STATUS_OK;
}

fbe_status_t fbe_persist_database_layout_get_sector_total_entries(fbe_persist_sector_type_t sector_type, fbe_u32_t *entry_count)
{
    fbe_persist_sector_order_t	*table = sector_order_sim;

	while (table->sector_type != FBE_PERSIST_SECTOR_TYPE_LAST) {
		if (sector_type == table->sector_type) {
			*entry_count = table->entries;
			return FBE_STATUS_OK;
		}

		table ++;
	}
	
	fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					  "%s illegal sector type:%d\n", __FUNCTION__, sector_type);

	return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t fbe_persist_database_layout_get_sector_offset(fbe_persist_sector_type_t sector_type, fbe_u32_t *offset)
{
    fbe_persist_sector_order_t	*table = sector_order_sim;
    fbe_u32_t					total_entries = 0;

    if(sector_type == FBE_PERSIST_SECTOR_TYPE_INVALID)
    {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Sector type is INVALID:%d\n", __FUNCTION__, sector_type);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    while (table->sector_type != FBE_PERSIST_SECTOR_TYPE_LAST) {
        if (sector_type == table->sector_type) {
            *offset = total_entries;
            return FBE_STATUS_OK;
        }

        total_entries += table->entries;
        table ++;
    }

    fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s illegal sector type:%d\n", __FUNCTION__, sector_type);

    return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t fbe_persist_database_layout_get_max_total_entries(fbe_u32_t *total)
{
    fbe_persist_sector_order_t	*table = sector_order_sim;
	fbe_u32_t					total_entries = 0;

	while (table->sector_type != FBE_PERSIST_SECTOR_TYPE_LAST) {
		total_entries+= table->entries;
		table ++;
	}
	
	*total = total_entries;

	return FBE_STATUS_OK;
}
