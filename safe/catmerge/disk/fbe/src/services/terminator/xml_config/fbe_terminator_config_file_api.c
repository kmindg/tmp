/***************************************************************************
 *  fbe_terminator_file_api.c
 ***************************************************************************
 *
 *  Description
 *      APIs to acess xml file in order to build a topology
 *
 *
 *  History:
 *      06/18/08    sharel  Created
 *
 ***************************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
#include <sys/types.h>
#include <sys/stat.h>
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "xml_field.h"
#include "xml_process.h"
#include "fbe/fbe_types.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_queue.h"
#include "fbe_terminator_file_api.h"
#include "fbe_terminator.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator_class_management.h"
#include "fbe_terminator_device_classes.h"
#include "terminator_enclosure.h"
#include "terminator_port.h"
#include "terminator_board.h"
#include "terminator_drive.h"
#include "terminator_enum_conversions.h"
#include "fbe/fbe_file.h"
//#include "fbe/fbe_ctype.h"
#include "fbe_terminator_common.h"
#include "EmcPAL_Memory.h"
#include "fbe/fbe_resume_prom_info.h"
#include "core_config.h"
#include "resume_prom.h"
#include "generic_utils_lib.h"

#define MAX_PROM_ID     PS_B_RESUME_PROM
#define RESUME_PATTERN(prom_id, data_index) (((data_index) % 2)? ((prom_id) + '0') : (((data_index) / 2) % 26) + 'A')

/**********************************/
/*        local variables         */
/**********************************/
static terminator_configuration_t current_configuration_format;

/**********************************/
/*        local variables         */
/**********************************/

/* These stuff are used by conversion routines used in obsolete XML loader.
 *  New XML loader is using Terminator enum conversion routines, and does
 *  not need these structures any more.
 */
static board_str_to_enum_t board_str_to_enum_table [] =
{
    {FBE_BOARD_TYPE_DELL,   ENUM_TO_STR(FBE_BOARD_TYPE_DELL)},
    {FBE_BOARD_TYPE_FLEET,  ENUM_TO_STR(FBE_BOARD_TYPE_FLEET)},
    {FBE_BOARD_TYPE_ARMADA, ENUM_TO_STR(FBE_BOARD_TYPE_ARMADA)},

    /* add here fleet boards in future */
    {FBE_BOARD_TYPE_LAST, NULL}
};

static platform_str_to_enum_t platform_str_to_enum_table [] =
{
    {SPID_CORSAIR_HW_TYPE,          ENUM_TO_STR(SPID_CORSAIR_HW_TYPE)},
    {SPID_SPITFIRE_HW_TYPE,         ENUM_TO_STR(SPID_SPITFIRE_HW_TYPE)},
    {SPID_LIGHTNING_HW_TYPE,        ENUM_TO_STR(SPID_LIGHTNING_HW_TYPE)},
    {SPID_HELLCAT_HW_TYPE,          ENUM_TO_STR(SPID_HELLCAT_HW_TYPE)},
    {SPID_HELLCAT_LITE_HW_TYPE,     ENUM_TO_STR(SPID_HELLCAT_LITE_HW_TYPE)},
    {SPID_HELLCAT_XM_HW_TYPE,       ENUM_TO_STR(SPID_HELLCAT_XM_HW_TYPE)},
    {SPID_MUSTANG_HW_TYPE,          ENUM_TO_STR(SPID_MUSTANG_HW_TYPE)},
    {SPID_MUSTANG_XM_HW_TYPE,       ENUM_TO_STR(SPID_MUSTANG_XM_HW_TYPE)},
    {SPID_PROMETHEUS_M1_HW_TYPE,    ENUM_TO_STR(SPID_PROMETHEUS_M1_HW_TYPE)},
    {SPID_PROMETHEUS_S1_HW_TYPE,    ENUM_TO_STR(SPID_PROMETHEUS_S1_HW_TYPE)},
    {SPID_DEFIANT_M1_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_M1_HW_TYPE)},
    {SPID_DEFIANT_M2_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_M2_HW_TYPE)},
    {SPID_DEFIANT_M3_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_M3_HW_TYPE)},
    {SPID_DEFIANT_M4_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_M4_HW_TYPE)},
    {SPID_DEFIANT_M5_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_M5_HW_TYPE)},
    {SPID_DEFIANT_S1_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_S1_HW_TYPE)},
    {SPID_DEFIANT_S4_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_S4_HW_TYPE)},
    {SPID_DEFIANT_K2_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_K2_HW_TYPE)},
    {SPID_DEFIANT_K3_HW_TYPE,       ENUM_TO_STR(SPID_DEFIANT_K3_HW_TYPE)},
    {SPID_SENTRY_HW_TYPE,           ENUM_TO_STR(SPID_SENTRY_HW_TYPE)},
    {SPID_EVERGREEN_HW_TYPE,        ENUM_TO_STR(SPID_EVERGREEN_HW_TYPE)},
    {SPID_NOVA1_HW_TYPE,            ENUM_TO_STR(SPID_NOVA1_HW_TYPE)},
    {SPID_NOVA3_HW_TYPE,            ENUM_TO_STR(SPID_NOVA3_HW_TYPE)},
    {SPID_NOVA3_XM_HW_TYPE,         ENUM_TO_STR(SPID_NOVA3_XM_HW_TYPE)},
    {SPID_NOVA_S1_HW_TYPE,          ENUM_TO_STR(SPID_NOVA_S1_HW_TYPE)},
    {SPID_OBERON_1_HW_TYPE,         ENUM_TO_STR(SPID_OBERON_1_HW_TYPE)},
    {SPID_OBERON_2_HW_TYPE,         ENUM_TO_STR(SPID_OBERON_2_HW_TYPE)},
    {SPID_OBERON_3_HW_TYPE,         ENUM_TO_STR(SPID_OBERON_3_HW_TYPE)},
    {SPID_OBERON_4_HW_TYPE,         ENUM_TO_STR(SPID_OBERON_4_HW_TYPE)},
    {SPID_OBERON_S1_HW_TYPE,        ENUM_TO_STR(SPID_OBERON_S1_HW_TYPE)},
    {SPID_HYPERION_1_HW_TYPE,       ENUM_TO_STR(SPID_HYPERION_1_HW_TYPE)},
    {SPID_HYPERION_2_HW_TYPE,       ENUM_TO_STR(SPID_HYPERION_2_HW_TYPE)},
    {SPID_HYPERION_3_HW_TYPE,       ENUM_TO_STR(SPID_HYPERION_3_HW_TYPE)},
    {SPID_TRITON_1_HW_TYPE,         ENUM_TO_STR(SPID_TRITON_1_HW_TYPE)},
    {SPID_ENTERPRISE_HW_TYPE,       ENUM_TO_STR(SPID_ENTERPRISE_HW_TYPE)},
    {SPID_MERIDIAN_ESX_HW_TYPE,     ENUM_TO_STR(SPID_MERIDIAN_ESX_HW_TYPE)},
    {SPID_TUNGSTEN_HW_TYPE,         ENUM_TO_STR(SPID_TUNGSTEN_HW_TYPE)},
    {SPID_BEARCAT_HW_TYPE,          ENUM_TO_STR(SPID_BEARCAT_HW_TYPE)},

    /* add here new platform types in future */
    {SPID_MAXIMUM_HW_TYPE, NULL}
};

static port_str_to_enum_t port_str_to_enum_table [] =
{
    {FBE_PORT_TYPE_SAS_LSI, ENUM_TO_STR(FBE_PORT_TYPE_SAS_LSI)},
    {FBE_PORT_TYPE_SAS_PMC, ENUM_TO_STR(FBE_PORT_TYPE_SAS_PMC)},
    {FBE_PORT_TYPE_FC_PMC,  ENUM_TO_STR(FBE_PORT_TYPE_FC_PMC)},
    {FBE_PORT_TYPE_ISCSI,   ENUM_TO_STR(FBE_PORT_TYPE_ISCSI)},
    {FBE_PORT_TYPE_FCOE,    ENUM_TO_STR(FBE_PORT_TYPE_FCOE)},

    {FBE_PORT_TYPE_LAST, NULL}
};

static encl_str_to_enum_t encl_str_to_enum_table [] =
{
    {FBE_SAS_ENCLOSURE_TYPE_BULLET,      ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_BULLET)},
    {FBE_SAS_ENCLOSURE_TYPE_VIPER,       ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_VIPER)},
    {FBE_SAS_ENCLOSURE_TYPE_MAGNUM,      ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_MAGNUM)},
    {FBE_SAS_ENCLOSURE_TYPE_BUNKER,      ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_BUNKER)},
    {FBE_SAS_ENCLOSURE_TYPE_CITADEL,     ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_CITADEL)},
    {FBE_SAS_ENCLOSURE_TYPE_DERRINGER,   ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_DERRINGER)},
    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)},
    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE)},    
    {FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP)},
    {FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP)},    
    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP)},
    {FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP)},    
    {FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP)},
    {FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP)},
    {FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP)},      
    {FBE_SAS_ENCLOSURE_TYPE_FALLBACK, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_FALLBACK)},
    {FBE_SAS_ENCLOSURE_TYPE_BOXWOOD, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_BOXWOOD)},
    {FBE_SAS_ENCLOSURE_TYPE_KNOT, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_KNOT)},
    {FBE_SAS_ENCLOSURE_TYPE_PINECONE, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_PINECONE)},
    {FBE_SAS_ENCLOSURE_TYPE_STEELJAW, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_STEELJAW)},
    {FBE_SAS_ENCLOSURE_TYPE_RAMHORN, ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_RAMHORN)},
    {FBE_SAS_ENCLOSURE_TYPE_CALYPSO,     ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_CALYPSO)},
    {FBE_SAS_ENCLOSURE_TYPE_RHEA,        ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_RHEA)},
    {FBE_SAS_ENCLOSURE_TYPE_MIRANDA,     ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_MIRANDA)},
    {FBE_SAS_ENCLOSURE_TYPE_TABASCO,     ENUM_TO_STR(FBE_SAS_ENCLOSURE_TYPE_TABASCO)},
    {FBE_SAS_ENCLOSURE_TYPE_LAST, NULL}
};

