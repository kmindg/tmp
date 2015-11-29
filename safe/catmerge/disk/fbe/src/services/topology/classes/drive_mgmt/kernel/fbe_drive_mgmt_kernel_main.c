#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_esp.h"
#include "fbe_topology.h"
#include "ntmirror_user_interface.h"

/* TEMP:*/
#include "fbe_panic_sp.h"
#include "spid_kernel_interface.h"
/* local */
#include "fbe_drive_mgmt_private.h"


static void fbe_drive_mgmt_dieh_expand_envs(fbe_u8_t * filepath);


/* USED FOR TESTING, NOT IMPLEMENTED FOR KERNEL CODE
 */

void fbe_drive_mgmt_inject_clar_id_for_simulated_drive_only(fbe_drive_mgmt_t *drive_mgmt, fbe_physical_drive_information_t *physical_drive_info)
{
}

/*!**************************************************************
 * @fn fbe_drive_mgmt_dieh_get_path
 ****************************************************************
 * @brief
 *  Kernel space function for getting the DIEH directory path.
 *  Path will contain a trailing separator '\'.
 * 
 * @param dir - string to copy path to.
 * @param buff_size - max size of dir string
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
    fbe_char_t default_dir[64];

    EmcutilFilePathMake(default_dir, sizeof(default_dir), EMCUTIL_BASE_CDRIVE, NULL);

    status = fbe_registry_read(NULL,
                               fbe_get_registry_path(),
                               FBE_DIEH_PATH_REG_KEY, 
                               dir,
                               buff_size,
                               FBE_REGISTRY_VALUE_SZ,
                               default_dir,   // default 
                               0,                                     // 0 = compute default length if string
                               FALSE);

    if (FBE_STATUS_OK != status)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_registry_read %s failed, status=%d\n", __FUNCTION__, FBE_DIEH_PATH_REG_KEY, status);
        return;
    }
    else
    {
        fbe_u8_t *searchStr = {"%BASE_REVISION%"};
        fbe_u8_t replaceStr[FBE_REG_BASE_REVISION_KEY_MAX_LEN];
        fbe_u8_t *foundStr = strstr(dir, searchStr);
        
        if (NULL != foundStr)
        {
            status = fbe_registry_read(NULL,
                                       K10_REG_ENV_PATH,
                                       FBE_REG_BASE_REVISION_KEY, 
                                       replaceStr,
                                       sizeof(replaceStr),
                                       FBE_REGISTRY_VALUE_SZ,
                                       "NULL",
                                       0,
                                       FALSE);
    
            if (FBE_STATUS_OK != status)
            {
                fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                         "%s fbe_registry_read %s failed, status=%d\n", __FUNCTION__, FBE_REG_BASE_REVISION_KEY, status);
            }
            else
            {
                size_t original_len = strlen(dir);
                size_t search_len = strlen(searchStr);
                size_t replace_len = strlen(replaceStr);
                memmove(foundStr + replace_len, foundStr + search_len, original_len - (foundStr - dir) -search_len + 1);
                memmove(foundStr, replaceStr, replace_len);
                dir[original_len - search_len + replace_len] = '\0';
            }
        }
    }


    if (strlen(dir) >= buff_size)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s buffer overrun, len=%d, size=%d\n", __FUNCTION__, (int)strlen(dir), buff_size);
    }
}


fbe_status_t fbe_drive_mgmt_get_esp_lun_location(fbe_char_t *location)
{
    fbe_char_t filename[64];

    EmcutilFilePathMake(filename, sizeof(filename), EMCUTIL_BASE_CDRIVE,
                            "dmo_lun.txt", NULL);
    strncpy(location, filename, sizeof(filename));
    return FBE_STATUS_OK;
}

fbe_status_t fbe_drive_mgmt_panic_sp(fbe_bool_t force_degraded_mode)
{
    if (force_degraded_mode){
        spidSetForceDegradedMode();
    }
    FBE_PANIC_SP((FBE_PANIC_SP_BASE + 2), FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
