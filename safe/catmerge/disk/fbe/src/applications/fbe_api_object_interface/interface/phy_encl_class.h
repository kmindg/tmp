/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011                    
 * All Rights Reserved.                                          
 * Licensed Material-Property of EMC Corporation.                
 * This software is made available solely pursuant to the terms  
 * of a EMC license agreement which governs its use.             
 *********************************************************************/

/*********************************************************************
 *
 *  Description: 
 *      This file defines the PHYSICAL ENCLOSURE INTERFACE Class.
 *
 *  History:
 *      18-July-2011 : inital version
 *
 *********************************************************************/

#ifndef T_PHY_ENCL_CLASS_H
#define T_PHY_ENCL_CLASS_H

#ifndef T_PHYCLASS_H
#include "phy_class.h"
#endif

/*********************************************************************
 *          Physical Enclosure class : bPhysical base class
 *********************************************************************/

class PhyEncl : public bPhysical
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned PhyEnclCount;

        // interface object name
        char  * idname;
  
        // Physical enclosure Interface function calls and usage 
        char * PhyEnclInterfaceFuncs; 
        char * usage_format;

        // Physical enclosure Interface fbe api data structure
        fbe_port_number_t port_num;
        fbe_enclosure_number_t encl_num;
        fbe_enclosure_slot_number_t drive_num;
        fbe_u32_t component_count;
        fbe_u32_t powerCycleDuration;
        fbe_u32_t powerCycleDelay;
        fbe_base_object_mgmt_drv_dbg_action_t cru_on_off_action;
        fbe_base_object_mgmt_get_enclosure_info_t enclosure_info;
        fbe_base_object_mgmt_drv_dbg_ctrl_t enclosureDriveDebugInfo;
        fbe_base_object_mgmt_drv_power_ctrl_t drivePowerInfo;
        fbe_base_object_mgmt_drv_power_action_t power_on_off_action;
        fbe_base_object_mgmt_exp_ctrl_t expanderInfo;
        fbe_base_object_mgmt_exp_ctrl_action_t expanderAction;
        fbe_enclosure_scsi_error_info_t scsi_error_info;
        fbe_enclosure_types_t enclosuretype;
        fbe_device_physical_location_t location;
        fbe_enclosure_mgmt_get_drive_slot_info_t getDriveSlotInfo;
        fbe_cooling_info_t fanInfo;
        fbe_enclosure_mgmt_ctrl_op_t expanderInfoTest;
        fbe_enclosure_mgmt_ctrl_op_t    esesPageOp;
        fbe_u8_t        *stringOutPtr ;
        
        // private methods 
        //display scsi error information
        void edit_scsi_error_informatiom(
                        fbe_enclosure_scsi_error_info_t *scsi_error_info);

        //display enclosure fan information
        void edit_enclosure_fan_information(fbe_cooling_info_t *fanInfo);

        //display enclosure drive slot information
        void edit_enclosure_drive_slot_information(
               fbe_enclosure_mgmt_get_drive_slot_info_t *getDriveSlotInfo);
        
        //Initializing variables
        void  phy_encl_initializing_variables();
        
    public:

        // Constructor/Destructor
        PhyEncl();
        ~PhyEncl(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        //get enclosure inforamtion
        //fbe_status_t get_enclosure_information(int argc, char **argv);

        //get enclosure object id
        fbe_status_t get_enclosure_object_id(int argc, char **argv);

        //send drive debug control to enclosure
        fbe_status_t send_drive_debug_control(int argc, char **argv);

        //send drive power control to enclosure
        fbe_status_t send_drive_power_control(int argc, char **argv);

        //send expander control to enclosure
        fbe_status_t send_expander_control(int argc, char **argv);

        //get scsi error information for enclosure
        fbe_status_t get_scsi_error_informatiom(int argc, char **argv);

        //get drive debug action
        fbe_status_t get_drive_debug_action(char **argv);

        //get drive power information
        fbe_status_t get_drive_power_info();

        //get expander information for drive
        fbe_status_t get_expander_info();

        //get number of drive on enclosure
        fbe_status_t get_drive_count();

        //get drive debug information
        fbe_status_t get_drive_debug_info();

        //get drive power action
        fbe_status_t get_drive_power_action(char **argv);

        //get expander action
        fbe_status_t get_expander_action(char **argv);

        //get enclosure type
        fbe_status_t get_enclosure_type(int argc, char **argv);

        //get enclosure drive slot information
        fbe_status_t get_enclosure_drive_slot_information(int argc, char **argv);

        //get enclosure fan information
        fbe_status_t get_enclosure_fan_information(int argc, char **argv);

        //convert mgmt status id to name
        char* convert_mgmt_status_id_to_name(fbe_mgmt_status_t mgmt_status);

        //convert env interface status id to name
        char* convert_env_interface_status_id_to_name(
                            fbe_env_inferface_status_t envInterfaceStatus);

        //send Expander test mode control
        fbe_status_t send_exp_test_mode_control(int argc, char **argv) ;

        //send expander string out control
        fbe_status_t  send_exp_string_out_control(int argc, char **argv) ;

        // ------------------------------------------------------------
};

#endif