/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_board_pe_utils.c
 ***************************************************************************
 * @brief
 *  This file contains the common functions for base_environment.
 *
 * @version
 *   15-Oct-2010:  Arun S - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "base_board_pe_private.h"
#include "edal_processor_enclosure_data.h"

/*!*************************************************************************
 *   @fn fbe_base_board_get_resume_prom_device_type_and_location(fbe_pe_resume_prom_id_t rp_comp_index,
 *                               fbe_device_physical_location_t *device_location, 
 *                               fbe_u64_t *device_type)
 **************************************************************************
 *  @brief
 *      This function will convert the resume prom enum to the appropriate 
 *      Device type and also assigns the SP and slot #.
 *
 *  @param rp_comp_index - The index to the resume prom enums.
 *  @param device_location - The pointer to the device_location (SP, slot, etc)
 *  @param device_type - The pointer to the device type.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data change
 *              notification for the component.
 *
 *  @version
 *    12-Oct-2010: Arun S - Created.
 *
 **************************************************************************/

fbe_status_t fbe_base_board_get_resume_prom_device_type_and_location(fbe_base_board_t * base_board,
                                                         fbe_pe_resume_prom_id_t rp_comp_index, 
                                                         fbe_device_physical_location_t *device_location, 
                                                         fbe_u64_t *device_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(rp_comp_index)
    {
        case FBE_PE_MIDPLANE_RESUME_PROM:
            device_location->slot = 0;
            *device_type = FBE_DEVICE_TYPE_ENCLOSURE;
            break;

        case FBE_PE_SPA_RESUME_PROM:
        case FBE_PE_SPB_RESUME_PROM:
            device_location->slot = 0;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_SP;
            break;
        
        case FBE_PE_SPA_PS0_RESUME_PROM:
            device_location->sp = SP_A;
            device_location->slot = PS_0;
            *device_type = FBE_DEVICE_TYPE_PS;
            break;

        case FBE_PE_SPB_PS0_RESUME_PROM:
            device_location->sp = SP_B;
            device_location->slot = base_board->psCountPerBlade + PS_0;
            *device_type = FBE_DEVICE_TYPE_PS;
            break;

        case FBE_PE_SPA_PS1_RESUME_PROM:
            device_location->sp = SP_A;
            device_location->slot = PS_1;
            *device_type = FBE_DEVICE_TYPE_PS;
            break;

        case FBE_PE_SPB_PS1_RESUME_PROM:
            device_location->sp = SP_B;
            device_location->slot = base_board->psCountPerBlade + PS_1;
            *device_type = FBE_DEVICE_TYPE_PS;
            break;
        
        case FBE_PE_SPA_MGMT_RESUME_PROM:
        case FBE_PE_SPB_MGMT_RESUME_PROM:
            device_location->slot = 0;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_MGMT_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_MGMT_MODULE;
            break;
        
        case FBE_PE_SPA_IO_ANNEX_RESUME_PROM:
        case FBE_PE_SPB_IO_ANNEX_RESUME_PROM:
            device_location->slot = 0;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_ANNEX_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_BACK_END_MODULE;
            break;
        
        case FBE_PE_SPA_IO_SLOT0_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT0_RESUME_PROM:
            device_location->slot = 0;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT0_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT1_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT1_RESUME_PROM:
            device_location->slot = 1;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT1_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT2_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT2_RESUME_PROM:
            device_location->slot = 2;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT2_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT3_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT3_RESUME_PROM:
            device_location->slot = 3;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT3_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT4_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT4_RESUME_PROM:
            device_location->slot = 4;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT4_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT5_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT5_RESUME_PROM:
            device_location->slot = 5;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT5_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT6_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT6_RESUME_PROM:
            device_location->slot = 6;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT6_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT7_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT7_RESUME_PROM:
            device_location->slot = 7;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT7_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT8_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT8_RESUME_PROM:
            device_location->slot = 8;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT8_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT9_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT9_RESUME_PROM:
            device_location->slot = 9;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT9_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_IO_SLOT10_RESUME_PROM:
        case FBE_PE_SPB_IO_SLOT10_RESUME_PROM:
            device_location->slot = 10;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_IO_SLOT10_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_PE_SPA_MEZZANINE_RESUME_PROM:
        case FBE_PE_SPB_MEZZANINE_RESUME_PROM:
            device_location->slot = 0;
            device_location->sp = (rp_comp_index == FBE_PE_SPA_MEZZANINE_RESUME_PROM) ? SP_A : SP_B;
            *device_type = FBE_DEVICE_TYPE_MEZZANINE;
            break;
        
        //case FBE_PE_CACHE_CARD_RESUME_PROM:
        default:
            *device_type = FBE_DEVICE_TYPE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/*!*************************************************************************
 *   @fn fbe_base_board_get_resume_prom_smb_device(fbe_base_board_t * base_board,
 *                               fbe_u64_t device_type,
 *                               fbe_device_physical_location_t * pLocation,
 *                               SMB_DEVICE *smb_device)     
 **************************************************************************
 *  @brief
 *      This function will get the appropriate resume prom SMB device based on the
 *      Device type and location (SP, slot).
 *
 *  @param IN device_type - Type of the Resume Prom Device.
 *  @param IN pLocation - The pointer to the location (SP, slot) which the device is associated with.
 *  @param OUT smb_device - The corresponding SMB device for the Resume prom Device.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data change notification for the component.
 *
 *  @version
 *    12-Oct-2010: Arun S - Created.
 *
 **************************************************************************/

fbe_status_t fbe_base_board_get_resume_prom_smb_device(fbe_base_board_t * base_board,
                                                       fbe_u64_t device_type, 
                                                       fbe_device_physical_location_t * pLocation, 
                                                       SMB_DEVICE *smb_device)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(device_type)
    {
        case FBE_DEVICE_TYPE_SP:
            *smb_device = (pLocation->sp == SP_A) ? SPA_SP_RESUME_PROM : SPB_SP_RESUME_PROM;
            break;

        case FBE_DEVICE_TYPE_ENCLOSURE :
            *smb_device = MIDPLANE_RESUME_PROM;
            break;
    
        case FBE_DEVICE_TYPE_MGMT_MODULE :
            *smb_device = (pLocation->sp == SP_A) ? SPA_MGMT_RESUME_PROM : SPB_MGMT_RESUME_PROM;
            break;
    
        case FBE_DEVICE_TYPE_BACK_END_MODULE :
            *smb_device = (pLocation->sp == SP_A) ? SPA_BEM_RESUME_PROM : SPB_BEM_RESUME_PROM;
            break;
    
        case FBE_DEVICE_TYPE_IOMODULE :
            {
                /* Switch by slot numer */
                switch (pLocation->slot)
                {
                    case 0:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_0_RESUME_PROM : SPB_IOSLOT_0_RESUME_PROM;
                        break;
                    case 1:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_1_RESUME_PROM : SPB_IOSLOT_1_RESUME_PROM;
                        break;
                    case 2:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_2_RESUME_PROM : SPB_IOSLOT_2_RESUME_PROM;
                        break;
                    case 3:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_3_RESUME_PROM : SPB_IOSLOT_3_RESUME_PROM;
                        break;
                    case 4:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_4_RESUME_PROM : SPB_IOSLOT_4_RESUME_PROM;
                        break;
                    case 5:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_5_RESUME_PROM : SPB_IOSLOT_5_RESUME_PROM;
                        break;
                    case 6:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_6_RESUME_PROM : SPB_IOSLOT_6_RESUME_PROM;
                        break;
                    case 7:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_7_RESUME_PROM : SPB_IOSLOT_7_RESUME_PROM;
                        break;
                    case 8:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_8_RESUME_PROM : SPB_IOSLOT_8_RESUME_PROM;
                        break;
                    case 9:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_9_RESUME_PROM : SPB_IOSLOT_9_RESUME_PROM;
                        break;
                    case 10:
                        *smb_device = (pLocation->sp == SP_A) ? SPA_IOSLOT_10_RESUME_PROM : SPB_IOSLOT_10_RESUME_PROM;
                        break;
                    default:
                        *smb_device = SMB_DEVICE_INVALID;
                        status = FBE_STATUS_GENERIC_FAILURE;
                        break;
                }
            }
            break;

        case FBE_DEVICE_TYPE_PS:
            if (base_board->psCountPerBlade > 0)
            {
                if((pLocation->slot)/(base_board->psCountPerBlade) == SP_A) 
                {
                    *smb_device = ((pLocation->slot)%(base_board->psCountPerBlade) == PS_0) ? 
                           SPA_PS0_RESUME_PROM : SPA_PS1_RESUME_PROM;
                }
                else
                {
                    *smb_device = ((pLocation->slot)%(base_board->psCountPerBlade) == PS_0) ? 
                           SPB_PS0_RESUME_PROM : SPB_PS1_RESUME_PROM;
                }
            }
            
            break;

        case FBE_DEVICE_TYPE_MEZZANINE:
            *smb_device = (pLocation->sp == SP_A) ? SPA_MEZZANINE_RESUME_PROM : SPB_MEZZANINE_RESUME_PROM;
            break;

        case FBE_DEVICE_TYPE_BATTERY:
            *smb_device = (pLocation->sp == SP_A) ? SPA_BATTERY_0_RESUME_PROM : SPB_BATTERY_0_RESUME_PROM;
            break;

        case FBE_DEVICE_TYPE_CACHE_CARD:
            *smb_device = CACHE_CARD_RESUME_PROM;
            break;

        case FBE_DEVICE_TYPE_FAN:
            switch (pLocation->slot)
            {
                case 0:
                    *smb_device = SPA_CM_0_RESUME_PROM;
                    break;
                case 1:
                    *smb_device = SPA_CM_1_RESUME_PROM;
                    break;
                case 2:
                    *smb_device = SPA_CM_2_RESUME_PROM;
                    break;
                case 3:
                    *smb_device = SPB_CM_0_RESUME_PROM;
                    break;
                case 4:
                    *smb_device = SPB_CM_1_RESUME_PROM;
                    break;
                case 5:
                    *smb_device = SPB_CM_2_RESUME_PROM;
                    break;
                default:
                    *smb_device = SMB_DEVICE_INVALID;
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;
            }
            break;

        default:
            *smb_device = SMB_DEVICE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/*!*************************************************************************
 *   @fn fbe_base_board_get_resume_prom_id(SP_ID sp, fbe_u32_t slotNum,
 *                               fbe_u64_t device_type, 
 *                               fbe_pe_resume_prom_id_t *resumePromId)     
 **************************************************************************
 *  @brief
 *      This function will get the appropriate resume prom ID device based on the
 *      Device type and location (SP, slot).
 *
 *  @param IN sp, slot - The location (SP, slot) which the device is associated with.
 *  @param IN device_type - Type of the Resume Prom Device.
 *  @param OUT resumePromId - The corresponding Resume Prom ID for the Resume prom Device.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data change notification for the component.
 *
 *  @version
 *    21-Oct-2010: Arun S - Created.
 *
 **************************************************************************/

fbe_status_t fbe_base_board_get_resume_prom_id(SP_ID sp, fbe_u32_t slotNum, 
                                               fbe_u64_t device_type, 
                                               fbe_pe_resume_prom_id_t *resumePromId)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(device_type)
    {
        case FBE_DEVICE_TYPE_IOMODULE:
        {
            switch(slotNum) 
            {
                case 0:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT0_RESUME_PROM : FBE_PE_SPA_IO_SLOT0_RESUME_PROM;
                    break;
        
                case 1:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT1_RESUME_PROM : FBE_PE_SPB_IO_SLOT1_RESUME_PROM;
                    break;
        
                case 2:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT2_RESUME_PROM : FBE_PE_SPB_IO_SLOT2_RESUME_PROM;
                    break;
        
                case 3:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT3_RESUME_PROM : FBE_PE_SPB_IO_SLOT3_RESUME_PROM;
                    break;
        
                case 4:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT4_RESUME_PROM : FBE_PE_SPB_IO_SLOT4_RESUME_PROM;
                    break;
        
                case 5:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT5_RESUME_PROM : FBE_PE_SPB_IO_SLOT5_RESUME_PROM;
                    break;

                case 6:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT6_RESUME_PROM : FBE_PE_SPB_IO_SLOT6_RESUME_PROM;
                    break;
        
                case 7:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT7_RESUME_PROM : FBE_PE_SPB_IO_SLOT7_RESUME_PROM;
                    break;

                case 8:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT8_RESUME_PROM : FBE_PE_SPB_IO_SLOT8_RESUME_PROM;
                    break;

                case 9:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT9_RESUME_PROM : FBE_PE_SPB_IO_SLOT9_RESUME_PROM;
                    break;

                case 10:
                    *resumePromId = (sp == SP_A)? FBE_PE_SPA_IO_SLOT10_RESUME_PROM : FBE_PE_SPB_IO_SLOT10_RESUME_PROM;
                    break;

                    
                                                
                default: 
                    *resumePromId = FBE_PE_INVALID_RESUME_PROM;
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;
            }
        }
        break;
        
        default: 
            *resumePromId = FBE_PE_INVALID_RESUME_PROM;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/*!*************************************************************************
 *   @fn fbe_base_board_pe_xlate_smb_status (fbe_u32_t smbusStatus,
 *                              fbe_resume_prom_status_t *resumePromStatus)     
 **************************************************************************
 *  @brief
 *      This function converts SMBus status code to something that Board object
 *      understands.
 *
 *  @param IN smbusStatus : SMBus status code that needs to be converted.
 *  @param OUT resumePromStatus - The corresponding errorCode for the SMBus code.
 *
 *  @return fbe_status_t 
 *              FBE_STATUS_OK - successful, 
 *              otherwise - failed or no need to send data change notification for the component.
 *
 *  @version
 *    21-Oct-2010: Arun S - Created.
 *
 **************************************************************************/

fbe_status_t fbe_base_board_pe_xlate_smb_status(fbe_u32_t smbusStatus, 
                                                fbe_resume_prom_status_t *resumePromStatus, 
                                                fbe_bool_t retries_xhausted)
{
    /* (1) SPECL does NOT clear its retriesExhausted flag when its transaction status
     * is successful or STATUS_SMB_DEVICE_NOT_VALID_FOR_PLATFORM or STATUS_SMB_DEVICE_NOT_PRESENT. 
     * So it is needed to check the status here with the retriesExhausted flag.
     * (2) SPECL should set the transactionStatus to IN_PROGRESS when a state change rather than NOT_PRESENT, 
     * when it detects a fru gets inserted.
     * (3) SPECL does NOT overwrite the transactionStatus to IN_PROGRESS when the status was faulted and 
     * it is still retrying the resume prom read.
     * (4) SPECL requeues commands after retriesExhausted is reported, consider the read operation 
     * still in progress. As a result only the status codes listed below are considered 
     * final read completed status.
     */
    switch(smbusStatus)
    {
        case EMCPAL_STATUS_SUCCESS :
            *resumePromStatus = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
            break;
    
        case STATUS_SMB_DEVICE_NOT_VALID_FOR_PLATFORM:
            *resumePromStatus = FBE_RESUME_PROM_STATUS_DEVICE_NOT_VALID_FOR_PLATFORM;
            break;

        case STATUS_SMB_DEVICE_NOT_PRESENT:
            *resumePromStatus = FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT;
            break;

        case STATUS_SMB_DEVICE_POWERED_OFF:
            *resumePromStatus = FBE_RESUME_PROM_STATUS_DEVICE_POWERED_OFF;
            break;
    
        case STATUS_SMB_INSUFFICIENT_RESOURCES:
            *resumePromStatus = FBE_RESUME_PROM_STATUS_INSUFFICIENT_RESOURCES;
            break;

        case STATUS_SPECL_TRANSACTION_NOT_YET_STARTED:
        case STATUS_SPECL_TRANSACTION_IN_PROGRESS:
        case STATUS_SPECL_TRANSACTION_PENDING:
            *resumePromStatus = FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;

        case STATUS_SPECL_INVALID_CHECKSUM:
            *resumePromStatus = (retries_xhausted) ? FBE_RESUME_PROM_STATUS_CHECKSUM_ERROR : FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;
    
        case STATUS_SMB_ARB_ERR:
            *resumePromStatus = (retries_xhausted) ? FBE_RESUME_PROM_STATUS_ARB_ERROR : FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;

        case STATUS_SMB_FAILED_ERR:
            *resumePromStatus = (retries_xhausted) ? FBE_RESUME_PROM_STATUS_DEVICE_ERROR : FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;

        case STATUS_SMB_INVALID_BYTE_COUNT:
            *resumePromStatus = (retries_xhausted) ? FBE_RESUME_PROM_STATUS_INVALID_FIELD : FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;

        case STATUS_SMB_BUS_ERR:
        case STATUS_SMB_SELECTBITS_FAILED:  
            *resumePromStatus = (retries_xhausted) ? FBE_RESUME_PROM_STATUS_SMBUS_ERROR : FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;

        case STATUS_SMB_IO_TIMEOUT:
            *resumePromStatus = (retries_xhausted) ? FBE_RESUME_PROM_STATUS_DEVICE_TIMEOUT : FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;

        /* We don't handle the below SMB Dev error. Let it fall thru default case. */
        //case STATUS_SMB_DEV_ERR:
        default:
            // For unknown smbusErrorCode or device error, treat
            // as device dead, rather than panicing
            *resumePromStatus = (retries_xhausted) ? FBE_RESUME_PROM_STATUS_DEVICE_DEAD : FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_board_decode_device_type(fbe_u64_t deviceType)
 ****************************************************************
 * @brief
 *  This function decodes the device type.
 *
 * @param deviceType - 
 *
 * @return the device type string.
 *
 * @version
 *  21-Jan-2011:  Arun S - Created. 
 *
 ****************************************************************/
char * fbe_base_board_decode_device_type(fbe_u64_t deviceType)
{
    char * deviceTypeStr;

    switch(deviceType)
    {
        case FBE_DEVICE_TYPE_ENCLOSURE:
            deviceTypeStr = "ENCLOSURE";
            break;

        case FBE_DEVICE_TYPE_SP:
            deviceTypeStr = "SP";
            break;

        case FBE_DEVICE_TYPE_PS:
            deviceTypeStr = "PS";
            break;

        case FBE_DEVICE_TYPE_LCC:
            deviceTypeStr = "LCC";
            break;

        case FBE_DEVICE_TYPE_FAN:
            deviceTypeStr = "FAN";
            break;

        case FBE_DEVICE_TYPE_SPS:
            deviceTypeStr = "SPS";
            break;

        case FBE_DEVICE_TYPE_IOMODULE:
            deviceTypeStr = "IOMODULE";
            break;

        case FBE_DEVICE_TYPE_BACK_END_MODULE:
            deviceTypeStr = "IOANNEX";
            break;

        case FBE_DEVICE_TYPE_DRIVE:
            deviceTypeStr = "DRIVE";
            break;

        case FBE_DEVICE_TYPE_MEZZANINE:
            deviceTypeStr = "MEZZANINE";
            break;

        case FBE_DEVICE_TYPE_MGMT_MODULE:
            deviceTypeStr = "MGMT_MODULE";
            break;

        case FBE_DEVICE_TYPE_SLAVE_PORT:
            deviceTypeStr = "SLAVE_PORT";
            break;

        case FBE_DEVICE_TYPE_PLATFORM:
            deviceTypeStr = "PLATFORM";
            break;

        case FBE_DEVICE_TYPE_SUITCASE:
            deviceTypeStr = "SUITCASE";
            break;

        case FBE_DEVICE_TYPE_MISC:
            deviceTypeStr = "MISC";
            break;

        case FBE_DEVICE_TYPE_BMC:
            deviceTypeStr = "BMC";
            break;

        case FBE_DEVICE_TYPE_SFP:
            deviceTypeStr = "SFP";
            break;

        default:
            deviceTypeStr = "UNKNOWN";
            break;
    }

    return deviceTypeStr;
}


