/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_becheck.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for becheck
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   06/03/2015:  Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe_block_transport_trace.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cli_physical_drive_obj.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_eses.h"
#include "../fbe/src/lib/enclosure_data_access/edal_base_enclosure_data.h"
#include "fbe_eses_enclosure_debug.h"
//#include "fbe_sas_enclosure_utils.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_limits.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_cli_becheck.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_api_database_interface.h"

/* neob backend check codes */
#define C4_BE_UNSPECIFIED             0x80b0
#define C4_BE_DISK_MISSING            0x80b4
#define C4_BE_NO_SAS_CONTROLLER       0x80b7
#define C4_BE_INVALID_DISK_TYPE       0x80b8
#define C4_BE_NO_IO_TO_SAS_DISKS      0x80b9
#define C4_BE_FAULTED_DISK            0x80bd
#define C4_BE_FAILED_TO_RUN_CHECK     0x80be
#define C4_BE_MISMATCHED_DISK_TYPES   0x80bf
#define C4_BE_INVALID_DISK_BSIZE      0x80c0
#define C4_BE_MISMATCHED_DISK_SIZES   0x80c1
#define C4_BE_INVALID_CHASSIS_SN      0x80c2

#define C4_FBE_CLI_BECHECK_TYPE_INSTALL   0
#define C4_FBE_CLI_BECHECK_TYPE_NORMAL    1
#define C4_FBE_CLI_BECHECK_TYPE_FULL      2

#define C4_FBE_CLI_BECHECK_DISKS_TO_CHECK 4
#define C4_FBE_CLI_MAX_SAS_PORTS          2
#define C4_FBE_CLI_MAX_ENCLOSURES         17
#define C4_FBE_CLI_MAX_EVERGREEN_DPE_DISK_SLOTS  25
#define C4_FBE_CLI_MAX_SENTRY_DPE_DISK_SLOTS     25
#define C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS 25
#define C4_FBE_CLI_MAX_DPE_DISK_SLOTS     25

#define C4_FBE_CLI_VALID_DISK_BLOCK_SIZE  	520
#define C4_FBE_CLI_VALID_DISK_BLOCK_SIZE_4K	(520 * 8)

#define C4_FBE_CLI_MAX_DRIVE_NAME_SIZE         30
#define C4_FBE_CLI_MAX_ENCLOSURE_NAME_SIZE     30
#define C4_FBE_CLI_MAX_ENCLOSURE_STATE_SIZE    16
#define C4_FBE_CLI_MAX_DRIVE_ES_SIZE            4
#define C4_FBE_CLI_MAX_DRIVE_STATE              4

typedef enum es_state_e {
    ES_REM,
    ES_READY,
    ES_FAULTED,
} es_state_t;

typedef struct be_slot_ds_t_s {
    es_state_t      es_state;
    char            es_state_str[C4_FBE_CLI_MAX_DRIVE_ES_SIZE];
    unsigned int    block_size;
    fbe_lba_t       capacity;
    unsigned int    media_error;
    unsigned int    hw_err;
    unsigned int    hwchk_err;
    unsigned int    link_err;
    unsigned int    data_err;
    char    class_name[C4_FBE_CLI_MAX_DRIVE_NAME_SIZE];
    char    vendor_name[C4_FBE_CLI_MAX_DRIVE_NAME_SIZE];
    char    model_name[C4_FBE_CLI_MAX_DRIVE_NAME_SIZE];
} be_slot_ds_t;


typedef struct be_encl_ds_t_s {
    char            name[C4_FBE_CLI_MAX_ENCLOSURE_NAME_SIZE];
    fbe_lifecycle_state_t state;
    char            fw_rev[MAX_FW_REV_STR_LEN+1];
    int             installed;

    int             num_drives;
    int             drives_installed;

    int             missing_disks;
    int             missing_disk_ids[C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS];
    int             faulted_disks;
    int             faulted_disk_ids[C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS];
    int             wrong_disk_type;
    int             wrong_disk_type_ids[C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS];
    int             wrong_disk_block;
    int             wrong_disk_block_ids[C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS];
    int             faulted_missing_disk_ids[C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS];
    be_slot_ds_t drives[C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS];
} be_encl_ds_t;

