/***************************************************************************
* Copyright (C) EMC Corporation 2012
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_emv_process.c
***************************************************************************
*
* @brief
*  This file contains functions needed to process expected memory value.
*  
* @version
*  05/15/2012 - Created. Zhipeng Hu
*
***************************************************************************/
#include "fbe_database_private.h"
#include "fbe_database_registry.h"
#include "fbe_database_system_db_interface.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"


/*!***************************************************************
 * @fn database_check_system_memory_configuration
 ****************************************************************
 *   Half of the shared expected memory information (fbe_u64_t) in the system db header
 *   contains the Shared Expected Memory (SEMV) Value and the other half 
 *   contains the Conversion Target Memory (CTM) value.
 *
 *   This routine validates the SEMV against the Local Expected Memory Value 
 *   (LEMV), which is stored in the registry of each SP.
 *
 *   These values are to ensure that the array does not boot in a mismatched memory
 *   configuration and to enable the update of the SEMV in In-Family Conversion scenarios.
 *
 *   The SEMV is must match the LEMV value if we are not in a conversion scenario (ie CTM = 0).
 *   Otherwise the SEMV is written to SharedExpectedMemoryValue registry and enters service mode.
 *   k10_software_start script will detect this registry is set, then runs platform specific 
 *   settings and sets LEMV to SEMV
 *
 *   If an In-Family Conversion is in progress, the CTM is set to the value of the 
 *   target SP's memory, the SEMV is the source SP's memory and LEMV is the current SP's
 *   memory.  If a conversion is not in progress the CTM value is 0.
 *
 *   If conversion is in progress and :
 *   (1) we are running on target SP, we are converting and the SEMV must be set to the value 
 *       of the CTM, then CTM is cleared
 *   (2) we are running on source SP, we are returning to source SP and just clear the CTM
 *
 *   Note that this routine coordinates the update of the SEMV between the
 *   two SPs in the conversion scenario.  The active SP reaching this routine
 *   sets the SEMV and clears CTM declaring the conversion "complete".  The
 *   passive SP must match the memory saved in the SEMV or it must boot in
 *   service mode. 
 *
 *   @Version
 *   05/18/2012 - Created. Zhipeng Hu.
 */
fbe_status_t database_check_system_memory_configuration(fbe_database_service_t *database_service_p)
{
    fbe_status_t    status;
    fbe_u32_t        local_emv;

    /*  define a semv_info union to get seperate part of the Shared Expected Memory info data.
     *  Shared Expected Memory info is a 64-bits data, whose high 32 bits represent CTM while
     *  the low 32 bits represent SEMV value*/
    union 
    {
        fbe_u64_t      semv_info;
        struct
        {
            fbe_u32_t     semv;    //  Shared Expected Memory Value
            fbe_u32_t     ctm;     //  Conversion Target Memory
        } semv_info_data;
    } semv_info_u;

    if(NULL == database_service_p)
        return FBE_STATUS_GENERIC_FAILURE;

    local_emv = 0;

    /***********************************************************************
     ******************FIRST PREPARE DATA: SEMV LEMV CTM*********************
     **********************************************************************/

    /* read local expected memory value from the registry */
    status = fbe_database_registry_get_local_expected_memory_value(&local_emv);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_INFO, 
                 "%s: failed to get LEMV from registry\n", __FUNCTION__);
        return status;
    }

    /* copy shared expected memory info to the defined union
     * no need to lock data because this func is called in boot stage, where
     * there will be no write to the semv_info except in this func*/
    semv_info_u.semv_info = database_service_p->shared_expected_memory_info.shared_expected_memory_info;

    database_trace(FBE_TRACE_LEVEL_INFO,
             FBE_TRACE_MESSAGE_ID_INFO, 
             "%s: SEMV:0x%x CTM:0x%x LEMV:0x%x\n", 
             __FUNCTION__,
             semv_info_u.semv_info_data.semv,
             semv_info_u.semv_info_data.ctm,
             local_emv);

    /***********************************************************************
     ******************THEN BEGIN THE VERIFICATION LOGIC*********************
     **********************************************************************/

    if(0 == semv_info_u.semv_info_data.ctm) /*we are not doing in-family conversion*/
    {
        if(local_emv != semv_info_u.semv_info_data.semv)
        {
            /* If the two EMVs do not match, SEMV should be written to SharedExpectedMemoryValue
            * registry and database enters service mode.
            * k10_software_start script will detect this registry is set, then runs platform specific 
            * settings and sets LEMV to SEMV
            * {{{NO---TI---CE}}} !:
            * once the k10_software_start is updated to be able to send IOCTLs to get database
            * state (whether in service mode or not) and SEMV from SEP driver, this registry set
            * would not be needed. */
            status = fbe_database_registry_set_shared_expected_memory_value(semv_info_u.semv_info_data.semv);
            if(FBE_STATUS_OK != status)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_INFO, 
                         "%s: failed to set SharedExpectedMemoryValue registry\n", __FUNCTION__);
            }

			fbe_event_log_write(SEP_ERROR_CODE_EMV_MISMATCH_WITH_PEER,
								NULL, 0,
								"%x %x",
								local_emv,
								semv_info_u.semv_info_data.semv);
			database_trace(FBE_TRACE_LEVEL_ERROR,
                     		FBE_TRACE_MESSAGE_ID_INFO, 
                     		"%s: local_emv(0x%x) is not equal with semv(0x%x)\n", 
                     		__FUNCTION__, local_emv,
                     		semv_info_u.semv_info_data.semv);
            fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INVALID_MEMORY_CONF_CHECK);
            return FBE_STATUS_SERVICE_MODE;
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_INFO,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     "%s: no conversion in progress and LEMV=SEMV so normally boot\n", 
                     __FUNCTION__);
        }

    }
    else
    { /*we are in in-family conversion (IFC)*/

        fbe_database_emv_t persist_semv_info;

        if(local_emv == semv_info_u.semv_info_data.ctm)
        {
            /*we are running on target SP in IFC process, so we are converting and the SEMV 
            *must be set to the value of the CTM, then CTM is cleared*/
            
            database_trace(FBE_TRACE_LEVEL_INFO,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     "%s: we are running on target SP in IFC progress\n", 
                     __FUNCTION__);
            
            semv_info_u.semv_info_data.ctm = 0;
            semv_info_u.semv_info_data.semv = local_emv;
            persist_semv_info.shared_expected_memory_info = semv_info_u.semv_info;
            status = database_set_shared_expected_memory_info(database_service_p, persist_semv_info);
            if(FBE_STATUS_OK != status)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_INFO, 
                         "%s: set semv info failed when we are running on target SP in IFC\n", 
                         __FUNCTION__);
                return status;
            }            
        }
        else if(local_emv == semv_info_u.semv_info_data.semv)
        {
            /*we are running on source SP, which means we are returning to source SP 
            *so just clear the CTM*/

            database_trace(FBE_TRACE_LEVEL_INFO,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     "%s: we are running on source SP in IFC progress\n", 
                     __FUNCTION__);
            
            semv_info_u.semv_info_data.ctm = 0;
            persist_semv_info.shared_expected_memory_info = semv_info_u.semv_info;
            status = database_set_shared_expected_memory_info(database_service_p, persist_semv_info);
            if(FBE_STATUS_OK != status)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_INFO, 
                         "%s: set semv info failed when we are running on source SP in IFC\n", 
                         __FUNCTION__);
                return status;
            }    

        }
        else
        {
            /*we should never ever get here !*/
            database_trace(FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     "%s: In-family Conversion failed due to invalid LEMV, SEMV or CTM\n", 
                     __FUNCTION__);
			fbe_event_log_write(SEP_ERROR_CODE_MEMORY_UPGRADE_FAIL,
								NULL, 0,
								"%x %x %x",
								local_emv,
								semv_info_u.semv_info_data.semv,
								semv_info_u.semv_info_data.ctm);
			fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_INVALID_MEMORY_CONF_CHECK);
            return FBE_STATUS_SERVICE_MODE;
        }

    }

    /*YEAH! CURRENT MEMORY CONFIGURATION PASSES ALL CHECKS. LET'S START SYSTEM NORMALLY*/

    return FBE_STATUS_OK;
    
}

