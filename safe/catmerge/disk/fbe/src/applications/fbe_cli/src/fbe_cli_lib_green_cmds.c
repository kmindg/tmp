/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_cli_lib_green_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contain fbe_cli functions used for green command.
 *
 * @version
 *  11-Apr-2011: Created  Shubhada Savdekar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_green_cmds.h"
#include "fbe_cli_private.h"
#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"

/****************************** 
 *    LOCAL FUNCTIONS
 ******************************/
static fbe_status_t fbe_cli_lib_green_set_rg_state    (int argc, char** argv, fbe_system_power_saving_info_t  *power_save_info );
static fbe_status_t fbe_cli_lib_green_set_state    (int argc, char** argv,fbe_system_power_saving_info_t  power_save_info);
static fbe_status_t fbe_cli_lib_green_set_wakeup_time    ( fbe_u64_t    wakeup_time );
static fbe_status_t fbe_cli_lib_green_set_drive_state    (int argc, char** argv, fbe_system_power_saving_info_t  power_save_info); 
static fbe_status_t fbe_cli_lib_green_list    (int argc, char **argv);
static fbe_status_t fbe_cli_lib_get_all_green_drives    ( void);
static fbe_status_t fbe_cli_get_bus_enclosure_slot_info_by_object_id    (fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id);
static fbe_status_t fbe_cli_green_print_list    (fbe_u32_t  bus,fbe_u32_t  enclosure, fbe_u32_t  slot);

static  fbe_bool_t header_print_done  =  FBE_FALSE;

 /*!***************************************************************
 * fbe_cli_green(int argc, char** argv)
 ****************************************************************
 * @brief
 *  This function is used to perform green operations on the system and disks.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @author
 *  2/10/2011: Created:  Musaddique Ahmed
 *  4/13/2011: modified:  Shubhada Savdekar
 *                    broke down the functions and redesigned.
 *
 ****************************************************************/
void fbe_cli_green(int argc, char** argv)
{

    fbe_status_t                    status;
    fbe_system_power_saving_info_t  power_save_info;
    fbe_s64_t                       wakeup_time = ZERO;
    header_print_done  =  FBE_FALSE;


    if (argc == 0) {
        fbe_cli_printf("%s", GREEN_USAGE);
        return;
    }
    status = fbe_api_get_system_power_save_info(&power_save_info);

    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: failed to get system info\n", __FUNCTION__);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)) {
        
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", GREEN_USAGE);
        return;
    }// end of -help

    if ((strcmp(*argv, "-state") == 0)){
        status= fbe_cli_lib_green_set_state(argc, argv, power_save_info);

        if(status != FBE_STATUS_OK) {
            fbe_cli_error("Invalid system  operation !!\n");
            return;
        }

        return;
    }//end of -state operation

    if(strcmp(*argv, "-rg") == 0) {
        status=fbe_cli_lib_green_set_rg_state(argc, argv,&power_save_info );

        if(status != FBE_STATUS_OK){
            fbe_cli_error("Invalid  RG operation !!\n");
            return;
        }

        return;
    }//end of if -rg operation

    if ((strcmp(*argv, "-wakeup_time") == 0)) {

        argc--;
        argv++;
        if (argc <= 0) {
            fbe_cli_printf("Time in minutes is expected\n\n");
            fbe_cli_printf("%s", GREEN_USAGE);
            return;
        }

        wakeup_time = atoi(*argv);

        if(wakeup_time <= 0) {
             fbe_cli_printf("Invalid Time in minutes entered.\n");
             return ;
        }
         
        status= fbe_cli_lib_green_set_wakeup_time(wakeup_time);
        if(status != FBE_STATUS_OK) {
            fbe_cli_printf("Invalid wakeup operation !!\n");
            return;
        }
         
        return;
    }//end of -wakeup_time operation
    
    if ((strcmp(*argv, "-disk") == 0)) {
        status= fbe_cli_lib_green_set_drive_state(argc, argv, power_save_info);

        if(status != FBE_STATUS_OK) {
            fbe_cli_printf("Invalid disk operation !!\n");
            fbe_cli_printf("%s", GREEN_USAGE);
            return;
        }
        return;
   } //end of -disk operation

    if ((strcmp(*argv, "-list") == 0)) {
        status = fbe_cli_lib_green_list(argc, argv);

        if (status != FBE_STATUS_OK){
        fbe_cli_printf("%s", GREEN_USAGE);
        }
        return;
    }//end of -list operation

    fbe_cli_printf("\nInvalid operation!!\n\n");
    fbe_cli_printf("%s", GREEN_USAGE);
    return;

}