typedef struct fbe_cli_becheck_s {
    fbe_u32_t       mode;
    fbe_bool_t      verbose;
    fbe_bool_t      show_disk_id;
    fbe_bool_t      debug;

    be_encl_ds_t    encl_ds;

    /* Error types */
    int     missing_serial_num;
} fbe_cli_becheck_t;


#define becheck_debug(context, fmt, ...)    do {        \
    if (context->debug) {                               \
        fbe_cli_printf(fmt, ##__VA_ARGS__);             \
    }                                                   \
} while (0)

#define becheck_verbose(context, fmt, ...)    do {      \
    if (context->verbose) {                             \
        fbe_cli_printf(fmt, ##__VA_ARGS__);             \
    }                                                   \
} while (0)

#define becheck_error(context, code, fmt, ...) do { \
    becheck_verbose(context, fmt, ##__VA_ARGS__);   \
    becheck_print_status(context, code);            \
} while (0)


extern fbe_edal_status_t
fbe_cli_fill_enclosure_drive_info(fbe_edal_block_handle_t baseCompBlockPtr,
                                  slot_flags_t *driveStat,
                                  fbe_u32_t *component_count);


static void becheck_print_status(fbe_cli_becheck_t *becheck, fbe_u32_t code)
{
    fbe_cli_printf("0x%.4x\n", code);
}

static void becheck_copy_string(char *dest, const char *src, unsigned int size)
{
    if (size == 0) {
        return;
    }
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}


static int becheck_encl_get_fw(fbe_cli_becheck_t *becheck,
                               fbe_edal_block_handle_t baseCompBlockPtr,
                               char fw_rev[FBE_EDAL_FW_REV_STR_SIZE+1])
{
    fbe_edal_status_t       status;
    fbe_u8_t                side_id;
    fbe_edal_fw_info_t      fw_info;
    fbe_u32_t               index;

    fbe_zero_memory(fw_rev, MAX_FW_REV_STR_LEN + 1);
    fbe_edal_getEnclosureSide(baseCompBlockPtr,&side_id);

    index = fbe_edal_find_first_edal_match_U8(baseCompBlockPtr,
                                              FBE_ENCL_COMP_SIDE_ID,  //attribute
                                              FBE_ENCL_LCC,  // Component type
                                              0, //starting index
                                              side_id);

    if (index == FBE_ENCLOSURE_VALUE_INVALID) {
        return -1;
    }
    status = fbe_edal_getStr(baseCompBlockPtr,
                             FBE_ENCL_COMP_FW_INFO,
                             FBE_ENCL_LCC,
                             index,
                             sizeof(fw_info),
                             (char *)&fw_info);
    if (status != FBE_EDAL_STATUS_OK) {
        becheck_debug(becheck, "Failed to get fw information\n");
        return -1;
    }
    if (!fw_info.valid) {
        becheck_debug(becheck, "FW information is invalid\n");
        return -1;
    }

    fbe_copy_memory(fw_rev, fw_info.fwRevision, MAX_FW_REV_STR_LEN);

    return 0;
}

static es_state_t becheck_decode_slot_state(fbe_cli_becheck_t *becheck, slot_flags_t *driveDataPtr)
{
   if(driveDataPtr->driveInserted == FALSE) {
       return ES_REM;
   }

   if((driveDataPtr->driveInserted == TRUE) &&
           (driveDataPtr->driveFaulted == FALSE)&&
           (driveDataPtr->drivePoweredOff == FALSE)&&
           (driveDataPtr->driveBypassed == FALSE))
   {
       return ES_READY;
   }

   return ES_FAULTED;
}

static void becheck_fill_drive_info(fbe_cli_becheck_t *becheck,
                                    slot_flags_t *driveStat, fbe_u32_t max_slot)
{
    fbe_u32_t i;
    be_slot_ds_t *slot_ds = &becheck->encl_ds.drives[0];
    be_encl_ds_t *encl_ds = &becheck->encl_ds;

    encl_ds->num_drives = max_slot;
    encl_ds->drives_installed = 0;

    for (i = 0; i < max_slot; i += 1) {
        es_state_t state = becheck_decode_slot_state(becheck, &driveStat[i]);

        slot_ds[i].es_state = state;
        strncpy(slot_ds[i].es_state_str, fbe_eses_debug_decode_slot_state(&driveStat[i]),
                sizeof(slot_ds[i].es_state_str));
        slot_ds[i].es_state_str[sizeof(slot_ds[i].es_state_str) - 1] = '\0';

        if (state != ES_REM) {
            encl_ds->drives_installed += 1;
        }
        becheck_debug(becheck, "Slot %d: %s\n", i, slot_ds[i].es_state_str);
    }
}

