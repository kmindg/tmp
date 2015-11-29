/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved. Licensed material -- property of EMC
* Corporation
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe/fbe_registry.h"
#include "fbe_database_private.h"
#include "fbe_database_registry.h"
#include "resume_prom.h"

/****************************
* MACRO DEFINATIONS
****************************/
#define REGISTRY_LENGTH   256
#define REGISTRY_SIM_FILE_PATH_LENGTH 64
#ifdef ALAMOSA_WINDOWS_ENV
#define REGISTRY_SIM_FILE_PATH_FORMAT ".\\sp_sim_pid%d\\sep_reg_pid%d.txt"
#define REGISTRY_SIM_TEMP_FILE_PATH_FORMAT ".\\sp_sim_pid%d\\sep_reg_temp_pid%d.txt"
#else
#define REGISTRY_SIM_FILE_PATH_FORMAT "./sp_sim_pid%d/sep_reg_pid%d.txt"
#define REGISTRY_SIM_TEMP_FILE_PATH_FORMAT "./sp_sim_pid%d/sep_reg_temp_pid%d.txt"
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */

#define REGISTRY_ESP_USER_MODIFIED_WWN_SEED_INFO_KEY "ESPEnclMgmtUserModifiedWwnSeedInfo"


/**************************************************************************
*          fbe_database_registry_get_ndu_flag
* ************************************************************************
*  @brief
*  Called to read the ndu flag value from the registry.
*
*  @param ndu flag - Set or Clear (0/1)
*
*  @return fbe_status_t - status
* ***********************************************************************/
fbe_status_t fbe_database_registry_get_ndu_flag(fbe_bool_t *ndu_flag)
{
	fbe_status_t status;
	fbe_u32_t DefaultInputValue;

	DefaultInputValue = 0;

	status = fbe_registry_read(NULL,
		SEP_REG_PATH,
		NDU_KEY,
		ndu_flag,
		sizeof(fbe_bool_t),
		FBE_REGISTRY_VALUE_BINARY,
		&DefaultInputValue,
		sizeof(fbe_bool_t),
		TRUE);

	if(status != FBE_STATUS_OK)
	{        
		database_trace (FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_INFO,
			"%s Failed to get registry entry status:0x%x\n", __FUNCTION__,status);
		return FBE_STATUS_GENERIC_FAILURE;        
	}

	return (status);
}

