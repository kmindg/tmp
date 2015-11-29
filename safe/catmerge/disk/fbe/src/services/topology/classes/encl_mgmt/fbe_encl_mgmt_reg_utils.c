/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_enclosure_mgmt_reg_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Enclosure Management reg utils
 *  code.
 *
 *  This file contains some common utility functions for enclosure object management.
 *
 * @ingroup enclosure_mgmt_class_files
 *
 * @version
 *   22-Aug-2012:  Created. Dongz
 *
 ***************************************************************************/

#include "fbe/fbe_registry.h"
#include "fbe/fbe_enclosure.h"
#include "fbe_encl_mgmt_private.h"

#define FBE_ENCL_MGMT_SYSTEM_INFO_SN_KEY "ESPEnclMgmtSysInfoSN"
#define FBE_ENCL_MGMT_SYSTEM_INFO_PN_KEY "ESPEnclMgmtSysInfoPN"
#define FBE_ENCL_MGMT_SYSTEM_INFO_REV_KEY "ESPEnclMgmtSysInfoRev"
#define FBE_ENCL_MGMT_USER_MODIFIED_SYSTEM_ID_INFO_KEY "ESPEnclMgmtUserModifiedSystemIdInfo"
#define FBE_ENCL_MGMT_USER_MODIFIED_WWN_SEED_INFO_KEY "ESPEnclMgmtUserModifiedWwnSeedInfo"