/**
 * Return number dirves to check.
 * install: first 4 drives
 * normal: first 4 drives
 * full: All drives.
 */
static int becheck_get_disks_to_check(fbe_cli_becheck_t *becheck)
{
    be_encl_ds_t *encl_ds = &becheck->encl_ds;

    if (becheck->mode == C4_FBE_CLI_BECHECK_TYPE_FULL) {
        return encl_ds->num_drives;
    } else {
        return C4_FBE_CLI_BECHECK_DISKS_TO_CHECK;
    }
}

static int becheck_setup_becheck(fbe_cli_becheck_t *becheck)
{
    return 0;
}

/*!*******************************************************************
 * @var becheck_setup_enclosure()
 *********************************************************************
 * @brief Function to qurey status of slot
 *
 * @return - none.
 *
 * @author
 *  06/03/2015 - Created. Jamin Kang
 *********************************************************************/
static int becheck_setup_enclosure(fbe_cli_becheck_t *becheck)
{
    fbe_u32_t port = 0, encl = 0;
    fbe_u32_t status;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids;
    fbe_object_id_t obj_id;
    fbe_lifecycle_state_t state;
    fbe_base_object_mgmt_get_enclosure_info_t *enclosure_info_ptr = NULL;
    fbe_base_object_mgmt_get_enclosure_info_t *comp_enclosure_info_ptr = NULL;
    char fw_rev[FBE_EDAL_FW_REV_STR_SIZE+1];

    slot_flags_t   driveStat[FBE_API_ENCLOSURE_SLOTS] = {0};
    fbe_enclosure_component_block_t *compBlockPtr;
    fbe_u32_t  component_count;
    fbe_u8_t max_slot = 0;
    fbe_edal_status_t edalStatus = FBE_EDAL_STATUS_OK;
    be_encl_ds_t *encl_ds = &becheck->encl_ds;

    status = fbe_api_get_enclosure_object_ids_array_by_location(port, encl, &enclosure_object_ids);
    if (status != FBE_STATUS_OK) {
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Get enclosure object id failed\n");
        return -1;
    }
    obj_id = enclosure_object_ids.enclosure_object_id;
    status = fbe_api_get_object_lifecycle_state(obj_id, &state, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Get enclosure lifecycle failed\n");
        return -1;
    }
    if (state == FBE_LIFECYCLE_STATE_FAIL) {
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Enclosure in failed state. Check Enclosure status for possible errors\n");
        return -1;
    }
    status = fbe_api_enclosure_setup_info_memory(&enclosure_info_ptr);
    if(status != FBE_STATUS_OK){
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Failed to allocate enclosure information memory\n");
        return -1;
    }

    // Get the enclosure component data blob via the API.
    status = fbe_api_enclosure_get_info(obj_id, enclosure_info_ptr);
    if(status != FBE_STATUS_OK) {
        fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Failed to get enclosure data\n");
        return -1;
    }
    /* Enclosure Data contains some pointers that must be converted */
    edalStatus = fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)enclosure_info_ptr);
    if(edalStatus != FBE_EDAL_STATUS_OK) {
        fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. EDAL: failed to update block pointers %p, status:0x%x\n",
                      enclosure_info_ptr, edalStatus);
        return -1;
    }
    compBlockPtr = (fbe_enclosure_component_block_t *)(fbe_edal_block_handle_t)enclosure_info_ptr;
    if (becheck_encl_get_fw(becheck, (fbe_edal_block_handle_t)enclosure_info_ptr, &fw_rev[0])) {
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. EDAL: failed to get fw rev\n");
        return -1;
    }

    encl_ds->installed = 1;
    becheck_copy_string(&encl_ds->name[0], fbe_edal_print_Enclosuretype(compBlockPtr->enclosureType),
                        sizeof(encl_ds->name));
    encl_ds->state = state;
    becheck_copy_string(&encl_ds->fw_rev[0], fw_rev, sizeof(encl_ds->fw_rev));

    becheck_debug(becheck, "%s", encl_ds->name);
    becheck_debug(becheck, " (Bus:%d Enclosure:%d) ", port, encl);
    becheck_debug(becheck, "LifeCycleState:%s ",
                  fbe_cli_print_LifeCycleState(encl_ds->state));
    becheck_debug(becheck, "BundleRev:%s", encl_ds->fw_rev);
    becheck_debug(becheck, "\n");

    edalStatus = fbe_edal_getU8((fbe_edal_block_handle_t)enclosure_info_ptr,
                                FBE_ENCL_MAX_SLOTS,
                                FBE_ENCL_ENCLOSURE,
                                0,
                                &max_slot);
    if (edalStatus != FBE_EDAL_STATUS_OK) {
        fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. EDAL: could not retrieve enclosure max slots\n");
        return -1;
    }

    edalStatus = fbe_cli_fill_enclosure_drive_info((fbe_edal_block_handle_t)enclosure_info_ptr, driveStat, &component_count);
    if (edalStatus != FBE_EDAL_STATUS_OK) {
        fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. EDAL: failed to fill the enclosure drive info\n");
        return -1;
    }

    if (max_slot > C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS) {
        max_slot = C4_FBE_CLI_MAX_BEACHCOMBER_DPE_DISK_SLOTS;
    }

    becheck_fill_drive_info(becheck, driveStat, max_slot);

    if(enclosure_info_ptr != NULL) {
        fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
    }
    if(comp_enclosure_info_ptr != NULL) {
        fbe_api_enclosure_release_info_memory(comp_enclosure_info_ptr);
    }

    return 0;
}

