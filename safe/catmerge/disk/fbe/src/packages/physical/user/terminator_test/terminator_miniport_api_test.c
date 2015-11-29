#include "terminator.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_port.h"
/* #include "fbe/fbe_lcc.h" */
#include "fbe/fbe_physical_drive.h"


#include "fbe_trace.h"
#include "fbe_transport_memory.h"

#include "fbe_service_manager.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_base_object.h"
#include "fbe_scsi.h"
#include "fbe_topology.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_sas.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe_cpd_shim.h"
#include "fbe_test_io_api.h"


static fbe_status_t terminator_miniport_api_test_cpd_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);
//static fbe_status_t terminator_miniport_api_test_cpd_io_complete (struct fbe_io_block_s * io_block, fbe_io_block_completion_context_t context);
static fbe_status_t terminator_miniport_api_test_issue_write (void);
static fbe_status_t terminator_miniport_api_test_issue_read (void);
static fbe_status_t terminator_miniport_api_test_issue_inquiry (void);
//static fbe_status_t
//io_block_build_receive_diagnostic(fbe_io_block_t * io_block, fbe_cpd_device_id_t cpd_device_id);
static fbe_status_t terminator_miniport_api_test_issue_phy_receive_diagnostic (void);
//static fbe_status_t terminator_miniport_api_test_PHY_io_complete (
    //struct fbe_io_block_s * io_block, fbe_io_block_completion_context_t context);
static  void port_reset_test_init_callback_structs(void);
static  void port_reset_test_init_structs(void);
static fbe_status_t terminator_miniport_api_port_reset_callback(
    fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);

// Remove this after wards..
static fbe_status_t terminator_miniport_api_test_issue_encl_inquiry (void);

//fbe_status_t io_block_build_inquiry(fbe_io_block_t * io_block, fbe_cpd_device_id_t cpd_device_id, fbe_time_t timeout);
//fbe_status_t
//fbe_transport_build_block_execute_cdb(fbe_io_block_t *const io_block_p,
//                                     fbe_cpd_device_id_t cpd_device_id,
//                                     fbe_time_t timeout,
//                                     fbe_payload_block_operation_opcode_t opcode,
//                                     fbe_lba_t lba,
//                                     fbe_block_count_t blocks,
//                                     fbe_block_size_t block_size);
//

/**********************************/
/*        local variables         */
/**********************************/
static fbe_semaphore_t      update_semaphore;
static fbe_u32_t port_reset_callback_logins;
static fbe_u32_t port_reset_callback_logouts;
static fbe_u32_t port_reset_callback_reset_start;
static fbe_u32_t port_reset_callback_reset_completed;
// one for each port. now only using port0
static fbe_u32_t port_reset_no_logouts[10];
static fbe_u32_t port_reset_no_logins[10];
static fbe_u32_t port_reset_no_encls[10];
static fbe_u32_t port_reset_no_drives[10];
static fbe_u32_t port_reset_no_vphys[10];
static fbe_u32_t port_reset_no_ports;





void terminator_miniport_api_test_init(void)
{
	fbe_status_t 					status;
	fbe_terminator_board_info_t 	board_info;
	fbe_terminator_sas_port_info_t	sas_port;
	fbe_terminator_sas_encl_info_t	sas_encl;
        fbe_terminator_api_device_handle_t port_handle;
        fbe_terminator_api_device_handle_t encl_id, drive_id;
    fbe_terminator_sas_drive_info_t sas_drive;

	/*before loading the physical package we initialize the terminator*/
	status = fbe_terminator_api_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

	/* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
	board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
	status  = fbe_terminator_api_insert_board (&board_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*insert a port*/
	sas_port.sas_address = (fbe_u64_t)0x123456789;
	sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;

	status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert a port*/
	sas_port.sas_address = (fbe_u64_t)0x22222222;
	sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 1;
    sas_port.backend_number = 1;

	status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*insert an enclosure*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
	sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
	sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.connector_id = 0;
	status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id);/*add encl on port 0*/
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	
	/*insert a drive to the enclosure in slot 0*/
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
	sas_drive.slot_number = 0;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
	sas_drive.sas_address = (fbe_u64_t)0x443456780;
	status = fbe_terminator_api_insert_sas_drive (encl_id, 0, &sas_drive, &drive_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
	mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

}

