#define I_AM_NATIVE_CODE
#include <windows.h>
#include "base_object_private.h"
#include "fbe_module_mgmt_private.h"

fbe_status_t fbe_module_mgmt_get_esp_lun_location(fbe_char_t *location)
{
    fbe_u32_t pid = 0;
    pid = csx_p_get_process_id();
    fbe_sprintf(location, 256, "./sp_sim_pid%d/esp_lun_pid%d.txt", pid, pid);
    return FBE_STATUS_OK;
}

void fbe_module_mgmt_handle_file_write_completion(fbe_base_object_t * base_object, fbe_u32_t thread_delay)
{
    CSX_UNREFERENCED_PARAMETER(base_object);
    CSX_UNREFERENCED_PARAMETER(thread_delay);
    // no delay in simulation
    return;
}

fbe_u32_t fbe_module_mgmt_port_affinity_setting(void)
{
    // not supported in simulation
    return 0;
}

/*!*******************************************************************************
 * @fn fbe_module_mgmt_fup_build_image_file_name
 *********************************************************************************
 * @brief
 *  In sim mode, we build the LCC image file name and its length. 
 *  Please modify VAVOOM_TEST_ENCL_IMAGE_FILE_PATH_AND_NAME and 
 *  fbe_encl_mgmt_fup_build_image_file_name accordingly if this
 *  function gets changed.
 * 
 * @param  pModuleMgmt (INPUT)-
 * @param  pImageFileNameBuffer (OUTPUT)-
 * @param  pImageFileNameConstantPortion (INPUT)-
 * @param  bufferLen (INPUT)-
 * @param  pImageFileNameLen (OUTPUT)-
 *
 * @return fbe_status_t - 
 *
 * @version
 *  31-Oct-2012 Rui Chang -- Created.
 *******************************************************************************/
fbe_status_t fbe_module_mgmt_fup_build_image_file_name(fbe_module_mgmt_t * pModuleMgmt,
                                                       fbe_base_env_fup_work_item_t * pWorkItem,
                                                       fbe_u8_t * pImageFileNameBuffer,
                                                       char * pImageFileNameConstantPortion,
                                                       fbe_u8_t bufferLen,
                                                       fbe_u32_t * pImageFileNameLen)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_u32_t pid = 0;
    char pid_str[16] = "\0";

    FBE_UNREFERENCED_PARAMETER(pModuleMgmt);
    FBE_UNREFERENCED_PARAMETER(pWorkItem);

    pid = csx_p_get_process_id();
    fbe_sprintf(pid_str, sizeof(pid_str), "_pid%d", pid);

    fbe_zero_memory(pImageFileNameBuffer, bufferLen);

    if((strlen(pImageFileNameConstantPortion) + strlen(pid_str) + 4) < bufferLen)
    {
        // Get the constant portion of the filename
        strncat(pImageFileNameBuffer, pImageFileNameConstantPortion, strlen(pImageFileNameConstantPortion));

        // Append the PID
        strcat(pImageFileNameBuffer, pid_str);

        // Append the extension
        strncat(pImageFileNameBuffer, ".bin", 4);

        *pImageFileNameLen = (fbe_u32_t)strlen(pImageFileNameBuffer);

        status = FBE_STATUS_OK;
    }
    else
    {
        *pImageFileNameLen = 0;

        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }  
    
    return status;

} //End of function fbe_module_mgmt_fup_build_image_file_name

// End of file fbe_module_mgmt_sim_main.c
