;//++
;// Copyright (C) EMC Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;
;#ifndef ESP_MESSAGES_H
;#define ESP_MESSAGES_H
;
;//
;//++
;// File:            EspMessages.h (MC)
;//
;// Description:     This file defines ESP Package Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  ESP_PACKAGE_xxx and ESP_PACKAGE_ADMIN_xxx.
;//
;// Notes:
;//   For simulation add the message string also in file fbe_event_log_esp_msgs.c
;//   under the appropriate function according to message severity. 
;//
;//  Information Status Codes Range:  0x0000-0x3FFF
;//  Warning Status Codes Range:      0x4000-0x7FFF
;//  Error Status Codes Range:        0x8000-0xBFFF
;//  Critical Status Codes Range:     0xC000-0xFFFF
;//
;// 
;//--
;//
;//
;#include "k10defs.h"
;
MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( ESP = 0x167 : FACILITY_ESP_PACKAGE )

SeverityNames= ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational= 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning      = 0x2 : STATUS_SEVERITY_WARNING
                  Error        = 0x3 : STATUS_SEVERITY_ERROR
                )

;//-----------------------------------------------------------------------------
;//  Info Status Codes
;//-----------------------------------------------------------------------------
MessageId   = 0x0000
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_GENERIC
Language    = English
Generic Information.
.
;
;#define ESP_ADMIN_INFO_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0001
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_LOAD_VERSION
Language    = English
Driver compiled for %2.
.
;
;#define ESP_ADMIN_INFO_LOAD_VERSION                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_LOAD_VERSION)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0002
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SPS_INSERTED
Language    = English
%2 Inserted.
.
;
;#define ESP_ADMIN_INFO_SPS_INSERTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SPS_INSERTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0003
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SPS_STATE_CHANGE
Language    = English
%2 State Changed (%3 to %4).
.
;
;#define ESP_ADMIN_INFO_SPS_STATE_CHANGE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SPS_STATE_CHANGE)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0004
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SPS_TEST_TIME
Language    = English
Scheduled SPS Test Time.
.
;
;#define ESP_ADMIN_INFO_SPS_TEST_TIME                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SPS_TEST_TIME)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0005
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SPS_NOT_RECOMMENDED
Language    = English
This type of SPS is not recommended for this array type.
.
;
;#define ESP_ADMIN_INFO_SPS_NOT_RECOMMENDED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SPS_NOT_RECOMMENDED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0006
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SPS_CONFIG_CHANGE
Language    = English
SPS Configuration Change (SPS Test needed to verify cabling).
.
;
;#define ESP_ADMIN_INFO_SPS_CONFIG_CHANGE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SPS_CONFIG_CHANGE)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0007
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_PS_INSERTED
Language    = English
%2 Inserted.
.
;
;#define ESP_ADMIN_INFO_PS_INSERTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_PS_INSERTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0008
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FUP_STARTED
Language    = English
%2 firmware upgrade started, current revision %3, product id %4.
.
;
;#define ESP_ADMIN_INFO_FUP_STARTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FUP_STARTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0009
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FUP_UP_TO_REV
Language    = English
%2 firmware revision %3 is up to date, firmware upgrade is not needed.
.
;
;#define ESP_ADMIN_INFO_FUP_UP_TO_IMAGE_REV                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FUP_UP_TO_REV)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x000A
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FUP_SUCCEEDED
Language    = English
%2 firmware upgrade succeeded, current revision %3, old revision %4.
.
;
;#define ESP_ADMIN_INFO_FUP_SUCCEEDED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FUP_SUCCEEDED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x000B
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FUP_ABORTED
Language    = English
%2 firmware upgrade aborted, current revision %3, old revision %4.
.
;
;#define ESP_ADMIN_INFO_FUP_ABORTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FUP_ABORTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x000C
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_IO_MODULE_ENABLED
Language    = English
%2 is enabled and %3.
.
;
;#define ESP_ADMIN_INFO_IO_MODULE_ENABLED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_IO_MODULE_ENABLED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x000D
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_IO_MODULE_REMOVED
Language    = English
%2 is removed and %3.
.
;
;#define ESP_ADMIN_INFO_IO_MODULE_REMOVED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_IO_MODULE_REMOVED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x000E
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_IO_PORT_ENABLED
Language    = English
%2 port %3 is enabled and %4.
.
;
;#define ESP_ADMIN_INFO_IO_PORT_ENABLED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_IO_PORT_ENABLED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0023
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_DIEH_LOAD_SUCCESS
Language    = English
DIEH successfully loaded.
.
;
;#define ESP_ADMIN_INFO_DIEH_LOAD_SUCCESS                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_DIEH_LOAD_SUCCESS)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x002F
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_DRIVE_ONLINE
Language    = English
%2 came online. SN:%3 TLA:%4 Rev:%5.
.
;
;#define ESP_ADMIN_INFO_DRIVE_ONLINE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_DRIVE_ONLINE)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0030
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_RESUME_PROM_READ_SUCCESS
Language    = English
%2 Resume Prom Read completed successfully.
.
;
;#define ESP_ADMIN_INFO_RESUME_PROM_READ_SUCCESS                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_RESUME_PROM_READ_SUCCESS)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0032
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SERIAL_NUM
Language    = English
%2 Serial Number:%3.
.
;
;#define ESP_ADMIN_INFO_SERIAL_NUM                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SERIAL_NUM)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0033
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_PEER_SP_BOOT_SUCCESS
Language    = English
PBL: %2 status: %3(0x%4).
.
;
;#define ESP_ADMIN_INFO_PEER_SP_BOOT_SUCCESS                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_PEER_SP_BOOT_SUCCESS)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0034
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_PEER_SP_BOOT_OTHER
Language    = English
PBL: %2 status: %3(0x%4).
.
;
;#define ESP_ADMIN_INFO_PEER_SP_BOOT_OTHER                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_PEER_SP_BOOT_OTHER)
;
;//-----------------------------------------------------------------------------
;// This message is Obsolete
MessageId   = 0x0035
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MCU_HOST_RESET
Language    = English
%2 has been reset. Reason: %3. Reset status code[%4]: 0x%5. No action needs to be taken.
.
;
;#define ESP_ADMIN_INFO_MCU_HOST_RESET_INFO                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MCU_HOST_RESET)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0036
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_PORT_RESTORE_COMPLETED
Language    = English
%2 management port configuration request failed. Restoring last known working configuration completed.
Please retry after making sure the requested configuration is supported.
.
;
;#define ESP_ADMIN_INFO_MGMT_PORT_RESTORE_COMPLETED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_PORT_RESTORE_COMPLETED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0037
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_PORT_CONFIG_COMPLETED
Language    = English
%2 management port speed configuration request completed successfully.
.
;
;#define ESP_ADMIN_INFO_MGMT_PORT_CONFIG_COMPLETED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_PORT_CONFIG_COMPLETED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0038
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_PORT_CONFIG_FAILED
Language    = English
%2 management port speed configuration request failed. This requested configuration is not supported.
.
;
;#define ESP_ADMIN_INFO_MGMT_PORT_CONFIG_FAILED                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_PORT_CONFIG_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0039
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_PORT_RESTORE_FAILED
Language    = English
%2 management port speed configuration request failed. Restoring last known working configuration also failed.
Please gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_INFO_MGMT_PORT_RESTORE_FAILED                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_PORT_RESTORE_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x003A
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_FRU_INSERTED
Language    = English
%2 is inserted.
.
;
;#define ESP_ADMIN_INFO_MGMT_FRU_INSERTED                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_FRU_INSERTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x003B
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_FRU_LINK_UP
Language    = English
%2 network connection is restored.
.
;
;#define ESP_ADMIN_INFO_MGMT_FRU_LINK_UP                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_FRU_LINK_UP)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x003C
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_FRU_NO_NETWORK_CONNECTION
Language    = English
%2 network connection not detected.
.
;
;#define ESP_ADMIN_INFO_MGMT_FRU_NO_NETWORK_CONNECTION                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_FRU_NO_NETWORK_CONNECTION)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x003D
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_FRU_SERVICE_PORT_LINK_UP
Language    = English
The link on the service port on the %2 is up.
.
;
;#define ESP_ADMIN_INFO_MGMT_FRU_SERVICE_PORT_LINK_UP                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_FRU_SERVICE_PORT_LINK_UP)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x003E
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_FRU_SERVICE_PORT_NO_LINK
Language    = English
The link on the service port on the %2 is down.
.
;
;#define ESP_ADMIN_INFO_MGMT_FRU_SERVICE_PORT_NO_LINK                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_FRU_SERVICE_PORT_NO_LINK)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x003F
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_PORT_FORCE_AUTONEG_ON
Language    = English
Auto-negotiation cannot be turned off for the Management Port because it is running at 1000Mbps.
.
;
;#define ESP_ADMIN_INFO_MGMT_PORT_FORCE_AUTONEG_ON                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_PORT_FORCE_AUTONEG_ON)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0040
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SFP_INSERTED
Language    = English
SFP located in %2 in slot %3 port %4 is inserted.
.
;
;#define ESP_ADMIN_INFO_SFP_INSERTED                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SFP_INSERTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0041
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SFP_REMOVED
Language    = English
SFP located in %2 in slot %3 port %4 is removed.
.
;
;#define ESP_ADMIN_INFO_SFP_REMOVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SFP_REMOVED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0042
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_CONNECTOR_INSERTED
Language    = English
%2 is inserted.
.
;
;#define ESP_ADMIN_INFO_CONNECTOR_INSERTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_CONNECTOR_INSERTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0043
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_CONNECTOR_REMOVED
Language    = English
%2 is removed.
.
;
;#define ESP_ADMIN_INFO_CONNECTOR_REMOVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_CONNECTOR_REMOVED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0044
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_CONNECTOR_STATUS_CHANGE
Language    = English
%2 cable status changed from %3 to %4.
.
;
;#define ESP_ADMIN_INFO_CONNECTOR_STATUS_CHANGE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_CONNECTOR_STATUS_CHANGE)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0045
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FAN_INSERTED
Language    = English
%2 Inserted
.
;
;#define ESP_ADMIN_INFO_FAN_INSERTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FAN_INSERTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0046
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_ENCL_SHUTDOWN_CLEARED
Language    = English
%2 shutdown condition has been cleared.
.
;
;#define ESP_ADMIN_INFO_ENCL_SHUTDOWN_CLEARED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_ENCL_SHUTDOWN_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0047
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_LINK_STATUS_CHANGE
Language    = English
%2 %3 Port %4 %5 %6 link status changed from %7 to %8.
.
;
;#define ESP_ADMIN_INFO_LINK_STATUS_CHANGEE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_LINK_STATUS_CHANGE)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0048
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_LCC_INSERTED
Language    = English
%2 Inserted
.
;
;#define ESP_ADMIN_INFO_LCC_INSERTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_LCC_INSERTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0049
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_PORT_CONFIG_COMMAND_RECEIVED
Language    = English
User requested %2 management port speed change to auto neg: %3, speed: %4, duplex mode: %5.
.
;
;#define ESP_ADMIN_INFO_MGMT_PORT_CONFIG_COMMAND_RECEIVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_PORT_CONFIG_COMMAND_RECEIVED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0050
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_ENV_ABORT_UPGRADE_COMMAND_RECEIVED
Language    = English
Aborting %2 firmware upgrade.
.
;
;#define ESP_ADMIN_INFO_ENV_ABORT_UPGRADE_COMMAND_RECEIVED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_ENV_ABORT_UPGRADE_COMMAND_RECEIVED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0051
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_ENV_RESUME_UPGRADE_COMMAND_RECEIVED
Language    = English
Resuming %2 firmware upgrade.
.
;
;#define ESP_ADMIN_INFO_ENV_RESUME_UPGRADE_COMMAND_RECEIVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_ENV_RESUME_UPGRADE_COMMAND_RECEIVED)

