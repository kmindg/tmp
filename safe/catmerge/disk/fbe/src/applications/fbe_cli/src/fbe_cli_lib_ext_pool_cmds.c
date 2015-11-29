/**************************************************************************
 * Copyright (C) EMC Corporation 2014 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 *************************************************************************/

/*!************************************************************************
* @file fbe_cli_lib_ext_pool_cmds.c
 **************************************************************************
 *
 * @brief
 *  This file contains extent pool commands.
 * 
 * @ingroup fbe_cli
 *
 * @date
 *  6/24/2014 - Created. Rob Foley
 **************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_extent_pool_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_api_raid_group_interface.h"
#include "fbe_cli_extpool.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_cli_lib_paged_utils.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "fbe_cli_luninfo.h"
#include "fbe_bvd_interface.h"
#include "fbe_api_bvd_interface.h"
#include "fbe/fbe_random.h"

static void fbe_cli_extpool_parse_cmds(int argc, char** argv);

static void fbe_cli_extpool_create(fbe_u32_t argc, char** argv);
static void fbe_cli_create_extent_pool(fbe_u32_t pool_id, fbe_u32_t drive_count, fbe_drive_type_t drive_type);

static void fbe_cli_extpool_lun_create(fbe_u32_t argc, char** argv);
static void fbe_cli_create_extent_pool_lun(fbe_u32_t pool_id, fbe_u32_t lun_id, fbe_lba_t capacity);

static void fbe_cli_extpool_destroy(fbe_u32_t argc, char** argv);
static void fbe_cli_destroy_extent_pool(fbe_u32_t pool_id);
static void fbe_cli_extpool_lun_destroy(fbe_u32_t argc, char** argv);
static void fbe_cli_destroy_extent_pool_lun(fbe_u32_t lun_id);

static void fbe_cli_extpool_lun_info(fbe_u32_t argc, char** argv);
static void fbe_cli_extpool_lun_display_details(fbe_object_id_t lun_id);
/*!*******************************************************************
 * @var fbe_cli_extpool
 *********************************************************************
 * @brief Function to implement rginfo commands.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *********************************************************************/
void fbe_cli_extpool(int argc, char** argv)
{
    fbe_cli_printf("%s", "\n");
    fbe_cli_extpool_parse_cmds(argc, argv);
}
/******************************************
 * end fbe_cli_extpool()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_extpool_parse_cmds()
 ****************************************************************
 *
 * @brief   Parse extent pool commands
 * 
 * @param   argument count
 * @param arguments list
 *
 * @return  None.
 *
 * @author
 *  6/24/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void fbe_cli_extpool_parse_cmds(int argc, char** argv)
{
    if (argc == 0) {
        fbe_cli_printf("rginfo: ERROR: Too few args.\n");
        fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
        return;
    }
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0) ) {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
        return;
    }
    /* Parse args
     *  
     */
    while (argc && **argv == '-') {
        /* Display the raid group information. 
         */
        if (!strcmp(*argv, "-create")) {
            argc--;
            argv++;
            fbe_cli_extpool_create(argc, argv);
            return;
        } else if (!strcmp(*argv, "-createlun")) {
            argc--;
            argv++;
            fbe_cli_extpool_lun_create(argc, argv);
            return;
        } else if (!strcmp(*argv, "-luninfo")) {
            argc--;
            argv++;
            fbe_cli_extpool_lun_info(argc, argv);
            return;
        } else if (!strcmp(*argv, "-destroy")) {
            argc--;
            argv++;
            fbe_cli_extpool_destroy(argc, argv);
            return;
        } else if (!strcmp(*argv, "-destroylun")) {
            argc--;
            argv++;
            fbe_cli_extpool_lun_destroy(argc, argv);
            return;
        } else {
            fbe_cli_printf("rginfo: ERROR: Invalid switch.\n");
            fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
            return;
        }
        argv++;
    }
    return;
}
/******************************************
 * end fbe_cli_extpool_parse_cmds()
 ******************************************/

