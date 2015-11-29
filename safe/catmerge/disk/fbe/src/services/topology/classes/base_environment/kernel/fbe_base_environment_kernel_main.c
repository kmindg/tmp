 /***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_kernel_main.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for ps_mgmt kernel mode functionality.
 *
 * @version
 *   25-Oct-2010:  PHE - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe_base_environment_private.h"

/*!*******************************************************************************
 * @fn fbe_base_env_fup_build_image_file_name
 *********************************************************************************
 * @brief
 *  This function builds the image file name based on a constant string, the
 *  subenclosure product id and the extension.
 * 
 *  This function may not be appropriate to reside at base level because the ESES 1
 *  processing is specific to power supplies and cooling modules, but not lcc.
 * 
 *  CDES 1 format:
 *      SXP36x6g_Power_Supply_xxxxxxxx.bin where xxxxxxxx is the subenclosure product id
 *      CDES_1_1_Power_Supply_xxxxxxxx.bin where xxxxxxxx is the subenclosure product id
 *      CDES_Power_Supply_xxxxxxxx.bin where xxxxxxxx is the subenclosure product id
 *      CDES_1_1_Colling_Module_xxxxxxxx.bin where xxxxxxxx is the subenclosure product id
 * 
 * CDES 2 format (subenclosure product id is not used):
 *      laserbeak.bin
 *      octane.bin
 *      juno2.bin
 *      3gve.bin
 *      optimus_acbel.bin
 *      optimus_flex.bin
 *
 * @return fbe_status_t - 
 *
 * @version
 *  25-Oct-2010 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_base_env_fup_build_image_file_name(fbe_u8_t * pImageFileNameBuffer,
                                                   char * pImageFileNameConstantPortion,
                                                   fbe_u8_t bufferLen,
                                                   fbe_u8_t * pSubenclProductId,
                                                   fbe_u32_t * pImageFileNameLen,
                                                   fbe_u16_t esesVersion)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u16_t       bufferSizeOk = FBE_FALSE;

    fbe_zero_memory(pImageFileNameBuffer, bufferLen);

    if(esesVersion == ESES_REVISION_1_0)
    {
        if((strlen(pImageFileNameConstantPortion) + FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 4) < bufferLen)
        {
            bufferSizeOk = FBE_TRUE;
        }
    }
    else
    {
        if((strlen(pImageFileNameConstantPortion) + 4) < bufferLen)
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

        // Append the extension
        strncat(pImageFileNameBuffer, ".bin", 4);

        *pImageFileNameLen = (fbe_u32_t)strlen(pImageFileNameBuffer);

        status = FBE_STATUS_OK;
    }
    else
    {
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }  
    
    return status;

} //End of function fbe_base_env_fup_build_image_file_name

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
    if(pBaseEnv->spid == SP_A) 
    {
        pBaseEnv->upgradeDelayInSec = delayInSecOnSPA; 
    }
    else
    {
        pBaseEnv->upgradeDelayInSec = delayInSecOnSPB; 
    }

    return FBE_STATUS_OK;
}

/*!*******************************************************************************
 * @fn fbe_base_env_fup_build_manifest_file_name
 *********************************************************************************
 * @brief
 *  This function adds the ".json" to the manifest file name. 
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
    FBE_UNREFERENCED_PARAMETER(pBaseEnv);

    fbe_zero_memory(pManifestFileNameBuffer, bufferLen);
         
    /* Get the constant portion of the filename */
    strncat(pManifestFileNameBuffer, pManifestFileNameConstantPortion, strlen(pManifestFileNameConstantPortion));

    /* Append the extension */
    strncat(pManifestFileNameBuffer, ".json", 5);

    *pManifestFileNameLen = (fbe_u32_t)strlen(pManifestFileNameBuffer);

    return FBE_STATUS_OK;

} //End of function fbe_base_env_fup_build_manifest_file_name

/* End of file fbe_base_enviroment_kernel_main.c */