;//-----------------------------------------------------------------------------
;// This message is Obsolete
MessageId   = 0x0052
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MCU_BIST_FAULT_CLEARED
Language    = English
%2 BIST(Build In Self Test) Fault cleared.
.
;
;#define ESP_ADMIN_INFO_MCU_BIST_FAULT_CLEARED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MCU_BIST_FAULT_CLEARED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0053
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SUITCASE_FAULT_CLEARED
Language    = English
%2 Fault cleared (%3).
.
;
;#define ESP_ADMIN_INFO_SUITCASE_FAULT_CLEARED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SUITCASE_FAULT_CLEARED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0054
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_PS_OVER_TEMP_CLEARED
Language    = English
%2 over temperature condition has been cleared.
.
;
;#define ESP_ADMIN_INFO_PS_OVER_TEMP_CLEARED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_PS_OVER_TEMP_CLEARED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0055
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_PS_FAULT_CLEARED
Language    = English
%2 fault condition has been cleared.
.
;
;#define ESP_ADMIN_INFO_PS_FAULT_CLEARED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_PS_FAULT_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0056
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SPS_CABLING_VALID
Language    = English
%2 Cabling has been validated.
.
;
;#define ESP_ADMIN_INFO_SPS_CABLING_VALID                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SPS_CABLING_VALID)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0057
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_ENCL_OVER_TEMP_WARNING
Language    = English
%2 is reporting over temperature warning.
.
;
;#define ESP_ADMIN_INFO_ENCL_OVER_TEMP_WARNING                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_ENCL_OVER_TEMP_WARNING)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0058
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_ENCL_OVER_TEMP_CLEARED
Language    = English
%2 over temperature %3 condition has been cleared.
.
;
;#define ESP_ADMIN_INFO_ENCL_OVER_TEMP_CLEARED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_ENCL_OVER_TEMP_CLEARED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0059
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FUP_QUEUED
Language    = English
%2 firmware upgrade queued, current revision %3, product id %4.
.
;
;#define ESP_ADMIN_INFO_FUP_QUEUED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FUP_QUEUED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x005A
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_ENCL_ADDED
Language    = English
%2 added.
.
;
;#define ESP_ADMIN_INFO_ENCL_ADDED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_ENCL_ADDED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x005B
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_ENCL_SHUTDOWN_DELAY_INPROGRESS
Language    = English
%2 shutdown delay is in progress.
.
;
;#define ESP_ADMIN_INFO_ENCL_SHUTDOWN_DELAY_INPROGRESS            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_ENCL_SHUTDOWN_DELAY_INPROGRESS)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x005C
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_RESET_ENCL_SHUTDOWN_TIMER
Language    = English
%2 shutdown timer has been reset.
.
;
;#define ESP_ADMIN_INFO_RESET_ENCL_SHUTDOWN_TIMER            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_RESET_ENCL_SHUTDOWN_TIMER)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x005D
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_EXT_PEER_BOOT_STATUS_CHANGE
Language    = English
The core software is loading on %2. %3 - %4(0x%5).
.
;
;#define ESP_ADMIN_INFO_EXT_PEER_BOOT_STATUS_CHANGE                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_EXT_PEER_BOOT_STATUS_CHANGE)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x005E
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_USER_MODIFIED_WWN_SEED
Language    = English
A user command is received to modify the array wwn seed to 0x%2.
.
;
;#define ESP_ADMIN_INFO_USER_MODIFIED_WWN_SEED            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_USER_MODIFIED_WWN_SEED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x005F
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_USER_CHANGE_SYS_ID_INFO
Language    = English
A user command is received to modify the system id. SN %2, ChangeSN %3, PN %4, ChangePN %5.
.
;
;#define ESP_ADMIN_INFO_USER_CHANGE_SYS_ID_INFO            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_USER_CHANGE_SYS_ID_INFO)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0060
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FAN_FAULT_CLEARED
Language    = English
%2 fault has been cleared (%3).
.
;
;#define ESP_ADMIN_INFO_FAN_FAULT_CLEARED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FAN_FAULT_CLEARED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0061
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION
Language    = English
%2 Predicted to exceed drive specified write endurance in %3 days.
.
;
;#define ESP_ADMIV_INFO_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION)
;