/*!*************************************************************
 * fbe_cli_lib_green_list(int argc, char **argv)
 ***************************************************************
 * @brief
 *  This function is used to get the list of all the drives, and their power saving 
 *  status.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 * 
 * @author
 *    4/13/2011: Created: Shubhada Savdekar
 *                    
 ****************************************************************/

static fbe_status_t fbe_cli_lib_green_list(int argc, char **argv) {
    fbe_u32_t                       bus = FBE_PORT_NUMBER_INVALID;
    fbe_u32_t                       enclosure = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_u32_t                       slot = FBE_SLOT_NUMBER_INVALID;
    fbe_status_t status=FBE_STATUS_OK;

    argc--;
    argv++;
       
    while (argc > 0) {
    if ((strcmp(*argv, "-b") == 0)) {
        argc--;
        argv++;

        if (argc <= 0) {
            fbe_cli_printf("Invalid Input !!\n");
            fbe_cli_printf("%s", GREEN_USAGE);
            return status;
        }

        bus = atoi(*argv);

        argc--;
        argv++;
    }
    else if ((strcmp(*argv, "-e") == 0)) {
        argc--;
        argv++;

        if (argc <= 0) {
            fbe_cli_printf("Invalid Input !!\n");
            fbe_cli_printf("%s", GREEN_USAGE);
            return status;
        }
        enclosure = atoi(*argv);

        argc--;
        argv++;
    }
    else if ((strcmp(*argv, "-s") == 0)) {
        argc--;
        argv++;

        if (argc <= 0) {
            fbe_cli_printf(" Invalid Input !!\n");
            fbe_cli_printf("%s", GREEN_USAGE);
            return status;
        }
        slot = atoi(*argv);

        argc--;
        argv++;
    }
            
    }/*while (argc > 0)*/
  
    /* get power saving state of single drive*/
    if( bus != FBE_PORT_NUMBER_INVALID || 
        enclosure != FBE_ENCLOSURE_NUMBER_INVALID || 
        slot != FBE_SLOT_NUMBER_INVALID ) {
        
        status= fbe_cli_green_print_list(bus, enclosure, slot);

        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to get  power save state of green drives\n", __FUNCTION__);
            return  status;

        }
    }

     /* get power saving state of all the drives*/
     else {
        status=fbe_cli_lib_get_all_green_drives( );
        if (status!=FBE_STATUS_OK){
            fbe_cli_error( "%s: failed to get  power save state of green drives\n", __FUNCTION__);
            return  status;
        }
     }

    return status;
}// end of fbe_cli_lib_green_list()

 /*!***************************************************************
 * fbe_cli_lib_get_all_green_drives(void)
 ****************************************************************
 * @brief
 *    This function is used to retrieve the power saving state of all drives. 
 *    The drives are of each type, i.e. SAS, SATA etc..
 *    We retrieve the power saving status of each drive and print it on the screen. *
 * 
 * @author
 *    4/13/2011: Created: Shubhada Savdekar
 *                    
 ****************************************************************/

static fbe_status_t fbe_cli_lib_get_all_green_drives( void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_u32_t context = 0;

    /* Port, enclosure and slot are not used but necessary to call
     * fbe_cli_execute_command_for_objects(), defaults to all port,
     * enclosure and slot.
     */
    fbe_u32_t port = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t slot = FBE_API_ENCLOSURE_SLOTS;

    /*    Execute the command against all base physical drives    */
    status = fbe_cli_execute_command_for_objects(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE, 
                                                 package_id, 
                                                 port,
                                                 enclosure,
                                                 slot,
                                                 fbe_cli_get_bus_enclosure_slot_info_by_object_id, 
                                                 context);

    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
        return status;
    }