/*!**************************************************************
 * fbe_cli_extpool_create()
 ****************************************************************
 * @brief
 *  Create an extent pool
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * 
 * @return fbe_status_t   
 *
 * @author
 *  6/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_cli_extpool_create(fbe_u32_t argc, char** argv)
{
    fbe_u32_t         pool_id = FBE_U32_MAX;
    fbe_u32_t         drive_count = FBE_U32_MAX; 
    fbe_drive_type_t  drive_type = FBE_DRIVE_TYPE_INVALID;

    if (argc == 0) {
        fbe_cli_printf("rginfo: ERROR: Too few args.\n");
        fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-') {
        if (!strcmp(*argv, "-pool")) {
            argv++;
            argc--;
            pool_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        } else if (!strcmp(*argv, "-drive_count")) {
            argc--;
            argv++;
            drive_count = (fbe_u32_t)strtoul(*argv, 0, 0);
        } else if (!strcmp(*argv, "-drive_type")) {
            argc--;
            argv++;
            drive_type = (fbe_u32_t)strtoul(*argv, 0, 0);
        } else {
            fbe_cli_printf("extpool: ERROR: Invalid switch.\n");
            fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
            return;
        }
        argc--;
        argv++;
    }
    fbe_cli_create_extent_pool(pool_id, drive_count, drive_type);
    return;
}
/**************************************
 * end fbe_cli_extpool_create
 **************************************/
/*!**************************************************************
 * fbe_cli_create_extent_pool()
 ****************************************************************
 * @brief
 *  This function creates an exent pool.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  6/24/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void 
fbe_cli_create_extent_pool(fbe_u32_t pool_id, fbe_u32_t drive_count, fbe_drive_type_t drive_type)
{
    fbe_api_job_service_create_extent_pool_t create_pool;
    fbe_status_t status;

    fbe_cli_printf("creating extent pool 0x%x drives: %u drive_type: %u\n", pool_id, drive_count, drive_type);

    create_pool.drive_count = drive_count;
    create_pool.drive_type = drive_type;
    create_pool.pool_id = pool_id;
	status = fbe_api_job_service_create_extent_pool(&create_pool);

    if (status == FBE_STATUS_OK) {
        fbe_cli_printf("create pool lun completed Successfully\n");
    } else {
        fbe_cli_printf("create pool lun completed with status 0x%x\n", status);
    }
    return;
}
/******************************************
 * end fbe_cli_create_extent_pool()
 ******************************************/

/*!**************************************************************
 * fbe_cli_extpool_lun_create()
 ****************************************************************
 * @brief
 *  Create an extent pool
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * 
 * @return fbe_status_t   
 *
 * @author
 *  6/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_cli_extpool_lun_create(fbe_u32_t argc, char** argv)
{
    fbe_u32_t         pool_id = FBE_U32_MAX;
    fbe_u32_t         lun_id = FBE_U32_MAX; 
    fbe_lba_t         capacity = FBE_U32_MAX;

    if (argc == 0) {
        fbe_cli_printf("rginfo: ERROR: Too few args.\n");
        fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-') {
        argc--;
        if (!strcmp(*argv, "-pool")) {
            argc--;
            argv++;
            pool_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        } else if (!strcmp(*argv, "-lun")) {
            argc--;
            argv++;
            lun_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        } else if (!strcmp(*argv, "-capacity")) {
            argc--;
            argv++;
            capacity = (fbe_u32_t)strtoul(*argv, 0, 0);
        }else if (!strcmp(*argv, "-capacitymb")) {
            argc--;
            argv++;
            capacity = (fbe_u32_t)strtoul(*argv, 0, 0);
            capacity *= FBE_BLOCKS_PER_MEGABYTE; // scale up to MB
        }else if (!strcmp(*argv, "-capacitygb")) {
            argc--;
            argv++;
            capacity = (fbe_u32_t)strtoul(*argv, 0, 0);
            capacity *= FBE_BLOCKS_PER_GIGABYTE; // scale up to GB
        } else {
            fbe_cli_printf("extpool: ERROR: Invalid switch.\n");
            fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
            return;
        }
        argv++;
    }
    fbe_cli_create_extent_pool_lun(pool_id, lun_id, capacity);
    return;
}
/**************************************
 * end fbe_cli_extpool_lun_create
 **************************************/