/**************************************************************************
*          fbe_database_registry_set_ndu_flag
* ************************************************************************
*  @brief
*  Called to write ndu flag value to the registry
*
*  @param ndu flag - Set or Clear (0/1)
*
*  @return fbe_status_t - status
* ***********************************************************************/
fbe_status_t fbe_database_registry_set_ndu_flag(fbe_bool_t *ndu_flag)
{
	fbe_status_t status;

	status = fbe_registry_write(NULL,
		SEP_REG_PATH,
		NDU_KEY, 
		FBE_REGISTRY_VALUE_BINARY,
		&ndu_flag, 
		sizeof(fbe_bool_t));


	if(status != FBE_STATUS_OK)
	{
		database_trace (FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_INFO,
			"%s Failed to set registry entry status:0x%x\n", __FUNCTION__,status);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return (status);
}

/**************************************************************************
*          fbe_database_registry_get_local_expected_memory_value
* ************************************************************************
*  @brief
*  Called to read local expected memory value from the registry
*
*  @param[out] lemv - the read local expected memory value
*
*  @return fbe_status_t - status
*
*  @version
*     17-May-2012 Created. Zhipeng Hu
* ***********************************************************************/
fbe_status_t fbe_database_registry_get_local_expected_memory_value(fbe_u32_t   *lemv)
{
    fbe_status_t status;
    fbe_u32_t DefaultInputValue;

    if(NULL == lemv)
        return FBE_STATUS_GENERIC_FAILURE;

    DefaultInputValue = 0;

    status = fbe_registry_read(NULL,
                    K10DriverConfigRegPath,
                    K10EXPECTEDMEMORYVALUEKEY,
                    lemv,
                    sizeof(fbe_u32_t),
                    FBE_REGISTRY_VALUE_DWORD,
                    &DefaultInputValue,
                    sizeof(fbe_u32_t),
                    FALSE);

    if(status != FBE_STATUS_OK)
    {        
        database_trace (FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s Failed to get registry entry status:0x%x\n", __FUNCTION__,status);
        return FBE_STATUS_GENERIC_FAILURE;        
    }

    return status;
}

/**************************************************************************
*          fbe_database_registry_set_shared_expected_memory_value
* ************************************************************************
*  @brief
*  Called to set shared expected memory value in the registry
*
*  @param[in] semv - the input shared expected memory value
*
*  @return fbe_status_t - status
*
*  @version
*     17-May-2012 Created. Zhipeng Hu
* ***********************************************************************/
fbe_status_t fbe_database_registry_set_shared_expected_memory_value(fbe_u32_t   semv)
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                    K10DriverConfigRegPath,
                    K10SHAREDEXPECTEDMEMORYVALUEKEY, 
                    FBE_REGISTRY_VALUE_DWORD,
                    &semv, 
                    sizeof(fbe_u32_t));

    if(status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s Failed to set registry entry status:0x%x\n", __FUNCTION__,status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}



/**************************************************************************
*          fbe_database_registry_get_addl_supported_drive_types
* ************************************************************************
*  @brief
*  Get the registry with the additional drive types supported.
* 
*  @return fbe_status_t - status
*
*  @version
*     15-Aug-2015 Created.  Wayne Garrett
* ***********************************************************************/
fbe_status_t fbe_database_registry_get_addl_supported_drive_types(fbe_database_additional_drive_types_supported_t *supported_drive_types)
{
    fbe_status_t status;
    fbe_u32_t DefaultInputValue = (fbe_u32_t)FBE_DATABASE_ADDITIONAL_DRIVE_TYPE_SUPPORTED_DEFAULT;
    fbe_u32_t supported;

    status = fbe_registry_read(NULL,
                    SEP_REG_PATH,
                    FBE_REG_ADDITIONAL_SUPPORTED_DRIVE_TYPES,
                    &supported,
                    sizeof(supported),
                    FBE_REGISTRY_VALUE_DWORD,
                    &DefaultInputValue,
                    sizeof(DefaultInputValue),
                    FALSE);

    if(status != FBE_STATUS_OK)
    {        
        database_trace (FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Reg %s not set. Using default\n", 
                        __FUNCTION__, FBE_REG_ADDITIONAL_SUPPORTED_DRIVE_TYPES);
        *supported_drive_types = DefaultInputValue;
    }
    else
    {
        *supported_drive_types = (fbe_database_additional_drive_types_supported_t)supported;
    }

    database_trace (FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s types: 0x%x\n", 
                    __FUNCTION__, *supported_drive_types);

    return status;
}

/**************************************************************************
*          fbe_database_registry_addl_set_supported_drive_types
* ************************************************************************
*  @brief
*  Updates the registry with the additional drive types supported
* 
*  @return fbe_status_t - status
*
*  @version
*     15-Aug-2015 Created.  Wayne Garrett
* ***********************************************************************/
fbe_status_t fbe_database_registry_addl_set_supported_drive_types(fbe_database_additional_drive_types_supported_t supported)
{
    fbe_status_t status;  

    status = fbe_registry_write(NULL,
                    SEP_REG_PATH,
                    FBE_REG_ADDITIONAL_SUPPORTED_DRIVE_TYPES, 
                    FBE_REGISTRY_VALUE_DWORD,
                    &supported, 
                    sizeof(supported));

    if(status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s Failed to set registry entry %s, status:0x%x\n", __FUNCTION__, FBE_REG_ADDITIONAL_SUPPORTED_DRIVE_TYPES, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        database_trace (FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s types: 0x%x\n", 
                        __FUNCTION__, supported);
    }

    return status;
}

/**************************************************************************
 *          fbe_database_registry_set_user_modified_wwn_seed_info
 * ************************************************************************
 *  @brief
 *  Called to set the user modified wwn seed flag registry key.
 *
 *  @param pEnclMgmt - object context
 *  @param user_modified_wwn_seed_info - user modified wwn seed flag
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  01-May-2014: bphilbin - Created
 * ***********************************************************************/
fbe_status_t fbe_database_registry_set_user_modified_wwn_seed_info(fbe_bool_t user_modified_wwn_seed_flag)
{
    fbe_status_t status = FBE_STATUS_OK;


    database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s writing the registry for userModifiedWwnSeedFlag\n", 
                        __FUNCTION__);

    status = fbe_registry_write(NULL,
            ESP_REG_PATH,
            REGISTRY_ESP_USER_MODIFIED_WWN_SEED_INFO_KEY,
            FBE_REGISTRY_VALUE_DWORD,
            &user_modified_wwn_seed_flag,
            sizeof(fbe_bool_t));

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Failed to set registry entry status:0x%x\n", 
                        __FUNCTION__,status);
    }

    return (status);
}//end of fbe_encl_mgmt_set_user_modified_wwn_seed_info