;//-----------------------------------------------------------------------------
MessageId       = 0x0062
Severity        = Informational
Facility        = ESP
SymbolicName    = ESP_INFO_BMC_SHUTDOWN_WARNING_CLEARED
Language        = English
%2 shutdown condition has been cleared.
.
;
;#define ESP_ADMIN_INFO_BMC_SHUTDOWN_WARNING_CLEARED            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_BMC_SHUTDOWN_WARNING_CLEARED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0063
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_BMC_BIST_FAULT_CLEARED
Language    = English
%2 BIST(Build In Self Test) fault cleared.
.
;
;#define ESP_ADMIN_INFO_BMC_BIST_FAULT_CLEARED            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_BMC_BIST_FAULT_CLEARED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x0064
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_DRIVE_POWERED_ON
Language    = English
%2 powered on.
.
;
;#define ESP_ADMIN_INFO_DRIVE_POWERED_ON                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_DRIVE_POWERED_ON)

;//-----------------------------------------------------------------------------
MessageId   = 0x0065
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_DRIVE_POWERED_OFF
Language    = English
%2 powered off.
.
;
;#define ESP_ADMIN_INFO_DRIVE_POWERED_OFF                    \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_DRIVE_POWERED_OFF)
;//-----------------------------------------------------------------------------
MessageId   = 0x0066
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_CACHE_STATUS_CHANGED
Language    = English
%2 CacheStatus changes from: %3 to %4.
.
;
;#define ESP_ADMIN_INFO_CACHE_STATUS_CHANGED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_CACHE_STATUS_CHANGED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0067
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_PEER_SP_INSERTED
Language    = English
Peer %2 has been Inserted.
.
;
;#define ESP_ADMIN_INFO_PEER_SP_INSERTED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_PEER_SP_INSERTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0068
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_LOW_BATTERY_FAULT_CLEARED
Language    = English
%2 %3, low battery fault cleared.
.
;
;#define ESP_ADMIN_INFO_LOW_BATTERY_FAULT_CLEARED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_LOW_BATTERY_FAULT_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0069
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FUP_TERMINATED
Language    = English
%2 firmware upgrade terminated, current revision %3, old revision %4.
.
;
;#define ESP_ADMIN_INFO_FUP_TERMINATED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FUP_TERMINATED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0070
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_BM_ECB_FAULT_CLEARED
Language    = English
%2 fault has been cleared. SP drive power distribution has returned to normal.
.
;
;#define ESP_ADMIN_INFO_BM_ECB_FAULT_CLEARED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_BM_ECB_FAULT_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId    = 0x0071
Severity     = Informational
;//NOSSPG C4type=User
Facility     = ESP
SymbolicName = ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED
Language     = English
%2 environmental interface failure cleared.
.
;
;#define ESP_ADMIN_INFO_ENV_INTERFACE_FAILURE_CLEARED                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId    = 0x0072
Severity     = Informational
Facility     = ESP
SymbolicName = ESP_INFO_IO_PORT_CONFIGURED
Language     = English
%2 port %3 configured as %4.
.
;
;#define ESP_ADMIN_INFO_IO_PORT_CONFIGURED                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_IO_PORT_CONFIGURED)
;
;//-----------------------------------------------------------------------------
MessageId    = 0x0073
Severity     = Informational
Facility     = ESP
SymbolicName = ESP_INFO_PEERSP_CLEAR
Language     = English
Peer Boot Logging: %2 has BIOS/POST fault during last boot. The fault has been cleared.
.
;
;#define ESP_ADMIN_INFO_PEERSP_CLEAR                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_PEERSP_CLEAR)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x0074
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_MGMT_FRU_FAULT_CLEARED
Language    = English
%2 fault cleared.
.
;
;#define ESP_ADMIN_INFO_MGMT_FRU_FAULT_CLEARED                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_MGMT_FRU_FAULT_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId	= 0x0075
Severity	= Informational
Facility	= ESP
SymbolicName	= ESP_INFO_CACHE_CARD_FAULT_CLEARED
Language	= English
%2 fault cleared.
.
;
;#define ESP_ADMIN_INFO_CACHE_CARD_FAULT_CLEARED               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_CACHE_CARD_FAULT_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId	= 0x0076
Severity	= Informational
Facility	= ESP
SymbolicName	= ESP_INFO_CACHE_CARD_INSERTED
Language	= English
%2 is inserted.
.
;
;#define ESP_ADMIN_INFO_CACHE_CARD_INSERTED              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_CACHE_CARD_INSERTED)
;
;//-----------------------------------------------------------------------------
MessageId	= 0x0077
Severity	= Informational
Facility	= ESP
SymbolicName	= ESP_INFO_DIMM_FAULT_CLEARED
Language	= English
%2 fault cleared.
.
;
;#define ESP_ADMIN_INFO_DIMM_FAULT_CLEARED               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_DIMM_FAULT_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId    = 0x0078
Severity     = Informational
Facility     = ESP
SymbolicName = ESP_INFO_DRIVE_BMS_FOR_WORST_HEAD
Language     = English
%2 SN:%3 BMS found %4 entries for head: %5. Total entries: %6.
.
;
;#define ESP_ADMIN_INFO_DRIVE_BMS_FOR_WORST_HEAD                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_DRIVE_BMS_FOR_WORST_HEAD)
;
;//-----------------------------------------------------------------------------
MessageId    = 0x0079
Severity     = Informational
Facility     = ESP
SymbolicName = ESP_INFO_DRIVE_BMS_LBB
Language     = English
%2 SN:%3 Lambda_BB for BMS is %4.
.
;
;#define ESP_ADMIN_INFO_DRIVE_BMS_LBB                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_DRIVE_BMS_LBB)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x007A
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_LCC_FAULT_CLEARED
Language    = English
%2 fault cleared.
.
;
;#define ESP_ADMIN_INFO_LCC_FAULT_CLEARED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_LCC_FAULT_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId	= 0x007B
Severity	= Informational
Facility	= ESP
SymbolicName	= ESP_INFO_CACHE_CARD_CONFIGURATION_ERROR_CLEARED
Language	= English
%2 configuration becomes correct.
.
;
;#define ESP_ADMIN_INFO_CACHE_CARD_CONFIGURATION_ERROR_CLEARED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_CACHE_CARD_CONFIGURATION_ERROR_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x007C
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FUP_WAIT_PEER_PERMISSION
Language    = English
%2 firmware upgrade waiting for peer SP permission, current revision %3.
.
;
;#define ESP_ADMIN_INFO_FUP_WAIT_PEER_PERMISSION                   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FUP_WAIT_PEER_PERMISSION)
;