/**************************************************************************
 *          fbe_encl_mgmt_set_system_id_sn_reg
 * ************************************************************************
 *  @brief
 *  Called to set system id serial number from reg.
 *
 *  @param pEnclMgmt - object context
 *  @param system_id_sn - system id serial number
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  28-Aug-2012:  Created. dongz
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_set_system_id_sn(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_char_t *system_id_sn)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_registry_write(NULL,
            ESP_REG_PATH,
            FBE_ENCL_MGMT_SYSTEM_INFO_SN_KEY,
            FBE_REGISTRY_VALUE_SZ,
            system_id_sn,
            RESUME_PROM_PRODUCT_SN_SIZE);

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Failed to set registry entry status:0x%x\n",
                __FUNCTION__, status);
    }

    return (status);
}

/**************************************************************************
 *          fbe_encl_mgmt_get_system_id_sn_reg
 * ************************************************************************
 *  @brief
 *  Called to get system id serial number from reg.
 *
 *  @param pEnclMgmt - object context
 *  @param system_id_sn - system id serial number
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  28-Aug-2012:  Created. dongz
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_get_system_id_sn(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_char_t *system_id_sn)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_char_t DefaultInputValue[RESUME_PROM_PRODUCT_SN_SIZE+1] = {0};

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               FBE_ENCL_MGMT_SYSTEM_INFO_SN_KEY,
                               system_id_sn,
                               RESUME_PROM_PRODUCT_SN_SIZE+1,
                               FBE_REGISTRY_VALUE_SZ,
                               &DefaultInputValue,
                               0,
                               FALSE);

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get registry entry status:0x%x\n",
                              __FUNCTION__, status);
        fbe_copy_memory(system_id_sn, DefaultInputValue, RESUME_PROM_PRODUCT_SN_SIZE);
    }

    return (status);
}

/**************************************************************************
 *          fbe_encl_mgmt_set_system_id_pn_reg
 * ************************************************************************
 *  @brief
 *  Called to set system id part number from reg.
 *
 *  @param pEnclMgmt - object context
 *  @param system_id_pn - system id part number
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  28-Aug-2012:  Created. dongz
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_set_system_id_pn(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_char_t *system_id_pn)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_registry_write(NULL,
            ESP_REG_PATH,
            FBE_ENCL_MGMT_SYSTEM_INFO_PN_KEY,
            FBE_REGISTRY_VALUE_SZ,
            system_id_pn,
            RESUME_PROM_PRODUCT_PN_SIZE);

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Failed to set registry entry status:0x%x\n",
                __FUNCTION__, status);
    }

    return (status);
}

/**************************************************************************
 *          fbe_encl_mgmt_get_system_id_pn_reg
 * ************************************************************************
 *  @brief
 *  Called to get system id part number from reg.
 *
 *  @param pEnclMgmt - object context
 *  @param system_id_pn - system id part number
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  28-Aug-2012:  Created. dongz
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_get_system_id_pn(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_char_t *system_id_pn)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_char_t DefaultInputValue[RESUME_PROM_PRODUCT_PN_SIZE+1] = {0};

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               FBE_ENCL_MGMT_SYSTEM_INFO_PN_KEY,
                               system_id_pn,
                               RESUME_PROM_PRODUCT_PN_SIZE+1,
                               FBE_REGISTRY_VALUE_SZ,
                               &DefaultInputValue,
                               0,
                               FALSE);

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get registry entry status:0x%x\n",
                              __FUNCTION__, status);
        fbe_copy_memory(system_id_pn, DefaultInputValue, RESUME_PROM_PRODUCT_PN_SIZE);
    }

    return (status);
}

/**************************************************************************
 *          fbe_encl_mgmt_set_system_id_rev_reg
 * ************************************************************************
 *  @brief
 *  Called to set system id rev from reg.
 *
 *  @param pEnclMgmt - object context
 *  @param system_id_rev - system id rev
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  28-Aug-2012:  Created. dongz
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_set_system_id_rev(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_char_t *system_id_rev)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_registry_write(NULL,
            ESP_REG_PATH,
            FBE_ENCL_MGMT_SYSTEM_INFO_REV_KEY,
            FBE_REGISTRY_VALUE_SZ,
            system_id_rev,
            RESUME_PROM_PRODUCT_REV_SIZE);

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Failed to set registry entry status:0x%x\n",
                __FUNCTION__, status);
    }

    status = fbe_flushRegistry();
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Failed to flush registry status:0x%x\n",
                __FUNCTION__, status);
    }

    return (status);
}

/**************************************************************************
 *          fbe_encl_mgmt_get_system_id_rev_reg
 * ************************************************************************
 *  @brief
 *  Called to get system id rev from reg.
 *
 *  @param pEnclMgmt - object context
 *  @param system_id_rev - system id rev
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  28-Aug-2012:  Created. dongz
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_get_system_id_rev(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_char_t *system_id_rev)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_char_t DefaultInputValue[RESUME_PROM_PRODUCT_REV_SIZE+1] = {0};

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               FBE_ENCL_MGMT_SYSTEM_INFO_REV_KEY,
                               system_id_rev,
                               RESUME_PROM_PRODUCT_REV_SIZE+1,
                               FBE_REGISTRY_VALUE_SZ,
                               &DefaultInputValue,
                               0,
                               FALSE);

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get registry entry status:0x%x\n",
                              __FUNCTION__, status);
        fbe_copy_memory(system_id_rev, DefaultInputValue, RESUME_PROM_PRODUCT_REV_SIZE);
    }

    return (status);
}

/*!**************************************************************
 * fbe_encl_mgmt_load_persistent_data()
 ****************************************************************
 * @brief
 *  This function reads in data from the persistent storage and
 *  loads the configuration into the system id info structures.
 *
 * @param - encl_mgmt - context
 *
 * @author
 *  28-Aug-2012:  Created. dongz
 *
 ****************************************************************/
void fbe_encl_mgmt_load_persistent_data(fbe_encl_mgmt_t *encl_mgmt)
{
    fbe_encl_mgmt_reg_get_system_id_sn(encl_mgmt, encl_mgmt->system_id_info.product_serial_number);

    fbe_encl_mgmt_reg_get_system_id_pn(encl_mgmt, encl_mgmt->system_id_info.product_part_number);

    fbe_encl_mgmt_reg_get_system_id_rev(encl_mgmt, encl_mgmt->system_id_info.product_revision);
    fbe_base_object_trace((fbe_base_object_t *) encl_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, get sn: %.16s, pn: %.16s, rev: %.3s\n",
                          __FUNCTION__,
                          encl_mgmt->system_id_info.product_serial_number,
                          encl_mgmt->system_id_info.product_part_number,
                          encl_mgmt->system_id_info.product_revision);
}

