 /***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_board_mgmt_sim_main.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for board management simulation mode functionality.
 *
 * @version
 *   16-July-2012:  Chengkai Hu - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#define I_AM_NATIVE_CODE
#include <windows.h>
#include "fbe/fbe_winddk.h"
#include "fbe_board_mgmt_private.h"

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

    /* cmmGetTotalPhysicalMemory cannot called in user mode, use dummy value here */
    memory_size = 0;

    return(memory_size);
} //End of function fbe_board_mgmt_get_local_sp_physical_memory

/*!*******************************************************************************
 * @fn fbe_board_mgmt_fup_build_image_file_name
 *********************************************************************************
 * @brief
 *  In sim mode, we build the LCC image file name and its length. 
 *  Please modify VAVOOM_TEST_ENCL_IMAGE_FILE_PATH_AND_NAME and 
 *  fbe_encl_mgmt_fup_build_image_file_name accordingly if this
 *  function gets changed.
 * 
 * @param  pBoardMgmt(INPUT) - 
 * @param  pImageFileNameBuffer (OUTPUT)-
 * @param  pImageFileNameConstantPortion (INPUT)-
 * @param  bufferLen (INPUT)-
 * @param  pImageFileNameLen (OUTPUT)-
 *
 * @return fbe_status_t - 
 *
 * @version
 *  31-Nov-2012 Rui Chang -- Created.
 *******************************************************************************/
fbe_status_t fbe_board_mgmt_fup_build_image_file_name(fbe_board_mgmt_t * pBoardMgmt,
                                                      fbe_base_env_fup_work_item_t * pWorkItem,
                                                      fbe_u8_t * pImageFileNameBuffer,
                                                      char * pImageFileNameConstantPortion,
                                                      fbe_u8_t bufferLen,
                                                      fbe_u32_t * pImageFileNameLen)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_u64_t pid = 0;
    char pid_str[16] = "\0";

    FBE_UNREFERENCED_PARAMETER(pBoardMgmt);
    FBE_UNREFERENCED_PARAMETER(pWorkItem);
 
    pid = (fbe_u64_t)csx_p_get_process_id();
    fbe_sprintf(pid_str, sizeof(pid_str), "_pid%llu", pid);
 
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

// End of file fbe_board_mgmt_sim_main.c