/*!**************************************************************
 * fbe_cli_create_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function creates an exent pool lun.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  6/24/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void 
fbe_cli_create_extent_pool_lun(fbe_u32_t pool_id, fbe_u32_t lun_id, fbe_lba_t capacity)
{
    fbe_api_job_service_create_ext_pool_lun_t create_lun;
    fbe_status_t status;
    fbe_assigned_wwid_t  wwn;
    fbe_u32_t assign_wwn_count;
    fbe_u32_t random_seed = (fbe_u32_t)fbe_get_time();

    fbe_cli_generate_random_wwn_number(&wwn);
    wwn.bytes[FBE_WWN_BYTES-1] = '\0';
    assign_wwn_count = 1;
    if (fbe_cli_checks_for_simillar_wwn_number(wwn)) {
        /* The random seed allows us to make tests more random. 
         * We seed the random number generator with the time. 
         */
        fbe_random_set_seed(random_seed);
    }
    while (fbe_cli_checks_for_simillar_wwn_number(wwn)) {
        fbe_cli_generate_random_wwn_number(&wwn);
        wwn.bytes[FBE_WWN_BYTES-1] = '\0';
        if (assign_wwn_count++ > 200) {
            fbe_cli_printf("Try to assign wwn for %d times\n", assign_wwn_count);
            if (assign_wwn_count > 5000)
                break;
        }
    }

    fbe_cli_printf("creating extent pool lun: %d capacity: 0x%llx pool_id: %d\n", lun_id, capacity, pool_id);

    create_lun.lun_id = lun_id;
    create_lun.pool_id = pool_id;
    create_lun.capacity = capacity;
    create_lun.world_wide_name = wwn;
	status = fbe_api_job_service_create_ext_pool_lun(&create_lun);

    if (status == FBE_STATUS_OK) {
        fbe_cli_printf("create pool lun completed Successfully\n");
    } else {
        fbe_cli_printf("create pool lun completed with status 0x%x\n", status);
    }
    return;
}
/******************************************
 * end fbe_cli_create_extent_pool_lun()
 ******************************************/

/*!**************************************************************
 * fbe_cli_extpool_lun_info()
 ****************************************************************
 * @brief
 *  Display ext pool lun info.
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * 
 * @return fbe_status_t   
 *
 * @author
 *  6/25/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_cli_extpool_lun_info(fbe_u32_t argc, char** argv)
{
    fbe_status_t      status;
    fbe_u32_t         lun_id = FBE_U32_MAX; 
    fbe_object_id_t   lun_object_id;
    fbe_object_id_t   lookup_object_id;
    fbe_cli_lurg_lun_details_t      lun_details;
    fbe_database_lun_info_t         *lun_info = NULL;

    if (argc == 0) {
        fbe_cli_printf("rginfo: ERROR: Too few args.\n");
        fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-') {
        argc--;
        if (!strcmp(*argv, "-lun")) {
            argc--;
            argv++;
            lun_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (!strcmp(*argv, "-get_all")) {
            fbe_u32_t total_luns;
            fbe_u32_t total_ext_pool_luns;
            fbe_u32_t returned_luns;
            fbe_u32_t lun_index;

            status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &total_luns);
            if (status != FBE_STATUS_OK) {
                fbe_cli_printf("failed to enumerate luns status: 0x%x\n", status);
            }
            status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_EXTENT_POOL_LUN, FBE_PACKAGE_ID_SEP_0, &total_ext_pool_luns);
            if (status != FBE_STATUS_OK) {
                fbe_cli_printf("failed to enumerate ext pool luns status: 0x%x\n", status);
            }
            lun_info = (fbe_database_lun_info_t *)fbe_api_allocate_memory((total_luns + total_ext_pool_luns) * sizeof(fbe_database_lun_info_t));
            if (lun_info == NULL) {
                fbe_cli_printf("failed to allocate lun info\n");
                return;
            }
            fbe_zero_memory(lun_info, (total_luns + total_ext_pool_luns) * sizeof(fbe_database_lun_info_t));
            status = fbe_api_database_get_all_luns(lun_info, total_luns + total_ext_pool_luns, &returned_luns);
            if (status != FBE_STATUS_OK) {
                fbe_cli_printf("failed to get all ext pool luns status: 0x%x\n", status);
            }
            fbe_cli_printf("found %u LUNs\n", returned_luns);
            for (lun_index = 0; lun_index < returned_luns; lun_index++) {
                fbe_cli_printf("found LUN: %u object_id: 0x%x capacity: 0x%llx offset: 0x%llx\n", 
                               lun_info[lun_index].lun_number, lun_info[lun_index].lun_object_id,
                               lun_info[lun_index].capacity, lun_info[lun_index].offset);
                fbe_cli_extpool_lun_display_details(lun_info[lun_index].lun_object_id);
                fbe_cli_lib_lun_display_raid_info(lun_info[lun_index].lun_object_id);
            }
            return;
        }
        argv++;
    }

    status = fbe_api_database_lookup_ext_pool_lun_by_number(lun_id, &lun_object_id);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("could not lookup extent pool lun %u status: %u\n", lun_id, status);
        return;
    }

    fbe_cli_get_lun_details(lun_object_id, &lun_details);
    status = fbe_api_database_lookup_lun_by_wwid(lun_details.lun_info.world_wide_name, &lookup_object_id);
    if (status == FBE_STATUS_OK) {
        fbe_cli_printf("Lookup of LUN object: 0x%x by WWN successful.\n", lun_object_id);
    } else {
        fbe_cli_printf("Unable to lookup this LUN object: 0x%x by WWN\n", lun_object_id);
    }
    fbe_cli_extpool_lun_display_details(lun_object_id);
    fbe_cli_lib_lun_display_raid_info(lun_object_id);
    return;
}
/**************************************
 * end fbe_cli_extpool_lun_info
 **************************************/