fbe_cli_printf("\n--------------------------------------------------\n");

    /* Execute the command against all sas physical drives.        */
    status = fbe_cli_execute_command_for_objects(FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,    
                                                 package_id,  
                                                 port,
                                                 enclosure,
                                                 slot,
                                                 fbe_cli_get_bus_enclosure_slot_info_by_object_id,  
                                                 context);

    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
        return status;
    }
    
    fbe_cli_printf("\n--------------------------------------------------\n");

    /* Execute the command against all sata physical drives.       */
    status = fbe_cli_execute_command_for_objects(FBE_CLASS_ID_SATA_PHYSICAL_DRIVE, 
                                                 package_id,  
                                                 port,
                                                 enclosure,
                                                 slot,
                                                 fbe_cli_get_bus_enclosure_slot_info_by_object_id,  
                                                 context );
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
        return status;
    }
    
    fbe_cli_printf("\n--------------------------------------------------\n");
    
    return status; 
}/*fbe_cli_lib_get_all_green_drives*/


 /*!***********************************************************************************************
 * fbe_cli_get_bus_enclosure_slot_info_by_object_id(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id)
 **************************************************************************************************
 * @brief
 *  This function is used to get the bus, enclosure and slot information for the particular drive.
 *
 *  @param    object_id 
 *  @param    context  
 *  @param    package_id 
 *
 * @author 
 *  4/13/2011: Created: Shubhada Savdekar
 *                  
 *************************************************************************************************/

static fbe_status_t fbe_cli_get_bus_enclosure_slot_info_by_object_id(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p = NULL;
    fbe_job_service_bes_t phy_disk = {FBE_PORT_NUMBER_INVALID, FBE_ENCLOSURE_NUMBER_INVALID, FBE_SLOT_NUMBER_INVALID};

    /*  Get the class id information for the input object.  */
    status = fbe_cli_get_class_info(object_id,package_id,&class_info_p);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Can't get object class information for id [0x%X],package id:0x%X status: 0x%X\n",object_id,package_id ,status);
        return status;
    }
    
    /*   Get the bus, enclosure, slot information for this object.   */
    status = fbe_cli_get_bus_enclosure_slot_info(object_id,  class_info_p->class_id,  &phy_disk.bus,  &phy_disk.enclosure,  &phy_disk.slot,  package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Can't get bus, enclosure, slot information for object id [0x%X], class id [0x%X], package id:0x%x status:0x%X\n",object_id,class_info_p->class_id,package_id, status);
        return status;
    }

    status = fbe_cli_green_print_list(phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
    if(status != FBE_STATUS_OK) {
        fbe_cli_printf("%s", GREEN_USAGE);
    }

    return status;

}


 /*!***************************************************************
 * fbe_cli_green_print_list(fbe_u32_t  bus,fbe_u32_t  enclosure, fbe_u32_t  slot)
 ****************************************************************
 * @brief
 *  This function is used to print the state of each drive on the console.
 *
 *  @param    bus 
 *  @param    enclosure 
 *  @param    slot  
 *  
 * @author
 *  4/13/2011: Created  Shubhada Savdekar
 *
 ****************************************************************/

