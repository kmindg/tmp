#include "terminator.h" /* Each MUT testsuite should include this header */
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


void terminator_init_destroy(void)
{
	fbe_status_t status;
    fbe_terminator_board_info_t board_info;
	fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_api_device_handle_t port_handle;

	/*before loading the physical package we initialize the terminator*/
	status = fbe_terminator_api_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

    /* now setup the board */
    board_info.board_type = FBE_BOARD_TYPE_DELL;
	status  = fbe_terminator_api_insert_board (&board_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*insert a port*/
	sas_port.sas_address = (fbe_u64_t)0x22222222;
	sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
	status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    status = physical_package_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: physical package initialized.", __FUNCTION__);
   	EmcutilSleep(10000); /* Sleep 10 sec. to give a chance to monitor */

	status =  physical_package_destroy();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: physical package destroyed.", __FUNCTION__);

	status = fbe_terminator_api_destroy();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);

}