/************************************************************************** 
* fbe_cli_extpool_lun_display_details ()
***************************************************************************
* @brief 
*  This function prints the information of a single lun.
*
* @param lun_id - LUN Object id
* 
* @return
*  None
*
* @version
*  6/25/2014 - Created. Rob Foley
***************************************************************************/
static void fbe_cli_extpool_lun_display_details(fbe_object_id_t lun_id)
{
    fbe_status_t                    status;
    fbe_cli_lurg_lun_details_t      lun_details;
    //fbe_status_t                    lifecycle_status = FBE_STATUS_INVALID;
    //fbe_lifecycle_state_t           lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u32_t                       wwn_index = 0;
    fbe_u32_t                       wwn_size = 0;
    fbe_object_id_t                 bvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_volume_attributes_flags     bvd_attribute;


    /*Get lun details*/
    fbe_cli_get_lun_details(lun_id,&lun_details);

    /* Get BVD object id */
    status = fbe_api_bvd_interface_get_bvd_object_id(&bvd_object_id);
    if (status != FBE_STATUS_OK ){
        fbe_cli_error ("Failed to get BVD ID, status : 0x%X \n", status);
        return;
    }

    /* Get attribute from BVD object */
    status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &bvd_attribute, lun_id);
    if (status != FBE_STATUS_OK ){
        fbe_cli_error ("Failed to get attribute from BVD object!, status : 0x%X \n", status);
    }
    
    fbe_cli_printf("Lun information:\n");
    fbe_cli_printf("----------------\n");
    fbe_cli_printf("Logical Unit:    %d\n", lun_details.lun_number);
    fbe_cli_printf("  Lifecycle State: %d [%s]\n", lun_details.lifecycle_state, lun_details.p_lifecycle_state_str);
    fbe_cli_printf("  Lun Object-id:   0x%x\n", lun_id);
    fbe_cli_printf("  Offset:          0x%llx\n",(unsigned long long)lun_details.lun_info.offset);
    fbe_cli_printf("  Capacity:        0x%llx\n",(unsigned long long)lun_details.lun_info.capacity);
    fbe_cli_printf("  Pool ID:         0x%x\n", lun_details.lun_info.rg_number);
    fbe_cli_printf("  Pool Object ID:  0x%x\n", lun_details.lun_info.raid_group_obj_id);

    wwn_size = sizeof(lun_details.lun_info.world_wide_name.bytes);
    fbe_cli_printf("  world_wide_name: ");
    for(wwn_index = 0; wwn_index < wwn_size; ++wwn_index)
    {
        fbe_cli_printf("%02x",(fbe_u32_t)(lun_details.lun_info.world_wide_name.bytes[wwn_index]));
        fbe_cli_printf(":");    
    }
    fbe_cli_printf("\n");

    fbe_cli_printf("  Bind Time :      0x%llx\n",(unsigned long long)lun_details.lun_info.bind_time);
    fbe_cli_printf("  User Private :   %s\n",(lun_details.lun_info.user_private==FBE_TRUE)?"TRUE":"FALSE");
    fbe_cli_printf("  Attributes:      0x%X\n",lun_details.lun_info.attributes);
    fbe_cli_printf("  Rotational_rate: %d\n",lun_details.lun_info.rotational_rate);
    fbe_cli_printf("  BVD Attributes : 0x%X\n", bvd_attribute);
    fbe_cli_printf("\n");
    fbe_cli_display_metadata_element_state(lun_id);

    fbe_cli_printf("\n");
}
/******************************************
 * end fbe_cli_extpool_lun_display_details()
 ******************************************/


