/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_eas_lib_eas_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contain fbe_eas functions used for EAS command.
 *
 * @version
 *  22-Jul-2014: Created 
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "fbe_eas_lib_eas_cmds.h"
#include "fbe_eas_private.h"
#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_physical_drive_interface.h"

typedef enum EAS_state_e {
    EAS_STATE_INVALID,
    EAS_STATE_ERASE_NEEDED,
    EAS_STATE_ERASE_STARTED,
    EAS_STATE_ERASE_DONE,
    EAS_STATE_ERASE_COMMITTED,
} eas_pvd_state_t;

typedef struct {
    fbe_object_id_t     pvd_obj_id;
    fbe_object_id_t     pdo_obj_id;
    eas_pvd_state_t     eas_state;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_bool_t          action_required;
    fbe_u32_t           port_number;
    fbe_u32_t           enclosure_number;
    fbe_u32_t           slot_number;
    fbe_u8_t            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
} eas_pvd_object_record_t;

/****************************** 
 *    LOCAL FUNCTIONS
 ******************************/
static fbe_status_t fbe_eas_lib_eas_start(int argc, char** argv);
static fbe_status_t fbe_eas_lib_eas_erase(int argc, char** argv);
static fbe_status_t fbe_eas_lib_eas_state(int argc, char** argv);
static fbe_status_t fbe_eas_lib_eas_commit(int argc, char** argv);

extern fbe_status_t  fbe_cli_convert_diskname_to_bed(fbe_u8_t disk_name_a[], fbe_job_service_bes_t * phys_location);

 /*!***************************************************************
 * fbe_cli_eas(int argc, char** argv)
 ****************************************************************
 * @brief
 *  This function is used to perform EAS operations on the system.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @author
 *  04-Oct-2013: Created
 *
 ****************************************************************/
