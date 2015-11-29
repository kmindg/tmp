/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
 * @file fbe_cli_lib_resum_prom_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the read, write and initialize operations for prom info.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  12-August-2011:  Shubhada Savdekar - created
 *  9-November-2011:  Harshal Wanjari
 *..18-May-2012..Zhou Eric
 *  12-Jul-2012: Feng Ling
 *
 ***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cli_private.h"
#include "fbe/fbe_cli.h"
#include "adm_api.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe_base_environment_debug.h"
#include "fbe_cli_lib_resume_prom_cmds.h"
#include "string.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe_base_environment_debug.h"

#define CASENAME(E) case E: return #E
#define DFLTNAME(V, E)  default: return fbe_cli_get_unknown_value(#V, E)

/*!*********************************************************************
* @fbe_cli_get_prom_info(int argc, char**argv)
*********************************************************************
*
*  @brief
*      Select the option for resumeprom command.
*
*  @param :
*   argc     (INPUT) NUMBER OF ARGUMENTS
*   argv     (INPUT) ARGUMENT LIST
*
*  @return:
*      void
* 
*  @author
* * 12-Aug 2011: created:  Shubhada Savdekar
*********************************************************************/
void fbe_cli_get_prom_info(int argc, char**argv) {

    if (argc == 0) {
        fbe_cli_printf("ERROR : Too few arguments.\n");
        fbe_cli_printf("%s", GP_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", GP_USAGE);
        return;
    }


    if(strcmp(*argv, "-r") == 0) /*Read operation*/
    {
        fbe_cli_lib_resume_prom_read((fbe_u32_t)argc, (fbe_char_t**)argv);
    }
    else if(strcmp(*argv, "-w") == 0) /*Write operation*/
    {
        fbe_cli_lib_resume_prom_write((fbe_u32_t)argc, (fbe_char_t**)argv);
    }
    else
    {
        fbe_cli_printf("resumeprom: ERROR : -r or -w required first argument.\n");
    }
}

/*!********************************************************************
* @fbe_cli_lib_resume_prom_read(fbe_u32_t argc, fbe_char_t**argv) 
*********************************************************************
*
*  @brief
*      read the prom info.
*
* @param :
*   argc     (INPUT) NUMBER OF ARGUMENTS
*   argv     (INPUT) ARGUMENT LIST
*
* @return:
*      void
* 
* @author
* * 12-Aug 2011: created:  Shubhada Savdekar
*********************************************************************/
void fbe_cli_lib_resume_prom_read(fbe_u32_t argc, fbe_char_t**argv) 
{
    fbe_status_t                                  status = FBE_STATUS_OK;
    fbe_esp_base_env_get_resume_prom_info_cmd_t * pGetResumePromInfo = NULL;
    fbe_u64_t                             dev_type = FBE_DEVICE_TYPE_INVALID;
//    fbe_esp_board_mgmt_board_info_t               boardInfo = {0};
    fbe_bool_t                                    bus_number_set = FALSE;
    fbe_bool_t                                    encl_number_set = FALSE;
    fbe_bool_t                                    slot_number_set = FALSE;
    fbe_bool_t                                    sp_set = FALSE;
    fbe_bool_t                                    device_type_set = FALSE;
    fbe_bool_t                                    check_param_ok = TRUE;

    argc--;
    argv++;

    /* Allocate memory for pGetResumeProm */
    pGetResumePromInfo = (fbe_esp_base_env_get_resume_prom_info_cmd_t *) fbe_api_allocate_memory(sizeof (fbe_esp_base_env_get_resume_prom_info_cmd_t));
    if(pGetResumePromInfo == NULL)
    {
        fbe_cli_printf("resumeprom: ERROR: Memory allocation failed.\n");
        return;
    }

    fbe_zero_memory(pGetResumePromInfo, sizeof(fbe_esp_base_env_get_resume_prom_info_cmd_t));

    /*Extract the command*/
    while(argc > 0){
        if(strcmp(*argv, "-all") == 0){
            fbe_cli_lib_resume_prom_read_all(pGetResumePromInfo);
            fbe_api_free_memory(pGetResumePromInfo);
            return;
        }
        /*Get bus number*/
        else if(strcmp(*argv, "-b") == 0){
            argc--;
            argv++;
            status = get_and_verify_bus_number(argv, &pGetResumePromInfo->deviceLocation);
            if(status != FBE_STATUS_OK){
               fbe_api_free_memory(pGetResumePromInfo);
                fbe_cli_printf("resumeprom: ERROR: Bus number not valid.\n");
                return;
            }
            bus_number_set = TRUE;
        }

        /*Get enclosure number*/
       else if(strcmp(*argv, "-e") == 0){
            argc--;
            argv++;
            status = get_and_verify_enclosure_number(argv, &pGetResumePromInfo->deviceLocation);
            if(status != FBE_STATUS_OK){
                            fbe_api_free_memory(pGetResumePromInfo); 
                fbe_cli_printf("resumeprom: ERROR: Bus number not valid.\n");
                return;
            }
            encl_number_set = TRUE;
        }

        /*Get slot number*/
        else if(strcmp(*argv, "-s") == 0){
            argc--;
            argv++;
            status = get_slot_number(argv, &pGetResumePromInfo->deviceLocation);
            if(status != FBE_STATUS_OK){
                fbe_api_free_memory(pGetResumePromInfo);
                fbe_cli_printf("resumeprom: ERROR: Bus number not valid.\n");
                return;
            }
            slot_number_set = TRUE;
        }

        /*Get device type*/
        else if(strcmp(*argv, "-d") == 0){
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("Please provide device type.\n");
                fbe_cli_printf("%s", GP_USAGE);
                fbe_api_free_memory(pGetResumePromInfo);
                return;
            }
            status = fbe_cli_get_device_type(*argv, &dev_type);
            if((dev_type == FBE_DEVICE_TYPE_INVALID) || (status != FBE_STATUS_OK))
            {
                fbe_cli_printf("%s", GP_USAGE);
                fbe_api_free_memory(pGetResumePromInfo);
                return;
            }
            pGetResumePromInfo->deviceType = dev_type;
            device_type_set = TRUE;
        }

        /*Get SP*/
        else if(strcmp(*argv, "-sp") == 0){
            argc--;
            argv++;
            pGetResumePromInfo->deviceLocation.sp = fbe_atoi(*argv);
            sp_set = TRUE;
        }

        argc--;
        argv++;
    }

    //get resume prom command check, could not continue with not enough argument.
    if(!device_type_set)
    {
        fbe_cli_printf("Arguments ERROR! Device type must be specified!\n");
        check_param_ok = FALSE;
    }
    switch(pGetResumePromInfo->deviceType)
    {
        case FBE_DEVICE_TYPE_SP:
        case FBE_DEVICE_TYPE_IOMODULE:
        case FBE_DEVICE_TYPE_BACK_END_MODULE:
        case FBE_DEVICE_TYPE_MEZZANINE:
        case FBE_DEVICE_TYPE_MGMT_MODULE:
        case FBE_DEVICE_TYPE_BATTERY:
        case FBE_DEVICE_TYPE_CACHE_CARD:
            if(!sp_set || ! slot_number_set)
            {
                fbe_cli_printf("Arguments ERROR! For device type sp, iom, bm, mezz, bbu, cachecard and mgmt, sp and slot number must be specified!\n");
                check_param_ok = FALSE;
            }
            break;
        case FBE_DEVICE_TYPE_ENCLOSURE:
        case FBE_DEVICE_TYPE_PS:
        case FBE_DEVICE_TYPE_LCC:
        case FBE_DEVICE_TYPE_FAN:
        case FBE_DEVICE_TYPE_DRIVE_MIDPLANE:
            if(!bus_number_set || !encl_number_set || ! slot_number_set)
            {
                fbe_cli_printf("Arguments ERROR! For device type midplane, ps, lcc, fan, drivemidplane, bus number, encl number and slot number must be specified!\n");
                check_param_ok = FALSE;
            }
            break;
        default:
            fbe_cli_printf("Arguments ERROR! Invalidate device type!\n");
    }

    if(!check_param_ok)
    {
        fbe_cli_printf("%s", GP_USAGE);
        fbe_api_free_memory(pGetResumePromInfo);
        return;
    }

    status = fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("The resume prom read successfully!!\n");
    }
    fbe_api_free_memory(pGetResumePromInfo);

    return;

}