void terminator_miniport_api_test_destroy(void)
{
	fbe_status_t 					status;

	status = fbe_terminator_api_destroy();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);

}

void terminator_miniport_api_test_enumerate_ports(void)
{
	fbe_status_t 								status = FBE_STATUS_GENERIC_FAILURE;
	fbe_cpd_shim_enumerate_backend_ports_t  	cpd_shim_enumerate_backend_ports;

	status = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*check a null pointer*/
	status = fbe_terminator_miniport_api_enumerate_cpd_ports (NULL);
	MUT_ASSERT_FALSE(status == FBE_STATUS_OK);

}

void terminator_miniport_api_test_register_async (void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t	    context = 0;
    fbe_u32_t       port_index;

    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*regular good registration*/
    status =  fbe_terminator_miniport_api_register_callback(port_index,
                                                           terminator_miniport_api_test_cpd_callback,
                                                           (void*)&context);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*bad registration*/
    status =  fbe_terminator_miniport_api_register_callback(port_index,
                                                           NULL,
                                                           (void*)&context);
    MUT_ASSERT_FALSE(status == FBE_STATUS_OK);

    /*bad registration*/
    status =  fbe_terminator_miniport_api_register_callback(0xdfee,
                                                           terminator_miniport_api_test_cpd_callback,
                                                           (void*)&context);
    MUT_ASSERT_FALSE(status == FBE_STATUS_OK);


}

//void terminator_miniport_api_test_register_completion (void)
//{
//    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
//    fbe_u32_t       context = 0;
//
//    fbe_u32_t       port_index;
//
//    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    /*regular good registration*/
//    status =  fbe_terminator_miniport_api_register_io_completion(port_index,
//                                                                 terminator_miniport_api_test_cpd_io_complete,
//                                                                 (void*)&context);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    /*bad registration*/
//    status =  fbe_terminator_miniport_api_register_io_completion(port_index,
//                                                                 NULL,
//                                                                 (void*)&context);
//    MUT_ASSERT_FALSE(status == FBE_STATUS_OK);
//
//    /*bad registration*/
//    status =  fbe_terminator_miniport_api_register_io_completion(0xdfee,
//                                                                 terminator_miniport_api_test_cpd_io_complete,
//                                                                 (void*)&context);
//    MUT_ASSERT_FALSE(status == FBE_STATUS_OK);
//
//
//}

static fbe_status_t terminator_miniport_api_test_cpd_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    return FBE_STATUS_OK;
}

//fbe_status_t terminator_miniport_api_test_cpd_io_complete (struct fbe_io_block_s * io_block, fbe_io_block_completion_context_t context)
//{
//    KvTrace("%s: opcode:0x%x,return_status: %d\n", __FUNCTION__, io_block->operation.execute_cdb.cdb[0], io_block->operation.execute_cdb.io_block_request_status);
//    fbe_test_io_free_memory_with_sg(&io_block->sg_list);
//    free(io_block);
//    return FBE_STATUS_OK;
//}


void terminator_miniport_api_test_get_port_type (void)
{
	fbe_status_t		status = FBE_STATUS_GENERIC_FAILURE;
	fbe_port_type_t		port_type;

    fbe_u32_t       port_index;

    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*test good conditions*/
	status = fbe_terminator_miniport_api_get_port_type (port_index, &port_type);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*test bad conditions*/
	status = fbe_terminator_miniport_api_get_port_type (0xdead, &port_type);
	MUT_ASSERT_FALSE(status == FBE_STATUS_OK);

	/*test bad conditions*/
	status = fbe_terminator_miniport_api_get_port_type (port_index, NULL);
	MUT_ASSERT_FALSE(status == FBE_STATUS_OK);
}