static fbe_status_t fbe_cli_green_print_list (fbe_u32_t  bus,fbe_u32_t  enclosure, fbe_u32_t  slot) 
{
    fbe_u32_t drive_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_base_config_object_power_save_info_t object_power_save_info;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t provision_drive_info;

   /*  Get the object of a PVD in the desired location   */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot,&drive_obj_id);

    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: failed to get PVD ID\n", __FUNCTION__);
        return status;
    }
     
     /*  Get the power saving related information about the object  */
    status = fbe_api_get_object_power_save_info(drive_obj_id,&object_power_save_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("\nFailed status 0x%x to Power Save info for object_id: 0x%x\n", status, drive_obj_id);
        return status;
    }
    
    /*  Print state of the drive   */
    if (header_print_done == FBE_FALSE) {
        fbe_cli_printf("\n--------------------------------------------------------------------------------------\n");
        fbe_cli_printf("Bus  |  Enc |  Slot   |        State       |  Power Save Enabled  | Power Save Capable");
        fbe_cli_printf("\n--------------------------------------------------------------------------------------\n"); 
        header_print_done= FBE_TRUE;
    }

    fbe_cli_printf("%d\t%d\t%d\t",bus,enclosure,slot);

    switch(object_power_save_info.power_save_state) {

        case FBE_POWER_SAVE_STATE_NOT_SAVING_POWER:
            fbe_cli_printf ("%s\t","Not  saving  power");
            break;

        case FBE_POWER_SAVE_STATE_SAVING_POWER:
            fbe_cli_printf("%s\t", "Saving power");
            break;

        case FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE:
            fbe_cli_printf("%s\t","Entering power save mode");
            break;

        case FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE:
            fbe_cli_printf("%s\t","Exiting power save mode");
            break;

        case FBE_POWER_SAVE_STATE_INVALID:
        default:
            fbe_cli_printf("%s \t", "invalid");

    }

    status = fbe_api_provision_drive_get_info(drive_obj_id, &provision_drive_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: failed to get PVD info for object 0x%x.\n", __FUNCTION__, drive_obj_id);
        return status;
    }
    fbe_cli_printf("%s \t\t\t", (object_power_save_info.power_saving_enabled)?"Yes":"No");
    fbe_cli_printf("%s \n", (provision_drive_info.power_save_capable)?"Yes":"No");

    return status;
}


 /*!*************************************************************
 * fbe_cli_lib_green_set_wakeup_time(fbe_u64_t    wakeup_time )
****************************************************************
 * @brief
 *  This function is used to set the wakeup time for the system.
 *
 *  @param    
 *  @param    
 *
 * @author
 *  4/13/2011: Created: Shubhada Savdekar
 *                  
 ****************************************************************/
static fbe_status_t fbe_cli_lib_green_set_wakeup_time(fbe_u64_t    wakeup_time )
{
  
    fbe_status_t  status = FBE_STATUS_OK;
    status = fbe_api_set_power_save_periodic_wakeup(wakeup_time);

    if (status != FBE_STATUS_OK) {
      fbe_cli_error( "%s: failed to set wakeup time\n", __FUNCTION__);
      return status;
    }
    
    fbe_cli_printf("The wakeup time set to %llu minutes.\n",
		   (unsigned long long)wakeup_time);
    return status;
}


 /*!********************************************************************************
 * fbe_cli_lib_green_set_state(int argc, char** argv,fbe_system_power_saving_info_t  power_save_info)
***********************************************************************************
 * @brief
 *  This function is used to set the power save state of the system to enable OR disable.
 *
 *  @param    argc :argument count
 *  @param    argv :argument string
 *  @param    power_save_info : power save related information in the system
 *
 * @author
 *  4/13/2011: Created: Shubhada Savdekar
 *                  
 **********************************************************************************/
 
static fbe_status_t fbe_cli_lib_green_set_state(int argc, char** argv,fbe_system_power_saving_info_t  power_save_info)
{
    fbe_status_t   status = FBE_STATUS_OK;

    argv++;
    argc--;
        
    if(argc == 0 ) {
       fbe_cli_printf("%s", GREEN_USAGE);
       return status;
    }
        
    if(strcmp(*argv, "enable") == 0) {
        if(power_save_info.enabled == FBE_TRUE) {
            fbe_cli_printf("System Power Save is already enabled.\n");
            return status;
        }

        status  = fbe_api_enable_system_power_save();
        
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to enable Power Save\n", __FUNCTION__);
            return status;
        }
        fbe_cli_printf("System Power Save is enabled successfully! \n");  
    }
    
    else if(strcmp(*argv, "disable") == 0) {
        if(power_save_info.enabled == FBE_FALSE) {
            fbe_cli_printf("System Power Save is already disabled.\n");
            return status;
        }

        status  = fbe_api_disable_system_power_save();
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to disable Power Save\n", __FUNCTION__);
            return status;
        }
        fbe_cli_printf("System Power Save is disabled successfully!\n");
    }
    else if(strcmp(*argv, "state") == 0) {
        fbe_cli_printf("\nSystem Power Save is %s.\n", (power_save_info.enabled)?"Enabled":"Disabled");
        fbe_cli_printf("Wake up time in minutes is %d.\n\n", (int)power_save_info.hibernation_wake_up_time_in_minutes);
    }
    
    else {
        fbe_cli_printf("%s", GREEN_USAGE);
        return status;
    }
    return status;
} //end of fbe_cli_lib_green_set_state()


 /*!*************************************************************************************
 * fbe_cli_lib_green_set_drive_state(int argc, char** argv, fbe_system_power_saving_info_t  power_save_info)
****************************************************************************************
 * @brief
 *  This function is used to set the power save state of the drive to enable OR disable
 *
 *  @param    argc :argument count
 *  @param    argv :argument string
 *  @param    power_save_info : power save related information in the system
 *
 * @author
 *  4/13/2011: Created: Shubhada Savdekar
 *                  
 ***************************************************************************************/