/*!********************************************************************
* @fbe_cli_lib_resume_prom_display_info (RESUME_PROM_STRUCTURE *prom_info)
*********************************************************************
*
*  @brief
*      Display the retrived information.
*
*  @param 
*            prom_info - A resume prom structure
*
*  @returns:
*      void
* 
*  @author
* * 12-Aug 2011: created:  Shubhada Savdekar
*********************************************************************/
void fbe_cli_lib_resume_prom_display_info (RESUME_PROM_STRUCTURE *prom_info)
{
    fbe_u32_t  count;
    fbe_u32_t  numProgrammables;
     
    fbe_cli_printf("EMC TLA Part Number       : %.16s\n", prom_info->emc_tla_part_num);
    fbe_cli_printf("EMC TLA Artwork Revision  : %.3s\n", prom_info->emc_tla_artwork_rev);
    fbe_cli_printf("EMC TLA Assembly Revision : %.3s\n", prom_info->emc_tla_assembly_rev);
    fbe_cli_printf("EMC TLA Serial Number     : %.16s\n", prom_info->emc_tla_serial_num);
    fbe_cli_printf("EMC System HW Part Number : %.16s\n", prom_info->emc_system_hw_pn);
    fbe_cli_printf("EMC System HW Serial Number : %.16s\n", prom_info->emc_system_hw_sn);
    fbe_cli_printf("EMC System HW Revision    : %.3s\n", prom_info->emc_system_hw_rev);
    fbe_cli_printf("Product Part Number       : %.16s\n", prom_info->product_part_number);
    fbe_cli_printf("Product Serial Number     : %.16s\n", prom_info->product_serial_number);
    fbe_cli_printf("Product Revision          : %.3s\n", prom_info->product_revision);
    fbe_cli_printf("Vendor Part Number        : %.32s\n", prom_info->vendor_part_num);
    fbe_cli_printf("Vendor Artwork Revision   : %.3s\n", prom_info->vendor_artwork_rev);
    fbe_cli_printf("Vendor Assembly Revision  : %.3s\n", prom_info->vendor_assembly_rev);
    fbe_cli_printf("Vendor Unique Revision    : %.4s\n", prom_info->vendor_unique_rev);
    fbe_cli_printf("Vendor ACDC Input Type    : %.2s\n", prom_info->vendor_acdc_input_type);
    fbe_cli_printf("Vendor Serial Number      : %.32s\n", prom_info->vendor_serial_num);
    fbe_cli_printf("Vendor Name               : %.32s\n", prom_info->vendor_name);
    fbe_cli_printf("Manfacture Location       : %.32s\n", prom_info->loc_mft);
    fbe_cli_printf("Year of Manufacture       : %.4s\n", prom_info->year_mft);
    fbe_cli_printf("Month of Manufacture      : %.2s\n", prom_info->month_mft);
    fbe_cli_printf("Day of Manufacture        : %.2s\n", prom_info->day_mft);
    fbe_cli_printf("Assembly Name             : %.32s\n", prom_info->tla_assembly_name);
    fbe_cli_printf("Num. of Programmables     : %d\n", (UINT_8) prom_info->num_prog[0]);
    numProgrammables = (RESUME_PROM_MAX_PROG_COUNT > prom_info->num_prog[0])? prom_info->num_prog[0]: RESUME_PROM_MAX_PROG_COUNT;
    for(count = 0; count < numProgrammables; count++)
    {
        fbe_cli_printf("\tProg. Name            : %.8s\n",prom_info->prog_details[count].prog_name);
        fbe_cli_printf("\tProg. Revision        : %.4s\n",prom_info->prog_details[count].prog_rev);
    }
    fbe_cli_printf("WWN Seed                 : 0x%08X\n", prom_info->wwn_seed);
    fbe_cli_printf("SAS Address              : 0x%02X 0x%02X 0x%02X 0x%02X\n", prom_info->sas_address[0],
                   prom_info->sas_address[1],
                   prom_info->sas_address[2],
                   prom_info->sas_address[3]);
    fbe_cli_printf("PCBA Part Number         : %.16s\n", prom_info->pcba_part_num);
    fbe_cli_printf("PCBA Assembly Revision   : %.3s\n", prom_info->pcba_assembly_rev);
    fbe_cli_printf("PCBA Serial Number       : %.16s\n", prom_info->pcba_serial_num);
    fbe_cli_printf("Configuration Type       : 0x%02X\n", prom_info->configuration_type[0]);
    fbe_cli_printf("EMC Alternate MB Part #  : %.16s\n", prom_info->emc_alt_mb_part_num);
    fbe_cli_printf("Channel Speed            : 0x%02X\n", prom_info->channel_speed[0]);
    fbe_cli_printf("System Type              : 0x%02X\n", prom_info->system_type[0]);
    fbe_cli_printf("Enclosure ID             : 0x%02X\n", prom_info->dae_encl_id[0]);
    fbe_cli_printf("Rack ID                  : 0x%02X\n", prom_info->rack_id[0]);
    fbe_cli_printf("Slot ID                  : 0x%02X\n", prom_info->slot_id[0]);
    fbe_cli_printf("Drive SpinUp Select      : 0x%02X\n", prom_info->drive_spin_up_select[0]);
    fbe_cli_printf("SP Family FRU ID         : 0x%02X 0x%02X 0x%02X 0x%02X\n", prom_info->sp_family_fru_id[0],
                                                                       prom_info->sp_family_fru_id[1],
                                                                       prom_info->sp_family_fru_id[2],
                                                                       prom_info->sp_family_fru_id[3]);
    fbe_cli_printf("System Orientation       : 0x%02X\n", prom_info->fru_capability[0]);
    fbe_cli_printf("EMC Part Number          : %.16s\n", prom_info->emc_sub_assy_part_num);
    fbe_cli_printf("EMC Artwork Revision     : %.3s\n", prom_info->emc_sub_assy_artwork_rev);
    fbe_cli_printf("EMC Assembly Revision    : %.3s\n", prom_info->emc_sub_assy_rev);
    fbe_cli_printf("EMC Serial Number        : %.16s\n\n", prom_info->emc_sub_assy_serial_num);
    return;
}