void terminator_miniport_api_test_get_port_info (void)
{
	fbe_status_t		status = FBE_STATUS_GENERIC_FAILURE;
	fbe_port_info_t 	port_info;
    fbe_u32_t       port_index;

    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	/*test good conditions*/
	status = fbe_terminator_miniport_api_get_port_info (port_index, &port_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*test bad conditions*/
	status = fbe_terminator_miniport_api_get_port_info (0xdead, &port_info);
	MUT_ASSERT_FALSE(status == FBE_STATUS_OK);

	/*test bad conditions*/
	status = fbe_terminator_miniport_api_get_port_info (port_index, NULL);
	MUT_ASSERT_FALSE(status == FBE_STATUS_OK);

	/*check value goes through*/
	fbe_terminator_miniport_api_set_speed (port_index, FBE_PORT_SPEED_SAS_6_GBPS);
	status = fbe_terminator_miniport_api_get_port_info (port_index, &port_info);
	MUT_ASSERT_TRUE(port_info.link_speed == FBE_PORT_SPEED_SAS_6_GBPS);

}

//void terminator_miniport_api_test_generate_drive_io (void)
//{
//    fbe_status_t status = FBE_STATUS_OK;
//    fbe_u32_t       context = 0;
//    fbe_u32_t       port_index;
//
//    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    status =  fbe_terminator_miniport_api_register_io_completion(port_index,
//                                                                 terminator_miniport_api_test_cpd_io_complete,
//                                                                 (void*)&context);
//
//
//    status = terminator_miniport_api_test_issue_encl_inquiry();
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    status = terminator_miniport_api_test_issue_write ();
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    status = terminator_miniport_api_test_issue_read();
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    status = terminator_miniport_api_test_issue_inquiry();
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//}

//fbe_status_t terminator_miniport_api_test_issue_write (void)
//{
//    fbe_status_t status = FBE_STATUS_OK;
//    fbe_sg_element_t *sg_p = NULL;
//    fbe_io_block_t *io_block;
//    fbe_u32_t       port_index;
//
//    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    io_block =(fbe_io_block_t *)malloc(sizeof(fbe_io_block_t));
//    //io_block = io_block_new();
//
//
//    sg_p = fbe_test_io_alloc_memory_with_sg(520);
//    if (sg_p == NULL)
//    {
//        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
//        return status;
//    }
//    /* First, setup the io block for a logical read.
//     */
//    fbe_transport_build_block_execute_cdb(io_block, 4, 1, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
//        0x100, 0x1, 520);
//    io_block->operation.execute_cdb.port_number = 0;
//    io_block->sg_list = sg_p;
//
//    status = fbe_terminator_miniport_api_send_io(port_index, io_block);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    //fbe_ldo_test_free_memory_with_sg(&sg_p);
//    return(status);
//}

//fbe_status_t terminator_miniport_api_test_issue_read (void)
//{
//    fbe_status_t status = FBE_STATUS_OK;
//    fbe_sg_element_t *sg_p = NULL;
//    fbe_io_block_t *io_block;
//    fbe_u32_t       port_index;
//
//    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    io_block =(fbe_io_block_t *)malloc(sizeof(fbe_io_block_t));
//    //io_block = io_block_new();
//
//
//    sg_p = fbe_test_io_alloc_memory_with_sg(520);
//    if (sg_p == NULL)
//    {
//        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
//        return status;
//    }
//    /* First, setup the io block for a logical read.
//     */
//    fbe_transport_build_block_execute_cdb(io_block, 4, 1, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
//        0x100, 0x1, 520);
//    io_block->operation.execute_cdb.port_number = 0;
//    io_block->sg_list = sg_p;
//
//    status = fbe_terminator_miniport_api_send_io(port_index, io_block);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    //fbe_ldo_test_free_memory_with_sg(&sg_p);
//    return(status);
//}

//fbe_status_t terminator_miniport_api_test_issue_inquiry(void)
//{
//    fbe_status_t status = FBE_STATUS_OK;
//    fbe_sg_element_t *sg_p = NULL;
//    fbe_io_block_t *io_block;
//    fbe_u32_t       port_index;
//
//    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    io_block =(fbe_io_block_t *)malloc(sizeof(fbe_io_block_t));
//    //io_block = io_block_new();
//
//
//    sg_p = fbe_test_io_alloc_memory_with_sg(520);
//    if (sg_p == NULL)
//    {
//        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
//        return status;
//    }
//    /* First, setup the io block for a logical read.
//     */
//    io_block_build_inquiry(io_block, 4, 1);
//    io_block->operation.execute_cdb.port_number = 0;
//    io_block->sg_list = sg_p;
//
//    status = fbe_terminator_miniport_api_send_io(port_index, io_block);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    //fbe_ldo_test_free_memory_with_sg(&sg_p);
//    return(status);
//}

fbe_status_t terminator_miniport_api_test_issue_encl_inquiry(void)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;
    fbe_u32_t       port_index;

    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);

    payload = (fbe_payload_ex_t *)malloc(sizeof(fbe_payload_ex_t));;
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    sg_list = fbe_test_io_alloc_memory_with_sg(520);
    sg_list[0].count = FBE_SCSI_INQUIRY_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    fbe_payload_cdb_build_inquiry(payload_cdb_operation, 1);

    status = fbe_terminator_miniport_api_send_payload(port_index, 2, payload);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //fbe_ldo_test_free_memory_with_sg(&sg_p);
    return(status);  
}

