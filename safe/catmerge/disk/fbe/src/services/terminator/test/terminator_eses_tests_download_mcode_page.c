/***************************************************************************
 *  terminator_eses_tests_download_mcode_page.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure download microcode page realated tests.
 *      
 *
 *  History:
 *     Nov-21-08   Rajesh V. Created
 *    
 ***************************************************************************/

#include "terminator_test.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"

#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "terminator_eses_test.h"

#define LOCAL_LCC_SUBENCL_ID 0
#define PEER_LCC_SUBENCL_ID 1
#define CHASSIS_SUBENCL_ID 2
#define LOCAL_PS_SUBENCL_ID 3
#define PEER_PS_SUBENCL_ID 4

/**********************************/
/*        local variables        */
/**********************************/
typedef struct download_mcode_test_io_completion_context_s
{
    fbe_u8_t encl_id;
    fbe_download_status_desc_t download_status_desc;
}download_mcode_test_io_completion_context_t;

static download_mcode_test_io_completion_context_t download_mcode_test_io_completion_context;
static fbe_semaphore_t   download_mcode_test_continue_semaphore;

/**********************************/
/*       LOCAL FUNCTIONS          */
/**********************************/
static fbe_status_t terminator_eses_tests_download_mcode_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
static fbe_status_t terminator_eses_tests_download_mcode_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);
static void init_download_mcode_test_io_completion_context(void);


void terminator_download_mcode_page_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t        encl_id;
    fbe_terminator_api_device_handle_t        drive_id;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t  port_index;
    fbe_download_status_desc_t download_stat_desc;
    fbe_u8_t subencl_id = 4;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    fbe_semaphore_init(&download_mcode_test_continue_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_download_mcode_test_io_completion_context();

     /*initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x111111111;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_terminator_api_insert_sas_port (0, &sas_port);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, 
                                                           terminator_eses_tests_download_mcode_callback, 
                                                           NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*insert an enclosure*/
    // id 2
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure*/
    // id 4
    sas_encl.uid[0] = 4; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*insert a sas_drive to the second last enclosure*/
    // inserted in encl id 4, in slot 0.
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id, 0, &sas_drive, &drive_id);/*add drive to port 0 encl 3*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);



	/* now lets verify the status */
	status = fbe_terminator_miniport_api_register_payload_completion(0,
																    terminator_eses_tests_download_mcode_io_completion,
																    &download_mcode_test_io_completion_context);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   


    status = fbe_terminator_api_eses_get_subencl_id(0, 4, SES_SUBENCL_TYPE_LCC, 0, &subencl_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(LOCAL_LCC_SUBENCL_ID, subencl_id);

    status = fbe_terminator_api_eses_get_subencl_id(0, 4, SES_SUBENCL_TYPE_LCC, 1, &subencl_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(PEER_LCC_SUBENCL_ID, subencl_id);

    status = fbe_terminator_api_eses_get_subencl_id(0, 2, SES_SUBENCL_TYPE_PS, 0, &subencl_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(LOCAL_PS_SUBENCL_ID, subencl_id);

    status = fbe_terminator_api_eses_get_subencl_id(0, 2, SES_SUBENCL_TYPE_PS, 1, &subencl_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(PEER_PS_SUBENCL_ID, subencl_id);

    status = fbe_terminator_api_eses_get_subencl_id(0, 4, SES_SUBENCL_TYPE_CHASSIS, 0x7f, &subencl_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(CHASSIS_SUBENCL_ID, subencl_id);
  
    download_mcode_test_io_completion_context.download_status_desc.subencl_id = 0;
    download_mcode_test_io_completion_context.download_status_desc.status = 0x7;
    download_mcode_test_io_completion_context.download_status_desc.addl_status = 0x3;
    download_mcode_test_io_completion_context.encl_id = 4; 
    memcpy(&download_stat_desc,
           &download_mcode_test_io_completion_context.download_status_desc,
           sizeof(fbe_download_status_desc_t));
    // User terminator API to set the values accordingly.
    status = fbe_terminator_api_eses_set_download_microcode_stat_page_stat_desc(0, 4, download_stat_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    // issue request for the microcode download page.
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&download_mcode_test_continue_semaphore, NULL);

    download_mcode_test_io_completion_context.download_status_desc.subencl_id = 2;
    download_mcode_test_io_completion_context.download_status_desc.status = 0x6;
    download_mcode_test_io_completion_context.download_status_desc.addl_status = 0x9;
    download_mcode_test_io_completion_context.encl_id = 2; 
    memcpy(&download_stat_desc,
           &download_mcode_test_io_completion_context.download_status_desc,
           sizeof(fbe_download_status_desc_t));
    // User terminator API to set the values accordingly.
    status = fbe_terminator_api_eses_set_download_microcode_stat_page_stat_desc(0, 2, download_stat_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);
    // Issue eses request for the micro code download page.
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_download_microcode_stat, sg_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_wait(&download_mcode_test_continue_semaphore, NULL);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);

}

void init_download_mcode_test_io_completion_context(void)
{
    memset(&download_mcode_test_io_completion_context, 0, sizeof(download_mcode_test_io_completion_context_t));
}

static fbe_status_t terminator_eses_tests_download_mcode_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    return FBE_STATUS_OK;
}


static fbe_status_t terminator_eses_tests_download_mcode_io_completion (fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_cdb_operation_t *payload_cdb_operation = NULL;
    fbe_u8_t *cdb = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u8_t *sense_info_buffer = NULL;
    fbe_eses_download_status_page_t *download_mcode_page_hdr = NULL;
    fbe_u8_t *return_page_code = NULL;
    fbe_u8_t encl_id = ((download_mcode_test_io_completion_context_t *)context)->encl_id;
    fbe_download_status_desc_t *download_status_desc_expected = 
        &((download_mcode_test_io_completion_context_t *)context)->download_status_desc;

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);   
    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);
    fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_info_buffer);
    MUT_ASSERT_TRUE(sense_info_buffer != NULL);
    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    if (cdb[0] == FBE_SCSI_RECEIVE_DIAGNOSTIC)
    {   
        return_page_code = (fbe_u8_t *)fbe_sg_element_address(sg_list);
        if(*return_page_code == SES_PG_CODE_DOWNLOAD_MICROCODE_STAT)
        {
            download_mcode_page_hdr = (fbe_eses_download_status_page_t *)fbe_sg_element_address(sg_list);
            MUT_ASSERT_INT_EQUAL(download_status_desc_expected->addl_status,
                                 download_mcode_page_hdr->dl_status.addl_status);
            MUT_ASSERT_INT_EQUAL(download_status_desc_expected->status,
                                 download_mcode_page_hdr->dl_status.status);
            MUT_ASSERT_INT_EQUAL(download_status_desc_expected->subencl_id,
                                 download_mcode_page_hdr->dl_status.subencl_id);
        }
        else
        {
            MUT_ASSERT_TRUE(1);
        }
        
    }
    else
    {
        MUT_ASSERT_TRUE(1);
    }

    fbe_semaphore_release(&download_mcode_test_continue_semaphore, 0, 1, FALSE);
    fbe_ldo_test_free_memory_with_sg(&sg_list);
    return(FBE_STATUS_OK);
}