static int becheck_get_drives_information(fbe_cli_becheck_t *becheck)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           object_index;
    fbe_u32_t           total_objects = 0;
    fbe_object_id_t    *object_list_p = NULL;
    fbe_port_number_t port = 0;
    fbe_enclosure_number_t enclosure = 0;
    fbe_enclosure_slot_number_t drive_no = FBE_API_ENCLOSURE_SLOTS;
	fbe_u32_t			object_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_PHYSICAL;
    be_encl_ds_t *encl_ds = &becheck->encl_ds;

	status = fbe_api_get_total_objects(&object_count, package_id);
    if (status != FBE_STATUS_OK) {
		return status;
	}

    /* Allocate memory for the objects.
     */
    object_list_p = fbe_api_allocate_memory(sizeof(fbe_object_id_t) * object_count);

    if (object_list_p == NULL) {
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Unable to allocate memory for object list\n");
        return -1;
    }

    /* Find the count of total objects.
     */
    status = fbe_api_enumerate_objects(object_list_p, object_count, &total_objects, package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(object_list_p);
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Failed to enumerate objects\n");
        return status;
    }

    /* Loop over all objects found in the table.
     */
    for (object_index = 0; object_index < total_objects; object_index++) {
        fbe_object_id_t object_id = object_list_p[object_index];
        fbe_const_class_info_t *class_info;
        be_slot_ds_t *slot_ds;
        fbe_class_id_t class_id;
        fbe_physical_drive_attributes_t attributes;
        fbe_physical_drive_information_t physical_drive_info = {0};

        status = fbe_api_get_object_class_id(object_id, &class_id, package_id);
        if(status == FBE_STATUS_OK) {
            status = fbe_api_get_object_type (object_id, &obj_type, package_id);
        }
        if (status != FBE_STATUS_OK) {
            continue;
        }
        if ((class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST+1) ||
            (class_id >= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST)) {
            continue;
        }
        if (obj_type != FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) {
            continue;
        }
        if (fbe_api_get_object_drive_number(object_id, &drive_no) != FBE_STATUS_OK) {
            continue;
        }
        if (fbe_api_get_object_enclosure_number(object_id, &enclosure) != FBE_STATUS_OK) {
            continue;
        }
        if (fbe_api_get_object_port_number(object_id, &port) != FBE_STATUS_OK) {
            continue;
        }

        /* We only check drives on enclosure 0_0 */
        if ((enclosure != 0) || (port != 0)) {
            continue;
        }
        if (drive_no >= encl_ds->num_drives) {
            continue;
        }
        if (fbe_get_class_by_id(class_id, &class_info) != FBE_STATUS_OK) {
            continue;
        }

        slot_ds = &encl_ds->drives[drive_no];
        becheck_copy_string(slot_ds->class_name, class_info->class_name, sizeof(slot_ds->class_name));

        /* vendor name, model name */
        status = fbe_api_physical_drive_get_attributes(object_id, &attributes);
        if (status != FBE_STATUS_OK) {
            continue;
        }
        status = fbe_api_physical_drive_get_drive_information(object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK) {
            continue;
        }

        becheck_copy_string(slot_ds->vendor_name, physical_drive_info.vendor_id, sizeof(slot_ds->vendor_name));
        becheck_copy_string(slot_ds->model_name, physical_drive_info.product_id, sizeof(slot_ds->model_name));

        slot_ds->capacity = attributes.imported_capacity;
        slot_ds->block_size = attributes.physical_block_size;
    }/* Loop over all objects found. */

    return 0;
}