/**************************************************************************
 *          fbe_database_registry_get_user_modified_wwn_seed_info
 * ************************************************************************
 *  @brief
 *  Called to get the user modified wwn seed flag registry key.
 *
 *  @param pEnclMgmt - object context
 *  @param user_modified_wwn_seed_info - user modified wwn seed flag
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  01-May-2014: bphilbin - Created
 * ***********************************************************************/
fbe_status_t fbe_database_registry_get_user_modified_wwn_seed_info(fbe_bool_t *user_modified_wwn_seed_flag)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t DefaultInputValue;

    DefaultInputValue = FALSE;

    database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s reading the registry for userModifiedWwnSeedFlag\n", 
                        __FUNCTION__);

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               REGISTRY_ESP_USER_MODIFIED_WWN_SEED_INFO_KEY,
                               user_modified_wwn_seed_flag,
                               sizeof(fbe_bool_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &DefaultInputValue,
                               sizeof(fbe_bool_t),
                               TRUE);

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Failed to set registry entry status:0x%x\n", 
                        __FUNCTION__,status);
    }

    return (status);
}

/**************************************************************************
*          fbe_database_registry_get_c4_mirror_reinit_flag
* ************************************************************************
*  @brief
*  Called to read the reinit flag value from the registry.
*
*  @param reinit flag - Set or Clear (0/1)
*
*  @return fbe_status_t - status
* ***********************************************************************/
fbe_status_t fbe_database_registry_get_c4_mirror_reinit_flag(fbe_bool_t *reinit)
{
    fbe_status_t status;
    fbe_u8_t  reinitKeyValue[16] = {'\0'};  
    fbe_u8_t  defaultKeyValue[] = "1";

    strcpy(reinitKeyValue, defaultKeyValue);
    status = fbe_registry_read(NULL,
                              C4_MIRROR_REG_PATH,
                              C4_MIRROR_REINIT_KEY,
                              reinitKeyValue,
                              16,
                              FBE_REGISTRY_VALUE_SZ,
                              &defaultKeyValue,
                              (fbe_u32_t)strlen(defaultKeyValue),
                              TRUE);

    if(status != FBE_STATUS_OK)
    {        
    	database_trace (FBE_TRACE_LEVEL_ERROR,
    		FBE_TRACE_MESSAGE_ID_INFO,
    		"%s Failed to get registry entry status:0x%x\n", __FUNCTION__,status);
    	return FBE_STATUS_GENERIC_FAILURE;        
    }

    if (csx_p_strcmp(reinitKeyValue, "1") == 0)
    {
        *reinit = FBE_TRUE;
    } 
    else if (csx_p_strcmp(reinitKeyValue, "0") == 0)
    {
        *reinit = FBE_FALSE;
    }
    else
    {
	database_trace (FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_INFO,
			"%s invalid registry key value %c\n", __FUNCTION__, reinitKeyValue[0]);
	return FBE_STATUS_GENERIC_FAILURE;        
    }
    return (status);

}

/**************************************************************************
*          fbe_database_registry_set_c4_mirror_reinit_flag
* ************************************************************************
*  @brief
*  Called to write reinit flag value to the registry
*
*  @param reinit flag - Set or Clear (0/1)
*
*  @return fbe_status_t - status
* ***********************************************************************/
fbe_status_t fbe_database_registry_set_c4_mirror_reinit_flag(fbe_bool_t reinit)
{
    fbe_status_t status;
    fbe_u8_t reinitKeyValue[16] = {'\0'};

    if (reinit)
    {
        fbe_sprintf(reinitKeyValue, 16, "1");
    }
    else
    {
        fbe_sprintf(reinitKeyValue, 16, "0");
    }

    status = fbe_registry_write(NULL,
                                C4_MIRROR_REG_PATH,
                                C4_MIRROR_REINIT_KEY, 
                                FBE_REGISTRY_VALUE_SZ,
                                reinitKeyValue, 
                                (fbe_u32_t)strlen(reinitKeyValue));

    if(status != FBE_STATUS_OK)
    {
    	database_trace (FBE_TRACE_LEVEL_ERROR,
    		FBE_TRACE_MESSAGE_ID_INFO,
    		"%s Failed to set registry entry status:0x%x\n", __FUNCTION__,status);
    	return FBE_STATUS_GENERIC_FAILURE;
    }

    return (status);
}