//void terminator_miniport_api_test_generate_phy_io (void)
//{
//    fbe_status_t status = FBE_STATUS_OK;
//    fbe_u32_t       context = 0;
//    fbe_u32_t       port_index;
//
//    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    status =  fbe_terminator_miniport_api_register_io_completion(port_index,
//                                                                 terminator_miniport_api_test_PHY_io_complete,
//                                                                 (void*)&context);
//
//    status = terminator_miniport_api_test_issue_phy_receive_diagnostic();
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//}

//fbe_status_t terminator_miniport_api_test_issue_phy_receive_diagnostic (void)
//{
//    fbe_status_t status = FBE_STATUS_OK;
//    fbe_sg_element_t *sg_p = NULL;
//    fbe_io_block_t *io_block;
//
//    io_block =(fbe_io_block_t *)malloc(sizeof(fbe_io_block_t));
//    //io_block = io_block_new();
//
//
//    sg_p = fbe_test_io_alloc_memory_with_sg(520);
//    if (sg_p == NULL)
//    {
//        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
//        return status;
//    }
//    /* First, setup the io block.
//     */
//    io_block_build_receive_diagnostic(io_block, 3);
//    io_block->operation.execute_cdb.port_number = 0;
//    io_block->sg_list = sg_p;
//
//    status = fbe_terminator_miniport_api_send_io(0, io_block);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    //fbe_ldo_test_free_memory_with_sg(&sg_p);
//    return(status);
//
//}

//fbe_status_t
//io_block_build_receive_diagnostic(fbe_io_block_t * io_block, fbe_cpd_device_id_t cpd_device_id)
//{
//    fbe_eses_receive_diagnostic_results_cdb_t *cdb_page;
//
//    io_block->io_block_opcode = FBE_IO_BLOCK_OPCODE_EXECUTE_CDB;
//    io_block->operation.execute_cdb.cpd_device_id = cpd_device_id;
//
//    cdb_page = (fbe_eses_receive_diagnostic_results_cdb_t *)(io_block->operation.execute_cdb.cdb);
//    cdb_page->page_code_valid = 1;
//    cdb_page->control = 0;
//    cdb_page->operation_code = FBE_SCSI_RECEIVE_DIAGNOSTIC;
//    cdb_page->page_code = SES_PG_CODE_ENCL_STAT;
//
//    return FBE_STATUS_OK;
//}