static int becheck_check_product_serial_number(fbe_cli_becheck_t *becheck)
{
    fbe_esp_base_env_get_resume_prom_info_cmd_t getResumePromInfo;
    fbe_device_physical_location_t *deviceLocation = &getResumePromInfo.deviceLocation;
    fbe_status_t status;
    int serial_len;
    char verbose_error[100];

    memset(&getResumePromInfo, 0, sizeof(getResumePromInfo));

    deviceLocation->bus = 0;
    deviceLocation->enclosure = 0;
    deviceLocation->slot = 0;
    getResumePromInfo.deviceType = FBE_DEVICE_TYPE_ENCLOSURE;
    status = fbe_api_esp_common_get_resume_prom_info(&getResumePromInfo);
    if (status == FBE_STATUS_COMPONENT_NOT_FOUND) {
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Get Product serial number failed. Enclosure 0_0 not found.\n");
        return -1;
    }
    if (status != FBE_STATUS_OK) {
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. Failed to get product serial number: %d\n", status);
        return -1;
    }
    if (getResumePromInfo.op_status != FBE_RESUME_PROM_STATUS_READ_SUCCESS) {
        becheck_error(becheck, C4_BE_FAILED_TO_RUN_CHECK,
                      "\tError. resume prod read op status %s.\n",
                      fbe_base_env_decode_resume_prom_status(getResumePromInfo.op_status));
        return -1;
    }
    getResumePromInfo.resume_prom_data.product_serial_number[RESUME_PROM_PRODUCT_SN_SIZE - 1] = '\0';
    verbose_error[0] = '\0';
    serial_len = (int)strlen(getResumePromInfo.resume_prom_data.product_serial_number);
    if (serial_len <= 13) {
        sprintf(verbose_error,"This is not a valid serial, needs to be > 13 chars.%d\n", serial_len);
        becheck->missing_serial_num = C4_BE_INVALID_CHASSIS_SN;
    }
    becheck_verbose(becheck, "Serial Number is:%s, length is %d\n%s",
                    getResumePromInfo.resume_prom_data.product_serial_number, serial_len,
                    verbose_error);
    return 0;
}

static fbe_u32_t becheck_get_supported_drive_types(fbe_board_mgmt_platform_info_t *platform_info)
{
    fbe_u32_t drive_types;

#if defined(SIMMODE_ENV)
    fbe_status_t status;
    status = fbe_api_database_get_additional_supported_drive_types(&drive_types);
    if (status == FBE_STATUS_OK)
    {
        return drive_types;
    }
#else /*defined(UMODE_ENV)*/
    EMCPAL_STATUS    status = EMCPAL_STATUS_SUCCESS;
    fbe_u32_t DefaultInputValue = (fbe_u32_t)FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE;

    status = EmcpalGetRegistryValueAsUint32(SEP_REG_PATH, 
                                           FBE_REG_ADDITIONAL_SUPPORTED_DRIVE_TYPES, 
                                           &drive_types);
    if (EMCPAL_IS_SUCCESS(status))
    {
        return drive_types;
    }
    /* No registry set, use default value */
    drive_types = DefaultInputValue;

#endif

    switch (platform_info->hw_platform_info.cpuModule) {
        case BLACKWIDOW_CPU_MODULE:
        case WILDCATS_CPU_MODULE:
        case MAGNUM_CPU_MODULE:
        case BOOMSLANG_CPU_MODULE:
        case INTREPID_CPU_MODULE:
        case ARGONAUT_CPU_MODULE:
        case SENTRY_CPU_MODULE:
        case EVERGREEN_CPU_MODULE:
        case DEVASTATOR_CPU_MODULE:
        case MEGATRON_CPU_MODULE:
        case STARSCREAM_CPU_MODULE:
        case JETFIRE_CPU_MODULE:
        case BEACHCOMBER_CPU_MODULE:
        case SILVERBOLT_CPU_MODULE:
        case TRITON_ERB_CPU_MODULE:
        case TRITON_CPU_MODULE:
        case MERIDIAN_CPU_MODULE:
        case TUNGSTEN_CPU_MODULE:
            drive_types |= FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD;
            break;

        case HYPERION_CPU_MODULE:
        case OBERON_CPU_MODULE:
        case CHARON_CPU_MODULE:
            /* use default */
            break;

        /* add new platform support here */

        default:
            break;
    }

    return drive_types;
}

