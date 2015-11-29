/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file winnie_the_pooh.c
 ***************************************************************************
 *
 * @brief
 *  This file contains handling of Resume Prom SMB status for the PE components
 *  
 * @version
 *   22/10/2010 Arun S - Created.
 *
 ***************************************************************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe_notification.h"
#include "fbe_base_board.h"
#include "specl_sfi_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_resume_prom_interface.h"

char * winnie_the_pooh_short_desc = "Test Resume Prom SMB Status for processor enclosure components.";
char * winnie_the_pooh_long_desc =
    "\n"
    "\n"
    "The winnie_the_pooh scenario tests the Resume Prom SMB status for physical package processor enclosure components.\n"
    "It includes: \n"
    "    - The Resume Prom SMB status verification for all the PE enclosure components;\n"
    "    - PE components: Midplane, SP, PS, IO Annex, IO Module, Mezzanine, Mgmt Module;\n"
    "\n"
    "Dependencies:\n"
    "    - The SPECL should be able to work in the user space to read the status from the terminator.\n"
    "    - The terminator should be able to provide the interface function to insert/remove the components in the processor enclosure.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] MAUI Config (SPID_DEFIANT_M1_HW_TYPE)armada board with processor enclosure data\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"  
    "STEP 2: Shutdown the Terminator/Physical package.\n"
    ;

static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_class_id_t           expected_class_id = FBE_CLASS_ID_BASE_BOARD;
static fbe_u64_t        expected_device_type = FBE_DEVICE_TYPE_INVALID;
static fbe_device_data_type_t   expected_data_type = FBE_DEVICE_DATA_TYPE_INVALID;
static fbe_device_physical_location_t expected_phys_location = {0};

static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;


static void winnie_the_pooh_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);
static void winnie_the_pooh_test_verify_pe_resume_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
fbe_status_t winnie_the_pooh_resume_read_async_callback(fbe_packet_completion_context_t context, void *envContext);
static fbe_status_t winnie_the_pooh_get_device_type_and_location(fbe_pe_resume_prom_id_t rp_comp_index, 
                                                         fbe_device_physical_location_t *device_location, 
                                                         fbe_u64_t *device_type);
static fbe_status_t winnie_the_pooh_convert_resume_prom_id_to_smb_device(fbe_pe_resume_prom_id_t resumePromId, 
                                                         SMB_DEVICE *smb_device);

void winnie_the_pooh(void)
{
    fbe_status_t        fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     objectId;
    PSPECL_SFI_MASK_DATA    sfi_mask_data = {0};
                                                            
    fbe_semaphore_init(&sem, 0, 1);
    fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                                  winnie_the_pooh_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return;
    }

    /* Get handle to the board object */
    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the Resume Prom SMB status for all PE components ===\n");

    winnie_the_pooh_test_verify_pe_resume_data(objectId, sfi_mask_data);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified the Resume Prom SMB status for all PE components ===\n");

    fbeStatus = fbe_api_notification_interface_unregister_notification(winnie_the_pooh_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);

    return;
}

static void winnie_the_pooh_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
    fbe_bool_t           expected_notification = FALSE; 
    fbe_notification_data_changed_info_t * pDataChangeInfo = NULL;

    pDataChangeInfo = &update_object_msg->notification_info.notification_data.data_change_info;

    if((expected_object_id == update_object_msg->object_id) && 
       (expected_device_type == pDataChangeInfo->device_mask) &&
       (expected_class_id == FBE_CLASS_ID_BASE_BOARD))
    {
        switch(expected_device_type)
        {
            case FBE_DEVICE_TYPE_IOMODULE:
            case FBE_DEVICE_TYPE_PS:
            case FBE_DEVICE_TYPE_SP:
            case FBE_DEVICE_TYPE_ENCLOSURE:
            case FBE_DEVICE_TYPE_BACK_END_MODULE:
            case FBE_DEVICE_TYPE_MGMT_MODULE: 
            case FBE_DEVICE_TYPE_MEZZANINE:
                if(pDataChangeInfo->data_type == FBE_DEVICE_DATA_TYPE_RESUME_PROM_INFO)
                {
                    if((expected_phys_location.sp == pDataChangeInfo->phys_location.sp) &&
                       (expected_phys_location.slot == pDataChangeInfo->phys_location.slot))
                    {
                        expected_notification = TRUE; 
                    }
                }
                break;

            default:
                break;
        }
    }

    if(expected_notification)
    {
        fbe_semaphore_release(sem, 0, 1 ,FALSE);
    }

    return;
}