;//-----------------------------------------------------------------------------
MessageId   = 0x007D
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SSC_INSERTED
Language    = English
%2 Inserted.
.
;
;#define ESP_ADMIN_INFO_SSC_INSERTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SSC_INSERTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x007E
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_SSC_FAULT_CLEARED
Language    = English
%2 Fault has cleared.
.
;
;#define ESP_ADMIN_INFO_SSC_FAULT_CLEARED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_SSC_FAULT_CLEARED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x007F
Severity    = Informational
Facility    = ESP
SymbolicName    = ESP_INFO_FUP_ACTIVATION_DEFERRED
Language    = English
%2 firmware upgrade activation deferred, current revision %3, product id %4.
.
;
;#define ESP_ADMIN_INFO_FUP_ACTIVATION_DEFERRED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_INFO_FUP_ACTIVATION_DEFERRED)
;

;// ----------------------Add all Informational Messages above this ----------------------
;//
;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId   = 0x4000
Severity    = Warning
Facility    = ESP
SymbolicName    = ESP_WARN_GENERIC
Language    = English
Generic Information.
.
;
;#define ESP_ADMIN_WARN_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_WARN_GENERIC)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x4003
Severity    = Warning
Facility    = ESP
SymbolicName    = ESP_WARN_FAN_DERGADED
Language    = English
%2 is degraded. Please gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_WARN_FAN_DERGADED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_WARN_FAN_DERGADED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x4005
Severity    = Warning
Facility    = ESP
SymbolicName    = ESP_WARN_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION
Language    = English
%2 Predicted to exceed drive specified write endurance in %3 days.
.
;
;#define ESP_ADMIV_WARN_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_WARN_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x4006
Severity    = Warning
Facility    = ESP
SymbolicName    = ESP_WARN_IO_PORT_CONFIGURATION_DESTROYED
Language    = English
IO Port Configuration has been removed at the request of the user, the SPs will reboot.
.
;
;#define ESP_ADMIN_WARN_IO_PORT_CONFIGURATION_DESTROYED                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_WARN_IO_PORT_CONFIGURATION_DESTROYED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x4007
Severity    = Warning
Facility    = ESP
SymbolicName    = ESP_WARN_IO_PORT_CONFIGURATION_CHANGED
Language    = English
IO Port configuration has changed, the SP will reboot.
.
;
;#define ESP_ADMIN_WARN_IO_PORT_CONFIGURATION_DESTROYED                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_WARN_IO_PORT_CONFIGURATION_DESTROYED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x4010
Severity    = Warning
Facility    = ESP
SymbolicName    = ESP_WARN_DRIVE_BMS_OVER_WARN_THRESHOLD
Language    = English
%2 SN:%3 BMS found %4 entries for head %5. Total entries: %6. Drive recommended for Proactive Spare.
.
;
;#define ESP_ADMIN_WARN_DRIVE_BMS_OVER_WARN_THRESHOLD                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_WARN_DRIVE_BMS_OVER_WARN_THRESHOLD)
;
   