static sas_drive_str_to_enum_t sas_drive_str_to_enum_table [] =
{
    {FBE_SAS_DRIVE_CHEETA_15K, ENUM_TO_STR(FBE_SAS_DRIVE_CHEETA_15K)},

    {FBE_SAS_DRIVE_UNICORN_512, ENUM_TO_STR(FBE_SAS_DRIVE_UNICORN_512)},
    {FBE_SAS_DRIVE_UNICORN_4096, ENUM_TO_STR(FBE_SAS_DRIVE_UNICORN_4096)},
    {FBE_SAS_DRIVE_UNICORN_4160, ENUM_TO_STR(FBE_SAS_DRIVE_UNICORN_4160)},

    {FBE_SAS_DRIVE_SIM_520, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_520)},

    {FBE_SAS_NL_DRIVE_SIM_520, ENUM_TO_STR(FBE_SAS_NL_DRIVE_SIM_520)},

    {FBE_SAS_DRIVE_SIM_512, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_512)},

    {FBE_SAS_DRIVE_SIM_520_FLASH_HE, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_520_FLASH_HE)},       /* Simulation Fast Cache (EFD - HE) */

    {FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP)},
    {FBE_SAS_DRIVE_SIM_4160_FLASH_UNMAP, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_4160_FLASH_UNMAP)},
    {FBE_SAS_DRIVE_SIM_520_UNMAP, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_520_UNMAP)},
    {FBE_SAS_DRIVE_UNICORN_4160_UNMAP, ENUM_TO_STR(FBE_SAS_DRIVE_UNICORN_4160_UNMAP)},

    {FBE_SAS_DRIVE_COBRA_E_10K,   ENUM_TO_STR(FBE_SAS_DRIVE_COBRA_E_10K)},

    {FBE_SAS_DRIVE_SIM_520_12G,   ENUM_TO_STR(FBE_SAS_DRIVE_SIM_520_12G)},

    {FBE_SAS_DRIVE_SIM_520_FLASH_ME, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_520_FLASH_ME)}, /* Simulation Flash VP (ME) */
    {FBE_SAS_DRIVE_SIM_520_FLASH_LE, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_520_FLASH_LE)}, /* Simulation Flash VP (LE) */
    {FBE_SAS_DRIVE_SIM_520_FLASH_RI, ENUM_TO_STR(FBE_SAS_DRIVE_SIM_520_FLASH_RI)}, /* Simulation Flash VP (RI) */

    {FBE_SAS_DRIVE_LAST, NULL}

};

static sata_drive_str_to_enum_t sata_drive_str_to_enum_table [] =
{
    {FBE_SATA_DRIVE_HITACHI_HUA,   ENUM_TO_STR(FBE_SATA_DRIVE_HITACHI_HUA)},
    {FBE_SATA_DRIVE_SIM_512,       ENUM_TO_STR(FBE_SATA_DRIVE_SIM_512)},
    {FBE_SATA_DRIVE_SIM_512_FLASH, ENUM_TO_STR(FBE_SATA_DRIVE_SIM_512_FLASH)},

    {FBE_SATA_DRIVE_LAST, NULL}
};

/* all the config dirs and files are listed here
 */
fbe_u32_t terminator_access_mode = 0;
 
fbe_u8_t terminator_disk_storage_dir[FBE_MAX_PATH] = {""};

static fbe_u8_t terminator_config_dir[FBE_MAX_PATH] = {""};
static fbe_u8_t terminator_config_file_name[MAX_CONFIG_FILE_NAME] = {""};
static fbe_u8_t terminator_config_file_name_full[FBE_MAX_PATH + MAX_CONFIG_FILE_NAME] = {""};

/* we would have a queue per port
 */
fbe_queue_head_t device_list     [FBE_CPD_SHIM_MAX_PORTS]; /* this is the constant list */
fbe_queue_head_t temp_device_list[FBE_CPD_SHIM_MAX_PORTS]; /* this is the list we would read to check any changes */

/******************************/
/*     local function         */
/*****************************/
static fbe_status_t load_configuration(void);
static fbe_status_t open_configuration_file(fbe_file_handle_t * inputFile);
static fbe_status_t read_xml_line_from_file (fbe_file_handle_t inputFile, char *buffer, size_t buf_size, fbe_u32_t *total_bytes);
static fbe_status_t extract_device_type_from_line (fbe_u8_t *xml_line, term_device_type_t *device_type);
static fbe_status_t process_device(term_device_type_t device_type, fbe_u8_t *xml_line, fbe_s32_t port_number);
static fbe_status_t process_board_device(fbe_u8_t *xml_line);
static fbe_status_t process_port_device(fbe_u8_t *xml_line, fbe_s32_t port_number);
static fbe_status_t process_enclosure_device(fbe_u8_t *xml_line, fbe_s32_t port_number);
static fbe_status_t process_sas_drive_device(fbe_u8_t *xml_line, fbe_s32_t port_number);
static fbe_status_t extract_attribute_value (fbe_u8_t *buffer, fbe_u8_t *attr_name, fbe_u8_t *ret_value, size_t ret_val_size);
static fbe_status_t convert_board_string_to_enum(fbe_u8_t *board_str, fbe_board_type_t *board_enum);
static fbe_status_t convert_spid_string_to_enum(fbe_u8_t *spid_str, terminator_sp_id_t *spid);
static fbe_status_t convert_platform_string_to_enum(fbe_u8_t *platform_str, SPID_HW_TYPE *platform_enum);
static fbe_status_t convert_port_string_to_enum(fbe_u8_t *port_str, fbe_port_type_t *port_enum);
static fbe_status_t convert_enclosure_string_to_enum(fbe_u8_t *encl_str, fbe_sas_enclosure_type_t *encl_enum);
static fbe_status_t extract_sas_address_from_line(fbe_u8_t *xml_line, fbe_sas_address_t *sas_address);
static fbe_status_t extract_sn_from_line(fbe_u8_t *xml_line, fbe_u8_t *sn, fbe_u32_t length);
static fbe_status_t extract_uid_from_line(fbe_u8_t *xml_line, fbe_u8_t *uid, fbe_u32_t length);
static fbe_status_t extract_io_port_number_from_line(fbe_u8_t *xml_line, fbe_u32_t *io_port_number);
static fbe_status_t extract_portal_number_from_line(fbe_u8_t *xml_line, fbe_u32_t *portal_number);
static fbe_status_t extract_backend_number_from_line(fbe_u8_t *xml_line, fbe_u32_t *backend_number);
static fbe_status_t xtoi(const fbe_u8_t* hexStg, fbe_s32_t szlen, fbe_sas_address_t* result);
static fbe_status_t send_insert_to_port_device (term_device_info_t* new_term_element);
static fbe_status_t send_insert_to_enclosure_device (term_device_info_t* new_term_element);
static fbe_status_t send_insert_to_drive_device (term_device_info_t* new_term_element);
static fbe_status_t convert_sas_drive_string_to_enum(fbe_u8_t *drive_str, fbe_sas_drive_type_t *drive_enum);
static fbe_status_t convert_sata_drive_string_to_enum(fbe_u8_t *drive_str, fbe_sata_drive_type_t *drive_enum);
static fbe_status_t process_sata_drive_device(fbe_u8_t *xml_line, fbe_s32_t port_number);

/* new functions */
static fbe_status_t process_all_boards(const xml_field * boards);
static fbe_status_t process_all_ports(const xml_field * ports);
static fbe_status_t process_all_enclosures(const xml_field * enclosures);
static fbe_status_t process_all_drives(const xml_field * drives);
/* generalization of process_all* functions (_ports, _enclosures
   and _drives, board use slightly different approach at this time) */
static fbe_status_t process_all_devices_with_same_class(const xml_field *devices_field, const char *device_class_name);
//static fbe_status_t terminator_create_topology(void);
static fbe_status_t terminator_file_api_activate_devices(void);
static fbe_status_t terminator_file_api_insert_device(fbe_terminator_api_device_handle_t device_handle);

/* Some useful TRACE macros */
#define TRACE_MISSING_ACCESSOR(attribute, device_class)             \
        terminator_trace(FBE_TRACE_LEVEL_ERROR,                     \
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                       \
        "%s: could not find TCM accessor for %s attribute of %s\n", \
        __FUNCTION__, attribute, device_class);

#define TRACE_SETTER_FAILURE(value, attribute, device_class)   \
        terminator_trace(FBE_TRACE_LEVEL_ERROR,                \
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                  \
        "%s: could not set value %s for %s attribute of %s\n", \
        __FUNCTION__, value, attribute, device_class);

#define TRACE_TCM_CLASS_MISSING(device_class)    \
        terminator_trace(FBE_TRACE_LEVEL_ERROR,  \
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    \
        "%s: could not find TCM class for %s\n", \
        __FUNCTION__, device_class);

/* handler for errors in XML process during processing XML file */
static void xml_error_handler(void * process, int line_number, int code, const char * description)
{
    terminator_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "XML process: error processing file %s at line %d: %s (code = %d)\n",
                    ((xml_process *)process)->processed_file,
                    line_number, description, code);
}