void fbe_cli_eas(int argc, char** argv)
{

    fbe_status_t status = FBE_STATUS_OK;

    if (argc < 1) {
        fbe_cli_printf("%s", EAS_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) {

        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", EAS_USAGE);
        return;
    }// end of -help

    if ((strcmp(*argv, "-start") == 0)){
        /* Send EAS start to PVDs */
        status = fbe_eas_lib_eas_start(argc, argv);
        return;
    }

    if ((strcmp(*argv, "-erase") == 0)){
        /* Send EAS start to PVDs */
        status = fbe_eas_lib_eas_erase(argc, argv);
        return;
    }

    if ((strcmp(*argv, "-state") == 0)){
        /* Send EAS start to PVDs */
        status = fbe_eas_lib_eas_state(argc, argv);
        return;
    }

    if ((strcmp(*argv, "-commit") == 0)){
        /* Send EAS start to PVDs */
        status = fbe_eas_lib_eas_commit(argc, argv);
        return;
    }

    fbe_cli_printf("\nInvalid operation!!\n\n");
    fbe_cli_printf("%s", EAS_USAGE);
    return;

}

static fbe_status_t fbe_eas_lib_eas_get_drives(int argc, char** argv, eas_pvd_object_record_t ** pvd_list_pp)
{
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_bool_t all_drives = FBE_FALSE, ssd_only = FBE_FALSE, hdd_only = FBE_FALSE;
    fbe_bool_t is_ssd;
    fbe_u32_t drive_count = 0;
    fbe_object_id_t           *obj_list_p;
    eas_pvd_object_record_t *pvd_list_p = NULL;
    fbe_job_service_bes_t bes;
    fbe_u32_t i;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_bool_t is_started = FBE_FALSE, is_complete = FBE_FALSE;
    fbe_physical_drive_sanitize_info_t sanitize_info;

    argv++;
    argc--;

    if (argc == 0) {
        fbe_cli_printf("%s", EAS_USAGE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (strcmp(argv[0], "all_ssd") == 0) {
        ssd_only = FBE_TRUE;
    } else if (strcmp(argv[0], "all_hdd") == 0) {
        hdd_only = FBE_TRUE;
    } else if (strcmp(argv[0], "all") == 0) {
        all_drives = FBE_TRUE;
    } else {
        status = fbe_cli_convert_diskname_to_bed(argv[0], &bes);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s:\tSpecified disk %s is invalid. %X\n", __FUNCTION__, *argv, status);
            fbe_cli_printf("%s", EAS_USAGE);
            return status;
        }
        drive_count = 1;
    }

    if (drive_count == 0) {
        /* Get the number of PVDs in the system */
        status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &drive_count);
        if (status != FBE_STATUS_OK || drive_count == 0) {
            fbe_cli_error("Failed to enumerate classes. Status: %x\n", status);
            return status;
        }

        pvd_list_p = (eas_pvd_object_record_t *)fbe_api_allocate_memory((drive_count + 1) * sizeof(eas_pvd_object_record_t));
        if (pvd_list_p == NULL) {
            fbe_cli_error("failed to allocate memory \n");
            fbe_api_free_memory(obj_list_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Copy the pvd object id over */
        for (i = 0; i < drive_count; i++) {
            pvd_list_p[i].pvd_obj_id = obj_list_p[i];
        }
        pvd_list_p[drive_count].pvd_obj_id = FBE_OBJECT_ID_INVALID;

        fbe_api_free_memory(obj_list_p);
    } else {

        pvd_list_p = (eas_pvd_object_record_t *)fbe_api_allocate_memory(2 * sizeof(eas_pvd_object_record_t));
        if (pvd_list_p == NULL) {
            fbe_cli_error("failed to allocate memory \n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_api_provision_drive_get_obj_id_by_location(bes.bus, bes.enclosure, bes.slot, &pvd_list_p[0].pvd_obj_id);
        if (status != FBE_STATUS_OK)
        {
            /* If unable to get the object id of the drive, then return error from here */
            fbe_cli_printf ("Failed to get object id of %d_%d_%d.    %d\n", bes.bus, bes.enclosure, bes.slot, status);
            fbe_api_free_memory(pvd_list_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        pvd_list_p[1].pvd_obj_id = FBE_OBJECT_ID_INVALID;
    }

    /* Get the pvd infos */
    for (i = 0; i < drive_count; i++) {
        status = fbe_api_provision_drive_get_info(pvd_list_p[i].pvd_obj_id, &provision_drive_info);
        if (status != FBE_STATUS_OK)
        {
            pvd_list_p[i].eas_state = EAS_STATE_INVALID;
            continue;
        }

        /* We skip system drives */
        if (provision_drive_info.is_system_drive) {
            pvd_list_p[i].eas_state = EAS_STATE_INVALID;
            continue;
        }

        /* We skip consumed drives */
        if (provision_drive_info.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED) {
            pvd_list_p[i].eas_state = EAS_STATE_INVALID;
            continue;
        }

        if ((provision_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_HE)  ||
            (provision_drive_info.drive_type == FBE_DRIVE_TYPE_SATA_FLASH_HE) ||
			(provision_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_ME)  ||
            (provision_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE)  ||
            (provision_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)     ) {

            is_ssd = FBE_TRUE;
        } else {
            is_ssd = FBE_FALSE;
        }
        /* We skip non SSD drives if required */
		if ((ssd_only && !is_ssd) || (hdd_only && is_ssd)) {
            pvd_list_p[i].eas_state = EAS_STATE_INVALID;
            continue;
        }

        pvd_list_p[i].port_number = provision_drive_info.port_number;
        pvd_list_p[i].enclosure_number = provision_drive_info.enclosure_number;
        pvd_list_p[i].slot_number = provision_drive_info.slot_number;
        fbe_copy_memory(pvd_list_p[i].serial_num, provision_drive_info.serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
        
        status = fbe_api_get_physical_drive_object_id_by_location(pvd_list_p[i].port_number, pvd_list_p[i].enclosure_number, pvd_list_p[i].slot_number, &pvd_list_p[i].pdo_obj_id);
        if (status != FBE_STATUS_OK)
        {
            /* If unable to get the object id of the drive, then return error from here */
            fbe_cli_printf ("Failed to get pdo object id of %d_%d_%d.    %d\n", pvd_list_p[i].port_number, pvd_list_p[i].enclosure_number, pvd_list_p[i].slot_number, status);
            pvd_list_p[i].eas_state = EAS_STATE_INVALID;
            continue;
        }

        fbe_api_get_object_lifecycle_state(pvd_list_p[i].pvd_obj_id, &pvd_list_p[i].lifecycle_state, FBE_PACKAGE_ID_SEP_0);

        status = fbe_api_provision_drive_get_eas_state(pvd_list_p[i].pvd_obj_id, &is_started, &is_complete);
        if (is_complete)
        {
            pvd_list_p[i].eas_state = EAS_STATE_ERASE_COMMITTED;
        } else if (is_started) {
            pvd_list_p[i].eas_state = EAS_STATE_ERASE_STARTED;
            status = fbe_api_physical_drive_get_sanitize_status(pvd_list_p[i].pdo_obj_id, &sanitize_info);
            if (status == FBE_STATUS_OK && sanitize_info.status == FBE_DRIVE_SANITIZE_STATUS_OK)
            {
                pvd_list_p[i].eas_state = EAS_STATE_ERASE_DONE;
            }
        } else {
            pvd_list_p[i].eas_state = EAS_STATE_ERASE_NEEDED;
        }
    }

    * pvd_list_pp = pvd_list_p;
    return FBE_STATUS_OK;
}

static fbe_bool_t fbe_eas_lib_eas_get_confirmation(eas_pvd_object_record_t * pvd_list_p)
{
    eas_pvd_object_record_t *pvd_record_p = NULL;
    fbe_char_t buff[64];

    pvd_record_p = pvd_list_p;
    while (pvd_record_p->pvd_obj_id != FBE_OBJECT_ID_INVALID) {
        if (pvd_record_p->action_required) {
            fbe_cli_printf("\t Drive %d_%d_%d pvd_object_id 0x%x\n", 
                           pvd_record_p->port_number, pvd_record_p->enclosure_number, pvd_record_p->slot_number,
						   pvd_record_p->pvd_obj_id);
        }
        pvd_record_p++;
    }

    fbe_cli_printf("Do you wish to continue?\n");
    scanf("%s",buff); 
    if(strlen(buff) && (!strcmp(buff, "Y") || !strcmp(buff, "y")))
    {
        fflush(stdout);
        return FBE_TRUE;
    }

    fflush(stdout);
    return FBE_FALSE;
}

static char * fbe_eas_print_state(eas_pvd_state_t state)
{
    char * eas_state;
    switch (state)
    {
        case EAS_STATE_INVALID:
            eas_state = (char *)("Invalid");
            break;
        case EAS_STATE_ERASE_NEEDED:
            eas_state = (char *)("Erase_Needed");
            break;
        case EAS_STATE_ERASE_STARTED:
            eas_state = (char *)("Erase_Started");
            break;
        case EAS_STATE_ERASE_DONE:
            eas_state = (char *)("Erase_Done");
            break;
        case EAS_STATE_ERASE_COMMITTED:
            eas_state = (char *)("Erase_Committed");
            break;
        default:
            eas_state = (char *)( "Unknown ");
            break;
    }
    return (eas_state); 
}

static fbe_status_t fbe_eas_lib_eas_start(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    eas_pvd_object_record_t *pvd_list_p = NULL;
    eas_pvd_object_record_t *pvd_record_p = NULL;

    status = fbe_eas_lib_eas_get_drives(argc, argv, &pvd_list_p);
    if (status != FBE_STATUS_OK || pvd_list_p == NULL) {
        fbe_cli_error("EAS -start failed\n");
        return status;
    }

    /* Loop through all the drives */
    pvd_record_p = pvd_list_p;
    while (pvd_record_p->pvd_obj_id != FBE_OBJECT_ID_INVALID) {
        if ((pvd_record_p->eas_state == EAS_STATE_ERASE_NEEDED || pvd_record_p->eas_state == EAS_STATE_ERASE_COMMITTED) && pvd_record_p->lifecycle_state == FBE_LIFECYCLE_STATE_READY) {
            pvd_record_p->action_required = FBE_TRUE;
        } else {
            pvd_record_p->action_required = FBE_FALSE;
        }
        pvd_record_p++;
    }

    fbe_cli_printf("EAS: The following drives will start EAS to PVD:\n");
    if (fbe_eas_lib_eas_get_confirmation(pvd_list_p) != FBE_TRUE) {
        fbe_api_free_memory(pvd_list_p);
        return FBE_STATUS_OK;
    }

    pvd_record_p = pvd_list_p;
    while (pvd_record_p->pvd_obj_id != FBE_OBJECT_ID_INVALID) {
        if (pvd_record_p->action_required) {
            status = fbe_api_provision_drive_set_eas_start(pvd_record_p->pvd_obj_id);
            if (status != FBE_STATUS_OK) {
                fbe_cli_error("EAS -start on pvd 0x%x failed\n", pvd_record_p->pvd_obj_id);
            }
        }
        pvd_record_p++;
    }

    fbe_api_free_memory(pvd_list_p);
    return status;
}

static fbe_status_t fbe_eas_lib_eas_erase(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    eas_pvd_object_record_t *pvd_list_p = NULL;
    eas_pvd_object_record_t *pvd_record_p = NULL;
    fbe_u32_t pattern = 0x2;

    status = fbe_eas_lib_eas_get_drives(argc, argv, &pvd_list_p);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("EAS -erase failed\n");
        return status;
    }

    /* Loop through all the drives */
    pvd_record_p = pvd_list_p;
    while (pvd_record_p->pvd_obj_id != FBE_OBJECT_ID_INVALID) {
        if (pvd_record_p->eas_state == EAS_STATE_ERASE_STARTED || pvd_record_p->eas_state == EAS_STATE_ERASE_DONE) {
            pvd_record_p->action_required = FBE_TRUE;
        } else {
            pvd_record_p->action_required = FBE_FALSE;
        }
        pvd_record_p++;
    }

    fbe_cli_printf("EAS: The following drives will start EAS to PDO:\n");
    if (fbe_eas_lib_eas_get_confirmation(pvd_list_p) != FBE_TRUE) {
        fbe_api_free_memory(pvd_list_p);
        return FBE_STATUS_OK;
    }

    pvd_record_p = pvd_list_p;
    while (pvd_record_p->pvd_obj_id != FBE_OBJECT_ID_INVALID) {
        if (pvd_record_p->action_required) {
            status = fbe_api_physical_drive_sanitize(pvd_record_p->pdo_obj_id, pattern);
            if (status != FBE_STATUS_OK) {
                fbe_cli_error("EAS -erase on pdo 0x%x failed\n", pvd_record_p->pdo_obj_id);
            }
        }
        pvd_record_p++;
    }

    fbe_api_free_memory(pvd_list_p);
    return status;
}

static fbe_status_t fbe_eas_lib_eas_state(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    eas_pvd_object_record_t *pvd_list_p = NULL;
    eas_pvd_object_record_t *pvd_record_p = NULL;

    status = fbe_eas_lib_eas_get_drives(argc, argv, &pvd_list_p);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("EAS -erase state\n");
        return status;
    }

    pvd_record_p = pvd_list_p;
    while (pvd_record_p->pvd_obj_id != FBE_OBJECT_ID_INVALID) {
        if (pvd_record_p->eas_state != EAS_STATE_INVALID) {
            fbe_cli_printf("\t Drive %d_%d_%d pvd_object_id 0x%x state %s\n", 
                           pvd_record_p->port_number, pvd_record_p->enclosure_number, pvd_record_p->slot_number,
                           pvd_record_p->pvd_obj_id, fbe_eas_print_state(pvd_record_p->eas_state));
        }
        pvd_record_p++;
    }

    fbe_api_free_memory(pvd_list_p);
    return status;
}

static fbe_status_t fbe_eas_lib_eas_commit(int argc, char** argv)
{
    fbe_status_t   status = FBE_STATUS_OK;
    eas_pvd_object_record_t *pvd_list_p = NULL;
    eas_pvd_object_record_t *pvd_record_p = NULL;

    status = fbe_eas_lib_eas_get_drives(argc, argv, &pvd_list_p);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("EAS -commit failed\n");
        return status;
    }

    /* Loop through all the drives */
    pvd_record_p = pvd_list_p;
    while (pvd_record_p->pvd_obj_id != FBE_OBJECT_ID_INVALID) {
        if (pvd_record_p->eas_state == EAS_STATE_ERASE_DONE) {
            pvd_record_p->action_required = FBE_TRUE;
        } else {
            pvd_record_p->action_required = FBE_FALSE;
        }
        pvd_record_p++;
    }

    fbe_cli_printf("EAS: The following drives will commit EAS:\n");
    if (fbe_eas_lib_eas_get_confirmation(pvd_list_p) != FBE_TRUE) {
        fbe_api_free_memory(pvd_list_p);
        return FBE_STATUS_OK;
    }

    pvd_record_p = pvd_list_p;
    while (pvd_record_p->pvd_obj_id != FBE_OBJECT_ID_INVALID) {
        if (pvd_record_p->action_required) {
            status = fbe_api_provision_drive_set_eas_complete(pvd_record_p->pvd_obj_id);
            if (status != FBE_STATUS_OK) {
                fbe_cli_error("EAS -commit on pvd 0x%x failed\n", pvd_record_p->pvd_obj_id);
            }
        }
        pvd_record_p++;
    }

    fbe_api_free_memory(pvd_list_p);
    return status;
}