;//--------------------- Add all Warning messages above this -------------------
;//
;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId   = 0x8000
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_GENERIC
Language    = English
Generic Information.
.
;
;#define ESP_ADMIN_ERROR_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8001
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_SPS_REMOVED
Language    = English
%2 has been Removed.
.
;
;#define ESP_ADMIN_ERROR_SPS_REMOVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_SPS_REMOVED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8002
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_SPS_FAULTED
Language    = English
%2 is Faulted (%3 to %4).  Gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_SPS_FAULTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_SPS_FAULTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8003
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_SPS_CABLING_FAULT
Language    = English
%2 is cabled incorrectly (%3).  Gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_SPS_CABLING_FAULT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_SPS_CABLING_FAULT)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8004
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_SPS_NOT_SUPPORTED
Language    = English
%2 is not supported.  Gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_SPS_NOT_SUPPORTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_SPS_NOT_SUPPORTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8005
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_PS_REMOVED
Language    = English
%2 has been Removed.  Gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_PS_REMOVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_PS_REMOVED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8006
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_PS_FAULTED
Language    = English
%2 is Faulted (%3).  Gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_PS_FAULTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_PS_FAULTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8007
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_ERROR_FUP_FAILED
Language    = English
%2 firmware upgrade failed.  The current revision is %3.  The reason for failure is %4.
.
;
;#define ESP_ADMIN_ERROR_FUP_FAILED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_FUP_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8008
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_IO_MODULE_FAULTED
Language    = English
%2 has faulted because %3.
.
;
;#define ESP_ADMIN_ERROR_IO_MODULE_FAULTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_IO_MODULE_FAULTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8009
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_IO_MODULE_INCORRECT
Language    = English
%2 is incorrect found %3 expected %4.
.
;
;#define ESP_ADMIN_ERROR_IO_MODULE_INCORRECT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_IO_MODULE_INCORRECT)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x800A
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_IO_PORT_FAULTED
Language    = English
%2 port %3 is faulted because %4.
.
;
;#define ESP_ADMIN_ERROR_IO_PORT_FAULTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_IO_PORT_FAULTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x800B
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_PS_OVER_TEMP_DETECTED
Language    = English
%2 is reporting OverTemp.  The Power Supply itself is probably not the source of the problem.
Gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_PS_OVER_TEMP_DETECTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_PS_OVER_TEMP_DETECTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x800C
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_MGMT_FRU_FAULTED
Language    = English
%2 is faulted. Please gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_MGMT_FRU_FAULTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_MGMT_FRU_FAULTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x800D
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_MGMT_FRU_REMOVED
Language    = English
%2 is removed. Call your Service Provider.
.
;
;#define ESP_ADMIN_ERROR_MGMT_FRU_REMOVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_MGMT_FRU_REMOVED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x800E
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_ERROR_RESUME_PROM_READ_FAILED
Language    = English
%2 Resume Prom Read operation has failed.  Status: %3.
.
;
;#define ESP_ADMIN_ERROR_RESUME_PROM_READ_FAILED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_RESUME_PROM_READ_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8023
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DIEH_LOAD_FAILED
Language    = English
DIEH failed to load, status %2.
.
;
;#define ESP_ADMIN_ERROR_DIEH_LOAD_FAILED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DIEH_LOAD_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8028
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_CAPACITY_FAILED
Language    = English
%2 failed capacity check. SN:%3 TLA:%4 Rev:%5.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_CAPACITY_FAILED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_CAPACITY_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x802F
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_OFFLINE
Language    = English
%2 taken offline. SN:%3 TLA:%4 Rev:%5 (%6) Reason:%7. Action:%8.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_OFFLINE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_OFFLINE)
;

