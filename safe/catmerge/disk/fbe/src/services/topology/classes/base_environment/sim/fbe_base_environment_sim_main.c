 /***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_environment_sim_main.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for base environment simulation mode functionality.
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
#include "fbe_base_environment_private.h"


/*!*******************************************************************************
 * @fn fbe_base_env_fup_build_image_file_name
 *********************************************************************************
 * @brief
 *  This function builds the image file name based on the
 *  constant portion of the image file name and the subenclosure product id. 
 *
 *  Example 1, if a power supply image has a filename of the format 
 *  SXP36x6g_Power_Supply_xxxxxxxx.bin where xxxxxxxx is the subenclosure product id,
 *  the Power Supply with a subenclosure product id of 000B0015, will 
 *  be listed as 000B0015 in the config page (subenclosure product id),
 *  and it will take firmware file SXP36x6g_Power_Supply_000B0015.bin
 *
 *  Example 2, if a power supply image has a filename of the format 
 *  CDES_1_1_Power_Supply_xxxxxxxx.bin where xxxxxxxx is the subenclosure product id,
 *  the Power Supply with a subenclosure product id of 000B0015, will 
 *  be listed as 000B0015 in the config page (subenclosure product id),
 *  and it will take firmware file CDES_1_1_Power_Supply_000B0015.bin
 * 
 *  Please modify FBE_TEST_ESP_XXX_IMAGE_FILE_PATH_AND_NAME accordingly if this
 *  function gets changed.
 *
 * @param  pImageFileNameBuffer (OUTPUT)-
 * @param  pImageFileNameConstantPortion (INPUT) -
 * @param  pSubenclProductId (INPUT) -
 * @param  bufferLen (INPUT)-
 * @param  pImageFileNameLen (OUTPUT)-
 *
 * @return fbe_status_t - 
 *
 * @version
 *  15-Oct-2010 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_base_env_fup_build_image_file_name(fbe_u8_t * pImageFileNameBuffer,
                                                   char * pImageFileNameConstantPortion,
                                                   fbe_u8_t bufferLen,
                                                   fbe_u8_t * pSubenclProductId,
                                                   fbe_u32_t * pImageFileNameLen,
                                                   fbe_u16_t esesVersion)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       pid = 0;
    fbe_bool_t      bufferSizeOk = FBE_FALSE;
    char            pid_str[16] = "\0";

    pid = csx_p_get_process_id();
    fbe_sprintf(pid_str, sizeof(pid_str), "_pid%d", pid);

    fbe_zero_memory(pImageFileNameBuffer, bufferLen);

    if(esesVersion == ESES_REVISION_1_0)
    {
        if((strlen(pImageFileNameConstantPortion) + FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + strlen(pid_str) + 4) < bufferLen)
        {
            bufferSizeOk = FBE_TRUE;
        }
    }
    else
    {
        if((strlen(pImageFileNameConstantPortion) + strlen(pid_str) + 4) < bufferLen)
        {
            bufferSizeOk = FBE_TRUE;
        }
    }

    if(bufferSizeOk)
    {
        // Get the constant portion of the filename
        strncat(pImageFileNameBuffer, pImageFileNameConstantPortion, strlen(pImageFileNameConstantPortion));

        if(esesVersion == ESES_REVISION_1_0)
        {
            // Append the product id portion of the filename
            strncat(pImageFileNameBuffer, pSubenclProductId, FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE);
        }

        // Append the Process ID
        strncat(pImageFileNameBuffer, pid_str, 16);

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

} //End of function fbe_ps_mgmt_fup_build_image_file_name

/*!***************************************************************
 * fbe_base_env_init_fup_params()
 ****************************************************************
 * @brief
 *  This function initializes the upgrade delay at boot up.
 *
 * @param pBaseEnv -
 * @param delayInSecOnSPA - The time to wait before the upgrade starts on SPA.
 * @param delayInSecOnSPB - The time to wait before the upgrade starts on SPB.
 * 
 * @return fbe_status_t
 *
 * @author
 *  06-Dec-2012 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_init_fup_params(fbe_base_environment_t * pBaseEnv,
                                          fbe_u32_t delayInSecOnSPA,
                                          fbe_u32_t delayInSecOnSPB)
{
    pBaseEnv->upgradeDelayInSec = 0;
    
    return FBE_STATUS_OK;
}

/*!*******************************************************************************
 * @fn fbe_base_env_fup_build_manifest_file_name
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
fbe_status_t fbe_base_env_fup_build_manifest_file_name(fbe_base_environment_t * pBaseEnv,
                                                       fbe_u8_t * pManifestFileNameBuffer,
                                                       fbe_char_t * pManifestFileNameConstantPortion,
                                                       fbe_u8_t bufferLen,
                                                       fbe_u32_t * pManifestFileNameLen)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_u32_t pid = 0;
    char pid_str[16] = "\0";

    FBE_UNREFERENCED_PARAMETER(pBaseEnv);

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

} //End of function fbe_base_env_fup_build_manifest_file_name

/* End of file fbe_base_enviroment_sim_main.c */