/*!***************************************************************
 * @fn database_set_shared_expected_memory_info
 ****************************************************************
 * @brief
 *  This function is called in conversion senarios (on both source SP and target SP) 
 *  and sets shared expected memory info. To be specific:
 *  (1) persist the target shared emv to disk.
 *  (2) set the in-memory shared emv.
 *
 * @param[in]   database_service_p - the global database service variable
 * @param[in]   semv_info - the target shared emv we want to set
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  05/16/2012 - Created. Zhipeng Hu.
 *
 ****************************************************************/
fbe_status_t database_set_shared_expected_memory_info(fbe_database_service_t *database_service_p, fbe_database_emv_t semv_info)
{
    fbe_status_t    status = FBE_STATUS_OK;
    
    if(NULL == database_service_p)
        return FBE_STATUS_GENERIC_FAILURE;

    /*No need to sync the semv to be set to peer
     *Because when shared emv info is set by this func, it is in following 2 cases:
     *(1)System is rebooting for in-family conversion.
     *      In this case, this func is called by active SP when database is initializing.
     *      The peer's SEMV would be read from disks when the peer starts to initialize db
     *(2)IFC package is running to prepare for in-family conversion.
     *      In this case, this func is called by IOCTL sent from IFC package. Both sides 
     *      would receive this IOCTL and set SEMV info*/
        
    /*persist onto disk first*/
    if(database_common_cmi_is_active())
    {
         /* When doing in-family conversion, IFC package will run on both SPs
         * So both SP would receive the set SEMV IOCTL to call this func
         * But we only allow the active side to do persist*/
        status = fbe_database_system_db_persist_semv_info(&semv_info);
        if(FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_INFO, 
                     "%s: persist shared emv info failed\n", __FUNCTION__);
        }
    }

    /*then set the in-memory value*/
    if(FBE_STATUS_OK == status)
    {
        database_service_p->shared_expected_memory_info = semv_info;
    }

    return status;
}

/*!***************************************************************
 * @fn database_set_shared_expected_memory_info_from_local_emv
 ****************************************************************
 * @brief
 *  This function is called in ICA senarios and sets shared expected memory info. To be specific:
 *  >> set the in-memory shared emv.
 *  In later stage of database initialization, it will be persisted on to disks (because it is in system
 *  db header which would be persisted)
 *
 * @param[in]   db_service - the global database service ptr
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  05/15/2012 - Created. Zhipeng Hu.
 *
 ****************************************************************/
fbe_status_t database_initialize_shared_expected_memory_info(fbe_database_service_t *db_service)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t   local_emv = 0;

    if(NULL == db_service)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*read local expected memory value from registry and use it to initialize
     * shared expected memory info in system db header*/
    status = fbe_database_registry_get_local_expected_memory_value(&local_emv);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                 "%s: get local emv from registry failed\n", __FUNCTION__);
        return status;
    }

    db_service->shared_expected_memory_info.shared_expected_memory_info = (fbe_u64_t)local_emv;

    /*no need to persist because
     *  In later stage of database initialization, it will be persisted on to disks (because it is in system
     *  db header which would be persisted)*/
    return status;
}