static void winnie_the_pooh_test_verify_pe_resume_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                        dwWaitResult;
    fbe_status_t                 fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t                     *deviceStr = NULL;
    SMB_DEVICE                   smb_device = SMB_DEVICE_INVALID;
    fbe_pe_resume_prom_id_t      index;

    fbe_resume_prom_get_resume_async_t *pGetResumeProm = NULL;

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_data_type = FBE_DEVICE_DATA_TYPE_RESUME_PROM_INFO;

    for(index = FBE_PE_MIDPLANE_RESUME_PROM; index < FBE_PE_MAX_RESUME_PROM_IDS; index++)
    {
        /* Get the device type and Location */
        fbeStatus = winnie_the_pooh_get_device_type_and_location(index, &expected_phys_location, &expected_device_type);
        if(expected_device_type == FBE_DEVICE_TYPE_INVALID) 
        {
            continue;
        }

        /* Allocate memory for pGetResumeProm */
        pGetResumeProm = (fbe_resume_prom_get_resume_async_t *) malloc (sizeof (fbe_resume_prom_get_resume_async_t));
        if(pGetResumeProm == NULL)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Not enough memory to allocate for pGetResumeProm", __FUNCTION__);
            return;
        }

        fbe_zero_memory(pGetResumeProm, sizeof(fbe_resume_prom_get_resume_async_t));

        /* Allocate memory for pBuffer */
        pGetResumeProm->resumeReadCmd.pBuffer = (fbe_u8_t *) malloc (sizeof (RESUME_PROM_STRUCTURE));
        if(pGetResumeProm->resumeReadCmd.pBuffer == NULL)
        {
            free(pGetResumeProm);
            mut_printf(MUT_LOG_TEST_STATUS, "%s Not enough memory to allocate for pBuffer", __FUNCTION__);
            return;
        }

        /* Save the device type and location to GetResumeProm interface */
        pGetResumeProm->resumeReadCmd.deviceLocation = expected_phys_location;
        pGetResumeProm->resumeReadCmd.deviceType = expected_device_type;

        /* Set the buffer size and offset */
        pGetResumeProm->resumeReadCmd.bufferSize = sizeof(RESUME_PROM_STRUCTURE);
        pGetResumeProm->resumeReadCmd.resumePromField = RESUME_PROM_ALL_FIELDS;  
        pGetResumeProm->resumeReadCmd.offset = 0;

        /* Set the Callback function and context */
        pGetResumeProm->completion_function = winnie_the_pooh_resume_read_async_callback;
        pGetResumeProm->completion_context = NULL;

        /********************** SET the Resume Prom status *************************/

        deviceStr = fbe_base_board_decode_device_type(pGetResumeProm->resumeReadCmd.deviceType);
         
        if(pGetResumeProm->resumeReadCmd.deviceType == FBE_DEVICE_TYPE_ENCLOSURE)
        {
            mut_printf(MUT_LOG_TEST_STATUS, " === Device: %s ===", deviceStr);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, " === Device: %s, SP: %d, Slot: %d ===", 
                       deviceStr, 
                       pGetResumeProm->resumeReadCmd.deviceLocation.sp,
                       pGetResumeProm->resumeReadCmd.deviceLocation.slot);
        }

        fbeStatus = winnie_the_pooh_convert_resume_prom_id_to_smb_device(index, &smb_device);

        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, " === Injecting the Resume Prom Data: ===");

        /* Get the Resume info. */
        sfi_mask_data->structNumber = SPECL_SFI_RESUME_STRUCT_NUMBER;
        sfi_mask_data->smbDevice = smb_device;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.resumeStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the Resume info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.resumeStatus.transactionStatus = STATUS_SMB_FAILED_ERR;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the Resume Info ===");

        /* Get the Resume info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_resume_prom_read_async(objectId, pGetResumeProm);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /********************** CLEAR the Resume Prom status *************************/
        mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the Resume Info ===\n");

        /* Get the Resume info. */
        sfi_mask_data->structNumber = SPECL_SFI_RESUME_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.resumeStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the Resume info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.resumeStatus.transactionStatus = EMCPAL_STATUS_SUCCESS;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the Resume info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_resume_prom_read_async(objectId, pGetResumeProm);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Free the memory. */
        free(pGetResumeProm->resumeReadCmd.pBuffer);
        free(pGetResumeProm);
    }

    return;
}