;//-----------------------------------------------------------------------------
;// This message is Obsolete
MessageId   = 0x8030
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_MCU_HOST_RESET
Language    = English
%2 has been reset. Reason: %3. Reset status code[%4]: 0x%5. The %6 should be replaced. 
Please gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_MCU_HOST_RESET                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_MCU_HOST_RESET)
;
;//-----------------------------------------------------------------------------
;// This message is Obsolete
MessageId   = 0x8031
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_MCU_HOST_RESET_WITH_FRU_PART_NUM
Language    = English
%2 has been reset. Reason: %3. Reset status code[%4]: 0x%5. The %6 (Part Number: %7) should be replaced. 
Please gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_MCU_HOST_RESET_ERROR_WITH_FRU_PART_NUM                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_MCU_HOST_RESET_WITH_FRU_PART_NUM)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8032
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_SFP_FAULTED
Language    = English
SFP located in %2 in slot %3 port %4 is faulted and should be replaced.
.
;
;#define ESP_ADMIN_ERROR_SFP_FAULTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_SFP_FAULTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8033
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_FAN_REMOVED
Language    = English
%2 has been Removed.
.
;
;#define ESP_ADMIN_ERROR_FAN_REMOVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_FAN_REMOVED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8034
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_FAN_FAULTED
Language    = English
%2 is Faulted (%3).
.
;
;#define ESP_ADMIN_ERROR_FAN_FAULTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_FAN_FAULTED)
;


;//-----------------------------------------------------------------------------
MessageId   = 0x8035
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_ENCL_SHUTDOWN_DETECTED
Language    = English
%2 shutdown condition (%3) has been detected.
.
;
;#define ESP_ADMIN_ERROR_ENCL_SHUTDOWN_DETECTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENCL_SHUTDOWN_DETECTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8036
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_LCC_FAULTED
Language    = English
%2 is faulted, SN: %3, PN: %4.  Gather diagnostic materials and contact your service provider.
.
;
;#define ESP_ADMIN_ERROR_LCC_FAULTED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_LCC_FAULTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8037
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_LCC_REMOVED
Language    = English
%2 has been removed.
.
;
;#define ESP_ADMIN_ERROR_LCC_REMOVED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_LCC_REMOVED)
;

;//-----------------------------------------------------------------------------
;// This message is Obsolete
MessageId   = 0x8038
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_MCU_BIST_FAULT_DETECTED
Language    = English
%2 BIST(Build In Self Test) Fault detected (%3).
.
;
;#define ESP_ADMIN_ERROR_MCU_BIST_FAULT_DETECTED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_MCU_BIST_FAULT_DETECTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8039
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_SUITCASE_FAULT_DETECTED
Language    = English
%2 Fault detected (%3).
.
;
;#define ESP_ADMIN_ERROR_SUITCASE_FAULT_DETECTED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_SUITCASE_FAULT_DETECTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x803A
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_ENCL_OVER_TEMP_DETECTED
Language    = English
%2 is reporting over temperature failure.
.
;
;#define ESP_ADMIN_ERROR_ENCL_OVER_TEMP_DETECTED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENCL_OVER_TEMP_DETECTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x803B
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_ENCL_STATE_CHANGED
Language    = English
%2 state changed from %3 to %4.
.
;
;#define ESP_ADM_ERROR_ENCL_STATE_CHANGED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENCL_STATE_CHANGED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x803C
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_PRICE_CLASS_FAILED
Language    = English
%2 failed price class check. SN:%3 TLA:%4 Rev:%5.
.
;
;#define ESP_ADMIV_ERROR_DRIVE_PRICE_CLASS_FAILED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_PRICE_CLASS_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x803D
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION
Language    = English
%2 Predicted to exceed drive specified write endurance in %3 days. Drive replacement recommended.
.
;
;#define ESP_ADMIV_ERROR_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION)
;


;//-----------------------------------------------------------------------------
MessageId   = 0x803E
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_BMC_SHUTDOWN_WARNING_DETECTED
Language    = English
%2 shutdown condition(%3) has been detected.
.
;
;#define ESP_ADMIN_ERROR_BMC_SHUTDOWN_WARNING_DETECTED            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_BMC_SHUTDOWN_WARNING_DETECTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x803F
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_BMC_BIST_FAULT_DETECTED
Language    = English
%2 BIST(Built In Self Test) Fault detected. Reason:%3, Action:%4.
.
;
;#define ESP_ADMIN_ERROR_BMC_BIST_FAULT_DETECTED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_BMC_BIST_FAULT_DETECTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8040
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_PEER_SP_DOWN
Language    = English
Peer %2 is Down.
.
;
;#define ESP_ADMIN_ERROR_PEER_SP_DOWN                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_PEER_SP_DOWN)
;
;
;//-----------------------------------------------------------------------------

