#include "fbe/fbe_winddk.h"
#include "fbe_trace.h"
#include "fbe/fbe_esp.h"

__cdecl main (int argc , char ** argv)
{
    	fbe_status_t status;
	printf("%s %s \n",__FILE__, __FUNCTION__);
	
	/* Every test may change the default trace level before physical_package_init call */
	fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

	status = esp_init();
	EmcutilSleep(300000000); /* Sleep 30 sec. to give a chance to monitor */
	status =  esp_destroy();
	printf("PASS\n");
	exit(0);
	return 0;
}