/*!*******************************************************************
 * @fn winnie_the_pooh_resume_read_async_callback ()
 *********************************************************************
 * @brief:
 *  callback function of  winnie_the_pooh_resume_read_async_callback.
 *
 * @param context - context of structure 
 * @param envContext - context of base env 
 *
 * @return fbe_status_t - FBE_STATUS_OK in any case
 *
 * @version
 *    10-Jan-2011: Arun S  created
 *********************************************************************/
fbe_status_t winnie_the_pooh_resume_read_async_callback(fbe_packet_completion_context_t context, void *envContext)
{
    /* Do Nothing. Simply return. We are not 'actually' doing any async for board object components. */
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *   @fn winnie_the_pooh_get_device_type_and_location(fbe_pe_resume_prom_id_t rp_comp_index,
 *                               fbe_device_physical_location_t *device_location, 
 *                               fbe_u64_t *device_type)
 **************************************************************************
 *  @brief
 *      This function will convert the resume prom id to the appropriate 
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
static fbe_status_t winnie_the_pooh_get_device_type_and_location(fbe_pe_resume_prom_id_t rp_comp_index, 
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
        
        case FBE_PE_SPA_PS0_RESUME_PROM:  //SPID_DEFIANT_M1_HW_TYPE
            device_location->sp = SP_A;
            device_location->slot = 0;
            *device_type = FBE_DEVICE_TYPE_PS;
            break;

        case FBE_PE_SPB_PS0_RESUME_PROM:  //SPID_DEFIANT_M1_HW_TYPE
            device_location->sp = SP_B;
            device_location->slot = 1;
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
        case FBE_PE_SPA_PS1_RESUME_PROM:  //SPID_DEFIANT_M1_HW_TYPE
        case FBE_PE_SPB_PS1_RESUME_PROM:  //SPID_DEFIANT_M1_HW_TYPE
        default:
            *device_type = FBE_DEVICE_TYPE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}


/*!*************************************************************************
 *   @fn fbe_base_board_convert_resume_prom_id_to_smb_device(fbe_pe_resume_prom_id_t rp_comp_index,
 *                               fbe_device_physical_location_t *device_location, 
 *                               fbe_u64_t *device_type)
 **************************************************************************
 *  @brief
 *      This function will convert the resume prom id to the appropriate 
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
 *    22-Aug-2011: PHE - Created.
 *
 **************************************************************************/

static fbe_status_t winnie_the_pooh_convert_resume_prom_id_to_smb_device(fbe_pe_resume_prom_id_t resumePromId, 
                                                         SMB_DEVICE *smb_device)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(resumePromId)
    {
        case FBE_PE_MIDPLANE_RESUME_PROM:
            *smb_device = MIDPLANE_RESUME_PROM;
            break;

        case FBE_PE_SPA_RESUME_PROM:
            *smb_device = SPA_SP_RESUME_PROM;
            break;

        case FBE_PE_SPB_RESUME_PROM:
            *smb_device = SPB_SP_RESUME_PROM;
            break;
        
        case FBE_PE_SPA_PS0_RESUME_PROM:
            *smb_device = SPA_PS0_RESUME_PROM;
            break;

        case FBE_PE_SPA_PS1_RESUME_PROM:
            *smb_device = SPA_PS1_RESUME_PROM;
            break;

        case FBE_PE_SPB_PS0_RESUME_PROM:
            *smb_device = SPB_PS0_RESUME_PROM;
            break;

        case FBE_PE_SPB_PS1_RESUME_PROM:
            *smb_device = SPB_PS1_RESUME_PROM;
            break;
        
        case FBE_PE_SPA_MGMT_RESUME_PROM:
            *smb_device = SPA_MGMT_RESUME_PROM;
            break;
    
        case FBE_PE_SPB_MGMT_RESUME_PROM:
            *smb_device = SPB_MGMT_RESUME_PROM;
            break;
        
        case FBE_PE_SPA_IO_ANNEX_RESUME_PROM:
            *smb_device = SPA_BEM_RESUME_PROM;
            break;

        case FBE_PE_SPB_IO_ANNEX_RESUME_PROM:
            *smb_device = SPB_BEM_RESUME_PROM;
            break;
        
        case FBE_PE_SPA_IO_SLOT0_RESUME_PROM:
            *smb_device = SPA_IOSLOT_0_RESUME_PROM;
            break;

        case FBE_PE_SPB_IO_SLOT0_RESUME_PROM:
            *smb_device = SPB_IOSLOT_0_RESUME_PROM;
            break;

        case FBE_PE_SPA_IO_SLOT1_RESUME_PROM:
            *smb_device = SPA_IOSLOT_1_RESUME_PROM;
            break;

        case FBE_PE_SPB_IO_SLOT1_RESUME_PROM:
            *smb_device = SPB_IOSLOT_1_RESUME_PROM;
            break;

        case FBE_PE_SPA_IO_SLOT2_RESUME_PROM:
            *smb_device = SPA_IOSLOT_2_RESUME_PROM;
            break;

        case FBE_PE_SPB_IO_SLOT2_RESUME_PROM:
            *smb_device = SPB_IOSLOT_2_RESUME_PROM;
            break;

        case FBE_PE_SPA_IO_SLOT3_RESUME_PROM:
            *smb_device = SPA_IOSLOT_3_RESUME_PROM;
            break;

        case FBE_PE_SPB_IO_SLOT3_RESUME_PROM:
            *smb_device = SPB_IOSLOT_3_RESUME_PROM;
            break;

        case FBE_PE_SPA_IO_SLOT4_RESUME_PROM:
            *smb_device = SPA_IOSLOT_4_RESUME_PROM;
            break;

        case FBE_PE_SPB_IO_SLOT4_RESUME_PROM:
            *smb_device = SPB_IOSLOT_4_RESUME_PROM;
            break;

        case FBE_PE_SPA_IO_SLOT5_RESUME_PROM:
            *smb_device = SPA_IOSLOT_5_RESUME_PROM;
            break;

        case FBE_PE_SPB_IO_SLOT5_RESUME_PROM:
            *smb_device = SPB_IOSLOT_5_RESUME_PROM;
            break;
        
        case FBE_PE_SPA_IO_SLOT6_RESUME_PROM:
            *smb_device = SPA_IOSLOT_6_RESUME_PROM;
            break;
    
        case FBE_PE_SPB_IO_SLOT6_RESUME_PROM:
            *smb_device = SPB_IOSLOT_6_RESUME_PROM;
            break;
    
        case FBE_PE_SPA_IO_SLOT7_RESUME_PROM:
            *smb_device = SPA_IOSLOT_7_RESUME_PROM;
            break;
    
        case FBE_PE_SPB_IO_SLOT7_RESUME_PROM:
            *smb_device = SPB_IOSLOT_7_RESUME_PROM;
            break;
    
        case FBE_PE_SPA_IO_SLOT8_RESUME_PROM:
            *smb_device = SPA_IOSLOT_8_RESUME_PROM;
            break;
    
        case FBE_PE_SPB_IO_SLOT8_RESUME_PROM:
            *smb_device = SPB_IOSLOT_8_RESUME_PROM;
            break;
    
        case FBE_PE_SPA_IO_SLOT9_RESUME_PROM:
            *smb_device = SPA_IOSLOT_9_RESUME_PROM;
            break;
    
        case FBE_PE_SPB_IO_SLOT9_RESUME_PROM:
            *smb_device = SPB_IOSLOT_9_RESUME_PROM;
            break;
    
        case FBE_PE_SPA_IO_SLOT10_RESUME_PROM:
            *smb_device = SPA_IOSLOT_10_RESUME_PROM;
            break;
    
        case FBE_PE_SPB_IO_SLOT10_RESUME_PROM:
            *smb_device = SPB_IOSLOT_10_RESUME_PROM;
            break;
    
        case FBE_PE_SPA_MEZZANINE_RESUME_PROM:
            *smb_device = SPA_MEZZANINE_RESUME_PROM;
            break;

        case FBE_PE_SPB_MEZZANINE_RESUME_PROM:
            *smb_device = SPB_MEZZANINE_RESUME_PROM;
            break;
        
        //case FBE_PE_CACHE_CARD_RESUME_PROM:
        default:
            *smb_device = SMB_DEVICE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}
