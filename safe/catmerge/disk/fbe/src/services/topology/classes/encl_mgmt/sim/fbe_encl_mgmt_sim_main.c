 /***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_sim_main.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for encl_mgmt simulation mode functionality.
 *
 * @version
 *   25-Oct-2010:  PHE - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#define I_AM_NATIVE_CODE
#include <windows.h>
#include "fbe/fbe_winddk.h"
#include "fbe_encl_mgmt_private.h"


/*!*******************************************************************************
 * @fn fbe_encl_mgmt_fup_build_image_file_name
 *********************************************************************************
 * @brief
 *  In sim mode, we build the LCC image file name and its length. 
 *  Please modify VAVOOM_TEST_ENCL_IMAGE_FILE_PATH_AND_NAME and 
  * fbe_module_mgmt_fup_build_image_file_name accordingly if this
 *  function gets changed.
 *
 * @param  pEnclMgmt (INPUT)-
 * @param  pWorkItem (INPUT) - 
 * @param  pImageFileNameBuffer (OUTPUT)-
 * @param  pImageFileNameConstantPortion (INPUT)-
 * @param  bufferLen (INPUT)-
 * @param  pImageFileNameLen (OUTPUT)-
 *
 * @return fbe_status_t - 
 *
 * @version
 *  15-Oct-2010 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_encl_mgmt_fup_build_image_file_name(fbe_encl_mgmt_t * pEnclMgmt,
                                                     fbe_base_env_fup_work_item_t * pWorkItem,
                                                     fbe_u8_t * pImageFileNameBuffer,
                                                     char * pImageFileNameConstantPortion,
                                                     fbe_u8_t bufferLen,
                                                     fbe_u32_t * pImageFileNameLen)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_u32_t pid = 0;
    char pid_str[16] = "\0";

    FBE_UNREFERENCED_PARAMETER(pEnclMgmt);
    FBE_UNREFERENCED_PARAMETER(pWorkItem);

    pid = csx_p_get_process_id();
    fbe_sprintf(pid_str, sizeof(pid_str), "_pid%d", pid);

    fbe_zero_memory(pImageFileNameBuffer, bufferLen);

    if((strlen(pImageFileNameConstantPortion) + strlen(pid_str) + 4) < bufferLen)
    {
        // Get the constant portion of the filename
        strncat(pImageFileNameBuffer, pImageFileNameConstantPortion, strlen(pImageFileNameConstantPortion));

        // Append the PID
        strncat(pImageFileNameBuffer, pid_str, FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN-strlen(pImageFileNameBuffer)-1);

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

} //End of function fbe_encl_mgmt_fup_build_image_file_name
#if 0
/*!*******************************************************************************
 * @fn fbe_encl_mgmt_fup_build_manifest_file_name
 *********************************************************************************
 * @brief
 *  This function adds pid and ".json" to the manifest file name. 
 * 
 *  If this function need change, change
 *  fbe_module_mgmt_fup_build_manifest_file_name too. 
 * 
 * @param  pEnclMgmt (INPUT)-
 * @param  pManifestFileNameBuffer (OUTPUT)-
 * @param  pManifestFileNameConstantPortion (INPUT)-
 * @param  bufferLen (INPUT)-
 * @param  pManifestFileNameLen (OUTPUT)-
 *
 * @return fbe_status_t - 
 *
 * @version
 *  12-Sep-20104 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_encl_mgmt_fup_build_manifest_file_name(fbe_encl_mgmt_t * pEnclMgmt,
                                                        fbe_u8_t * pManifestFileNameBuffer,
                                                        fbe_char_t * pManifestFileNameConstantPortion,
                                                        fbe_u8_t bufferLen,
                                                        fbe_u32_t * pManifestFileNameLen)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_u32_t pid = 0;
    char pid_str[16] = "\0";

    FBE_UNREFERENCED_PARAMETER(pEnclMgmt);

    pid = csx_p_get_process_id();
    fbe_sprintf(pid_str, sizeof(pid_str), "_pid%d", pid);

    fbe_zero_memory(pManifestFileNameBuffer, bufferLen);

    if((strlen(pManifestFileNameConstantPortion) + strlen(pid_str) + 5) < bufferLen)
    {
        // Get the constant portion of the filename
        strncat(pManifestFileNameBuffer, pManifestFileNameConstantPortion, strlen(pManifestFileNameConstantPortion));

        // Append the PID
        strncat(pManifestFileNameBuffer, pid_str, FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN-strlen(pManifestFileNameBuffer)-1);

        // Append the extension
        strncat(pManifestFileNameBuffer, ".json", 5);

        *pManifestFileNameLen = (fbe_u32_t)strlen(pManifestFileNameBuffer);

        status = FBE_STATUS_OK;
    }
    else
    {
        *pManifestFileNameLen = 0;

        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }  
    
    return status;

} //End of function fbe_encl_mgmt_fup_build_manifest_file_name
#endif
// End of file fbe_encl_mgmt_sim_main.c