static int becheck_check_drives(fbe_cli_becheck_t *becheck, fbe_board_mgmt_platform_info_t *platform_info)
{
    be_encl_ds_t  *enc = NULL;
    be_slot_ds_t *slot =  NULL;
    be_slot_ds_t *base_slot=NULL;
    int drv_ndx;
    fbe_u32_t drive_types;

    drive_types = becheck_get_supported_drive_types(platform_info);
    becheck_debug(becheck, "support drive types %d\n", drive_types);

    enc = &becheck->encl_ds;
    /*
     * Only Check the first four drives for error reporting, 
     * even if doing a FULL becheck.  
     * Other disks in DPE may have different size than first four, 
     * be missing,or faulted but should not generate error that 
     * would keep array from booting.  Full verbose will list all
     * the DPE disk info which can be useful for quick check.
     */

    for (drv_ndx = 0; drv_ndx < C4_FBE_CLI_BECHECK_DISKS_TO_CHECK; drv_ndx++) {
        slot = &enc->drives[drv_ndx];
        if (slot->es_state == ES_READY) {
            if (base_slot == NULL) {
                base_slot = slot;
            }
            if (((slot->block_size != C4_FBE_CLI_VALID_DISK_BLOCK_SIZE) &&
                (slot->block_size != C4_FBE_CLI_VALID_DISK_BLOCK_SIZE_4K)) ||
                (slot->block_size == C4_FBE_CLI_VALID_DISK_BLOCK_SIZE && /* 520 HDD disabled */
                !strstr(slot->class_name, "flash") && 
                !(drive_types & FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD))) {
                enc->wrong_disk_block++;
                enc->wrong_disk_block_ids[drv_ndx] = 1;
            }
            if (!strstr(slot->class_name, "sas")) {
                enc->wrong_disk_type++;
                enc->wrong_disk_type_ids[drv_ndx] = 1;
            }
        } else {
            /* Case where drive not ready -
               this must mean that drive is missing or faulted*/
            if (slot->es_state == ES_REM) {
                enc->missing_disks++;
                enc->missing_disk_ids[drv_ndx] = 1;
                becheck_debug(becheck, "Slot %u is removed\n", drv_ndx);
            } else {
                enc->faulted_disks++;
                enc->faulted_disk_ids[drv_ndx] = 1;
                becheck_debug(becheck, "Slot %u is faulted\n", drv_ndx);
            }
        }
    }
    return 0;
}

static void becheck_display_be_ds(fbe_cli_becheck_t *becheck)
{
    int drive_ndx;
    be_slot_ds_t *slot;
    be_encl_ds_t *encl;
    int disks_checked;

    if (!becheck->verbose) {
        return;
    }
    encl = &becheck->encl_ds;
    disks_checked = becheck_get_disks_to_check(becheck);
    slot = &encl->drives[0];
    fbe_cli_printf("\n\nBus 0 Enclosure 0 : %s", encl->name);
    fbe_cli_printf("\nState = %s Firmware Rev = %s",
                   fbe_cli_print_LifeCycleState(encl->state),
                   encl->fw_rev);
    fbe_cli_printf("\nMax Drive Slots = %d  Drive slots Installed = %d Drives Checked %d",
                   encl->num_drives, encl->drives_installed, disks_checked);

    for (drive_ndx = 0; drive_ndx < disks_checked; drive_ndx++) {
        slot = &encl->drives[drive_ndx];
        fbe_cli_printf("\n--Drive %d State %s",
                       drive_ndx, slot->es_state_str);
        if (slot->es_state != ES_REM) {
            fbe_cli_printf("\n--Class %s Vendor %s Model %s",
                           slot->class_name,
                           slot->vendor_name, slot->model_name);
            fbe_cli_printf("\n--Block size = %d Capacity = 0x%llx\n",
                           slot->block_size, (unsigned long long)slot->capacity);
        }
    }
    fbe_cli_printf("\n");
}