/* loads new XML configuration */
fbe_status_t terminator_file_api_load_new_configuration(void)
{
    int i = 0;
    fbe_file_handle_t inputFile = NULL;
    xml_process * proc = NULL;
    xml_field * root = NULL;
    xml_field * clariion = NULL;
    xml_field * boards = NULL;
    xml_field * iomodules = NULL;
    xml_field * current_iom = NULL;
    xml_field * enclosures = NULL;
    xml_field * drives = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* open file */
    if( (inputFile = fbe_file_open(terminator_config_file_name_full, FBE_FILE_RDONLY, 0, NULL)) == FBE_FILE_INVALID_HANDLE )
    {
        // This should never happen
        terminator_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: can't find file %s\n", __FUNCTION__, terminator_config_file_name_full);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* create XML process */
    proc = create_xml_process();
    if( proc == NULL )
    {
        fbe_file_close(inputFile);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: can't create XML process\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set handler for XML errors */
    proc->error = xml_error_handler;

    /* parse XML file */
    root = (xml_field *)parse_stream(proc, (FILE*)inputFile);
    if( root == NULL )
    {
        fbe_file_close(inputFile);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not load XML data from file %s\n", __FUNCTION__, terminator_config_file_name_full);
        delete_xml_process(proc, 1);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* find subfields we need */
    clariion = find_subfield(root, "CLARiiON");
    if( clariion == NULL)
    {
        fbe_file_close(inputFile);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not find node \"%s\" in file %s\n", __FUNCTION__, "CLARiiON", terminator_config_file_name_full);
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    boards = find_subfield(clariion, "StorageProcessors");
    if( boards == NULL)
    {
        fbe_file_close(inputFile);
        terminator_trace(FBE_TRACE_LEVEL_WARNING,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not find node \"%s\" in file %s\n", __FUNCTION__, "StorageProcessors", terminator_config_file_name_full);
        terminator_trace(FBE_TRACE_LEVEL_WARNING,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: configuration considered EMPTY, returning FBE_STATUS_OK\n", __FUNCTION__ );
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return FBE_STATUS_OK;
    }

    status = process_all_boards(boards);
    if( status != FBE_STATUS_OK )
    {
        fbe_file_close(inputFile);
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return status;
    }

    iomodules = find_subfield(clariion, "IOModules");
    if( iomodules == NULL )
    {
        fbe_file_close(inputFile);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not find node \"%s\" in file %s\n", __FUNCTION__, "IOModules", terminator_config_file_name_full);
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    do
    {
        terminator_sp_id_t spid;
        (void) fbe_terminator_api_get_sp_id(&spid);
        current_iom = find_nth_subfield(iomodules, "IOModule", i++);
        /* retrieve additional attributes of IOModule if needed*/
        if(current_iom != NULL)
        {
            /* check that this IOModule has sp_id equal to the terminators configured sp_id */
            if((spid == TERMINATOR_SP_A && _stricmp("A", get_attribute_value(current_iom, "sp_id")))||
                (spid == TERMINATOR_SP_B && _stricmp("B", get_attribute_value(current_iom, "sp_id"))))
            {
                terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: configured as sp %s, ignoring %s with sp_id = %s\n", __FUNCTION__,
                         (spid==TERMINATOR_SP_A ? "A": "B"), "IOModule", get_attribute_value(current_iom, "sp_id"));
                continue;
            }
            status = process_all_ports(current_iom);
            if(status != FBE_STATUS_OK)
            {
                fbe_file_close(inputFile);
                delete_xml_process(proc, 1);
                delete_xml_field(root);
                return status;
            }
        }
    }
    while(current_iom != NULL);

    enclosures = find_subfield( clariion, "Enclosures" );
    if( enclosures == NULL)
    {
        fbe_file_close(inputFile);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not find node \"%s\" in file %s\n", __FUNCTION__, "Enclosures", terminator_config_file_name_full);
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = process_all_enclosures(enclosures);
    if( status != FBE_STATUS_OK )
    {
        fbe_file_close(inputFile);
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return status;
    }

    drives = find_subfield( clariion, "Drives" );
    if( drives == NULL)
    {
        fbe_file_close(inputFile);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not find node \"%s\" in file %s\n", __FUNCTION__, "Drives", terminator_config_file_name_full);
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = process_all_drives(drives);
    if( status != FBE_STATUS_OK )
    {
        fbe_file_close(inputFile);
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return status;
    }
    //status = terminator_create_topology();
    //if( status != FBE_STATUS_OK )
    //{
    //    delete_xml_process(proc, 1);
    //    delete_xml_field(root);
    //    return status;
    //}

    status = terminator_file_api_activate_devices();
    if( status != FBE_STATUS_OK )
    {
        fbe_file_close(inputFile);
        delete_xml_process(proc, 1);
        delete_xml_field(root);
        return status;
    }

    /* delete xml process */
    delete_xml_process(proc, 1);
    /* XML data is not needed any more, we can delete it */
    delete_xml_field(root);
    fbe_file_close(inputFile);
    return FBE_STATUS_OK;
}

fbe_status_t process_all_boards(const xml_field * boards)
{
    int i = 0, j = 0;
    xml_field * board = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_api_device_handle_t hdevice;
    const char * attr = NULL;
    const char * val = NULL;
    fbe_terminator_api_device_class_handle_t board_class_handle;
    terminator_sp_id_t spid;

    (void) fbe_terminator_api_get_sp_id(&spid);

    /* find class for board */
    status = fbe_terminator_api_find_device_class("Board", &board_class_handle);
    if(status != FBE_STATUS_OK)
    {
        TRACE_TCM_CLASS_MISSING("Board")
        return FBE_STATUS_GENERIC_FAILURE;
    }

    do
    {
        /* find next board */
        board = find_nth_subfield(boards, "SP", i++);
        if(board != NULL)
        {
            
            /* check that it is not SPb */
            if((spid == TERMINATOR_SP_A && _stricmp("A", get_attribute_value(board, "sp_id"))) ||
                (spid == TERMINATOR_SP_B && _stricmp("B",get_attribute_value(board, "sp_id"))))
            {
                terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: configured as sp %s, ignoring %s with sp_id = %s\n", __FUNCTION__,
                         (spid==TERMINATOR_SP_A?"A":"B"),"Board", get_attribute_value(board, "sp_id"));
                continue;
            }

            /* create device */
            status = fbe_terminator_api_create_device_class_instance(board_class_handle, NULL, &hdevice);
            if(status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s: could not instantiate %s class via TCM\n", __FUNCTION__, "Board");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* set device attributes */
            j = 0;
            /* retrieve next attribute */
            while((attr = board->attributes[j++])!= NULL)
            {
                /* retrieve attribute value */
                val = board->attributes[j++];
                if(val == NULL)
                {
                    break;
                }
                status = fbe_terminator_api_set_device_attribute(hdevice, attr, val);
                if(status != FBE_STATUS_OK)
                {
                    return status;
                }
            }
        }
    }
    while(board != NULL);
    return FBE_STATUS_OK;
}

/* The common function for process_all_ports/enclosures/drives() */
fbe_status_t process_all_devices_with_same_class(const xml_field *devices_field, const char *device_class_name)
{
    fbe_terminator_api_device_class_handle_t device_class_handle = NULL;

    const xml_field *device_field = NULL;
    int              device_index = 0;

    /* find the device class */
    fbe_status_t status = fbe_terminator_api_find_device_class(device_class_name, &device_class_handle);
    if (status != FBE_STATUS_OK)
    {
        TRACE_TCM_CLASS_MISSING(device_class_name)
        return status;
    }

    do
    {
        const char *fbe_drive_type = NULL,
                   *device_type    = NULL;

        fbe_terminator_api_device_handle_t device_handle = 0;

        int         device_attr_index = 0;
        const char *device_attr       = NULL,
                   *device_attr_val   = NULL;

        /* find a device */
        device_field = find_nth_subfield(devices_field, device_class_name, device_index++);
        if (NULL == device_field)
        {
            break;
        }

        if (!strcmp(device_class_name, "Drive"))
        {
            const char *drive_type = get_attribute_value(device_field, "Drive_type");
            if (drive_type != NULL)
            {
                if (!strcmp(drive_type, "FBE_SAS_DRIVE_SIM_520"))
                {
                    if(!strcmp(drive_type, "FBE_SAS_DRIVE_SIM_520"))
                    {
                        /* seems to be a SAS drive */
                        device_type = "FBE_DRIVE_TYPE_SAS";
                    }
                    else if(!strcmp(drive_type, "FBE_SAS_DRIVE_SIM_520_12G"))
                    {
                        /* seems to be a SAS 12Gb drive */
                        device_type = "FBE_DRIVE_TYPE_SAS";                        
                    }
                    else if(!strcmp(drive_type, "FBE_SAS_NL_DRIVE_SIM_520"))
                    {
                        /* seems to be a ML SAS 520 drive */
                        device_type = "FBE_DRIVE_TYPE_SAS_NL";                                                
                    }
                    else if(!strcmp(drive_type, "FBE_SAS_DRIVE_SIM_520_FLASH_HE"))
                    {
                        /* seems to be a SAS HE Flash drive */
                        device_type = "FBE_DRIVE_TYPE_SAS_FLASH_HE";
                    }
                    else if(!strcmp(drive_type, "FBE_SAS_DRIVE_SIM_520_FLASH_ME"))
                    {
                        /* seems to be a SAS ME Flash drive */
                        device_type = "FBE_DRIVE_TYPE_SAS_FLASH_ME";
                    }
                    else if(!strcmp(drive_type, "FBE_SAS_DRIVE_SIM_520_FLASH_LE"))
                    {
                        /* seems to be a SAS LE Flash drive */
                        device_type = "FBE_DRIVE_TYPE_SAS_FLASH_LE";
                    }
                    else if(!strcmp(drive_type, "FBE_SAS_DRIVE_SIM_520_FLASH_RI"))
                    {
                        /* seems to be a SAS RI Flash drive */
                        device_type = "FBE_DRIVE_TYPE_SAS_FLASH_RI";
                    }
                    else if(!strcmp(drive_type, "FBE_SAS_DRIVE_SIM_512"))
                    {
                        /* seems to be a SAS drive */
                        device_type = "FBE_DRIVE_TYPE_SAS";
                    }
                    else if(strstr(drive_type, "FBE_SAS_DRIVE") != NULL) /* default to standard SAS for all the mismatched SAS drives */
                    {
                        /* seems to be a SAS drive */
                        device_type = "FBE_DRIVE_TYPE_SAS";
                    }
                    else if (!strcmp(drive_type, "FBE_SATA_DRIVE_SIM_512_FLASH"))
                    {
                        /* seems to be a SATA 512-Flash drive */
                        device_type = "FBE_DRIVE_TYPE_SATA_FLASH_HE";
                    }
                    else if(strstr(drive_type, "FBE_SATA_DRIVE") != NULL)
                    {
                        /* seems to be a SATA drive */
                        device_type = "FBE_DRIVE_TYPE_SATA";
                    }
                    else
                    {
                        /* seems to be INVALID drive type */
                        device_type = "FBE_SAS_DRIVE_INVALID";
                    }
                }
            }
        }
        else if (!strcmp(device_class_name, "Port"))
        {
            ++terminator_port_count;
        }

        /* only set the device type when it is a drive and we have got
         *  the FBE drive type, otherwise, keep it NULL since we will
         *  set the default device type in terminator_port/enclosure/drive_allocator()
         *  functions.
         */
        if (fbe_drive_type != NULL)
        {
            device_type = fbe_drive_type;
        }

        /* create the device */
        status = fbe_terminator_api_create_device_class_instance(device_class_handle, device_type, &device_handle);
        if (status != FBE_STATUS_OK)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: could not instantiate %s class via TCM\n",
                             __FUNCTION__,
                             device_class_name);

            return status;
        }

        /* set device attributes */
        while ((device_attr = device_field->attributes[device_attr_index++]) != NULL)
        {
            /* retrieve the current device attribute value */
            device_attr_val = device_field->attributes[device_attr_index++];
            if (NULL == device_attr_val)
            {
                TRACE_MISSING_ACCESSOR(device_attr, device_class_name)
                break;
            }

            status = fbe_terminator_api_set_device_attribute(device_handle, device_attr, device_attr_val);
            if (status != FBE_STATUS_OK)
            {
                return status;
            }
        }
    } while (device_field != NULL);

    return status;
}

fbe_status_t process_all_ports(const xml_field * ports)
{
    return process_all_devices_with_same_class(ports, "Port");
}

fbe_status_t process_all_enclosures(const xml_field * enclosures)
{
    return process_all_devices_with_same_class(enclosures, "Enclosure");
}

fbe_status_t process_all_drives(const xml_field * drives)
{
    return process_all_devices_with_same_class(drives, "Drive");
}


fbe_status_t terminator_file_api_init_new(void)
{
    return terminator_file_api_load_new_configuration();
}

fbe_status_t terminator_file_api_destroy_new(void)
{
    return FBE_STATUS_OK;
}

fbe_status_t set_dummy_resume_info(fbe_terminator_api_device_handle_t   encl_device_id, 
                           fbe_u8_t   prom_id,
                           fbe_bool_t bad_chksum)
{
    fbe_u16_t                data_index;
    fbe_resume_prom_info_t   dummy_resume_info;

    /* Set the known pattern of data in resume buffer & put checksum. */
    for(data_index = 0; 
        data_index < sizeof(fbe_resume_prom_info_t) - RESUME_PROM_CHECKSUM_SIZE;
        data_index++)
    {
        *(((fbe_u8_t *)&dummy_resume_info) + data_index) = 
            RESUME_PATTERN(prom_id, data_index);
    }

    if(bad_chksum)
    {
        dummy_resume_info.resume_prom_checksum = 0;
    }
    else
    {
        dummy_resume_info.resume_prom_checksum = 
            calculateResumeChecksum((RESUME_PROM_STRUCTURE*) &dummy_resume_info);
    }

    /* Set the resume data in terminator. */
    return fbe_terminator_api_set_resume_prom_info(encl_device_id, prom_id,
        (fbe_u8_t *)&dummy_resume_info, sizeof(fbe_resume_prom_info_t));
}

static fbe_status_t terminator_file_api_activate_devices(void)
{
    fbe_u8_t fbe_prom_id;
    fbe_u32_t i;
    fbe_status_t status;
    tdr_device_ptr_t device_ptr;
    tdr_status_t tdr_status;    
    terminator_component_type_t type;

    /* get the list of all the devices */
    fbe_terminator_api_device_handle_t device_handle_array[TERMINATOR_MAX_NUMBER_OF_DEVICES];
    for(i = 0; i < TERMINATOR_MAX_NUMBER_OF_DEVICES; i++)
    {
        device_handle_array[i] = 0;
    }

    status = fbe_terminator_api_enumerate_devices(device_handle_array, TERMINATOR_MAX_NUMBER_OF_DEVICES);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    /*insert and activate each device */
    for(i = 0; i < TERMINATOR_MAX_NUMBER_OF_DEVICES; i++)
    {
        if(device_handle_array[i] != 0)
        {
            status = terminator_file_api_insert_device(device_handle_array[i]);
            if(status != FBE_STATUS_OK)
            {
                return status;
            }
            status = fbe_terminator_api_activate_device(device_handle_array[i]);
            if(status != FBE_STATUS_OK)
            {
                return status;
            }
            /* Set the resume prom info for all the PROMs in enclosure. */
            tdr_status = fbe_terminator_device_registry_get_device_ptr(device_handle_array[i], &device_ptr);
            type = base_component_get_component_type(device_ptr);
            if ( TERMINATOR_COMPONENT_TYPE_ENCLOSURE == type )
            {
                for(fbe_prom_id = 0; fbe_prom_id <= MAX_PROM_ID; fbe_prom_id++)
                {
                    status = set_dummy_resume_info(device_handle_array[i], fbe_prom_id, FALSE);
                    if(status != FBE_STATUS_OK)
                    {
                        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: error 0x%x setting PROM data.\n", __FUNCTION__,  status);
                        /* Ignore enclosure resume error for the case the enclosure is last device */
                        status = FBE_STATUS_OK;
                    }
                }
            }
        }
    }

    return status;
}

/* find the parent and insert the device to it */
static fbe_status_t terminator_file_api_insert_device(fbe_terminator_api_device_handle_t device_handle)
{
    tdr_device_ptr_t device_ptr;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t parent_ptr;
    fbe_terminator_api_device_handle_t parent_handle;

    fbe_u32_t port_number = -1;
    fbe_u32_t enclosure_number = -1;
    fbe_u32_t slot_number = -1;

    fbe_enclosure_type_t encl_protocol;
    fbe_drive_type_t drive_protocol;

    terminator_component_type_t type;
    void *attributes = NULL;

    tdr_status = fbe_terminator_device_registry_get_device_ptr(device_handle, &device_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Cannot find device in registry.\n",
                         __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    type = base_component_get_component_type(device_ptr);
    attributes = base_component_get_attributes(device_ptr);
    if(attributes == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Cannot get device attributes.\n",
                         __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* find its parent and insert the device into it */
    switch(type)
    {
    case TERMINATOR_COMPONENT_TYPE_BOARD:
        /* boards are not inserted */
        break;

    case TERMINATOR_COMPONENT_TYPE_PORT:
        /* insert port into the board */
        fbe_terminator_api_get_board_handle(&parent_handle);
        tdr_status = fbe_terminator_device_registry_get_device_ptr(parent_handle, &parent_ptr);
        if(tdr_status != TDR_STATUS_OK)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: Cannot find device in registry.\n",
                             __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        terminator_insert_device_new(parent_ptr, device_ptr);
        break;

    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        /* insert enclosure into the appropriate port */
        encl_protocol = enclosure_get_protocol((terminator_enclosure_t *)device_ptr);

        switch(encl_protocol)
            {
                case FBE_ENCLOSURE_TYPE_SAS:
                    port_number = ((terminator_sas_enclosure_info_t *)attributes)->backend;
                break;
                case FBE_ENCLOSURE_TYPE_FIBRE:
//                    port_number = ((terminator_fc_enclosure_info_t *)attributes)->backend;
//                break;
                default:
                    terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s: Unknown enclosure type.\n",
                                     __FUNCTION__);
                    status = FBE_STATUS_GENERIC_FAILURE;
                break;
             }
        fbe_terminator_api_get_port_handle(port_number, &parent_handle);
        tdr_status = fbe_terminator_device_registry_get_device_ptr(parent_handle, &parent_ptr);		
        if (tdr_status != TDR_STATUS_OK)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: Cannot get parent device pointer.\n",
                             __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        terminator_insert_device_new(parent_ptr, device_ptr);
        break;

    case TERMINATOR_COMPONENT_TYPE_DRIVE:
        drive_protocol = drive_get_protocol((terminator_drive_t *)device_ptr);

        switch(drive_protocol)
        {
            case FBE_DRIVE_TYPE_SAS:
            case FBE_DRIVE_TYPE_SAS_FLASH_HE: 
            case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            case FBE_DRIVE_TYPE_SAS_FLASH_RI:
                port_number      = ((terminator_sas_drive_info_t  *)attributes)->backend_number;
                enclosure_number = ((terminator_sas_drive_info_t  *)attributes)->encl_number;
                slot_number = ((terminator_sas_drive_info_t  *)attributes)->slot_number;
                break;
            case FBE_DRIVE_TYPE_SATA:
            case FBE_DRIVE_TYPE_SATA_FLASH_HE: 
                port_number      = ((terminator_sata_drive_info_t  *)attributes)->backend_number;
                enclosure_number = ((terminator_sata_drive_info_t  *)attributes)->encl_number;
                slot_number = ((terminator_sata_drive_info_t  *)attributes)->slot_number;

                break;
            case FBE_DRIVE_TYPE_FIBRE:
//              port_number      = ((terminator_fc_drive_info_t  *)attributes)->backend_number;
//              enclosure_number = ((terminator_fc_drive_info_t  *)attributes)->encl_number;
//              break;
            default:
                terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: Unknown drive type.\n",
                                 __FUNCTION__);
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
        }

        fbe_terminator_api_get_enclosure_handle(port_number, enclosure_number, &parent_handle);
        tdr_status = fbe_terminator_device_registry_get_device_ptr(parent_handle, &parent_ptr);
        if (tdr_status != TDR_STATUS_OK)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: Cannot get parent device pointer.\n",
                             __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        terminator_insert_device_new(parent_ptr, device_ptr);
        break;

    default:
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Unknown device type.\n",
                         __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }


    return FBE_STATUS_OK;
}

fbe_status_t terminator_file_api_init()
{
    fbe_u32_t port_index = 0;

    /* init queues for all the ports */
    for (port_index = 0; port_index < FBE_CPD_SHIM_MAX_PORTS; ++port_index)
    {
        fbe_queue_init(&device_list[port_index]);
        fbe_queue_init(&temp_device_list[port_index]);
    }

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: === loading config file, filename: %s ===\n",
                     __FUNCTION__,
                     terminator_config_file_name);

    return load_configuration();
}

