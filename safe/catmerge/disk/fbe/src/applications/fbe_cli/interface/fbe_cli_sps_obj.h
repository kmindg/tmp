#ifndef FBE_CLI_SPS_OBJ_H
#define FBE_CLI_SPS_OBJ_H

#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"

void fbe_cli_cmd_spsinfo(fbe_s32_t argc,char ** argv);
void fbe_cli_cmd_bbuinfo(fbe_s32_t argc,char ** argv);
static void fbe_cli_print_spsinfo( fbe_device_physical_location_t *location);
static void fbe_cli_print_overall_spsinfo(void);
static void fbe_cli_print_sps_manuf_info(fbe_device_physical_location_t *location, fbe_bool_t displayLocation);
static void fbe_cli_print_sps_input_power(fbe_device_physical_location_t *location);

extern char * fbe_cli_print_HwCacheStatus(fbe_common_cache_status_t hw_cache_status);

#define SPSINFO_USAGE  "\
spsinfo/si  Show all SPS related information \n\
Usage:  spsinfo -help   For this help information \n\
        spsinfo -setSimulateSps simulateSps \n\
        spsinfo -settesttime Day Hour Minute \n\
        spsinfo -status Show only SPS status information \n\
        spsinfo -power  Show only SPS input power information \n\
        spsinfo -manuf  Show only SPS manufacturing information \n\
        spsinfo  show all SPS related information \n"
#define BBUINFO_USAGE  "\
bbuinfo/bi  Show all BBU related information \n\
Usage:  info -help   For this help information \n\
        bbuinfo -setSimulateBbu simulateBbu \n\
        bbuinfo -settesttime Day Hour Minute \n\
        bbuinfo -runSelfTest \n\
        bbuinfo  show all BBU related information \n"

#endif /* FBE_CLI_SPS_OBJ */