static int becheck_get_code(fbe_cli_becheck_t *becheck)
{
    int num_of_becheck_disks;
    int be_code, ndx;
    int *display_disk_ids = NULL;
    be_encl_ds_t  *enc;
    fbe_u32_t fbe_check_type = becheck->mode;

    enc = &becheck->encl_ds;
 

    /*
     * If doing install check none of the first four disks can be missing
     */
    if ((enc->missing_disks >= 1) &&
        (fbe_check_type == C4_FBE_CLI_BECHECK_TYPE_INSTALL)) {
        be_code = C4_BE_DISK_MISSING;
        display_disk_ids = enc->missing_disk_ids;
        if (becheck->verbose) {
            fbe_cli_printf("\nError.  "
                           "First %d disks of the DPE are required for install...\n",
                           C4_FBE_CLI_BECHECK_DISKS_TO_CHECK);
        }
    }    
    /*  
     * Same for faulted disk during an install
     */
    else if ((enc->faulted_disks >= 1) &&
               (fbe_check_type == C4_FBE_CLI_BECHECK_TYPE_INSTALL)) {
        be_code = C4_BE_FAULTED_DISK;
        display_disk_ids = enc->faulted_disk_ids;
        if (becheck->verbose) {
            fbe_cli_printf("\nError.    "
                           "First %d disks of the DPE must be OK for install...\n",
                           C4_FBE_CLI_BECHECK_DISKS_TO_CHECK);
        }
    } else if (enc->wrong_disk_type) {
        be_code =  C4_BE_INVALID_DISK_TYPE;
        display_disk_ids = enc->wrong_disk_type_ids;
        if (becheck->verbose) {
            fbe_cli_printf("\nError.  Invalid disk type detected...\n");
        }
    }
     /*
      * For Normal mode may have 1 missing disk in first four 
      */
    else if (enc->missing_disks > 1) {
        be_code = C4_BE_DISK_MISSING;
        display_disk_ids = enc->missing_disk_ids;
        if (becheck->verbose) {
            fbe_cli_printf("\nError.    "
                           "At least %d out of the first %d disks of the DPE are required...\n",
                           (C4_FBE_CLI_BECHECK_DISKS_TO_CHECK- 1), 
                           C4_FBE_CLI_BECHECK_DISKS_TO_CHECK);
        }
    } 
    /*
     * For Normal mode may have 1 faulted disk in first four
     */
    else if (enc->faulted_disks > 1)
    {
        be_code = C4_BE_FAULTED_DISK;
        display_disk_ids = enc->faulted_disk_ids;
        if (becheck->verbose)
            fbe_cli_printf("\nError.    "     
                           "%d disks of the first %d disks DPE must be OK ...\n", 
                           (C4_FBE_CLI_BECHECK_DISKS_TO_CHECK-1),
                           (C4_FBE_CLI_BECHECK_DISKS_TO_CHECK));

    }
     /*
     * Also need to check that the number of faulted and number of missing
     * is not greater that 1.
     */
    else if ((enc->faulted_disks + enc->missing_disks) > 1 )
    {
        int dsk_cnt;
        be_code = C4_BE_FAULTED_DISK;  /* Report as faulted, but id both */
        display_disk_ids = enc->faulted_missing_disk_ids;
        for (dsk_cnt = 0; dsk_cnt < C4_FBE_CLI_BECHECK_DISKS_TO_CHECK; 
             dsk_cnt++) {
             enc->faulted_missing_disk_ids[dsk_cnt] = (enc->faulted_disk_ids[dsk_cnt] | enc->missing_disk_ids[dsk_cnt]);
        }                                              
        if (becheck->verbose)
            fbe_cli_printf("\nError.    "     
                           "Missing and faulted disk count of %d is greater than 1 for the first %d disk of DPE ...\n", 
                           (enc->faulted_disks + enc->missing_disks),
                           (C4_FBE_CLI_BECHECK_DISKS_TO_CHECK));
  
    } else if (enc->wrong_disk_block) {
        be_code =  C4_BE_INVALID_DISK_BSIZE;
        display_disk_ids = enc->wrong_disk_block_ids;
        if (becheck->verbose) {
            fbe_cli_printf("\nError.  Invalid disk sector size detected...\n");
        }
    } else if (becheck->missing_serial_num) {
        be_code = C4_BE_INVALID_CHASSIS_SN;
        if(becheck->verbose) {
            fbe_cli_printf("\nError.   The DPE resume is missing an EMC Product Serial number...\n");
        }
    } else {
        be_code = 0x0000;
    }

    fbe_cli_printf("0x%.4x", be_code);
    if (becheck->show_disk_id && display_disk_ids) {
      /* 
       * If doing full check, then display all the disks installed in DPE
       */
       if (fbe_check_type == C4_FBE_CLI_BECHECK_TYPE_FULL) {
           num_of_becheck_disks = enc->num_drives;
        } else {
           num_of_becheck_disks = C4_FBE_CLI_BECHECK_DISKS_TO_CHECK;
        }
        for (ndx = 0; ndx  < num_of_becheck_disks; ndx++ ) {
            if (display_disk_ids[ndx]) {
                fbe_cli_printf(" %d", ndx);
            }
        }
    }
    fbe_cli_printf("\n");
    return 0;
}

