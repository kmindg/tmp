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
 *      This file defines the Arguments class.
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_ARGCLASS_H
#define T_ARGCLASS_H

#ifndef T_INITFBEAPICLASS_H
#include "init_fbe_api_class.h"
#endif

#ifndef T_PHYDRIVECLASS_H
#include "phy_drive_class.h"
#endif

#ifndef T_SEP_LUN_CLASS_H
#include "sep_lun_class.h"
#endif

#ifndef T_SEP_PANIC_SP_CLASS_H
#include "sep_panic_sp_class.h"
#endif

#ifndef T_SEP_PROVISION_DRIVE_CLASS_H
#include "sep_provision_drive_class.h"
#endif

#ifndef T_SEP_RAID_GROUP_CLASS_H
#include "sep_raid_group_class.h"
#endif

#ifndef T_SYS_EVENT_LOG_CLASS_H
#include "sys_event_log_class.h"
#endif

#ifndef T_SEP_BIND_CLASS_H
#include "sep_bind_class.h"
#endif

#ifndef T_SEP_POWER_SAVING_CLASS_H
#include "sep_power_saving_class.h"
#endif

#ifndef T_SEP_CMI_CLASS_H
#include "sep_cmi_class.h"
#endif

#ifndef T_SEP_BVD_CLASS_H
#include"sep_bvd_class.h"
#endif

#ifndef T_ESP_DRIVE_MGMT_CLASS_H
#include "esp_drive_mgmt_class.h"
#endif

#ifndef T_PHYDISCOVERYCLASS_H
#include "phy_discovery_class.h"
#endif

#ifndef T_SYS_DISCOVERY_CLASS_H
#include "sys_discovery_class.h"
#endif

#ifndef T_PHY_ENCL_CLASS_H
#include "phy_encl_class.h"
#endif

#ifndef T_SEP_VIRTUAL_DRIVE_CLASS_H
#include "sep_virtual_drive_class.h"
#endif

#ifndef T_SEP_DATABASE_CLASS_H
#include "sep_database_class.h"
#endif

#ifndef T_ESP_SPS_MGMT_CLASS_H
#include "esp_sps_mgmt_class.h"
#endif

#ifndef T_ESP_EIR_CLASS_H
#include "esp_eir_class.h"
#endif

#ifndef T_ESP_ENCL_MGMT_CLASS_H
#include "esp_encl_mgmt_class.h"
#endif

#ifndef T_SEP_METADATA_CLASS_H
#include "sep_metadata_class.h"
#endif

#ifndef T_ESP_PS_MGMT_CLASS_H
#include "esp_ps_mgmt_class.h"
#endif

#ifndef T_PHY_BOARD_CLASS_H
#include "phy_board_class.h"
#endif

#ifndef T_ESP_BOARD_MGMT_CLASS_H
#include "esp_board_mgmt_class.h"
#endif

#ifndef T_ESP_COOLING_MGMT_CLASS_H
#include "esp_cooling_mgmt_class.h"
#endif

#ifndef T_SEP_JOB_SERVICE_CLASS_H
#include "sep_job_service_class.h"
#endif

#ifndef T_ESP_MODULE_MGMT_CLASS_H
#include "esp_module_mgmt_class.h"
#endif

#ifndef T_SEP_SCHEDULER_CLASS_H
#include "sep_scheduler_class.h"
#endif

#ifndef T_SEP_BASE_CONFIG_CLASS_H
#include "sep_base_config_class.h"
#endif

#ifndef T_SEP_LOGICAL_ERROR_INJECTION_CLASS_H
#include "sep_logical_error_injection_class.h"
#endif

#ifndef T_SEP_SYSTEM_BG_SERVICE_INTERFACE_CLASS_H
#include "sep_system_bg_service_interface_class.h"
#endif

#ifndef  T_SEP_SYS_TIME_THRESHOLD_CLASS_H
#include "sep_sys_time_threshold_class.h"
#endif

/*********************************************************************
 * Arguments base class : fileUtilClass
 *********************************************************************/

class Arguments : public fileUtilClass
{
    protected:
        // Every object has an idnumber
        unsigned idnumber;
        unsigned argCount;

        // fbesim arguments
        driver_t driverType;
        fbe_sim_transport_connection_target_t spId;

        // package name
        package_t pkg;

        // interface name
        package_subset_t pkgSubset;

        // index into arguments
        int index;

        // pointer to interface object
        Object * pBase;

        // pointer to initFbeApiClass object
        initFbeApiCLASS * pInit;

    public:

        // Constructor
        Arguments(int &argc, char *argv[]);

        // Destructor
        ~Arguments(){}

        // Accessor methods : object
        void dumpme(void);
        unsigned MyIdNumIs(void);
        Object * getBase(void);

        // Accessor methods : package
        package_t getPackage(void);
        package_subset_t getPackageSubset(void);
        
        // Accessor methods : command line
        void List_Arguments(int argc, char *argv[]);
        int getIndex(void);
        
        // Accessor methods : other 
        fbe_sim_transport_connection_target_t getSP(void);
        driver_t getDriver(void);

        // Validate arguments; select and instantiate interface.
        virtual fbe_status_t Do_Arguments (int &argc, char *argv[]);
        
        // Close fbe simulator connection
        void Close_connection (fbe_sim_transport_connection_target_t);
};

#endif