/*!********************************************************************
* @fbe_cli_lib_resume_prom_write(fbe_u32_t argc, fbe_char_t**argv) 
*********************************************************************
*
*@brief
*      write to the prom info.
*
* @param :
*   argc     (INPUT) NUMBER OF ARGUMENTS
*   argv     (INPUT) ARGUMENT LIST
*
*  @returns:
*      void
* 
* @author
* * 12-Aug 2011: created:  Shubhada Savdekar
*********************************************************************/
void fbe_cli_lib_resume_prom_write(fbe_u32_t argc, fbe_char_t**argv) 
{
    fbe_status_t                 status = FBE_STATUS_OK;
    fbe_resume_prom_cmd_t        writeResume = {0};
    fbe_u64_t            dev_type = FBE_DEVICE_TYPE_INVALID;
    RESUME_PROM_FIELD            field_type;
    writeResume.readOpStatus =   FBE_RESUME_PROM_STATUS_INVALID;

    argc--;
    argv++;

    /*Extract the command*/
    while(argc > 0){
        /*Get bus number*/
        if(strcmp(*argv, "-b") == 0){
            argc--;
            argv++;
            status = get_and_verify_bus_number(argv, &writeResume.deviceLocation);

            if(status != FBE_STATUS_OK){
                fbe_cli_printf("resumeprom: ERROR: Bus number not valid.\n");
                return;
            }
        }

        /*Get enclosure number*/
        else if(strcmp(*argv, "-e") == 0){
           argc--;
           argv++;
           status = get_and_verify_enclosure_number(argv, &writeResume.deviceLocation);

           if(status != FBE_STATUS_OK){
               fbe_cli_printf("resumeprom: ERROR: Bus number not valid.\n");
               return;
            }
       }

       /*Get slot number*/
       else if(strcmp(*argv, "-s") == 0){
           argc--;
           argv++;

           status = get_slot_number(argv, &writeResume.deviceLocation);

           if(status != FBE_STATUS_OK){
               fbe_cli_printf("resumeprom: ERROR: Bus number not valid.\n");
               return;
           }
       }

       /*Get device type*/
        else if(strcmp(*argv, "-d") == 0){
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("Please provide device type.\n");
                fbe_cli_printf("%s", GP_USAGE);
                return;
            }
            fbe_cli_get_device_type(*argv, &dev_type);
            if((dev_type == FBE_DEVICE_TYPE_INVALID) || (status != FBE_STATUS_OK))
            {
                fbe_cli_printf("%s", GP_USAGE);
                return;
            }
            writeResume.deviceType = dev_type;
        }

        /*Get field type*/
        else if(strcmp(*argv, "-f") == 0) {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("Please provide field type.\n");
                fbe_cli_printf("%s", GP_USAGE);
                return;
            }
            status = fbe_cli_get_field_type(*argv,  &field_type);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s", GP_USAGE);
                return;
            }
            writeResume.resumePromField = field_type;
        }


       /*Get offset*/
       else if(strcmp(*argv, "-o") == 0) {
           argc--;
            argv++;
            writeResume.offset = fbe_atoi(*argv);
        }

        /*Get buffer size*/
        else if(strcmp(*argv, "-bs") == 0) {
            argc--;
            argv++;
            writeResume.bufferSize= fbe_atoi(*argv);
        }

        argc--;
        argv++;
    }

    if(writeResume.resumePromField  != RESUME_PROM_CHECKSUM){
        if(writeResume.bufferSize == 0) 
        {
            fbe_cli_printf("-bs must be used to specify the number of bytes to write!\n");
            fbe_cli_printf("%s", GP_USAGE);
            return;
        }

        fbe_cli_printf("Please provide the data to be written\n");
        writeResume.pBuffer = (fbe_u8_t*)fbe_api_allocate_memory(writeResume.bufferSize);

        if(writeResume.pBuffer != NULL) 
        {
            status = fbe_cli_lib_resume_prom_get_data(&writeResume);
            if(status != FBE_STATUS_OK){
                fbe_cli_printf("ERROR: The data not provided or the data length exceeds the given buffer size.\n");
                fbe_cli_printf("Returned with status %s\n",get_trace_status(status));
                fbe_api_free_memory(writeResume.pBuffer);
                return;
            }
        }
        else
        {
            fbe_cli_printf("ERROR: Failed to allocate memory for the input data!\n");
            return;
        }
    }

    /* Write resume prom.*/
    status = fbe_api_esp_common_write_resume_prom(&writeResume);

    if (status == FBE_STATUS_NO_DEVICE)
    {
        fbe_cli_printf("ERROR: Invalid device type for the location provided.\n");
        fbe_cli_printf("Returned with status %s\n",get_trace_status(status));
    }
    else if(status == FBE_STATUS_COMPONENT_NOT_FOUND)
    {
        fbe_cli_printf("ERROR: ERROR in writing to the resume prom\n");
        fbe_cli_printf("Returned with status %s\n",get_trace_status(status));
        fbe_cli_printf("Please make sure the device resume prom is writable.\n");
    }
    else if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_printf("ERROR: ERROR in writing to the resume prom\n");
        fbe_cli_printf("Returned with status %s\n",get_trace_status(status));
    }
    else
    {
        /* Sleep 15 seconds for resume prom write complete.*/
        fbe_cli_printf("Please wait..\n");
        fbe_api_sleep(15000);
        fbe_cli_printf("The resume prom is written successfully!!\n");
    }

    if(writeResume.pBuffer != NULL) 
    {
        fbe_api_free_memory(writeResume.pBuffer);
    }

    return;
}