/**************************************************************************
 *          fbe_encl_mgmt_reg_set_user_modified_system_id_flag
 * ************************************************************************
 *  @brief
 *  Called to set the user modified system id flag registry key.
 *
 *  @param pEnclMgmt - object context
 *  @param user_modified_system_id_flag - has user modified system id?
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  28-Aug-2012:  Created. dongz
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_set_user_modified_system_id_flag(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_bool_t user_modified_system_id_flag)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Setting the registry key userModifiedSystemIdFlag to %d.\n",
                          user_modified_system_id_flag);

    status = fbe_registry_write(NULL,
            ESP_REG_PATH,
            FBE_ENCL_MGMT_USER_MODIFIED_SYSTEM_ID_INFO_KEY,
            FBE_REGISTRY_VALUE_DWORD,
            &user_modified_system_id_flag,
            sizeof(fbe_bool_t));

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "Failed to set the registry key userModifiedSystemIdFlag, status:0x%x\n",
                status);
    }

    return (status);
}

/**************************************************************************
 *          fbe_encl_mgmt_reg_get_user_modified_system_id_flag
 * ************************************************************************
 *  @brief
 *  Called to set get user modified system id flag registry key.
 *
 *  @param pEnclMgmt - object context
 *  @param user_modified_system_id_flag - has user modified system id?
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  28-Aug-2012:  Created. dongz
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_get_user_modified_system_id_flag(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_bool_t *user_modified_system_id_flag)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t DefaultInputValue;

    DefaultInputValue = 0;

    fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s reading the registry for userModifiedSystemIdFlag\n",
                          __FUNCTION__);

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               FBE_ENCL_MGMT_USER_MODIFIED_SYSTEM_ID_INFO_KEY,
                               user_modified_system_id_flag,
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
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get registry entry status:0x%x\n",
                              __FUNCTION__, status);
    }

    return (status);
}



/**************************************************************************
 *          fbe_encl_mgmt_set_user_modified_wwn_seed_info
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
 *  16-Aug-2012: Dongz - Created
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_set_user_modified_wwn_seed_info(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_bool_t user_modified_wwn_seed_flag)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s writing the registry for userModifiedWwnSeedFlag\n",
            __FUNCTION__);

    status = fbe_registry_write(NULL,
            ESP_REG_PATH,
            FBE_ENCL_MGMT_USER_MODIFIED_WWN_SEED_INFO_KEY,
            FBE_REGISTRY_VALUE_DWORD,
            &user_modified_wwn_seed_flag,
            sizeof(fbe_bool_t));

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Failed to set registry entry status:0x%x\n",
                __FUNCTION__, status);
    }

    return (status);
}//end of fbe_encl_mgmt_set_user_modified_wwn_seed_info

/**************************************************************************
 *          fbe_encl_mgmt_get_user_modified_wwn_seed_info
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
 *  16-Aug-2012: Dongz - Created
 * ***********************************************************************/
fbe_status_t fbe_encl_mgmt_reg_get_user_modified_wwn_seed_info(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_bool_t *user_modified_wwn_seed_flag)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t DefaultInputValue;

    DefaultInputValue = FALSE;

    fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s reading the registry for userModifiedWwnSeedFlag\n",
            __FUNCTION__);

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               FBE_ENCL_MGMT_USER_MODIFIED_WWN_SEED_INFO_KEY,
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
        fbe_base_object_trace((fbe_base_object_t *) pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get registry entry status:0x%x\n",
                              __FUNCTION__, status);
    }

    return (status);
}
//end of fbe_enclsoure_mgmt_reg_utils.c