/*!*******************************************************************
 * @var fbe_cli_becheck()
 *********************************************************************
 * @brief Function to check backend drives
 *
 * @return - none.
 *
 * @author
 *  06/03/2015 - Created. Jamin Kang
 *********************************************************************/

void fbe_cli_becheck_cmd(int argc, char **argv)
{
    fbe_cli_becheck_t   context;
    fbe_u32_t           mode = C4_FBE_CLI_BECHECK_TYPE_NORMAL;
    fbe_bool_t          verbose = FBE_FALSE;
    fbe_bool_t          show_disk_id = FBE_FALSE;
    fbe_bool_t          debug = FBE_FALSE;
    fbe_bool_t          simulation = FBE_FALSE;
    fbe_status_t        status;
    fbe_object_id_t     board_object_id;
    fbe_board_mgmt_platform_info_t platform_info = {0};

    memset(&context, 0, sizeof(context));
    while (argc > 0) {
        if (strcmp(*argv, "-v") == 0) {
            verbose = FBE_TRUE;
        } else if (strcmp(*argv, "-install") == 0) {
            mode = C4_FBE_CLI_BECHECK_TYPE_INSTALL;
        } else if (strcmp(*argv, "-all") == 0) {
            mode = C4_FBE_CLI_BECHECK_TYPE_FULL;
        } else if (strcmp(*argv, "-id") == 0) {
            show_disk_id = FBE_TRUE;
        } else if (strcmp(*argv, "-debug") == 0) {
            debug = FBE_TRUE;
        } else if (strcmp(*argv, "-No_Product_SN") == 0) {
            simulation = FBE_TRUE;
        } else {
            fbe_cli_printf("%s", BECHECK_CMD_USAGE);
            return;
        }

        argc -= 1;
        argv += 1;
    }

    /* Get the board object id*/
    status = fbe_api_get_board_object_id(&board_object_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in retrieving board object ID, Status %d", status);
        return;
    }

    status = fbe_api_board_get_platform_info(board_object_id, &platform_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in retrieving base board info, Status %d", status);
        return;
    }

    if (platform_info.hw_platform_info.platformType == SPID_MERIDIAN_ESX_HW_TYPE ||
        platform_info.hw_platform_info.platformType == SPID_TUNGSTEN_HW_TYPE)
    {
        /* Virtual VNX */
        fbe_cli_error("\n Error. Not supported in VIRTUAL environment...\n");
        return;
    }
    else if (platform_info.hw_platform_info.platformType == SPID_NOVA_S1_HW_TYPE ||
             platform_info.hw_platform_info.platformType == SPID_TRIN_S1_HW_TYPE)
    {
        /* BC Simulator */
        fbe_cli_error("\n Error. Not supported in SIM64 environment...\n");
        return;
    }

    context.mode = mode;
    context.verbose = verbose;
    context.show_disk_id = show_disk_id;
    context.debug = debug;

    if (becheck_setup_becheck(&context)) {
        return;
    }
    if (becheck_setup_enclosure(&context)) {
        return;
    }
    if (becheck_get_drives_information(&context)) {
        return;
    }
    if (!simulation && becheck_check_product_serial_number(&context)) {
        return;
    }
    becheck_check_drives(&context, &platform_info);
    becheck_display_be_ds(&context);
    becheck_get_code(&context);
    return;
}