/*!********************************************************************
* @get_and_verify_bus_number( fbe_char_t**argv,
*                                            fbe_device_physical_location_t*  location_p )
*********************************************************************
*
*  @brief
*      get the bus number from command line and verify it.
*
*  @param
*      **argv - pointer to arguments
*      *location_p - pointer to the physical location structure.
*
*  @return:
*      status - FBE_STATUS_OK or one of the other status codes.
*
* @author
* 12-Aug 2011: created:  Shubhada Savdekar
*********************************************************************/
fbe_status_t get_and_verify_bus_number( fbe_char_t**argv,
    fbe_device_physical_location_t*  location_p )
{
    location_p->bus = (fbe_u8_t)fbe_atoi(*argv);
    
    if(location_p->bus >= FBE_API_PHYSICAL_BUS_COUNT){

        /*For XPE enclosures the count FBE_XPE_PSEUDO_ENCL_NUM is valid*/
        if( location_p->bus >=FBE_XPE_PSEUDO_BUS_NUM ){
            return FBE_STATUS_OK;
        }
        
        fbe_cli_printf("Entered Bus number (%d) exceeds the total Bus Count (%d)\n",
            location_p->bus,
            FBE_API_PHYSICAL_BUS_COUNT);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*!********************************************************************
* @get_and_verify_enclosure_number(fbe_char_t**argv,
*                                                      fbe_device_physical_location_t*  location_p )
*********************************************************************
*
*  @brief
*      get the enclosure number from command line and verify it.
*
*  @param
*      **argv - pointer to arguments
*      *location_p - pointer to the physical location structure.
*
*  @return
*      status - FBE_STATUS_OK or one of the other status codes.
*
* @author
* 12-Aug 2011: created:  Shubhada Savdekar

*********************************************************************/
fbe_status_t get_and_verify_enclosure_number(fbe_char_t**argv,
                                            fbe_device_physical_location_t*  location_p )
{
    fbe_status_t status;
    fbe_u32_t    enclCount;
    location_p->enclosure = (fbe_u8_t)fbe_atoi(*argv);

    //Get  the total enclosure count 
   status =fbe_api_esp_encl_mgmt_get_encl_count_on_bus(location_p,&enclCount);

    if(location_p->enclosure >= enclCount){

        /*For XPE enclosures the count FBE_XPE_PSEUDO_ENCL_NUM is valid*/
        if(location_p->enclosure == FBE_XPE_PSEUDO_ENCL_NUM){
                return FBE_STATUS_OK;
        }
        
        fbe_cli_printf("Enclosure number (%d) must be less than the "
            " enclosure count (%d) on the bus.\n",
            location_p->enclosure, 
            enclCount);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*!********************************************************************
* @get_slot_number(fbe_char_t**argv, fbe_device_physical_location_t*  location_p )
*********************************************************************
*
*  @brief
*      get the slot number from command line and verify it.
*
*  @param
*      **argv       - pointer to arguments
*      *location_p - pointer to the physical location structure.
*
*  @return
*      status - FBE_STATUS_OK or one of the other status codes.
* 
* @author
* * 12-Aug 2011: created:  Shubhada Savdekar
*********************************************************************/
fbe_status_t get_slot_number(fbe_char_t**argv, 
                                       fbe_device_physical_location_t*  location_p )
{
    location_p->slot = (fbe_u8_t)fbe_atoi(*argv);

    return FBE_STATUS_OK;
}

/*!********************************************************************
* @fbe_cli_get_device_type(fbe_char_t* argv, fbe_u64_t *dev_type)
*********************************************************************
*
* @brief
*      Get the Device types.
*
* @param
*      argv - input string
*      dev_type - device type
*
*  @return
*      status - FBE_STATUS_OK or one of the other status codes.
* 
* @Revision
* * 12-Aug 2011: created:  Shubhada Savdekar
* * 12-Jul 2012: modified: Feng Ling
*********************************************************************/

fbe_status_t fbe_cli_get_device_type(fbe_char_t* argv, fbe_u64_t  *dev_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    if(strcmp(argv, "midplane") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_ENCLOSURE;
    }
    else if(strcmp(argv, "lcc") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_LCC;
    }
    else if(strcmp(argv, "fan") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_FAN;
    }
    else if(strcmp(argv, "iom") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_IOMODULE;
    }
    else if(strcmp(argv, "ps") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_PS;
    }
    else if(strcmp(argv, "mezz") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_MEZZANINE;
    }
    else if(strcmp(argv, "mgmt") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_MGMT_MODULE;
    }
    else if(strcmp(argv, "sp") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_SP;
    }
    else if(strcmp(argv, "drivemidplane") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_DRIVE_MIDPLANE;
    }
    else if(strcmp(argv, "bbu") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_BATTERY;
    }
    else if(strcmp(argv, "bm") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_BACK_END_MODULE;
    }
    else if(strcmp(argv, "cachecard") == 0)
    {
        *dev_type = FBE_DEVICE_TYPE_CACHE_CARD;
    }
    else 
    {
        fbe_cli_printf("Invalid device type\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*!********************************************************************
* @fbe_cli_get_field_type(fbe_char_t* argv, RESUME_PROM_FIELD* field_type)
*********************************************************************
*
* @brief
*      Get the field types.
*
* @param
*      argv - input string
*      field_type - field type
*
*  @return
*      status - FBE_STATUS_OK or one of the other status codes.
* 
* @Revision
* * 12-Jul 2012: created: Feng Ling
*********************************************************************/

fbe_status_t fbe_cli_get_field_type(fbe_char_t* argv, RESUME_PROM_FIELD* field_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    if(strcmp(argv, "prodpn") == 0)
    {
         *field_type = RESUME_PROM_PRODUCT_PN;
    }
    else if(strcmp(argv, "prodsn") == 0)
    {
         *field_type = RESUME_PROM_PRODUCT_SN;
    }
    else if(strcmp(argv, "prodrev") == 0)
    {
         *field_type = RESUME_PROM_PRODUCT_REV;
    }
    else if(strcmp(argv, "chksum") == 0)
    {
         *field_type = RESUME_PROM_CHECKSUM;
    }
    else if(strcmp(argv, "wwn") == 0)
    {
         *field_type = RESUME_PROM_WWN_SEED;
    }
    else if(strcmp(argv, "emcsn") == 0)
    {
         *field_type = RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM;
    }
    else if(strcmp(argv, "emcpn") == 0)
    {
         *field_type = RESUME_PROM_EMC_SUB_ASSY_PART_NUM;
    }
    else 
    {
         fbe_cli_printf("Invalid field type\n");
         status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*!********************************************************************
* @fbe_cli_lib_resume_prom_get_data( fbe_resume_prom_cmd_t*  writeResume)
*********************************************************************
*
*  @brief
*      Get the Data from console.
*
*  @param
*   writeResume   resume write structure.
*
*  @return
*      FBE_STATUS_OK on success.
* 
* @author
* * 05-Sep 2011: created:  Shubhada Savdekar
*********************************************************************/
fbe_status_t     fbe_cli_lib_resume_prom_get_data(fbe_resume_prom_cmd_t*  writeResume){
                                                      
    fbe_status_t       status = FBE_STATUS_OK;

    if(writeResume->resumePromField == RESUME_PROM_WWN_SEED) {
            status = fbe_cli_get_number(writeResume->pBuffer);

    }
    else {
        status = fbe_cli_getline(writeResume->pBuffer, writeResume->bufferSize);
    }
    return status;
}

/*!********************************************************************
* @fbe_cli_getline(fbe_char_t* buffer, fbe_u32_t lim)
*********************************************************************
*
*  @brief
*      Get the line from console and write in the buffer.
*
*  @param
*      fbe_char_t* buffer - buffer to which data should be written
*      fbe_u32_t lim - limi of the data to be read.
*
*  @return
*      FBE_STATUS_OK on success.
* 
* @author
* * 16-Sep 2011: created:  Shubhada Savdekar
*********************************************************************/

fbe_status_t fbe_cli_getline(fbe_char_t* buffer, fbe_u32_t lim){
    int ch;
    fbe_u32_t index = 0;

    while(((ch = getchar()) != EOF) && (ch != '\n'))
    {
        if(index >= lim)
        {
            fbe_cli_printf("ERROR: Line too long!!!\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
           index++;
           *buffer++ = ch;
        }
    }

    return FBE_STATUS_OK;
}

/*!********************************************************************
* @fbe_cli_get_number(fbe_char_t* buffer)
*********************************************************************
*
*  @brief
*      Get the number from console and write in the buffer.
*
*  @param
*      fbe_char_t* buffer - buffer to which data should be written
*      fbe_u32_t lim - limi of the data to be read.
*
*  @return
*      FBE_STATUS_OK on success.
* 
* @author
* * 09-Dec 2011: created:  Shubhada Savdekar
*********************************************************************/

fbe_status_t fbe_cli_get_number(fbe_char_t* buffer){
    fbe_u8_t    line[MAX_BUFSIZE] ={0}, *ptr=line;

    fbe_u32_t number =0 ;

    fflush(stdin);
    fgets(line, MAX_BUFSIZE , stdin );

    line[strlen(line)-1] = '\0';

    /*skip 0x */
    if(*line == '0' && (*(line+1) == 'x' || *(line+1) == 'X'))
    {
        ptr+=2;
    }

    number = fbe_atoh(ptr);
    memcpy(buffer , &number, sizeof(fbe_u32_t));

    return FBE_STATUS_OK;
}

/*!********************************************************************
* @fbe_cli_get_unknown_value(const fbe_u8_t *name, fbe_u32_t value)
*********************************************************************
*
*  @brief
*      Return the unknown as value. Used in default case of get_trace_status.
*
*  @param
*      fbe_u8_t *name 
*      fbe_u32_t value
*
*  @return
*      const fbe_u8_t*
* 
* @author
* * 16-Sep 2011: created:  Shubhada Savdekar
*********************************************************************/
static const fbe_u8_t *fbe_cli_get_unknown_value(const fbe_u8_t *name, fbe_u32_t value)
{
    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:\tunknown %s %d(%#x)\n", __FUNCTION__, name, value, value);
    return "?";
}

/*!********************************************************************
* @get_trace_status(fbe_status_t status)
*********************************************************************
*
*  @brief
*      Return the status as a string.
*
*  @param
*      fbe_status_t status
*
*  @return
*      const char*
* 
* @author
* * 16-Sep 2011: created:  Shubhada Savdekar
*********************************************************************/
const char *get_trace_status(fbe_status_t status)
{
    switch (status) {
    CASENAME(FBE_STATUS_INVALID);
    CASENAME(FBE_STATUS_OK);
    CASENAME(FBE_STATUS_MORE_PROCESSING_REQUIRED);
    CASENAME(FBE_STATUS_PENDING);
    CASENAME(FBE_STATUS_CANCEL_PENDING);
    CASENAME(FBE_STATUS_CANCELED);
    CASENAME(FBE_STATUS_TIMEOUT);
    CASENAME(FBE_STATUS_INSUFFICIENT_RESOURCES);
    CASENAME(FBE_STATUS_NO_DEVICE);
    CASENAME(FBE_STATUS_NO_OBJECT);

    CASENAME(FBE_STATUS_CONTINUE);

    CASENAME(FBE_STATUS_BUSY);
    CASENAME(FBE_STATUS_SLUMBER);
    CASENAME(FBE_STATUS_FAILED);
    CASENAME(FBE_STATUS_DEAD);

    CASENAME(FBE_STATUS_NOT_INITIALIZED);
    CASENAME(FBE_STATUS_TRAVERSE);

    CASENAME(FBE_STATUS_EDGE_NOT_ENABLED);
    CASENAME(FBE_STATUS_COMPONENT_NOT_FOUND);
    CASENAME(FBE_STATUS_HIBERNATION_EXIT_PENDING);
    CASENAME(FBE_STATUS_GENERIC_FAILURE);

    DFLTNAME(fbe_status_t, status);
    }
}


const char* get_device_name_string(fbe_u32_t dev_id)
{
    switch(dev_id) {
        CASENAME(FBE_DEVICE_TYPE_INVALID);
        CASENAME(FBE_DEVICE_TYPE_ENCLOSURE);
        CASENAME(FBE_DEVICE_TYPE_LCC);
        CASENAME(FBE_DEVICE_TYPE_PS);
        CASENAME(FBE_DEVICE_TYPE_FAN);
        CASENAME(FBE_DEVICE_TYPE_SPS);
        CASENAME(FBE_DEVICE_TYPE_IOMODULE);
        CASENAME(FBE_DEVICE_TYPE_DRIVE);
        CASENAME(FBE_DEVICE_TYPE_MEZZANINE);
        CASENAME(FBE_DEVICE_TYPE_MGMT_MODULE);
        CASENAME(FBE_DEVICE_TYPE_SLAVE_PORT);
        CASENAME(FBE_DEVICE_TYPE_PLATFORM);
        CASENAME(FBE_DEVICE_TYPE_SUITCASE);
        CASENAME(FBE_DEVICE_TYPE_MISC);
        CASENAME(FBE_DEVICE_TYPE_BMC);
        CASENAME(FBE_DEVICE_TYPE_SFP);
        CASENAME(FBE_DEVICE_TYPE_SP);
        CASENAME(FBE_DEVICE_TYPE_TEMPERATURE);
        CASENAME(FBE_DEVICE_TYPE_PORT_LINK);
        CASENAME(FBE_DEVICE_TYPE_SP_ENVIRON_STATE);
        CASENAME(FBE_DEVICE_TYPE_DRIVE_MIDPLANE);
        CASENAME(FBE_DEVICE_TYPE_BATTERY);
        CASENAME(FBE_DEVICE_TYPE_BACK_END_MODULE);
        CASENAME(FBE_DEVICE_TYPE_CACHE_CARD);

        DFLTNAME(fbe_u32_t, dev_id);

    }
}

/*************************************************************************
 *                            @fn fbe_cli_cmd_force_rp_read ()                                                           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_force_rp_read performs the execution of the forcerpread command
 *    initiate resume prom read.
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   26-August-2011:  Harshal Wanjari - created
 *
 *************************************************************************/
void fbe_cli_cmd_force_rp_read(fbe_s32_t argc,char ** argv)
{
    /*****************
    **    BEGIN    **
    *****************/
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_device_physical_location_t deviceLocation= {0};
    fbe_u64_t              dev_type = FBE_DEVICE_TYPE_INVALID;

    if (argc == 0) {
        fbe_cli_printf("forcerpread: Too few arguments.\n");
        fbe_cli_printf("%s", FORCE_RP_READ_USAGE);
        return;
    }

    /*Extract the command*/
    while(argc > 0){
        /*Get bus number*/
        if(strcmp(*argv, "-b") == 0){
            argc--;
            argv++;
            status = get_and_verify_bus_number(argv, &deviceLocation);
            if(status != FBE_STATUS_OK){
                fbe_cli_error("forcerpread: Bus number not valid.\n");
                return;
            }
        }

        /*Get enclosure number*/
       else if(strcmp(*argv, "-e") == 0){
            argc--;
            argv++;
            status = get_and_verify_enclosure_number(argv, &deviceLocation);
            if(status != FBE_STATUS_OK){
                fbe_cli_error("forcerpread: Bus number not valid.\n");
                return;
            }
        }

        /*Get slot number*/
        else if(strcmp(*argv, "-s") == 0){
            argc--;
            argv++;
            status = get_slot_number(argv, &deviceLocation);
            if(status != FBE_STATUS_OK){
                fbe_cli_error("forcerpread: Bus number not valid.\n");
                return;
            }
        }

        /*Get device type*/
        else if(strcmp(*argv, "-d") == 0){
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf(" Please provide device type.\n");
                fbe_cli_printf("%s", FORCE_RP_READ_USAGE);
                return;
            }
            fbe_cli_get_device_type(*argv, &dev_type);
            if((dev_type == FBE_DEVICE_TYPE_INVALID) || (status != FBE_STATUS_OK))
            {
                fbe_cli_printf("%s", FORCE_RP_READ_USAGE);
                return;
            }
        }

        /*Get SP*/
        else if(strcmp(*argv, "-sp") == 0){
            argc--;
            argv++;
            deviceLocation.sp = fbe_atoi(*argv);
        }

        else if(strcmp(*argv, "-h") == 0){
            argc--;
            argv++;
            /* If they are asking for help, just display help and exit.*/
            fbe_cli_printf("%s", FORCE_RP_READ_USAGE);
            return;
        }
        else{
            fbe_cli_error("forcerpread : Too few arguments.\n");
            fbe_cli_printf("%s", FORCE_RP_READ_USAGE);
            return;
        }

        argc--;
        argv++;
    }
    fbe_cli_force_rp_read(dev_type,deviceLocation);

    return;
}
/************************************************************************
 **                       End of fbe_cli_cmd_force_rp_read ()                **
 ************************************************************************/

/*!**************************************************************
 * fbe_cli_force_rp_read()
 ****************************************************************
 * @brief
 *      This fuction will initiate resume prom read
 *      Called by FBE cli forcerpread command
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   26-August-2011:  Harshal Wanjari - created
 *
 ****************************************************************/
static void fbe_cli_force_rp_read(fbe_u64_t deviceType,
                                  fbe_device_physical_location_t deviceLocation)
{
    fbe_status_t                                status = FBE_STATUS_OK;

    /*initiate resume prom read*/
    status = fbe_api_esp_common_initiate_resume_prom_read(deviceType, 
                                                     &deviceLocation);

    if (status == FBE_STATUS_NO_DEVICE)
    {
        fbe_cli_printf("resumeprom: ERROR: Invalid device type for the location provided.\n");
        fbe_cli_printf("Returned with status %s\n",get_trace_status(status));
        return;
    }
    else if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_error ( "%s:Failed to initiate resume prom read for device type:%lld"
            ",Bus no: %d ,Enclosure no: %d, slot no:%d  status:%d\n", 
            __FUNCTION__,deviceType,deviceLocation.bus,
            deviceLocation.enclosure, deviceLocation.slot, status);
        return;
    }

    fbe_cli_printf("Resume PROM read initated successfully.\n");

    return;
}
/******************************************************
 * end fbe_cli_force_rp_read() 
 ******************************************************/
/*!**************************************************************
 * fbe_cli_lib_resume_prom_read_all()
 ****************************************************************
 * @brief
 *      This fuction will loop through all devices in the system
 *      and display their resume prom data.
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   09-October-2012:  bphilbin - created
 *
 ****************************************************************/
void fbe_cli_lib_resume_prom_read_all(fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo)
{
    fbe_esp_board_mgmt_board_info_t board_info;
    fbe_u32_t bus, enclosure, slot;
    fbe_esp_encl_mgmt_encl_info_t   encl_info;
    fbe_device_physical_location_t location;
    fbe_esp_module_limits_info_t limits_info = {0};
    fbe_esp_module_mgmt_get_mgmt_comp_info_t mgmt_comp_info = {0};
    fbe_u32_t bob_count;
    fbe_status_t status;
    SP_ID sp;
    fbe_u32_t cache_card_count=0;
    fbe_u32_t i=0;
    fbe_u32_t spCount=SP_ID_MAX;


    status = fbe_api_esp_board_mgmt_getBoardInfo(&board_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get board management info, error code: %d.\n", __FUNCTION__, status);
        return;
    }
    if(board_info.platform_info.enclosureType > MAX_ENCLOSURE_TYPE)
    {
        fbe_cli_printf("Illegal enclosure type: %d.\n", board_info.platform_info.enclosureType);
        return;
    }

    if (board_info.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }

    for(sp=SP_A;sp<spCount;sp++)
    {
        pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_SP;
        pGetResumePromInfo->deviceLocation.sp = sp;
        location.sp = sp;
        fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);

        if(board_info.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
        {
            pGetResumePromInfo->deviceLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
            pGetResumePromInfo->deviceLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    
            fbe_cli_lib_resume_prom_display_all_enclosure_resume(FBE_XPE_PSEUDO_BUS_NUM, FBE_XPE_PSEUDO_ENCL_NUM);
    
            pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_ENCLOSURE;
            fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);
        }

        status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                "%s:Failed to get limit info  status:%d\n", 
                __FUNCTION__, status);
            return;
        }

        for(slot=0;slot<limits_info.discovered_hw_limit.num_bem;slot++)
        {
            pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_BACK_END_MODULE;
            pGetResumePromInfo->deviceLocation.slot = slot;
            fbe_cli_lib_resume_prom_gather_and_display_iom_info(pGetResumePromInfo);
        }
        for(slot=0;slot<limits_info.discovered_hw_limit.num_mezzanine_slots;slot++)
        {
            pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_MEZZANINE;
            pGetResumePromInfo->deviceLocation.slot = slot;
            fbe_cli_lib_resume_prom_gather_and_display_iom_info(pGetResumePromInfo);
        }
        for(slot=0;slot<limits_info.discovered_hw_limit.num_slic_slots;slot++)
        {
            pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_IOMODULE;
            pGetResumePromInfo->deviceLocation.slot = slot;
            fbe_cli_lib_resume_prom_gather_and_display_iom_info(pGetResumePromInfo);
        }
        for(slot=0;slot<limits_info.discovered_hw_limit.num_mgmt_modules;slot++)
        {
            pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_MGMT_MODULE;
            pGetResumePromInfo->deviceLocation.slot = slot;
            mgmt_comp_info.phys_location.slot = slot;
            status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("Failed to get MGMT module info.\n");
                continue;
            }
            if(mgmt_comp_info.mgmt_module_comp_info.bInserted == FBE_TRUE)
            {
                fbe_cli_lib_resume_prom_gather_and_display_iom_info(pGetResumePromInfo);
            }
        }

        status = fbe_api_esp_sps_mgmt_getBobCount(&bob_count);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to get bbu count.\n");
        }
        for(slot=0;slot<bob_count;slot++)
        {
            pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_BATTERY;
            pGetResumePromInfo->deviceLocation.slot = slot;
            pGetResumePromInfo->deviceLocation.sp = sp;
            fbe_cli_lib_resume_prom_gather_and_display_iom_info(pGetResumePromInfo);
        }

        status = fbe_api_esp_board_mgmt_getCacheCardCount(&cache_card_count);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to get Cache Card count, status:%d\n", status);
        }
        else
        {
            for (i=0;i<cache_card_count;i++)
            {
                pGetResumePromInfo->deviceLocation.sp = SP_A;
                pGetResumePromInfo->deviceLocation.slot = i;
                pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_CACHE_CARD;

                fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);
            }
        }
 
        for (bus = 0; bus < FBE_PHYSICAL_BUS_COUNT; bus ++)
        {
            location.bus = bus;
            for(enclosure = 0; enclosure < FBE_ENCLOSURES_PER_BUS; enclosure++)
            {
                location.enclosure = enclosure;
                status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Get enclosure info fail, location: sp %d, bus %d, enclosure %d.\n", __FUNCTION__, location.sp, location.bus, location.enclosure);
                    return;
                }
                if(encl_info.enclPresent)
                {
                    fbe_cli_lib_resume_prom_display_all_enclosure_resume(bus, enclosure);
                }
            }
        }
    }
    return;
}


/*!**************************************************************
 * fbe_cli_lib_resume_prom_gather_and_display_info()
 ****************************************************************
 * @brief
 *      This fuction will request the resume prom data for the
 *      specified device and display it.
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   09-October-2012:  bphilbin - created
 *
 ****************************************************************/
fbe_status_t fbe_cli_lib_resume_prom_gather_and_display_info(fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo)
{
    fbe_status_t status;
    //fbe_esp_board_mgmt_board_info_t boardInfo;
    fbe_u64_t               deviceType = FBE_DEVICE_TYPE_INVALID;

    deviceType = pGetResumePromInfo->deviceType;

    switch(pGetResumePromInfo->deviceType)
    {
        case FBE_DEVICE_TYPE_SP:
            fbe_cli_printf("%s Resume Prom:\n",
                pGetResumePromInfo->deviceLocation.sp == SP_A ? "SPA" : "SPB");
            break;
        case FBE_DEVICE_TYPE_IOMODULE:
        case FBE_DEVICE_TYPE_BACK_END_MODULE:
        case FBE_DEVICE_TYPE_MEZZANINE:
        case FBE_DEVICE_TYPE_MGMT_MODULE:
        case FBE_DEVICE_TYPE_BATTERY:
        case FBE_DEVICE_TYPE_CACHE_CARD:
            fbe_cli_printf("%s %s %d Resume Prom:\n",
                pGetResumePromInfo->deviceLocation.sp == SP_A ? "SPA" : "SPB",
                fbe_base_env_decode_device_type(deviceType),
                pGetResumePromInfo->deviceLocation.slot);
            break;

        case FBE_DEVICE_TYPE_ENCLOSURE:
        case FBE_DEVICE_TYPE_PS:
        case FBE_DEVICE_TYPE_LCC:
        case FBE_DEVICE_TYPE_FAN:
        case FBE_DEVICE_TYPE_DRIVE_MIDPLANE:
            if(pGetResumePromInfo->deviceLocation.bus == FBE_XPE_PSEUDO_BUS_NUM &&
               pGetResumePromInfo->deviceLocation.enclosure ==  FBE_XPE_PSEUDO_ENCL_NUM)
            {
                if(deviceType == FBE_DEVICE_TYPE_ENCLOSURE)
                {
                    fbe_cli_printf("XPE Midplane Resume Prom:\n");
                }
                else
                {
                   fbe_cli_printf("XPE %s %d Resume Prom:\n",
                           fbe_base_env_decode_device_type(deviceType),
                           pGetResumePromInfo->deviceLocation.slot);
                }
            }
            else
            {
                if(deviceType == FBE_DEVICE_TYPE_ENCLOSURE)
                {
                    fbe_cli_printf("Bus %d Enclosure %d %s Resume Prom:\n",
                        pGetResumePromInfo->deviceLocation.bus,
                        pGetResumePromInfo->deviceLocation.enclosure,
                        "Midplane");
                }
                else
                {
                    fbe_cli_printf("Bus %d Enclosure %d %s %d Resume Prom:\n",
                        pGetResumePromInfo->deviceLocation.bus,
                        pGetResumePromInfo->deviceLocation.enclosure,
                        fbe_base_env_decode_device_type(deviceType),
                        pGetResumePromInfo->deviceLocation.slot);
                }
            }
            break;

        default:
            fbe_cli_printf("Unknown Device!!! (sp %d, bus %d, encl %d device type %lld, slot %d)\n",
                        pGetResumePromInfo->deviceLocation.sp,
                        pGetResumePromInfo->deviceLocation.bus,
                        pGetResumePromInfo->deviceLocation.enclosure,
                        deviceType,
                        pGetResumePromInfo->deviceLocation.slot);

            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* Get the device Resume Info */
   status = fbe_api_esp_common_get_resume_prom_info(pGetResumePromInfo);
   if(status == FBE_STATUS_COMPONENT_NOT_FOUND)
   {
       fbe_cli_printf("The device does not have Resume Prom.\n");
       return status;
   }
   else if(status != FBE_STATUS_OK)
   {
       fbe_cli_printf("ERROR: %s.\n", get_trace_status(status));
       return status;
   }

   if(pGetResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_READ_SUCCESS)
   {
       fbe_cli_lib_resume_prom_display_info(&pGetResumePromInfo->resume_prom_data);
       return FBE_STATUS_OK;
   }
   else
   {
       fbe_cli_printf("ERROR: resume read op status %s.\n",
                      fbe_base_env_decode_resume_prom_status(pGetResumePromInfo->op_status));
       return FBE_STATUS_GENERIC_FAILURE;
   }

}

/*!**************************************************************
 * fbe_cli_lib_resume_prom_gather_and_display_iom_info()
 ****************************************************************
 * @brief
 *      This fuction will detect if the specified io module is
 *      inserted and request the resume prom data if it is.
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   09-October-2012:  bphilbin - created
 *
 ****************************************************************/
void fbe_cli_lib_resume_prom_gather_and_display_iom_info(fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo)
{
    fbe_esp_module_io_module_info_t iomodule_info = {0};
    fbe_status_t status;
    iomodule_info.header.sp = pGetResumePromInfo->deviceLocation.sp;
    iomodule_info.header.type = pGetResumePromInfo->deviceType;
    iomodule_info.header.slot = pGetResumePromInfo->deviceLocation.slot;

    status = fbe_api_esp_module_mgmt_getIOModuleInfo(&iomodule_info);
    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        return;
    }
    if(iomodule_info.io_module_phy_info.inserted == FBE_TRUE)
    {
        fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);
    }
}


