 /***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_encl_mgmt_kernel_main.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for encl_mgmt kernel mode functionality.
 *
 * @version
 *   25-Oct-2010:  PHE - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe_encl_mgmt_private.h"


/*!*******************************************************************************
 * @fn fbe_encl_mgmt_fup_build_image_file_name
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
 *  25-Oct-2010 PHE - Created.
 *  12-Sep-20104 PHE - Modified to handle the CDES-2 LCC firmware upgrade.
 *******************************************************************************/
fbe_status_t fbe_encl_mgmt_fup_build_image_file_name(fbe_encl_mgmt_t * pEnclMgmt,
                                                     fbe_base_env_fup_work_item_t * pWorkItem,
                                                     fbe_u8_t * pImageFileNameBuffer,
                                                     char * pImageFileNameConstantPortion,
                                                     fbe_u8_t bufferLen,
                                                     fbe_u32_t * pImageFileNameLen)
{
    FBE_UNREFERENCED_PARAMETER(pEnclMgmt);

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
} //End of function fbe_encl_mgmt_fup_build_image_file_name

//End of file fbe_encl_mgmt_kernel_main.c

