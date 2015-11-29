#include "base_object_private.h"
#include "fbe_module_mgmt_private.h"

extern fbe_status_t fbe_module_mgmt_get_enable_port_interrupt_affinity(fbe_u32_t *port_interrupt_affinity_enabled);

fbe_status_t fbe_module_mgmt_get_esp_lun_location(fbe_char_t *location)
{
    fbe_char_t filename[64];

    EmcutilFilePathMake(filename, sizeof(filename), EMCUTIL_BASE_CDRIVE,
							"esp_lun.txt", NULL);
    strncpy(location, filename, sizeof(filename));
    return FBE_STATUS_OK;
}

void fbe_module_mgmt_handle_file_write_completion(fbe_base_object_t * base_object, fbe_u32_t thread_delay)
{

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Delaying %d seconds to allow for disk flushing of keys/logs.\n",
                          __FUNCTION__,
                          thread_delay/1000);

    fbe_thread_delay(thread_delay);

    return;
}


fbe_u32_t fbe_module_mgmt_port_affinity_setting(void)
{
    fbe_u32_t interrupt_affinity_setting;

    fbe_module_mgmt_get_enable_port_interrupt_affinity(&interrupt_affinity_setting);
    
    return interrupt_affinity_setting;
}

/*!*******************************************************************************
 * @fn fbe_module_mgmt_fup_build_image_file_name
 *********************************************************************************
 * @brief
 *  In kernel mode, for CDES-1, the image path in the registry already includes the LCC image file name.
 *  So the image path in the registry does NOT include the LCC image file name.
 *  So here we don't need any image file name.
 * 
 *  In kernel mode, for CDES-2, we need to do the download for multiple images for LCC.
 *  So the image path in the registry does NOT include the LCC image file name.    
 * 
 *  If this function need change, change
 *  fbe_module_mgmt_fup_build_image_file_name too. 
 * 
 * @param  pModuleMgmt (INPUT)-
 * @param  pWorkItem (INPUT) - 
 * @param  pImageFileNameBuffer (OUTPUT)-
 * @param  pImageFileNameConstantPortion (INPUT)-
 * @param  bufferLen (INPUT)-
 * @param  pImageFileNameLen (OUTPUT)-
 *
 * @return fbe_status_t - 
 *
 * @version
 *  31-Oct-2012 Rui Chang -- Created.
 *  12-Sep-20104 PHE - Modified to handle the CDES-2 LCC firmware upgrade.
 *******************************************************************************/
fbe_status_t fbe_module_mgmt_fup_build_image_file_name(fbe_module_mgmt_t * pModuleMgmt,
                                                       fbe_base_env_fup_work_item_t * pWorkItem,
                                                       fbe_u8_t * pImageFileNameBuffer,
                                                       char * pImageFileNameConstantPortion,
                                                       fbe_u8_t bufferLen,
                                                       fbe_u32_t * pImageFileNameLen)
{
    FBE_UNREFERENCED_PARAMETER(pModuleMgmt);

    fbe_zero_memory(pImageFileNameBuffer, bufferLen);

    if((pWorkItem->firmwareTarget == FBE_FW_TARGET_LCC_EXPANDER) || 
       (pWorkItem->firmwareTarget == FBE_FW_TARGET_LCC_INIT_STRING) ||
       (pWorkItem->firmwareTarget == FBE_FW_TARGET_LCC_FPGA))
    {
        /* In kernel mode, for CDES-2, we need to do the download for multiple images for LCC.
         * So the image path in the registry does NOT include the LCC image file name.
         */
         
        // Get the constant portion of the filename
        strncat(pImageFileNameBuffer, pImageFileNameConstantPortion, strlen(pImageFileNameConstantPortion));

        // Append the extension
        strncat(pImageFileNameBuffer, ".bin", 4);

        *pImageFileNameLen = (fbe_u32_t)strlen(pImageFileNameBuffer);
    }
    else
    {
        /* In kernel mode, for CDES-1, the image path in the registry already includes the LCC image file name. 
         * So here we don't need any image file name here. 
         */
        *pImageFileNameLen =  0; 
    }

    return FBE_STATUS_OK;
} //End of function fbe_module_mgmt_fup_build_image_file_name

//End of file fbe_module_mgmt_kernel_main.c