static fbe_status_t fbe_cli_lib_green_set_drive_state(int argc, char** argv, fbe_system_power_saving_info_t  power_save_info)
{

    fbe_u32_t                       bus = FBE_PORT_NUMBER_INVALID;
    fbe_u32_t                       enclosure = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_u32_t                       slot = FBE_SLOT_NUMBER_INVALID;
    fbe_s64_t                       idle_time = ZERO;
    TEXT                           change_state [10] = " ";
    fbe_base_config_object_power_save_info_t  object_power_save_info; 
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t   status = FBE_STATUS_OK;
        
    argc--;
    argv++;

    if(argc == 0) {
        fbe_cli_printf("%s", GREEN_USAGE);
        return status;
    }
    
    while (argc > 0) 
    {
        if ((strcmp(*argv, "-state") == 0)) {

            argc--;
            argv++;
  
            if(argc ==0 ) {
                fbe_cli_printf("%s", GREEN_USAGE);
                return status;
            }

            if(strcmp(*argv, "enable")==0) {
                strcpy(change_state ,"enable");    
            }
            
            else if (strcmp(*argv, "disable")==0) {
                strcpy(change_state ,"disable");
            }

            else {
                fbe_cli_printf("Invalid power save state !!\n");
                fbe_cli_printf("%s", GREEN_USAGE);
                return status;
            }

            argc--;
            argv++;
       }

        else if ((strcmp(*argv, "-b") == 0)) {
            argc--;
            argv++;

            if (argc <= 0) {
                fbe_cli_printf("Bus number is expected\n");
                fbe_cli_printf("%s", GREEN_USAGE);
                return status;
            }
            bus = atoi(*argv);

            argc--;
            argv++;
        }

        else if ((strcmp(*argv, "-e") == 0)) {
            argc--;
            argv++;

            if (argc <= 0) {
                fbe_cli_printf("Enclosure number  is expected\n");
                return status;
            }
            enclosure = atoi(*argv);

            argc--;
            argv++;
        }

        else if ((strcmp(*argv, "-s") == 0)) {
            argc--;
            argv++;

            if (argc <= 0) {
                fbe_cli_printf("Slot number  is expected\n");
                return status;
            }
            slot = atoi(*argv);

            argc--;
            argv++;
        }
        
        else if ((strcmp(*argv, "-idle_time") == 0)) {
            argc--;
            argv++;

            if (argc <= 0) {
                fbe_cli_printf("Idle time in second(s) is expected\n\n");
                fbe_cli_printf("%s", GREEN_USAGE);
                return status;
            }
            idle_time = atoi(*argv);

            argc--;
            argv++;
        }
        
    }/*while (argc > 0)*/
  
    if( bus == FBE_PORT_NUMBER_INVALID || 
        enclosure == FBE_ENCLOSURE_NUMBER_INVALID || 
        slot == FBE_SLOT_NUMBER_INVALID ) {
        
        fbe_cli_printf("Disk bus, enclosure, slot is expected.\n\n");
        fbe_cli_printf("%s", GREEN_USAGE);
        return status;
    }

    if(power_save_info.enabled == FBE_FALSE) {
        fbe_cli_printf("System Power Save is disabled. Enable System Power Save first\n");
        return status;
    }
       
    /* get PVD Id from the b_e_s */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot, &pvd_object_id);
    
    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: failed to get PVD ID\n", __FUNCTION__);
        return status;
    }
        
    if(strcmp(change_state, "enable") == 0) {
        status = fbe_api_enable_object_power_save(pvd_object_id);
        if (status != FBE_STATUS_OK){
            fbe_cli_error( "%s: failed to enable Power Save for the disk\n", __FUNCTION__);
            return status;
        }
        fbe_cli_printf("Power Save enabled command at bus %d, enclosure %d, and slot %d successfully.\n", bus,enclosure,slot);
        fbe_cli_printf("Note: The physical drive may be not spin down qualified, so the drive may not start power save even though power save is enabled.\n");
    }

    if(strcmp(change_state, "disable") == 0) {
        status = fbe_api_disable_object_power_save(pvd_object_id);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to disable Power Save for the disk\n", __FUNCTION__);
            return status;
        }
        fbe_cli_printf("Power Save disabled at bus %d, enclosure %d, slot %d successfully.\n", bus,enclosure,slot);
    }
        
    status = fbe_api_get_object_power_save_info(pvd_object_id, &object_power_save_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: failed to get Power Save info\n", __FUNCTION__);
        return status;
    }
        
    /* Set idle_time only if Power save on that disk is already enabled. */
    if (idle_time != ZERO && 
        object_power_save_info.power_saving_enabled == FBE_TRUE) {

        status = fbe_api_set_object_power_save_idle_time(pvd_object_id,idle_time);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to set Power Save idle time for the disk\n", __FUNCTION__);
            return status;
        }
    }
        
    else if(idle_time != ZERO && 
               object_power_save_info.power_saving_enabled == FBE_FALSE) {
        fbe_cli_printf("Please enable objects Power Save first.\n");
    }
        
    /* No action intended, Just display the PS status for this disk */
    else if (idle_time == ZERO && (strcmp(change_state," ") == 0)) {
       if (object_power_save_info.power_saving_enabled == FBE_TRUE) {
            fbe_cli_printf("\n\nDisk Power Save is enabled\n\n");
            fbe_cli_printf("Disk Power Save  idle time: %llu sec\n\n",
			   (unsigned long long)object_power_save_info.configured_idle_time_in_seconds);
        }
        else if (object_power_save_info.power_saving_enabled == FBE_FALSE) {
            fbe_cli_printf("\n\nDisk Power Save is disabled\n\n");
        }

    }
    return status;
}//end of fbe_cli_lib_green_set_drive_state()


 /*!*************************************************************************************
 * fbe_cli_lib_green_set_rg_state(int argc, char** argv, fbe_system_power_saving_info_t  *power_save_info )
****************************************************************************************
 * @brief
 *  This function is used to set the power saving state of the RG to enable OR disable.
 *
 *  @param    argc :argument count
 *  @param    argv :argument string
 *  @param    power_save_info : power save related information in the system
 *
 * @author
 *  4/13/2011: Created: Shubhada Savdekar
 *  7/25/2012: Modified Vera Wang
 *                  
 ***************************************************************************************/