/*!**************************************************************
 * fbe_cli_extpool_destroy()
 ****************************************************************
 * @brief
 *  Destroy an extent pool
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * 
 * @return fbe_status_t   
 *
 * @author
 *  7/2/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void fbe_cli_extpool_destroy(fbe_u32_t argc, char** argv)
{
    fbe_u32_t         pool_id = FBE_U32_MAX;

    if (argc == 0) {
        fbe_cli_printf("rginfo: ERROR: Too few args.\n");
        fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-') {
        if (!strcmp(*argv, "-pool")) {
            argv++;
            argc--;
            pool_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        } else {
            fbe_cli_printf("extpool: ERROR: Invalid switch.\n");
            fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
            return;
        }
        argc--;
        argv++;
    }
    fbe_cli_destroy_extent_pool(pool_id);
    return;
}
/**************************************
 * end fbe_cli_extpool_destroy
 **************************************/
/*!**************************************************************
 * fbe_cli_destroy_extent_pool()
 ****************************************************************
 * @brief
 *  This function destroys an exent pool.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  7/2/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void 
fbe_cli_destroy_extent_pool(fbe_u32_t pool_id)
{
    fbe_api_job_service_destroy_extent_pool_t destroy_pool;
    fbe_status_t status;

    fbe_cli_printf("destroying extent pool 0x%x \n", pool_id);

    destroy_pool.pool_id = pool_id;
	status = fbe_api_job_service_destroy_extent_pool(&destroy_pool);

    if (status == FBE_STATUS_OK) {
        fbe_cli_printf("destroy pool completed Successfully\n");
    } else {
        fbe_cli_printf("destroy pool completed with status 0x%x\n", status);
    }
    return;
}
/******************************************
 * end fbe_cli_destroy_extent_pool()
 ******************************************/

/*!**************************************************************
 * fbe_cli_extpool_lun_destroy()
 ****************************************************************
 * @brief
 *  Create an extent pool
 *
 * @param argc - count of arguments              
 * @param argv - vector of arguments
 * 
 * @return fbe_status_t   
 *
 * @author
 *  7/2/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void fbe_cli_extpool_lun_destroy(fbe_u32_t argc, char** argv)
{
    fbe_u32_t         lun_id = FBE_U32_MAX; 

    if (argc == 0) {
        fbe_cli_printf("rginfo: ERROR: Too few args.\n");
        fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-') {
        argc--;
        if (!strcmp(*argv, "-lun")) {
            argc--;
            argv++;
            lun_id = (fbe_u32_t)strtoul(*argv, 0, 0);
        } else {
            fbe_cli_printf("extpool: ERROR: Invalid switch.\n");
            fbe_cli_printf("%s", EXTPOOL_CMD_USAGE);
            return;
        }
        argv++;
    }
    fbe_cli_destroy_extent_pool_lun(lun_id);
    return;
}
/**************************************
 * end fbe_cli_extpool_lun_destroy
 **************************************/

/*!**************************************************************
 * fbe_cli_destroy_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function destroys an exent pool lun.
 *
 * @param lun_id - lun id.               
 *
 * @return None.   
 *
 * @author
 *  7/2/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void 
fbe_cli_destroy_extent_pool_lun(fbe_u32_t lun_id)
{
    fbe_api_job_service_destroy_ext_pool_lun_t destroy_lun = {0};
    fbe_status_t status;

    fbe_cli_printf("destroying extent pool lun: %d\n", lun_id);

    destroy_lun.lun_id = lun_id;
    status = fbe_api_job_service_destroy_ext_pool_lun(&destroy_lun);

    if (status == FBE_STATUS_OK) {
        fbe_cli_printf("destroy pool lun completed Successfully\n");
    } else {
        fbe_cli_printf("destroy pool lun completed with status 0x%x\n", status);
    }
    return;
}
/******************************************
 * end fbe_cli_destroy_extent_pool_lun()
 ******************************************/

/*************************
 * end file fbe_cli_lib_ext_pool_cmds.c
 *************************/
