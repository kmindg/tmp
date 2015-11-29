#include "configuration_test.h"
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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "EmcPAL_Memory.h"

static fbe_u8_t file_name[255]=EmcpalEmpty;

void config_test_set_file_name(fbe_u8_t *name)
{
	memset (file_name, 0, 255);
	memcpy (file_name, name, strlen (name));

}
void config_file_test(void)
{
    fbe_status_t 					status;
	fbe_u32_t						x = 0;

	EmcutilSleep(1000);/*let the file check thread start nicely and not interrupt our printout*/

	/* !!! Re-enable the getchar if you want to interactively change the xml
	 * and see how physical package and terminator responds 
	 */
	//KvTrace("********************************************************************************\n");
	//KvTrace("*               Press <ENTER> to load the physical package                     *\n");
	//KvTrace("*       At this time, you can manipulate the XML file and see the changes      *\n");
	//KvTrace("*           To shut down the physical packge, press <ENTER> again              *\n");
	//KvTrace("********************************************************************************\n\n");

	///*wait for ue input*/
	//getchar();
	status = physical_package_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	///*wait for ue input*/
	//getchar();
}


void config_file_test_init(void)
{
	 fbe_status_t 					status;

	/*before loading the physical package we initialize the terminator*/
	status = fbe_terminator_api_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

    /*load the config file*/
	if (strlen (file_name) == 0) {
        fbe_terminator_api_load_config_file(".", "maui.xml");
	}else {
        fbe_terminator_api_load_config_file(".", file_name);
	}
}

void config_file_test_destroy(void)
{
	 fbe_status_t 					status;

    status =  physical_package_destroy();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*unload and clear memory*/
	fbe_terminator_api_unload_config_file();
	status = fbe_terminator_api_destroy();
	
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}