MessageId   = 0x8041
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_PEER_SP_REMOVED
Language    = English
Peer %2 has been Removed.
.
;
;#define ESP_ADMIN_ERROR_PEER_SP_REMOVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_PEER_SP_REMOVED)
;
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8042
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_ENCL_CROSS_CABLED
Language    = English
%2 LCC Cables, between 2 enclosures or between the SP and the first enclosure, are crossed between the A and B sides. 
Please check the enclosure loop connection.
.
;
;#define ESP_ADMIN_ERROR_ENCL_CROSS_CABLED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENCL_CROSS_CABLED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8043
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_ENCL_BE_LOOP_MISCABLED
Language    = English
%2 LCC Cables, between 2 enclosures or between the SP and the first enclosure, are not connected to the same loop. 
Please check the enclosure loop connection.
.
;
;#define ESP_ADMIN_ERROR_ENCL_BE_LOOP_MISCABLED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENCL_BE_LOOP_MISCABLED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8044
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_ENCL_LCC_INVALID_UID
Language    = English
%2 invalid serial number detected, not able to validate the cabling.
.
;
;#define ESP_ADMIN_ERROR_ENCL_LCC_INVALID_UID                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENCL_LCC_INVALID_UID)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8045
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_ENCL_EXCEEDED_MAX
Language    = English
%2 exceeds the limit for the drives and/or expanders on the bus.
.
;
;#define ESP_ADMIN_ERROR_ENCL_EXCEEDED_MAX                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENCL_EXCEEDED_MAX)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8046
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_LOW_BATTERY_FAULT_DETECTED
Language    = English
%2 CPU low battery fault detected.
.
;
;#define ESP_ADMIN_ERROR_LOW_BATTERY_FAULT_DETECTED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_LOW_BATTERY_FAULT_DETECTED)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x8047
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_ENHANCED_QUEUING_FAILED
Language    = English
%2 failed enhanced queuing check. SN:%3 TLA:%4 Rev:%5.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_ENHANCED_QUEUING_FAILED              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_ENHANCED_QUEUING_FAILED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8048
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_BM_ECB_FAULT_DETECTED
Language    = English
%2 power distribution control is not engaged. Only peer SP power is being supplied to the drives. Reseat the base module. If the fault persists, replace the base module.
.
;
;#define ESP_ADMIN_ERROR_BM_ECB_FAULT_DETECTED                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_BM_ECB_FAULT_DETECTED)
;
;//-----------------------------------------------------------------------------
MessageId    = 0x8049
Severity     = Error
;//NOSSPG C4type=User
Facility     = ESP
SymbolicName = ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED
Language     = English
%2 environmental interface failure detected. Reason: %3.
.
;
;#define ESP_ADMIN_ERROR_ENV_INTERFACE_FAILURE_DETECTED                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x804A
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_ENCL_TYPE_NOT_SUPPORTED
Language    = English
%2 Enclosure Type %3 is not supported by this Platform.
.
;
;#define ESP_ADMIN_ERROR_ENCL_TYPE_NOT_SUPPORTED              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_ENCL_TYPE_NOT_SUPPORTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x804B
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_CACHE_CARD_CONFIGURATION_ERROR
Language    = English
%2 configuration error (%3).
.
;
;#define ESP_ADMIV_ERROR_CACHE_CARD_CONFIGURATION_ERROR              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_CACHE_CARD_CONFIGURATION_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId   = 0x804C
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_CACHE_CARD_FAULTED
Language    = English
%2 is Faulted (%3). Call your Service Provider.
.
;
;#define ESP_ADMIV_ERROR_CACHE_CARD_FAULT_DETECTED            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_CACHE_CARD_FAULTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x804D
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_CACHE_CARD_REMOVED
Language    = English
%2 has been Removed.
.
;
;#define ESP_ADMIV_ERROR_CACHE_CARD_REMOVED              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_CACHE_CARD_REMOVED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x804E
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_SED_NOT_SUPPORTED_FAILED
Language    = English
%2 SED drives not supported. SN:%3 TLA:%4 Rev:%5.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_SED_NOT_SUPPORTED_FAILED               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_SED_NOT_SUPPORTED_FAILED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8050
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_BMS_OVER_MAX_ENTRIES
Language    = English
%2 SN:%3 BMS found max %4 entries. Drive recommended for Proactive Spare.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_BMS_OVER_MAX_ENTRIES              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_BMS_OVER_MAX_ENTRIES)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8051
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_BMS_LBB_OVER_THRESHOLD
Language    = English
%2 SN:%3 Lambda_BB for BMS is %4.  Drive recommended for Proactive Spare.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_BMS_LBB_OVER_THRESHOLD              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_BMS_LBB_OVER_THRESHOLD)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8052
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_ERROR_LCC_COMPONENT_FAULTED
Language    = English
%2 is faulted.  This failure may be caused by a component other than the LCC (Drive, Cable, Connector, ...).
.
;
;#define ESP_ADMIN_ERROR_LCC_COMPONENT_FAULTED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_LCC_COMPONENT_FAULTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8053
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_BMS_FOR_WORST_HEAD_OVER_WARN_THRESHOLD
Language    = English
%2 SN:%3 BMS found %4 entries for worst head %5. Total entries: %6. Drive recommended for Proactive Spare.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_BMS_FOR_WORST_HEAD_OVER_WARN_THRESHOLD                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_BMS_FOR_WORST_HEAD_OVER_WARN_THRESHOLD)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8054
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DIMM_FAULTED
Language    = English
%2 is Faulted. Call your Service Provider.
.
;
;#define ESP_ADMIV_ERROR_DIMM_FAULT_DETECTED            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DIMM_FAULTED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8055
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_IO_MODULE_UNSUPPORT
Language    = English
%2 is not supported for this platform. Please remove the slic and replace it with a supported slic.
.
;
;#define ESP_ADMIN_ERROR_IO_MODULE_UNSUPPORT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_IO_MODULE_UNSUPPORT)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8056
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_OFFLINE_ACTION_REINSERT
Language    = English
%2 taken offline. Reinsert the drive. SN:%3 TLA:%4 Rev:%5 (%6) Reason:%7.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_OFFLINE_ACTION_REINSERT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_OFFLINE_ACTION_REINSERT)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8057
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_OFFLINE_ACTION_REPLACE
Language    = English
%2 taken offline. Replace the drive. SN:%3 TLA:%4 Rev:%5 (%6) Reason:%7.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_OFFLINE_ACTION_REPLACE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_OFFLINE_ACTION_REPLACE)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8058
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_OFFLINE_ACTION_ESCALATE
Language    = English
%2 taken offline. Escalate to support. SN:%3 TLA:%4 Rev:%5 (%6) Reason:%7.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_OFFLINE_ACTION_ESCALATE                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_OFFLINE_ACTION_ESCALATE)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x8059
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_OFFLINE_ACTION_UPGRD_FW
Language    = English
%2 taken offline. Upgrade drive Firmware. SN:%3 TLA:%4 Rev:%5 (%6) Reason:%7.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_OFFLINE_ACTION_UPGRD_FW                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_OFFLINE_ACTION_UPGRD_FW)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x805A
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_OFFLINE_ACTION_RELOCATE
Language    = English
%2 taken offline. Replace vault drive with compatible drive. SN:%3 TLA:%4 Rev:%5 (%6) Reason:%7.
.
;
;#define ESP_ADMIN_ERROR_DRIVE_OFFLINE_ACTION_RELOCATE                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_OFFLINE_ACTION_RELOCATE)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x805C
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_DRIVE_PLATFORM_LIMITS_EXCEEDED
Language    = English
%2 Exceeded platform limit of %3 slots. SN:%4 TLA:%5 Rev:%6. %7
.
;
;#define ESP_ADMIN_ERROR_DRIVE_PLATFORM_LIMITS_EXCEEDED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_DRIVE_PLATFORM_LIMITS_EXCEEDED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x805D
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_SSC_REMOVED
Language    = English
%2 has been Removed. Please insert the System Status Card.
.
;
;#define ESP_ADMIN_ERROR_SSC_REMOVED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_SSC_REMOVED)
;
;//-----------------------------------------------------------------------------
MessageId   = 0x805E
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_ERROR_SSC_FAULTED
Language    = English
%2 is faulted.  Please replace the System Status Card.
.
;
;#define ESP_ADMIN_ERROR_SSC_FAULTED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_ERROR_SSC_FAULTED)
;
;//-------------------- Add all Error Message above this -----------------------
;//-----------------------------------------------------------------------------
;//  Critical Status Codes
;//-----------------------------------------------------------------------------
MessageId   = 0xC000
Severity    = Error
Facility    = ESP
SymbolicName    = ESP_CRIT_GENERIC
Language    = English
Generic Information.
.
;
;#define ESP_ADMIN_CRIT_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_GENERIC)
;//-----------------------------------------------------------------------------
MessageId   = 0xC001
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEER_SP_POST_FAIL
Language    = English
PBL: %2 is faulted. The fault cannot be isolated. status: %3(0x%4).
Please call your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEER_SP_POST_FAIL                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_PEER_SP_POST_FAIL)
;
;//-----------------------------------------------------------------------------
MessageId   = 0xC002
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEER_SP_POST_HUNG
Language    = English
PBL: %2 is in a hung state. Last status:%3(0x%4).
Please call your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEER_SP_POST_HUNG                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_PEER_SP_POST_HUNG)
;
;//-----------------------------------------------------------------------------
MessageId   = 0xC003
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEER_SP_POST_FAIL_BLADE
Language    = English
PBL: %2 is faulted. Fault Code: 0x%3. The SP and one or all of its components (CPU, I/O Module, I/O Carrier if applicable)
has failed and should be replaced. Please contact your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEER_SP_POST_FAIL_BLADE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_PEER_SP_POST_FAIL_BLADE)
;