/*!**************************************************************
 * fbe_cli_lib_resume_prom_display_all_enclosure_resume()
 ****************************************************************
 * @brief
 *      This fuction gathers counts of the various devices in an
 *      enclosure and requests to display all the device resume proms.
 *
 *  @param :
 *      fbe_u32_t bus - bus number.
 *      fbe_u32_t encl - enclosure number.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   09-October-2012:  bphilbin - created
 *
 ****************************************************************/
void fbe_cli_lib_resume_prom_display_all_enclosure_resume(fbe_u32_t bus, fbe_u32_t encl)
{
    fbe_device_physical_location_t location;
    fbe_esp_encl_mgmt_encl_info_t  encl_info;
    fbe_u32_t slot;
    fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo;
    fbe_status_t status;

    pGetResumePromInfo = (fbe_esp_base_env_get_resume_prom_info_cmd_t *) fbe_api_allocate_memory(sizeof (fbe_esp_base_env_get_resume_prom_info_cmd_t));
    if(pGetResumePromInfo == NULL)
    {
        fbe_cli_printf("resumeprom: ERROR: Memory allocation failed.\n");
        return;
    }

    location.bus = bus;
    location.enclosure = encl;
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get enclosure info, error code: %d.\n\n", __FUNCTION__, status);
    }
    
    pGetResumePromInfo->deviceLocation.bus = bus;
    pGetResumePromInfo->deviceLocation.enclosure = encl;
    pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_ENCLOSURE;
    fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);

    for(slot=0; slot<encl_info.fanCount; slot++)
    {
        pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_FAN;
        pGetResumePromInfo->deviceLocation.slot = slot;
        fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);
    }

    for(slot=0; slot<encl_info.psCount;slot++)
    {
        pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_PS;
        pGetResumePromInfo->deviceLocation.slot = slot;
        fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);
    }

    for(slot=0; slot<encl_info.lccCount;slot++)
    {
        pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_LCC;
        pGetResumePromInfo->deviceLocation.slot = slot;
        fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);
    }

    for(slot=0; slot<encl_info.driveMidplaneCount;slot++)
    {
        pGetResumePromInfo->deviceType = FBE_DEVICE_TYPE_DRIVE_MIDPLANE;
        pGetResumePromInfo->deviceLocation.slot = slot;
        fbe_cli_lib_resume_prom_gather_and_display_info(pGetResumePromInfo);
    }

    fbe_api_free_memory(pGetResumePromInfo);
    return;
}

/*************************
 * end file fbe_cli_lib_resum_prom_cmds.c
 *************************/