fbe_status_t terminator_file_api_destroy (void)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   d = 0;
    term_device_info_t  *       term_device_info_ptr;


    /*kill the device list queue*/
    for (d = 0; d < FBE_CPD_SHIM_MAX_PORTS; d++) {
        if (device_list[d].next != NULL)
        {
            /*delete all the devices we created*/
            while (!fbe_queue_is_empty(&device_list[d])) {
                term_device_info_ptr = (term_device_info_t*)fbe_queue_pop (&device_list[d]);
                fbe_terminator_free_memory ((void *)term_device_info_ptr);
            }
            /*destroy the device list queue*/
            fbe_queue_destroy (&device_list[d]);
        }
        if (temp_device_list[d].next != NULL)
        {
            while (!fbe_queue_is_empty(&temp_device_list[d])) {
                term_device_info_ptr = (term_device_info_t*)fbe_queue_pop (&temp_device_list[d]);
                fbe_terminator_free_memory ((void *)term_device_info_ptr);
            }
            fbe_queue_destroy (&temp_device_list[d]);
        }
    }

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: Done - Terminator File API exit\n", __FUNCTION__);

    return status;

}


static fbe_status_t load_configuration(void)
{

    fbe_file_handle_t   inputFile = NULL;
    fbe_u8_t            line_buffer[MAX_LINE_SIZE]={""};
    fbe_u32_t           read_bytes = 0;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t            port_number_str[MAX_GENERIC_NUMBER_CHARS]={""};
    fbe_s32_t           port_number = -1;
    fbe_bool_t          port_found = FBE_FALSE;
    term_device_type_t  device_type;

    status = open_configuration_file(&inputFile);
    if(inputFile == FBE_FILE_INVALID_HANDLE){
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: XML file not found, make sure it's in the same directory with simulation exe "
#ifdef ALAMOSA_WINDOWS_ENV
                         "or in <view>\\catmerge\\disk\\fbe\\src\\services\\terminator\\config_files\\\n",
#else
                         "or in <view>/catmerge/disk/fbe/src/services/terminator/config_files/\n",
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
                         __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    do{
        /*start fresh*/
        fbe_zero_memory (line_buffer, MAX_LINE_SIZE);
        fbe_zero_memory (port_number_str, MAX_GENERIC_NUMBER_CHARS);

        /*read a line from the file*/
        status = read_xml_line_from_file (inputFile, line_buffer, MAX_LINE_SIZE, &read_bytes);
        if (status != FBE_STATUS_OK) {
            fbe_file_close (inputFile);
            return status;
        }

        if (strstr (line_buffer, "<?xml") != NULL || strstr (line_buffer, "</")) {
            continue;/*skip the header or end tags*/
        } else if (read_bytes == 0) {
            break;/*we are done*/
        }

        /*see what kind of a device it is, if it's a port, we would remember the number,
        we might need it for later and in some instances we might not be able to traverse back the file to look for it*/
        status = extract_device_type_from_line (line_buffer, &device_type);
        if (status != FBE_STATUS_OK) {
            fbe_file_close (inputFile);
            return status;
        }

        /*in the future, add a check for fibre port for example by doing || device_type == FIBRE_PORT_DEVICE*/
        if (device_type == SAS_PORT_DEVICE) {
            /*extract the port number field from the input line*/
            status  = extract_attribute_value (line_buffer, "Backend_number", port_number_str, MAX_GENERIC_NUMBER_CHARS);
            if (status != FBE_STATUS_OK) {
                terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: failed to get Backend port number\n", __FUNCTION__);
                fbe_file_close (inputFile);
                return status;
            }

            port_number = atoi (port_number_str);
            port_found = FBE_TRUE;

        }

        // a port_number of -1 will cause an illegal array reference.
        // do not call process device with port_number = -1 for anything
        // other than a board device
        if (( BOARD_DEVICE != device_type ) && (port_number == -1)) {
            fbe_file_close (inputFile);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*and add this device to our internal map and the terminator at this stage. If it's not the board device
        we assume the port is found*/
        status = process_device(device_type, line_buffer,port_number);
        if (status != FBE_STATUS_OK) {

            fbe_file_close (inputFile);
            return status;
        }


    }while (read_bytes !=0);

    fbe_file_close(inputFile);

    return FBE_STATUS_OK;
}

static fbe_status_t open_configuration_file(fbe_file_handle_t* inputFile)
{
    *inputFile = fbe_file_open (terminator_config_file_name_full, FBE_FILE_RDONLY, 0, NULL);
    if (FBE_FILE_INVALID_HANDLE == *inputFile)
    {
        // This shouldn't happen since we should have already opened the file to
        // check the version.
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "Error opening %s\n", terminator_config_file_name_full);
    }

    return  FBE_STATUS_OK;
}

static fbe_status_t read_xml_line_from_file (fbe_file_handle_t inputFile, char *buffer, size_t buf_size, fbe_u32_t *total_bytes)
{
    char c = 0;
    int c_raw = 0;
    fbe_u8_t *end_addr = buffer + buf_size;

    *total_bytes = 0;

    do{
      c_raw = fbe_file_fgetc (inputFile);
      c = (char) c_raw;
      if (c_raw == EOF){
          if (buffer == (char *)end_addr){
              terminator_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "file ended without a closing tag, data not valid\n");
              return FBE_STATUS_GENERIC_FAILURE;
          }
          if (buffer[-1] == '\n' || buffer[-1] == '>'){
                *total_bytes = 0;
                break;
          }

          c = '\n';
          *total_bytes = 0;
          break;
      }
      if (buffer == (char *)end_addr){
          terminator_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "Simultion device too big to read into buffer, buffer size is %llu chars \n", (unsigned long long)buf_size);
          return FBE_STATUS_GENERIC_FAILURE;
      }

      *buffer++ = c;
      (*total_bytes)++;

    } while (c != '>');/*we look for a closing XML tag*/

    return FBE_STATUS_OK;
}