;//-----------------------------------------------------------------------------
MessageId   = 0xC004
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEER_SP_POST_FAIL_FRU_PART_NUM
Language    = English
PBL: %2 is faulted. Fault Code: 0x%3. The Faulted FRU: %4 - Part Number: %5 should be replaced. Please contact your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEER_SP_POST_FAIL_FRU_PART_NUM                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_PEER_SP_POST_FAIL_FRU_PART_NUM)
;
;
;//-----------------------------------------------------------------------------

MessageId   = 0xC005
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEERSP_POST_FAIL_FRU
Language    = English
Peer Boot Logging: %2 is faulted. Recommend replacement of %3. Please contact your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEERSP_POST_FAIL_FRU                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_PEERSP_POST_FAIL_FRU)
;
;
;//-----------------------------------------------------------------------------

MessageId   = 0xC006
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_NUM
Language    = English
Peer Boot Logging: %2 is faulted. Recommend replacement of %3 %4. Please contact your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEERSP_POST_FAIL_FRU_SLOT_NUM                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_NUM)
;
;
;//-----------------------------------------------------------------------------

MessageId   = 0xC007
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEERSP_POST_FAIL_FRU_PART_NUM
Language    = English
Peer Boot Logging: %2 is faulted. Recommend replacement of %3 - Part Number: %4. Please contact your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEERSP_POST_FAIL_FRU_PART_NUM                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_PEERSP_POST_FAIL_FRU_PART_NUM)
;
;
;//-----------------------------------------------------------------------------

MessageId   = 0xC008
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_PART_NUM
Language    = English
Peer Boot Logging: %2 is faulted. Recommend replacement of %3 %4 - Part Number: %5. Please contact your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEERSP_POST_FAIL_FRU_SLOT_PART_NUM                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_PART_NUM)
;
;
;//-----------------------------------------------------------------------------

MessageId   = 0xC00A
Severity    = Error
;//NOSSPG C4type=User
Facility    = ESP
SymbolicName    = ESP_CRIT_PEERSP_POST_FAIL
Language    = English
Peer Boot Logging: %2 is faulted. %3. Please contact your service provider.
.
;
;#define ESP_ADMIN_CRIT_PEERSP_POST_FAIL                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(ESP_CRIT_PEERSP_POST_FAIL)
;
;
;//-----------------------------------------------------------------------------

;#endif