//fbe_status_t terminator_miniport_api_test_PHY_io_complete (struct fbe_io_block_s * io_block, fbe_io_block_completion_context_t context)
//{
//    ses_pg_encl_stat_struct  *encl_status_diag_page_hdr = NULL;
//    ses_stat_elem_array_dev_slot_struct *array_dev_slot_stat_ptr = NULL;
//    fbe_u32_t i=0;
//
//    KvTrace("%s: opcode:0x%x,return_status: %d\n", __FUNCTION__, io_block->operation.execute_cdb.cdb[0], io_block->operation.execute_cdb.io_block_request_status);
//    encl_status_diag_page_hdr = (ses_pg_encl_stat_struct *)fbe_sg_element_address(io_block->sg_list);
//    KvTrace("%s: diagnostic page code returned ->0x%x\n", __FUNCTION__, encl_status_diag_page_hdr->pg_code);
//    array_dev_slot_stat_ptr = (ses_stat_elem_array_dev_slot_struct *) (((fbe_u8_t *)encl_status_diag_page_hdr) + FBE_ESES_PAGE_HEADER_SIZE);
//
//    KvTrace("%s: overall status element has ELEMENT STAT CODE: 0x%x\n", __FUNCTION__,
//            array_dev_slot_stat_ptr->cmn_stat.elem_stat_code);
//    for(i=0; i<15; i++)
//    {
//        array_dev_slot_stat_ptr++;
//        KvTrace("%s: Individual status element i.e. phy slot num:%d has ELEMENT STAT CODE 0x%x\n", __FUNCTION__, i,
//            array_dev_slot_stat_ptr->cmn_stat.elem_stat_code);
//    }
//    fbe_test_io_free_memory_with_sg(&io_block->sg_list);
//    free(io_block);
//    return FBE_STATUS_OK;
//}
//
void terminator_miniport_api_test_port_reset(void)
{
    //First do some insertions to terminator just like miniport_init
    //But the difference is we need to keep track of each object.

    fbe_status_t 					status;
    fbe_terminator_board_info_t 	board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_terminator_api_device_handle_t  port_handle;
    fbe_terminator_api_device_handle_t  encl_id, drive_id;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_u32_t		                context = 0;
    fbe_u32_t       port_index;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);
	fbe_semaphore_init(&update_semaphore, 0, FBE_SEMAPHORE_MAX);    
    port_reset_test_init_structs();

	/*before loading the physical package we initialize the terminator*/
	status = fbe_terminator_api_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);
    
	/* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
	board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
	status  = fbe_terminator_api_insert_board (&board_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    

    /*insert a port*/
    /* id =0 */
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
	
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_ports++;

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x22222222;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 1;
    sas_port.backend_number = 1;
	
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_ports++;

    /*insert first enclosure */
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_encls[0]++;
    port_reset_no_vphys[0]++;

	
    /*insert a drive to the first enclosure in slot 0*/
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 0;
	sas_drive.slot_number = 0;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456780;
    status = fbe_terminator_api_insert_sas_drive (encl_id, 0, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_drives[0]++;
    
    /*insert second enclosure*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 1;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.sas_address = (fbe_u64_t)0x123456781;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_encls[0]++;
    port_reset_no_vphys[0]++;

    /*insert a drive to the second enclosure in slot 0*/
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 1;
	sas_drive.slot_number = 0;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456781;
    status = fbe_terminator_api_insert_sas_drive (encl_id, 0, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_drives[0]++;

    /*insert third enclosure*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 3;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.uid[0] = 3; // some unique ID.
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_encls[0]++;
    port_reset_no_vphys[0]++;

    /*insert a drive to the third enclosure in slot 0*/
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 2;
	sas_drive.slot_number = 0;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456782;
    status = fbe_terminator_api_insert_sas_drive (encl_id, 0, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_drives[0]++;

    /*insert a drive to the third enclosure in slot 1*/
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 2;
	sas_drive.slot_number = 1;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456783;
    status = fbe_terminator_api_insert_sas_drive (encl_id, 1, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_drives[0]++;

    /*insert a drive to the third enclosure in slot 2*/
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 2;
	sas_drive.slot_number = 2;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456784;
    status = fbe_terminator_api_insert_sas_drive (encl_id, 2, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_drives[0]++;

    /*insert fourth enclosure*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 3;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.sas_address = (fbe_u64_t)0x123456783;
    sas_encl.uid[0] = 4; // some unique ID.
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_encls[0]++;
    port_reset_no_vphys[0]++;

    /*insert a drive to the fourth enclosure in slot 2*/
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 3;
	sas_drive.slot_number = 2;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456785;
    status = fbe_terminator_api_insert_sas_drive (encl_id, 2, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_drives[0]++;

    /*insert a drive to the fourth enclosure in slot 8*/
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 3;
	sas_drive.slot_number = 8;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456786;
    status = fbe_terminator_api_insert_sas_drive (encl_id, 8, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_drives[0]++;

    /*insert fifth enclosure*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 4;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.sas_address = (fbe_u64_t)0x123456784;
    sas_encl.uid[0] = 5; // some unique ID.
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id);/*add encl on port 1*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    port_reset_no_encls[0]++;
    port_reset_no_vphys[0]++;

    port_reset_no_logins[0] = port_reset_no_encls[0] +
        port_reset_no_drives[0]+ port_reset_no_vphys[0] ;

    port_reset_no_logouts[0] = port_reset_no_encls[0] +
        port_reset_no_drives[0] + port_reset_no_vphys[0];

    //Register Io completion function.
    status = fbe_terminator_miniport_api_port_init(1, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status =  fbe_terminator_miniport_api_register_callback(port_index,
               terminator_miniport_api_port_reset_callback,
               (void*)&context);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    //the login_all thread will be run during callback register. 
    // we dont callback to confuse our user generated logins with
    // the login thread logins. So sleep.
    mut_printf(MUT_LOG_HIGH, "Sleeping... for 5 secs\n ");
    EmcutilSleep(5000);

    /* MANUAL PORT RESET TEST START
     */
    port_reset_test_init_callback_structs();
    status = fbe_terminator_api_start_port_reset(port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    //wait till the reset begin processing is completed
    status = fbe_terminator_api_wait_on_port_reset_clear(port_handle,
        FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    //logout the port
    status = fbe_terminator_api_logout_all_devices_on_port(port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);  
    //wait till all logouts complete
    status = fbe_terminator_api_wait_port_logout_complete(port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    //start the reset complete
    status = fbe_terminator_api_complete_port_reset(port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);  
    //wait till the reset complete processing is completed
    status = fbe_terminator_api_wait_on_port_reset_clear(port_handle,
        FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    //login the port again
    status = fbe_terminator_api_login_all_devices_on_port(port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    //wait till the callback function gets all expected logouts
    //This will hang if not all expected logins and logouts are received.
    fbe_semaphore_wait(&update_semaphore, NULL);
    /* MANUAL PORT RESET TEST END
     */
    
    /* AUTO PORT RESET TEST START
     */
    port_reset_test_init_callback_structs();
    status = fbe_terminator_api_auto_port_reset(port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    //wait till the callback function gets all expected logouts
    //This will hang if not all expected logins and logouts are received.
    fbe_semaphore_wait(&update_semaphore, NULL);
    /* AUTO PORT RESET TEST END
     */
}

fbe_bool_t login_all_thread_events_start = FBE_FALSE;

static fbe_status_t terminator_miniport_api_port_reset_callback(
    fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{

    //First ignore all login events from the login_all thread.
    if((callback_info->callback_type == FBE_PMC_SHIM_CALLBACK_TYPE_PORT_UP) ||
       (callback_info->callback_type == FBE_PMC_SHIM_CALLBACK_TYPE_LINK_UP))
    {
        login_all_thread_events_start = FBE_TRUE;
        return(FBE_STATUS_OK);
    }
    
    // As the behaviour of the login_all thread is being changed
    // comment the code for now.
    /*
    if(login_all_thread_events_start)
    {
        switch(callback_info->callback_type)
        {
            case FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN:
                port_reset_callback_logins++;
                // Event thread will not do login for port object.
                if(port_reset_callback_logins == port_reset_no_logins[0])
                {
                     login_all_thread_events_start = FBE_FALSE;
					 fbe_semaphore_release(&update_semaphore, 0, 1, FALSE);
                }
                break;
            default:
                MUT_ASSERT_TRUE(1==1);
                break;
        }
        return(FBE_STATUS_OK);
    }
    // we should be done with the login all thread by now.  
    */
    switch(callback_info->callback_type)
    {
        //case FBE_PMC_SHIM_CALLBACK_TYPE_DRIVER_RESET:
        //    if(callback_info->info.driver_reset.driver_reset_event_type ==
        //        FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN)
        //    {
        //        MUT_ASSERT_TRUE(((port_reset_callback_logins==0) && 
        //                         (port_reset_callback_logouts== 0) && 
        //                         (port_reset_callback_reset_start==0) &&
        //                         (port_reset_callback_reset_completed==0)));
        //        port_reset_callback_reset_start = 1;
        //    }
        //    else if(callback_info->info.driver_reset.driver_reset_event_type ==
        //        FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED)
        //    {
        //        MUT_ASSERT_TRUE(((port_reset_callback_logins==0) && 
        //                         (port_reset_callback_logouts==port_reset_no_logouts[0]) && 
        //                         (port_reset_callback_reset_start==1) &&
        //                         (port_reset_callback_reset_completed==0)));
        //        port_reset_callback_reset_completed =1;
        //    }
        //    else
        //    {
        //        MUT_ASSERT_TRUE(1==1);
        //    }
        //    break;
        //case FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT:
        //    MUT_ASSERT_TRUE(((port_reset_callback_reset_start==1) &&
        //                     (port_reset_callback_reset_completed==0)));
        //    port_reset_callback_logouts++;
        //    break;
        //case FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN:
        //    MUT_ASSERT_TRUE(((port_reset_callback_logouts==port_reset_no_logouts[0]) && 
        //                     (port_reset_callback_reset_start==1) &&
        //                     (port_reset_callback_reset_completed==1)));
        //     port_reset_callback_logins++;
        //     if(port_reset_callback_logins == port_reset_no_logins[0])
        //     {
        //         //wake up the test thread
        //         fbe_semaphore_release(&update_semaphore, 0, 1, FALSE);
        //     }
        //    break;
        //default:
        //    MUT_ASSERT_TRUE(1==1);
        //    break;

    }
    return(FBE_STATUS_OK);
}

 void port_reset_test_init_callback_structs(void)
 {
   port_reset_callback_logins = 0;
   port_reset_callback_logouts = 0;
   port_reset_callback_reset_start = 0;
   port_reset_callback_reset_completed = 0;
 }

 void port_reset_test_init_structs(void)
 {
     fbe_u8_t i;

     port_reset_no_ports = 0;
     for(i=0;i<10;i++)
     {
        port_reset_no_logouts[i] = 0;
        port_reset_no_logins[i] = 0;
        port_reset_no_encls[i] = 0;
        port_reset_no_drives[i] = 0;
        port_reset_no_vphys[i] = 0;

     }
 }