static fbe_status_t process_device(term_device_type_t device_type, fbe_u8_t *xml_line, fbe_s32_t port_number)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
        
    switch (device_type) {
        case BOARD_DEVICE:
            status = process_board_device (xml_line);
            break;
        case SAS_PORT_DEVICE:
            status = process_port_device (xml_line, port_number);
            break;
        case SAS_ENCLOSURE_DEVICE:
            status = process_enclosure_device (xml_line, port_number);
            break;
        case SAS_DRIVE_DEVICE:
            status = process_sas_drive_device (xml_line, port_number);
            break;
        case SATA_DRIVE_DEVICE:
            status = process_sata_drive_device (xml_line, port_number);
            break;
        default:
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: unknown device type:%d\n", __FUNCTION__, device_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;

}

static fbe_status_t process_board_device(fbe_u8_t *xml_line)
{
    fbe_u8_t                        device_type_buf[MAX_GENERIC_STRING_CHARS]={""};
    fbe_u8_t                        spid_buf[MAX_GENERIC_STRING_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t     board_info;
    terminator_sp_id_t              spid;

    /*extract the device type field from the input line*/
    status  = extract_attribute_value (xml_line, "Board_type", device_type_buf, MAX_GENERIC_STRING_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get board type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*convert the board type string to enum*/
    (void )convert_board_string_to_enum (device_type_buf, &board_info.board_type);

    status = extract_attribute_value(xml_line , "sp_id", spid_buf , MAX_GENERIC_STRING_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Failed to get sp_id \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = convert_spid_string_to_enum(spid_buf, &spid); 
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Invalid SP_ID string (not \"A\" or \"B\" \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else {
        fbe_terminator_api_set_sp_id(spid);
    }

    /*extract the device type field from the input line*/
    status  = extract_attribute_value (xml_line, "Platform_type", device_type_buf, MAX_GENERIC_STRING_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get platform type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*convert the platform type string to enum*/
    status = convert_platform_string_to_enum (device_type_buf, &board_info.platform_type);

    /*and insert it to the terminator*/
    status = fbe_terminator_api_insert_board (&board_info);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get insert board to terminator\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

static fbe_status_t process_port_device(fbe_u8_t *xml_line, fbe_s32_t port_number)
{
    fbe_u8_t                        device_type_buf[MAX_GENERIC_STRING_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_sas_port_info_t  port_info;
    term_device_info_t *            sas_to_dev_ptr = NULL;
    fbe_u8_t                        sn[MAX_SN_CHARS];

    EmcpalZeroVariable(sn);
    /*extract the device type field from the input line*/
    status  = extract_attribute_value (xml_line, "Port_type", device_type_buf, MAX_GENERIC_STRING_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get port type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*convert the port type string to enum*/
    status = convert_port_string_to_enum (device_type_buf, &port_info.port_type);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to convert port string to enum\n", __FUNCTION__);
        return status;
    }

    /*extract sas address*/
    status = extract_sas_address_from_line (xml_line, &port_info.sas_address);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sas addr\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*extract sn*/
    status = extract_sn_from_line (xml_line, sn, MAX_SN_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sn\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*extract io_port_number*/
    status = extract_io_port_number_from_line (xml_line, &port_info.io_port_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:failed to get io_port_number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*extract portal_number*/
    status = extract_portal_number_from_line (xml_line, &port_info.portal_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:failed to get portal_number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*extract backend_number*/
    status = extract_backend_number_from_line (xml_line, &port_info.backend_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:failed to get portal_number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*we also keep track of the sas address and cpd device id couples to be able to remove it if needed
    for the port we cheat and don't really put a device id because it has none*/
    sas_to_dev_ptr = fbe_terminator_allocate_memory(sizeof(term_device_info_t));
    if (sas_to_dev_ptr == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for sas device info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    sas_to_dev_ptr->device_id   = 0;
    sas_to_dev_ptr->port_number = port_number;
    sas_to_dev_ptr->handle      = NULL;
    fbe_copy_memory(sas_to_dev_ptr->sn, sn, MAX_SN_CHARS);
    sas_to_dev_ptr->device_type = SAS_PORT_DEVICE;
    sas_to_dev_ptr->slot        = FBE_SLOT_NUMBER_INVALID;
    fbe_copy_memory(&sas_to_dev_ptr->type_info.sas_port_info, &port_info, sizeof(fbe_terminator_sas_port_info_t));

    /*and insert it to the terminator*/
    status = send_insert_to_port_device(sas_to_dev_ptr);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: failed to insert port to terminator\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_queue_push (&device_list[port_number], &sas_to_dev_ptr->queue_element);

    return status;
}

static fbe_status_t process_enclosure_device(fbe_u8_t *xml_line, fbe_s32_t port_number)
{
    fbe_u8_t                        device_type_buf[MAX_GENERIC_STRING_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_sas_encl_info_t  encl_info;
    term_device_info_t *            sas_to_dev_ptr = NULL;
    fbe_u8_t                        sn[MAX_SN_CHARS];

    EmcpalZeroVariable(sn);
    /*extract the device type field from the input line*/
    status  = extract_attribute_value (xml_line, "Enclosure_type", device_type_buf, MAX_GENERIC_STRING_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get port type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*convert the encl type string to enum*/
    status = convert_enclosure_string_to_enum (device_type_buf, &encl_info.encl_type);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to convert enclosure string to enum\n", __FUNCTION__);
        return status;
    }

    /*extract sas address*/
    status = extract_sas_address_from_line (xml_line, &encl_info.sas_address);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sas addr\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*extract uid*/
    status = extract_uid_from_line (xml_line, encl_info.uid, MAX_UID_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:failed to get uid\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*extract sn*/
    status = extract_sn_from_line (xml_line, sn, MAX_SN_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sn\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we also keep track of the sas address and cpd device id couples to be able to remove it if needed*/
    sas_to_dev_ptr = (term_device_info_t *)fbe_terminator_allocate_memory(sizeof(term_device_info_t));
    if (sas_to_dev_ptr == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for sas device info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    sas_to_dev_ptr->device_id   = 0;
    sas_to_dev_ptr->port_number = port_number;
    sas_to_dev_ptr->handle      = NULL;
    fbe_copy_memory(sas_to_dev_ptr->sn, sn, MAX_SN_CHARS);
    sas_to_dev_ptr->device_type = SAS_ENCLOSURE_DEVICE;
    sas_to_dev_ptr->slot        = FBE_SLOT_NUMBER_INVALID;
    fbe_copy_memory(&sas_to_dev_ptr->type_info.sas_encl_info, &encl_info, sizeof(fbe_terminator_sas_encl_info_t));

    /*and insert it to the terminator*/
    status = send_insert_to_enclosure_device(sas_to_dev_ptr);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to insert enclosure to terminator\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_queue_push (&device_list[port_number], &sas_to_dev_ptr->queue_element);

    return status;
}

static fbe_status_t process_sas_drive_device(fbe_u8_t *xml_line, fbe_s32_t port_number)
{
    fbe_u8_t                        device_type_buf[MAX_GENERIC_STRING_CHARS]={""};
    fbe_u8_t                        slot_number[MAX_GENERIC_NUMBER_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_sas_drive_info_t drive_info;
    term_device_info_t *            drive_to_dev_ptr = NULL;
    fbe_u8_t                        sn[MAX_SN_CHARS];
    fbe_u32_t                       slot = 0;

    EmcpalZeroVariable(sn);
    /*extract the device type field from the input line*/
    status  = extract_attribute_value (xml_line, "Drive_type", device_type_buf, MAX_GENERIC_STRING_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get port type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*extract the slot number from the input line*/
    status  = extract_attribute_value (xml_line, "Slot", slot_number, MAX_GENERIC_NUMBER_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get slot number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    slot = atoi (slot_number);

    /*convert the drive type string to enum*/
    status = convert_sas_drive_string_to_enum (device_type_buf, &drive_info.drive_type);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to convert drive string to enum\n", __FUNCTION__);
        return status;
    }

    /*extract sas address*/
    status = extract_sas_address_from_line (xml_line, &drive_info.sas_address);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sas addr\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*extract sn*/
    status = extract_sn_from_line (xml_line, sn, MAX_SN_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sn\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&drive_info.drive_serial_number, sn, MAX_SN_CHARS);

    /*we also keep track of the sas address and cpd device id couples to be able to remove it if needed*/
    drive_to_dev_ptr = (term_device_info_t *)fbe_terminator_allocate_memory(sizeof(term_device_info_t));
    if (drive_to_dev_ptr == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for device info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_to_dev_ptr->device_id   = 0;
    drive_to_dev_ptr->port_number = port_number;
    drive_to_dev_ptr->handle      = NULL;
    fbe_copy_memory(drive_to_dev_ptr->sn, sn, MAX_SN_CHARS);
    drive_to_dev_ptr->device_type = SAS_DRIVE_DEVICE;
    drive_to_dev_ptr->slot        = slot;
    fbe_copy_memory(&drive_to_dev_ptr->type_info.sas_drive_info, &drive_info, sizeof(fbe_terminator_sas_drive_info_t));

    fbe_queue_push (&device_list[port_number], &drive_to_dev_ptr->queue_element);

    /*and insert it to the terminator*/
    status = send_insert_to_drive_device(drive_to_dev_ptr);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to insert drive to terminator\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*********************************************************************
 *            extract_attribute_value ()
 *********************************************************************
 *
 *  Description:
 *    read the value of an attribute from a buffer
 *
 *  Input:buffer - the line to read from
 *        attr_name - the attribute to read
 *        ret_value - where to store the result
 *        ret_val_size - size of result buffer
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/19/08: sharel   created
 *
 *********************************************************************/
static fbe_status_t extract_attribute_value (fbe_u8_t *buffer, fbe_u8_t *attr_name, fbe_u8_t *ret_value, size_t ret_val_size)
{
    fbe_u8_t *      value_start = NULL;
    fbe_u8_t *      value_end = NULL;
    fbe_u8_t *      value_string = strstr (buffer, attr_name);

    /*can we find the requested attribue in the line ?*/
    if (value_string == NULL){

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory (ret_value, (fbe_u32_t)ret_val_size);/*start with a clean buffer*/

    /*let's extract the value out*/
    value_start = strchr (value_string, '"');/*after this one it's the value*/
    if (value_start == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    value_end = strchr (value_start + 1, '"');/*this is the " sign after the value*/

    if (value_end == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*does the return buffer has enough space ?*/
    if (((size_t)(value_end - value_start - 1)) > ret_val_size) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory (ret_value, value_start + 1, (fbe_u32_t)(value_end - value_start - 1));

    return FBE_STATUS_OK;

}

static fbe_status_t convert_board_string_to_enum(fbe_u8_t *board_str, fbe_board_type_t *board_enum)
{
    board_str_to_enum_t *table = board_str_to_enum_table;

    while (table->board_type != FBE_BOARD_TYPE_LAST) {
        if (!strcmp (board_str, table->board_str)) {
            *board_enum = table->board_type;
            return FBE_STATUS_OK;
        }

        table++;
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

static fbe_status_t convert_spid_string_to_enum(fbe_u8_t *spid_str, terminator_sp_id_t *spid)
{
    if (!strcmp (spid_str, "A")){
        *spid = TERMINATOR_SP_A;
        return FBE_STATUS_OK;
    }
    else if (!strcmp (spid_str, "B")) {
        *spid = TERMINATOR_SP_B;
        return FBE_STATUS_OK;
    }
    else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

static fbe_status_t convert_platform_string_to_enum(fbe_u8_t *platform_str, SPID_HW_TYPE *platform_enum)
{
    platform_str_to_enum_t *   table = platform_str_to_enum_table;

    while (table->platform_type != SPID_MAXIMUM_HW_TYPE) {
        if (!strcmp (platform_str, table->platform_str)) {
            *platform_enum = table->platform_type;
            return FBE_STATUS_OK;
        }

        table++;
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

static fbe_status_t extract_device_type_from_line (fbe_u8_t *xml_line, term_device_type_t *device_type)
{
    fbe_u8_t *                  xml_open_tag = NULL;/*the < sign*/
    fbe_u8_t *                  first_space = NULL;/*the < sign*/
    fbe_u8_t                    devie_type_str[50] = {""};
    fbe_u8_t                    generic_string[MAX_GENERIC_STRING_CHARS] = {""};
    fbe_status_t                status;
    fbe_port_type_t             port_type;

    xml_open_tag = strchr (xml_line, '<');/*after this one it's the value*/
    first_space = strchr (xml_open_tag + 1, ' ');/*first space after the <*/

    fbe_copy_memory (devie_type_str, xml_open_tag + 1, (fbe_u32_t)(first_space - xml_open_tag - 1));

    /*check if this is a borad*/
    if (!strcmp (devie_type_str, "Board")) {
        *device_type = BOARD_DEVICE;
        return FBE_STATUS_OK;
    }
    /*check if this is a port*/
    if (!strcmp (devie_type_str, "Port")){
        /*extract the port type the input line*/
        status  = extract_attribute_value (xml_line, "Port_type", generic_string, MAX_GENERIC_STRING_CHARS);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: failed to get port attribute\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = convert_port_string_to_enum (generic_string, &port_type);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: failed to convert port string to enum\n", __FUNCTION__);
            return status;
        }

        switch (port_type) {
            case FBE_PORT_TYPE_SAS_PMC:
            case FBE_PORT_TYPE_SAS_LSI:
                *device_type = SAS_PORT_DEVICE;
                return FBE_STATUS_OK;
            case FBE_PORT_TYPE_FC_PMC:
                *device_type = FC_PORT_DEVICE;
                return FBE_STATUS_OK;
            case FBE_PORT_TYPE_ISCSI:
                *device_type = ISCSI_PORT_DEVICE;
                return FBE_STATUS_OK;
            case FBE_PORT_TYPE_FCOE:
                *device_type = FCOE_PORT_DEVICE;
                return FBE_STATUS_OK;
            default:
                terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: unknown port type type: %s\n", __FUNCTION__, generic_string);
                return FBE_STATUS_GENERIC_FAILURE;
        }

    }

    /*check if this is an enclosure*/
    if (!strcmp (devie_type_str, "Enclosure")){
        /*extract the enclosure type the input line*/
        status  = extract_attribute_value (xml_line, "Enclosure_type", generic_string, MAX_GENERIC_STRING_CHARS);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: failed\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (!strcmp (generic_string, "FBE_SAS_ENCLOSURE_TYPE_VIPER") || !strcmp (generic_string, "FBE_SAS_ENCLOSURE_TYPE_BULLET")) {
            *device_type = SAS_ENCLOSURE_DEVICE;
            return FBE_STATUS_OK;
        } else {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: unknown enclosure type type: %s\n", __FUNCTION__, generic_string);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*check if this is a drive*/
    if (!strcmp (devie_type_str, "Drive")){
        /*extract the enclosure type the input line*/
        status  = extract_attribute_value (xml_line, "Drive_type", generic_string, MAX_GENERIC_STRING_CHARS);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: failed\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (!strcmp (generic_string, "FBE_SAS_DRIVE_CHEETA_15K")) {
            *device_type = SAS_DRIVE_DEVICE;
            return FBE_STATUS_OK;
        } else if (!strcmp (generic_string, "FBE_SAS_DRIVE_UNICORN_512")) {
            *device_type = SAS_DRIVE_DEVICE;
            return FBE_STATUS_OK;
        } else if (!strcmp (generic_string, "FBE_SAS_DRIVE_SIM_520")) {
            *device_type = SAS_DRIVE_DEVICE;
            return FBE_STATUS_OK;
        } else if (!strcmp (generic_string, "FBE_SAS_DRIVE_SIM_512")) {
            *device_type = SAS_DRIVE_DEVICE;
            return FBE_STATUS_OK;
        } else if (!strcmp (generic_string, "FBE_SATA_DRIVE_HITACHI_HUA")) {
            *device_type = SATA_DRIVE_DEVICE;
            return FBE_STATUS_OK;
        } else if (!strcmp (generic_string, "FBE_SATA_DRIVE_SIM_512")) {
            *device_type = SATA_DRIVE_DEVICE;
            return FBE_STATUS_OK;
        } else if (!strcmp (generic_string, "FBE_SAS_DRIVE_COBRA_E_10K")) {
            *device_type = SAS_DRIVE_DEVICE;
            return FBE_STATUS_OK;
        } else {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: unknown drive type:%s\n", __FUNCTION__, generic_string);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    }

    terminator_trace(FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: unknown element type: %s\n", __FUNCTION__, devie_type_str);
    return FBE_STATUS_GENERIC_FAILURE;
}


static fbe_status_t convert_port_string_to_enum(fbe_u8_t *port_str, fbe_port_type_t *port_enum)
{
    port_str_to_enum_t *table = port_str_to_enum_table;

    while (table->port_type != FBE_PORT_TYPE_LAST) {
        if (!strcmp (port_str, table->port_str)) {
            *port_enum = table->port_type;
            return FBE_STATUS_OK;
        }

        table++;
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;

}
static fbe_status_t extract_io_port_number_from_line(fbe_u8_t *xml_line, fbe_u32_t *io_port_number)
{
    fbe_u8_t                        io_port_number_buf[MAX_GENERIC_NUMBER_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

    /*extract the sas address field from the input line*/
    status  = extract_attribute_value (xml_line, "IO_port_number", io_port_number_buf, MAX_GENERIC_NUMBER_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:failed to get io_port_number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *io_port_number  = atoi(io_port_number_buf);
    return status;
}

static fbe_status_t extract_portal_number_from_line(fbe_u8_t *xml_line, fbe_u32_t *portal_number)
{
    fbe_u8_t                        portal_number_buf[MAX_GENERIC_NUMBER_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

    /*extract the sas address field from the input line*/
    status  = extract_attribute_value (xml_line, "Portal_number", portal_number_buf, MAX_GENERIC_NUMBER_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:failed to get portal_number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *portal_number  = atoi (portal_number_buf);
    return FBE_STATUS_OK;
}

static fbe_status_t extract_backend_number_from_line(fbe_u8_t *xml_line, fbe_u32_t *backend_number)
{
    fbe_u8_t                        backend_number_buf[MAX_GENERIC_NUMBER_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

    /*extract the sas address field from the input line*/
    status  = extract_attribute_value (xml_line, "Backend_number", backend_number_buf, MAX_GENERIC_NUMBER_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:failed to get backend_number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *backend_number = atoi (backend_number_buf);
    return FBE_STATUS_OK;
}

static fbe_status_t extract_sas_address_from_line(fbe_u8_t *xml_line, fbe_sas_address_t *sas_address)
{
    fbe_u8_t                        sas_addr_buf[MAX_GENERIC_NUMBER_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

    /*extract the sas address field from the input line*/
    status  = extract_attribute_value (xml_line, "Sas_address", sas_addr_buf, MAX_GENERIC_NUMBER_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sas address\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = xtoi (sas_addr_buf, (fbe_s32_t)strlen(sas_addr_buf), sas_address );
    return status;
}

static fbe_status_t xtoi(const fbe_u8_t* hexStg, fbe_s32_t szlen, fbe_sas_address_t* result)
{

      fbe_s32_t      n= 0;               /* position in string*/
      fbe_u64_t      intValue = 0;  /* integer value of hex string*/
      fbe_u32_t      digit[32];        /* hold values to convert*/
      fbe_u32_t      p = 0;             /*power factor*/

      EmcpalZeroVariable(digit);
      /*tokenize all digits to an array containing their value*/
      while (n < szlen)
      {
         if (hexStg[n]=='\0')
            break;
         if (hexStg[n] > 0x29 && hexStg[n] < 0x40 ) /*if 0 to 9*/
            digit[n] = hexStg[n] & 0x0f;   /*convert to int*/
         else if (hexStg[n] >='a' && hexStg[n] <= 'f') /*if a to f*/
            digit[n] = (hexStg[n] & 0x0f) + 9;      /*convert to int*/
         else if (hexStg[n] >='A' && hexStg[n] <= 'F') /*if A to F*/
            digit[n] = (hexStg[n] & 0x0f) + 9;      /*convert to int*/
         else break;
        n++;
      }

      n--;                  //back one digit

      while(n >= 0)
      {
         intValue = intValue + (digit[n] * (fbe_u64_t)pow (2, p));
         n--;   /* next digit to process*/
         p+=4;  /*everytime the power goes up in 4*/
      }

     *result = (fbe_sas_address_t)intValue;

     return FBE_STATUS_OK;
}

static fbe_status_t convert_enclosure_string_to_enum(fbe_u8_t *encl_str, fbe_sas_enclosure_type_t *encl_enum)
{
    encl_str_to_enum_t *table = encl_str_to_enum_table;

    while (table->encl_type != FBE_SAS_ENCLOSURE_TYPE_LAST) {
        if (!strcmp (encl_str, table->encl_str)) {
            *encl_enum = table->encl_type;
            return FBE_STATUS_OK;
        }

        table++;
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}


                //fbe_terminator_api_remove_sas_port (const_term_device_info_ptr->port_number);
static fbe_status_t extract_sn_from_line(fbe_u8_t *xml_line, fbe_u8_t *sn, fbe_u32_t length)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

    /*extract the sn address field from the input line*/
    status  = extract_attribute_value (xml_line, "SN", sn, length);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get serial number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

static fbe_status_t extract_uid_from_line(fbe_u8_t *xml_line, fbe_u8_t *uid, fbe_u32_t length)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

    /*extract the sn address field from the input line*/
    status  = extract_attribute_value (xml_line, "UID", uid, length);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:failed to get UID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*this insert would send a login message based on the port type*/
static fbe_status_t send_insert_to_port_device (term_device_info_t* new_term_element)
{
    switch (new_term_element->device_type) {
    case SAS_PORT_DEVICE:
        //fbe_terminator_api_insert_sas_port(&new_term_element->port_number, &new_term_element->type_info.sas_port_info);
        break;
    default:
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: got an illegal port type(%d)\n", __FUNCTION__, new_term_element->device_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*this insert would generate a login for the enclosure*/
static fbe_status_t send_insert_to_enclosure_device (term_device_info_t* new_term_element)
{
    switch (new_term_element->device_type) {
    case SAS_ENCLOSURE_DEVICE:
//      fbe_terminator_api_insert_sas_enclosure (new_term_element->port_number,
//                                               &new_term_element->type_info.sas_encl_info,
//                                               &new_term_element->device_id);
        break;
    default:
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: got an illegal enclosure type(%d)\n", __FUNCTION__, new_term_element->device_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

static fbe_status_t send_insert_to_drive_device (term_device_info_t* term_element)
{
    term_device_info_t *        current_element = NULL;
    fbe_terminator_api_device_handle_t  temp_device_id = 0;

    /*first of all, we need to go over the list and find the device id of our parent enclosure
    this needs to be improved a bit to be walked backwards*/
    current_element = (term_device_info_t *)fbe_queue_front (&device_list[term_element->port_number]);
    if(current_element != NULL)
    {
        do{
            if (current_element->device_type == SAS_ENCLOSURE_DEVICE) {
                temp_device_id = current_element->device_id;/*remember the last device id*/
            }

            current_element = (term_device_info_t *)fbe_queue_next (&device_list[term_element->port_number], &current_element->queue_element);
        }while (current_element != NULL && (memcmp(current_element, term_element, sizeof (term_device_info_t)) != 0));
    }

    switch (term_element->device_type) {
    case SAS_DRIVE_DEVICE:
//      fbe_terminator_api_insert_sas_drive (term_element->port_number,
//                                           temp_device_id,
//                                           term_element->slot,
//                                           &term_element->type_info.sas_drive_info,
//                                           &term_element->device_id);
        break;
    case SATA_DRIVE_DEVICE:
        fbe_terminator_api_insert_sata_drive (temp_device_id,
                                             term_element->slot,
                                             &term_element->type_info.sata_drive_info,
                                             &term_element->device_id);
        break;
    default:
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: got an illegal drive type(%d)\n", __FUNCTION__, term_element->device_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}


static fbe_status_t convert_sas_drive_string_to_enum(fbe_u8_t *drive_str, fbe_sas_drive_type_t *drive_enum)
{
    sas_drive_str_to_enum_t *table = sas_drive_str_to_enum_table;

    while (table->drive_type != FBE_SAS_DRIVE_LAST) {
        if (!strcmp (drive_str, table->drive_str)) {
            *drive_enum = table->drive_type;
            return FBE_STATUS_OK;
        }

        table++;
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

static fbe_status_t convert_sata_drive_string_to_enum(fbe_u8_t *drive_str, fbe_sata_drive_type_t *drive_enum)
{
    sata_drive_str_to_enum_t *table = sata_drive_str_to_enum_table;

    while (table->drive_type != FBE_SATA_DRIVE_LAST) {
        if (!strcmp (drive_str, table->drive_str)) {
            *drive_enum = table->drive_type;
            return FBE_STATUS_OK;
        }

        table++;
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

static fbe_status_t check_for_sata_drive_attribute_changes (fbe_terminator_sata_drive_info_t * const_drive, fbe_terminator_sata_drive_info_t * temp_drive)
{

    return FBE_STATUS_OK;
}

static fbe_status_t process_sata_drive_device(fbe_u8_t *xml_line, fbe_s32_t port_number)
{
    fbe_u8_t                        device_type_buf[MAX_GENERIC_STRING_CHARS]={""};
    fbe_u8_t                        slot_number[MAX_GENERIC_NUMBER_CHARS]={""};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_sata_drive_info_t    drive_info;
    term_device_info_t *            drive_to_dev_ptr = NULL;
    fbe_u8_t                        sn[MAX_SN_CHARS];
    fbe_u32_t                       slot = 0;

    EmcpalZeroVariable(sn);
    /*extract the device type field from the input line*/
    status  = extract_attribute_value (xml_line, "Drive_type", device_type_buf, MAX_GENERIC_STRING_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get port type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*extract the slot number from the input line*/
    status  = extract_attribute_value (xml_line, "Slot", slot_number, MAX_GENERIC_NUMBER_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get slot number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    slot = atoi (slot_number);

    /*convert the drive type string to enum*/
    status = convert_sata_drive_string_to_enum (device_type_buf, &drive_info.drive_type);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to convert drive string to enum\n", __FUNCTION__);
        return status;
    }

    /*extract sas address*/
    status = extract_sas_address_from_line (xml_line, &drive_info.sas_address);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sas addr\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*extract sn*/
    status = extract_sn_from_line (xml_line, sn, MAX_SN_CHARS);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to get sn\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&drive_info.drive_serial_number, sn, MAX_SN_CHARS);

    /*we also keep track of the sas address and cpd device id couples to be able to remove it if needed*/
    drive_to_dev_ptr = (term_device_info_t *)fbe_terminator_allocate_memory(sizeof(term_device_info_t));
    if (drive_to_dev_ptr == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for device info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_to_dev_ptr->device_id   = 0;
    drive_to_dev_ptr->port_number = port_number;
    drive_to_dev_ptr->handle      = NULL;
    fbe_copy_memory(drive_to_dev_ptr->sn, sn, MAX_SN_CHARS);
    drive_to_dev_ptr->device_type = SATA_DRIVE_DEVICE;
    drive_to_dev_ptr->slot        = slot;
    fbe_copy_memory(&drive_to_dev_ptr->type_info.sata_drive_info, &drive_info, sizeof(fbe_terminator_sata_drive_info_t));

    fbe_queue_push(&device_list[port_number], &drive_to_dev_ptr->queue_element);

    /*and insert it to the terminator*/
    status = send_insert_to_drive_device(drive_to_dev_ptr);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to insert drive to terminator\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

terminator_configuration_t terminator_file_api_get_configuration_format(fbe_u8_t *file_dir, const char *file_name)
{
    size_t file_dir_len              = strlen(file_dir),
           terminator_config_dir_len = file_dir_len;
#ifndef C4LX_VMWARE
#ifdef ALAMOSA_WINDOWS_ENV
    const char * const this_source_file_with_dir         = "xml_config\\fbe_terminator_config_file_api.c";
#else
    const char * const this_source_file_with_dir         = "xml_config/fbe_terminator_config_file_api.c";
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
    size_t terminator_path_len = strlen(__FILE__) - strlen(this_source_file_with_dir);
#ifdef ALAMOSA_WINDOWS_ENV
    const char * const config_dir_rel_to_simulation_exes = "..\\..\\..\\..\\catmerge\\disk\\fbe\\src\\services\\terminator\\config_files\\",
               * const config_dir_name                   = "config_files\\";
#else
    const char * const config_dir_rel_to_simulation_exes = "../../../../catmerge/disk/fbe/src/services/terminator/config_files/",
               * const config_dir_name                   = "config_files/";
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
#endif /* C4LX_VMWARE */

    fbe_file_handle_t file_handle  = NULL;

    char buffer[XML_CONFIGURATION_STRING_LEN] = {0};
    int i           = 0;
    int char_buffer = 0;

    /* make the config dir and make sure it ends with '\'
     */
    strncpy(terminator_config_dir, file_dir, sizeof(terminator_config_dir) - 1);

#ifndef C4LX_VMWARE
#ifdef ALAMOSA_WINDOWS_ENV
    if (terminator_config_dir[file_dir_len - 1] != '\\')
    {
        terminator_config_dir[file_dir_len] = '\\';
        ++terminator_config_dir_len;
    }
#else
    if (terminator_config_dir[file_dir_len - 1] != '/')
    {
        terminator_config_dir[file_dir_len] = '/';
        ++terminator_config_dir_len;
    }
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
#endif /* C4LX_VMWARE */

    /* make the config file name
     */
    strncpy(terminator_config_file_name, file_name, sizeof(terminator_config_file_name) - 1);

    /* make the full config file name
     */
    strncpy(terminator_config_file_name_full, terminator_config_dir,       sizeof(terminator_config_file_name_full) - 1);
    strncat(terminator_config_file_name_full, terminator_config_file_name, sizeof(terminator_config_file_name_full) - 1 - terminator_config_dir_len);

    /* open the config file
     */
    file_handle = fbe_file_open(terminator_config_file_name_full, FBE_FILE_RDONLY, 0, NULL);
    if (FBE_FILE_INVALID_HANDLE == file_handle)
    {
        terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: could not load XML configuration from %s\n",
                         __FUNCTION__,
                         terminator_config_file_name_full);

#ifndef C4LX_VMWARE
        /* now try the relative path to simulation exes like safe\Targets\armada64_checked\simulation\exec\fbe_test.exe
         */
        fbe_zero_memory(terminator_config_file_name_full, sizeof(terminator_config_file_name_full));

        strncpy(terminator_config_file_name_full, config_dir_rel_to_simulation_exes, sizeof(terminator_config_file_name_full) - 1);
        strncat(terminator_config_file_name_full, terminator_config_file_name,       sizeof(terminator_config_file_name_full) - 1 - strlen(config_dir_rel_to_simulation_exes));

        terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: will try to load XML configuration from %s\n",
                         __FUNCTION__,
                         terminator_config_file_name_full);

        /* open the relative path config file
         */
        file_handle = fbe_file_open(terminator_config_file_name_full, FBE_FILE_RDONLY, 0, NULL);
        if (FBE_FILE_INVALID_HANDLE == file_handle)
        {
            terminator_trace(FBE_TRACE_LEVEL_INFO,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: could not load XML configuration from %s\n",
                             __FUNCTION__,
                             terminator_config_file_name_full);

            /* now try the fixed path
             *
             * XML file may be          catmerge\disk\fbe\src\services\terminator\config_files\terminator_config_file_name,
             *  and this source file is catmerge\disk\fbe\src\services\terminator\xml_config\fbe_terminator_config_file_api.c.
             */
            fbe_zero_memory(terminator_config_file_name_full, sizeof(terminator_config_file_name_full));

            if (sizeof(terminator_config_file_name_full) - 1 < terminator_path_len)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: length of path to Terminator exceeds the config file full name buffer.\n",
                                 __FUNCTION__);

                return TC_MISSING;
            }

            strncpy(terminator_config_file_name_full, __FILE__, terminator_path_len);
            strncat(terminator_config_file_name_full, config_dir_name,             sizeof(terminator_config_file_name_full) - 1 - terminator_path_len);
            strncat(terminator_config_file_name_full, terminator_config_file_name, sizeof(terminator_config_file_name_full) - 1 - terminator_path_len - strlen(config_dir_name));

            terminator_trace(FBE_TRACE_LEVEL_INFO,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: will try to load XML configuration from %s\n",
                             __FUNCTION__,
                             terminator_config_file_name_full);

            /* open the fixed path config file
             */
            file_handle = fbe_file_open(terminator_config_file_name_full, FBE_FILE_RDONLY, 0, NULL);
        }
#endif // C4LX_VMWARE
    }

    if (FBE_FILE_INVALID_HANDLE == file_handle)
    {
        terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: could not load XML configuration from %s\n",
                         __FUNCTION__,
                         terminator_config_file_name_full);

        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: XML configuration file not found, make sure it's in one of the following directories: \n"
                         "1. Input dir.\n"
#ifdef ALAMOSA_WINDOWS_ENV
                         "2. catmerge\\disk\\fbe\\src\\services\\terminator\\config_files\\\n",
#else
                         "2. catmerge/disk/fbe/src/services/terminator/config_files/\n",
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
                         __FUNCTION__);

        return TC_MISSING;
    }

    /* search for first < */
    do
    {
        char_buffer = fbe_file_fgetc(file_handle);
    } while( (FBE_FILE_ERROR != char_buffer) && ('<' != char_buffer));
    if(FBE_FILE_ERROR == char_buffer)
    {
        fbe_file_close(file_handle);
        return TC_INVALID_XML;
    }
    /* search for > */
    do
    {
        char_buffer = fbe_file_fgetc(file_handle);
    } while( (FBE_FILE_ERROR != char_buffer) && ('>' != char_buffer));
    if(FBE_FILE_ERROR == char_buffer)
    {
        fbe_file_close(file_handle);
        return TC_INVALID_XML;
    }
    /* search for second < */
    do
    {
        char_buffer = fbe_file_fgetc(file_handle);
    } while( (FBE_FILE_ERROR != char_buffer) && ('<' != char_buffer));
    if(FBE_FILE_ERROR == char_buffer)
    {
        fbe_file_close(file_handle);
        return TC_INVALID_XML;
    }
    /* skip spaces */
    do
    {
        char_buffer = fbe_file_fgetc(file_handle);
    } while( (FBE_FILE_ERROR != char_buffer) && (char_buffer == ' '));
    if(FBE_FILE_ERROR == char_buffer)
    {
        fbe_file_close(file_handle);
        return TC_INVALID_XML;
    }

    /* copy first char from above read */
    buffer[i++] = char_buffer;

    /* read following word to buffer */
    while (1)
    {
        char_buffer = fbe_file_fgetc(file_handle);
        if ( (FBE_FILE_ERROR == char_buffer) ||
            (!isalnum(char_buffer)) ||
            ( i >= XML_CONFIGURATION_STRING_LEN - 1 ) )
        {
            break;
        }

        buffer[i++] = char_buffer;
    }
    if(FBE_FILE_ERROR == char_buffer)
    {
        fbe_file_close(file_handle);
        return TC_OLD_XML;
    }
    else
    {
        //append \0
        buffer[i] = '\0';
        if(!strcmp(buffer, "Board"))
        {
            fbe_file_close(file_handle);
            return TC_OLD_XML; /* old format has "Board" in the root node */
        }
        else
        {
            fbe_file_close(file_handle);
            return TC_NEW_XML; /* may be new parser is able to handle this? */
        }
    }
}

fbe_status_t fbe_terminator_api_load_config_file (fbe_u8_t *file_dir, fbe_u8_t *file_name)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    switch(current_configuration_format = terminator_file_api_get_configuration_format(file_dir, file_name))
    {
    case TC_INVALID_XML:
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "Configuration file %s seems malformed\n", file_name);
        return terminator_file_api_init(); /* Try to load empty configuration*/
    case TC_NEW_XML:
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "Configuration file %s has new file format\n", file_name);
        return terminator_file_api_init_new();
    case TC_OLD_XML:
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "Configuration file %s has old file format\n", file_name);
        return terminator_file_api_init();
    case TC_MISSING:
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "Configuration file %s not found\n", file_name);
        return FBE_STATUS_GENERIC_FAILURE;
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

fbe_status_t fbe_terminator_api_unload_config_file ()
{

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    switch(current_configuration_format)
    {
    case TC_NEW_XML:
        return terminator_file_api_destroy_new();
    case TC_OLD_XML:
        return terminator_file_api_destroy();/*destroy config file internal data strucutres*/
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

fbe_status_t fbe_terminator_api_load_disk_storage_dir(fbe_u8_t *disk_storage_dir)
{
    size_t disk_storage_dir_len = strlen(disk_storage_dir);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n",
                     __FUNCTION__);

    strncpy(terminator_disk_storage_dir, disk_storage_dir, sizeof(terminator_disk_storage_dir) - 1);

#ifdef ALAMOSA_WINDOWS_ENV
    if (terminator_disk_storage_dir[disk_storage_dir_len - 1] != '\\')
    {
        terminator_disk_storage_dir[disk_storage_dir_len] = '\\';
    }
#else
    if (terminator_disk_storage_dir[disk_storage_dir_len - 1] != '/')
    {
        terminator_disk_storage_dir[disk_storage_dir_len] = '/';
    }
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_load_access_mode(fbe_u32_t access_mode)
{
    terminator_access_mode = terminator_access_mode | access_mode;
    return FBE_STATUS_OK;
}
