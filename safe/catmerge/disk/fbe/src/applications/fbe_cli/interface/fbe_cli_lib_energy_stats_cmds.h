#ifndef FBE_CLI_LIB_ENERGY_STATS_CMD_H
#define FBE_CLI_LIB_ENERGY_STATS_CMD_H


#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_esp_eir_interface.h"
#include "fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_eir_info.h"


/* Function prototypes*/
void fbe_cli_cmd_eirstat(fbe_s32_t argc,char ** argv);
static void fbe_cli_displayInputPowerStats(BOOL displayVerbose);
static void fbe_cli_displaySpsInputPowerStatus(fbe_device_physical_location_t location, BOOL displayVerbose);
static void fbe_cli_displayAirInletTempStats(BOOL displayVerbose);
static void fbe_cli_displayArrayUtilizationStats(BOOL displayVerbose);
void fbe_cli_decodeStatsStatus(fbe_u32_t statsStatus, char *statusStr);
void fbe_cli_displayPsSupportedStatus(void);

#define EIR_STAT_STAUS_STRING_LENGTH 80
#define EIR_USAGE         "\n  eirstat \n\
    -ip         -Displays Input Power status only\n\
    -ait        -Displays Air Inlet Temperature stats only\n\
    -v          -Displays verbose output (sample values)\n\
    -support    -Displays Power Supply Support Status\n\
"
#endif /*FBE_CLI_LIB_ENERGY_STATS_CMD_H*/