static fbe_status_t fbe_cli_lib_green_set_rg_state(int argc, char** argv, fbe_system_power_saving_info_t  *power_save_info )
{
    fbe_raid_group_number_t         rg_id;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;   
    fbe_s64_t                       idle_time = ZERO;
    fbe_raid_group_get_power_saving_info_t rg_power_save_info;

    argv++;
    argc--;

    if(argc == 0) {

        fbe_cli_printf("%s", GREEN_USAGE);
        return status;
    }

    rg_id = atoi(*argv);

    if(rg_id < 0) {
        fbe_cli_printf("Enter valid RG ID\n");
        return status;
    }

    status = fbe_api_database_lookup_raid_group_by_number(rg_id, &rg_object_id);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: failed to get raid group object id for rg:%d\n", __FUNCTION__,rg_id);
        return status;
    }

    argv++;
    argc--;
    /* no argument for this raid group, then we only display the power saving info. */
    if(argc == 0) {
        status = fbe_api_raid_group_get_power_saving_policy(rg_object_id, &rg_power_save_info);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to get Power Save info for raid group 0x%x. \n", __FUNCTION__, rg_object_id);
            return status;
        }
        fbe_cli_printf("\nRaid group 0x%x Power save info: \n", rg_object_id);
        fbe_cli_printf("\tPower saving is: %s \n",(rg_power_save_info.power_saving_enabled)?"Enabled":"Disabled");
        fbe_cli_printf("\tIdle time in seconds: %d \n",(int)rg_power_save_info.idle_time_in_sec);
        fbe_cli_printf("\tMax raid latency time in seconds: %d \n\n",(int)rg_power_save_info.max_raid_latency_time_is_sec);
    }

    while (argc > 0) 
    {
        if(strcmp(*argv, "-state") == 0) {
            argv++;
            argc--;

            if(argc ==0 ) {
                fbe_cli_printf("%s", GREEN_USAGE);
                return status;
            }

            if(power_save_info->enabled == FBE_FALSE) {
                fbe_cli_printf("System Power Save is disabled. Enable system power save first\n");
                return status;
            }

            if(strcmp(*argv, "enable")==0) {
                status = fbe_api_enable_raid_group_power_save(rg_object_id);
                if (status == FBE_STATUS_INSUFFICIENT_RESOURCES) {
                    fbe_cli_error( "%s:Failed to Enable Raidgroup PS for rg:%d. One or more disks in this raid group is not power saving capable.\n", __FUNCTION__, rg_id);
                    return status;
                }
                else if (status != FBE_STATUS_OK) {
                    fbe_cli_error( "%s: failed to Enable Raidgroup PS for rg:%d\n", __FUNCTION__,rg_id);
                    return status;
                }
                fbe_cli_printf("Successfully enabled Power Save on raid group %d\n",rg_id);
            }
            else if(strcmp(*argv, "disable")==0){
                status = fbe_api_disable_raid_group_power_save(rg_object_id);
                if (status != FBE_STATUS_OK){
                    fbe_cli_error( "%s: failed to disable Raidgroup PS for rg:%d\n", __FUNCTION__,rg_id);
                    return status;
                }
                fbe_cli_printf("Successfully disabled Power Save on raid group %d\n",rg_id);
            }
            else if (strcmp(*argv, "state") ==0) {
                status = fbe_api_raid_group_get_power_saving_policy(rg_object_id, &rg_power_save_info);
                if (status != FBE_STATUS_OK) {
                fbe_cli_error( "%s: failed to get Power Save info for raid group 0x%x. \n", __FUNCTION__, rg_object_id);
                return status;
                }
                if (rg_power_save_info.power_saving_enabled == FBE_TRUE) {
                    fbe_cli_printf("RaidGroup %d Power Save is enabled\n", rg_id);
                }
                else if (rg_power_save_info.power_saving_enabled == FBE_FALSE) {
                    fbe_cli_printf("RaidGroup %d Power Save is disabled\n", rg_id);
                }   
            }
        }
        else if ((strcmp(*argv, "-idle_time") == 0)) {
            argc--;
            argv++;

            if (argc <= 0) {
                fbe_cli_printf("Idle time in second(s) is expected\n\n");
                fbe_cli_printf("%s", GREEN_USAGE);
                return status;
            }
            idle_time = atoi(*argv);
            if (idle_time != 0) {
                status = fbe_api_set_raid_group_power_save_idle_time(rg_object_id,idle_time);
                if (status != FBE_STATUS_OK) {
                    fbe_cli_error( "%s: failed to set Power Save idle time for the raid group 0x%x.\n", __FUNCTION__, rg_object_id);
                    return status;
                }
            }
            else{
                fbe_cli_printf("Please set a valid idle time in second(s) other than zero.\n\n");
                return status;
            }
        }
        argc--;
        argv++;
    }
 
    return status; 		
}//end of fbe_cli_lib_green_rg_state(void)
