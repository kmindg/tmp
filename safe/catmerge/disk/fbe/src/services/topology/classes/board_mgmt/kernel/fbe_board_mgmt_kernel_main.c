 /***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_kernel_main.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for board management kernel mode functionality.
 *
 * @version
 *   16-July-2012:  Chengkai Hu - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "cmm.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_board_mgmt_private.h"
#include "fbe/fbe_api_esp_common_interface.h"

/*!**************************************************************
 * @fn fbe_board_mgmt_get_local_sp_physical_memory()
 ****************************************************************
 * @brief
 *  This function return local physical memory size, and also fill it
 *      in board mgmt object.
 *
 * @return fbe_u32_t - size of local physical memory 
 *
 * @author
 *  16-July-2012:  Created. Chengkai
 *
 ****************************************************************/
fbe_u32_t fbe_board_mgmt_get_local_sp_physical_memory(void)
{
    fbe_u32_t   memory_size = 0;
    fbe_bool_t is_service_mode = FALSE;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    status = fbe_api_esp_common_get_is_service_mode(FBE_CLASS_ID_ENCL_MGMT, &is_service_mode);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
                        "%s: Getting service mode status failed, status 0x%X.\n",
                        __FUNCTION__, status);
        return (memory_size);
    }

    if (is_service_mode != TRUE) 
    {
        memory_size = (fbe_u32_t)((cmmGetTotalPhysicalMemory() + (MEGABYTE - 1)) / MEGABYTE);
    }

    return(memory_size);
} //End of function fbe_board_mgmt_get_local_sp_physical_memory

/*!*******************************************************************************
 * @fn fbe_board_mgmt_fup_build_image_file_name
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
 * @param  pBoardMgmt (INPUT)-
 * @param  pWorkItem (INPUT) - 
 * @param  pImageFileNameBuffer (OUTPUT)-
 * @param  pImageFileNameConstantPortion (INPUT)-
 * @param  bufferLen (INPUT)-
 * @param  pImageFileNameLen (OUTPUT)-
 *
 * @return fbe_status_t - 
 *
 * @version
 *  31-Nov-12 Rui Chang -- Created.
 *  12-Sep-20104 PHE - Modified to handle the CDES-2 LCC firmware upgrade.
 *******************************************************************************/
fbe_status_t fbe_board_mgmt_fup_build_image_file_name(fbe_board_mgmt_t * pBoardMgmt,
                                                      fbe_base_env_fup_work_item_t * pWorkItem,
                                                      fbe_u8_t * pImageFileNameBuffer,
                                                      char * pImageFileNameConstantPortion,
                                                      fbe_u8_t bufferLen,
                                                      fbe_u32_t * pImageFileNameLen)
{
    FBE_UNREFERENCED_PARAMETER(pBoardMgmt);

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
} //End of function fbe_board_mgmt_fup_build_image_file_name

//End of file fbe_board_mgmt_kernel_main.c
