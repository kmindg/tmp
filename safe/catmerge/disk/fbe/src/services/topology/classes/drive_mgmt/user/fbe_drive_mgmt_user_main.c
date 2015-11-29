#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "base_object_private.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_esp.h"
#include "fbe_drive_mgmt_private.h"



extern fbe_drive_mgmt_drive_capacity_lookup_t fbe_drive_mgmt_known_drive_capacity[];
extern fbe_u32_t                              fbe_drive_mgmt_known_drive_capacity_table_entries;


/*** File Scoped Globals ***/


/*** Local Prototypes ***/


/*** Function Definitions ***/


/* The following functions implemented for testing purposes in the user space 
 */

void fbe_drive_mgmt_inject_clar_id_for_simulated_drive_only(fbe_drive_mgmt_t *drive_mgmt, fbe_physical_drive_information_t *physical_drive_info)
{
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "drive_mgmt_inject_clar_id_4_sim_drive entry.  Checking capacity %llX\n",
                          (unsigned long long)physical_drive_info->gross_capacity);  
      
    /* Test for simulated drive
    */
    if ( fbe_equal_memory(physical_drive_info->product_id+FBE_DRIVE_MGMT_CLAR_ID_OFFSET, 
                          " CLAR1\0\0",         // all simulated drives have this hardcode ID.
                          FBE_DRIVE_MGMT_CLAR_ID_LENGTH))
    {
        /* Inject known clar ID if capacity is within a range of an expected capacity.  This will allow a
        *  known clar id to be inject for a valid capacity as well as an invalid one, so we can test the
        *  fail_drive case.
        */
        int i;
        for (i=0; i < fbe_drive_mgmt_known_drive_capacity_table_entries; i++)
        {
            if ( physical_drive_info->gross_capacity >= fbe_drive_mgmt_known_drive_capacity[i].gross_capacity &&
                 physical_drive_info->gross_capacity <= fbe_drive_mgmt_known_drive_capacity[i].gross_capacity+1 )
            {
                /* found one. inject string */
                strncpy(physical_drive_info->product_id+FBE_DRIVE_MGMT_CLAR_ID_OFFSET,
                        fbe_drive_mgmt_known_drive_capacity[i].clar_id,
                        FBE_DRIVE_MGMT_CLAR_ID_LENGTH);

                fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "drive_mgmt_inject_clar_id_4_sim_drive Modify product_id.clar_id:%s capacity:%llX\n",
                                      fbe_drive_mgmt_known_drive_capacity[i].clar_id, 
                                      (unsigned long long)physical_drive_info->gross_capacity);
                break;
            }
        }
    }

}

/*!**************************************************************
 * @fn fbe_drive_mgmt_dieh_get_path
 ****************************************************************
 * @brief
 *  Userspace function for getting the DIEH directory path.
 *  Path will contain a trailing separator '\'.
 * 
 * @param dir - string to copy path to.
 * @param max_size - max_size of dir string
 * 
 * @return none
 * 
 * @pre - dir must be zeroed
 *
 * @version
 *   03/15/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
void fbe_drive_mgmt_dieh_get_path(fbe_u8_t * dir, fbe_u32_t buff_size)
{
    fbe_status_t status;
    csx_char_t default_filepath[FBE_DIEH_PATH_LEN] = {0};

    sprintf(default_filepath, "%s", ".");

    EmcutilFilePathExtend(default_filepath, sizeof(default_filepath), FBE_DIEH_DEFAULT_FILENAME, NULL);

    status = fbe_registry_read(NULL,
                               fbe_get_registry_path(),
                               FBE_DIEH_PATH_REG_KEY, 
                               dir,
                               buff_size,
                               FBE_REGISTRY_VALUE_SZ,
                               default_filepath,               // default  
                               0,                        // 0 = compute default length if string
                               FALSE);

    if(status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Using default: %s\n", __FUNCTION__, default_filepath);        
        /* 
         * In the fbe_registry_sim.c, the defaultImagePath is not filled in
         * in case of the failure of reading the registry.
         * So, we need to set default
         */
        strncpy(dir, default_filepath, buff_size-1);
    }

    if (strlen(dir) >= buff_size)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s buffer overrun, len=%d, size=%d\n", __FUNCTION__, (int)strlen(dir), buff_size);
    }

}


fbe_status_t fbe_drive_mgmt_panic_sp(fbe_bool_t force_degraded_mode)
{
    return FBE_STATUS_OK;
}
