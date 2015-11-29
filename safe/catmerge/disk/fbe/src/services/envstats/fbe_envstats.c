
/*
 * fbe_envstats
 *
 * Environmental stats/metrics for the SP, DPE, and DAE.
 *
 * Initial code has only the metrics for the Storage Processor Temperature.
 * In future there will also be stats code for the DPE/DAE enclosures for power
 * supply input/output/average power and the enclosure temperatures.
 *
 */
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "generic_types.h"
#include "specl_types.h"
#include "specl_interface.h"
#include "fbe_envstats.h"

#ifdef C4_INTEGRATED
#include "stats_c_api.h"
#endif

#define DEF_META_WIDTH (-1)

fbe_status_t fbe_envstats_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_envstats_service_methods = {FBE_SERVICE_ID_ENVSTATS, 
						      fbe_envstats_control_entry};

static fbe_base_service_t            fbe_envstats_service;
static fbe_u32_t fbe_read_temp_trace_count; 
/* 
 * Metrics variables
 */
#ifdef C4_INTEGRATED
static FAMILY*  platform  = NULL;
FACT *spTemperature = NULL;
META *MetaSpTemperature;
#endif

void fbe_envstats_trace(fbe_trace_level_t trace_level, const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&fbe_envstats_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_envstats_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_ENVSTATS,
                     trace_level,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     fmt, 
                     args);
    va_end(args);
}

#ifdef C4_INTEGRATED

/*
 * getSPtemp
 *
 *  Returns the temperature for SP.
 */
static ps_ui32_t getSPtemp(void *p) 
{
    ps_ui32_t spaNewTemp = 0;

    Temperature tmp_data;
    tmp_data.TemperatureMSB = 0;
    tmp_data.TemperatureLSB = 0;


    if (getSpTemp(&tmp_data)) {
        spaNewTemp = (ps_ui32_t)(tmp_data.TemperatureMSB);
    } else {
        if (fbe_read_temp_trace_count == 0) {
            fbe_envstats_trace(FBE_TRACE_LEVEL_INFO, "Read SP temp failed\n");
	}
	if (fbe_read_temp_trace_count++ == FBE_RD_TEMP_TRACE_DELAY) {
	    fbe_read_temp_trace_count = 0;
	}
    }

    return spaNewTemp;
}

int create_platform_family (void) 
{

   int rc;

    /*
     * create family platform under sp
     */
    const char*         platformName      = "platform";
    const char*         platformSummary   = "Parent family for platform objects";
    const VISIBILITY    platformVis       = PRODUCTTREE;
    /*
     * create family platform
     */
    rc = createFamily(&platform, NULL, platformName, platformSummary, 
			  platformVis);
    if (STATS_SUCCESS != rc) {
        fbe_envstats_trace(FBE_TRACE_LEVEL_ERROR, 
			   "envstats Failed to create family [%s] :  [%d][%s]\n",
			   platformName, rc, getCAPIErrorMsg(rc));
	return rc;
    }

    MetaSpTemperature = createMeta("C", KSCALE_UNO, DEF_META_WIDTH, 
				   WRAP_W32);
    if(NULL == MetaSpTemperature ){
        fbe_envstats_trace(FBE_TRACE_LEVEL_ERROR, "platform family error");
	return -1;
    }
    rc = createVirtualFact(&spTemperature, platform, 
			   "storageProcessorTemperature",
			   "Storage Processor Temperature", 
			   (void *)&getSPtemp, PRODUCT, 
			   "SP Temperature", MetaSpTemperature, U_METER_32);
    if(STATS_SUCCESS !=rc ){
      fbe_envstats_trace(FBE_TRACE_LEVEL_ERROR, 
			 "Create Virtual Fact: StorageProcessorTemperature Failed: [%d] [%s]", 
			 rc, getCAPIErrorMsg(rc));
    }

    return rc;

}  
#endif

VOID fbe_envstats_init (void)
{
    fbe_envstats_trace(FBE_TRACE_LEVEL_INFO, "fbe envstats init");
    fbe_read_temp_trace_count = 0;
#ifdef C4_INTEGRATED
    create_platform_family();
#endif
}

static fbe_status_t fbe_envstats_destroy(fbe_packet_t *packet)
{
     fbe_status_t status = FBE_STATUS_OK;
     fbe_base_service_destroy(&fbe_envstats_service);
     fbe_transport_set_status(packet, status, 0);
     fbe_transport_complete_packet(packet);
     return status;

}

fbe_status_t fbe_envstats_control_entry (fbe_packet_t *packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        fbe_base_service_init(&fbe_envstats_service);
        fbe_envstats_trace(FBE_TRACE_LEVEL_INFO, "control entry");
        fbe_envstats_init();
	fbe_transport_set_status(packet, status, 0);
	fbe_transport_complete_packet(packet);
        return status;
    }
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_DESTROY) {
        status = fbe_envstats_destroy(packet);
        return status;
    }

    fbe_envstats_trace(FBE_TRACE_LEVEL_INFO, "gbe envstats: Unkown Control code");

    return FBE_STATUS_OK;
}
