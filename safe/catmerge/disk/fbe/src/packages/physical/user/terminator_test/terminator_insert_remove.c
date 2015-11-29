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


void terminator_insert_remove(void)
{
    fbe_status_t 						status;
    fbe_terminator_board_info_t 		board_info;
    fbe_terminator_sas_port_info_t		sas_port;
    fbe_terminator_sas_encl_info_t		sas_encl;
    fbe_terminator_api_device_handle_t	port_handle;
    fbe_terminator_api_device_handle_t	encl_id1, drive_id;
    fbe_terminator_sas_drive_info_t 	sas_drive;
    
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
    
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
    
    /* enumerate the ports */
    status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /*insert an enclosure*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id1);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    
    /*insert a drive to the enclosure in slot 0*/
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
	sas_drive.slot_number = 0;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;

    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456780;
    memset(&sas_drive.drive_serial_number, '1' , sizeof(sas_drive.drive_serial_number));
    sas_drive.capacity = 0x2000000;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (encl_id1, 0, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);
    status = physical_package_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    EmcutilSleep(20000); /* Sleep 20 sec. to give a chance to monitor */

	/*remove the drive*/
    status = fbe_terminator_api_remove_sas_drive (drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    EmcutilSleep(10000); /* Sleep 5 sec. to give drive remoal thead and logout messages to pass through*/
    
    /*re - insert the drive to the enclosure in slot 0*/
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
	sas_drive.slot_number = 0;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;

    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456781;
    memset(&sas_drive.drive_serial_number, '2' , sizeof(sas_drive.drive_serial_number));
    status = fbe_terminator_api_insert_sas_drive (encl_id1, 0, &sas_drive, &drive_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    EmcutilSleep(10000); /* Sleep 5 sec. to give drive remoal thead and logout messages to pass through*/
    
    /*remove an enclosure*/
    status  = fbe_terminator_api_remove_sas_enclosure (encl_id1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    EmcutilSleep(10000); /* Sleep 10 sec. to give a chance to monitor */
    
    /*re-insert an enclosure*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    sas_encl.sas_address = (fbe_u64_t)0x1234567FF;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id1);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    EmcutilSleep(10000); /* Sleep 10 sec. to give a chance to monitor */
    
    /*remove an enclosure*/
    status  = fbe_terminator_api_remove_sas_enclosure (encl_id1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    EmcutilSleep(10000); /* Sleep 10 sec. to give a chance to monitor */
    
    status =  physical_package_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);

}


void terminator_load_config_file_test(void)
{
	 fbe_status_t 					status;

	/*before loading the physical package we initialize the terminator*/
	status = fbe_terminator_api_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

//    /*load the config file*/
//    fbe_terminator_api_load_config_file(".", "maui.xml");
//
//    /*unload and clear memory*/
//    fbe_terminator_api_unload_config_file();
	status = fbe_terminator_api_destroy();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);


}
