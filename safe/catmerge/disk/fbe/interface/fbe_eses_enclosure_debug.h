#ifndef FBE_ESES_ENCLOSURE_DEBUG_H
#define FBE_ESES_ENCLOSURE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_eses_enclosure_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the eses enclosure debug library.
 *
 * @author
 *   12/9/2008:  Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_eses.h"
#include "../../disk/interface/fbe/fbe_enclosure_data_access_public.h"

#define MAX_FW_REV_STR_LEN       FBE_EDAL_FW_REV_STR_SIZE
typedef struct 
{
    fbe_bool_t              driveInserted;
    fbe_bool_t              driveFaulted;
    fbe_bool_t              drivePoweredOff;
    fbe_bool_t              driveBypassed ;
    fbe_bool_t              driveLoggedIn ;
    fbe_bool_t              phyReady;
}slot_flags_t;

typedef struct
{
    fbe_bool_t             driveLedFaulted;
    fbe_bool_t             driveLedMarked;
    fbe_bool_t             driveledOff;
    fbe_bool_t             driveInserted; 
}slot_led_flags_t;

char * fbe_eses_debug_decode_slot_state( slot_flags_t  *driveDataPtr);
char * fbe_eses_debug_decode_slot_Led_state( fbe_led_status_t *pEnclDriveFaultLeds);
fbe_status_t fbe_eses_enclosure_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr eses_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

fbe_status_t fbe_eses_enclosure_element_group_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr eses_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

fbe_status_t fbe_eses_enclosure_debug_prvt_data(const fbe_u8_t * module_name,
	                                       fbe_dbgext_ptr eses_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_prvt_data(void * enclPrvtData,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_fw_activating_status(fbe_bool_t inform_fw_activating, 
                                                   fbe_trace_func_t trace_func, 
                                                   fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_primary_port_entire_connector_index(fbe_u8_t port, 
                                                               fbe_trace_func_t trace_func, 
                                                               fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_encl_serial_number(char * serialNo, 
                                             fbe_trace_func_t trace_func, 
                                             fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_enclosure_type(fbe_u8_t enclosure_type, 
                                                   fbe_trace_func_t trace_func, 
                                                   fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_enclosureConfigBeingUpdated(fbe_bool_t enclConfUpdated, 
                                                                     fbe_trace_func_t trace_func, 
                                                                     fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_outstanding_write(fbe_u8_t outStandingWrite, 
                                                      fbe_trace_func_t trace_func, 
                                                      fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_emc_encl_ctrl_outstanding_write(fbe_u8_t outStandingWrite, 
                                                      fbe_trace_func_t trace_func, 
                                                      fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_mode_select_needed(fbe_u8_t mode_select_needed, 
                                                           fbe_trace_func_t trace_func, 
                                                           fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_test_mode_status(fbe_u8_t test_mode_status, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context);

void fbe_eses_enclosure_print_test_mode_rqstd(fbe_u8_t test_mode_rqstd, 
                                                   fbe_trace_func_t trace_func, 
                                                   fbe_trace_context_t trace_context);
void fbe_eses_enclosure_print_drive_info(void * enclosurePrvtInfo, 
                                                    fbe_trace_func_t trace_func, 
                                                    fbe_trace_context_t trace_context);

char* fbe_eses_enclosure_print_scsi_opcode(fbe_u32_t opcode);

char* fbe_eses_enclosure_print_power_action(fbe_u32_t power_action);

void fbe_eses_debug_getSpecificDriveData(fbe_edal_component_data_handle_t generalDataPtr,
                                                          fbe_u32_t index,
                                                          fbe_enclosure_types_t enclosure_type,
                                                          slot_flags_t *driveStat);

fbe_status_t fbe_eses_enclosure_debug_encl_stat(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr eses_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);
void
fbe_eses_getFwRevs(fbe_edal_component_data_handle_t generalDataPtr,
                   fbe_enclosure_types_t enclosure_type,
                   fbe_enclosure_component_types_t comp_type, 
                   fbe_base_enclosure_attributes_t comp_attr,
                                              char *fw_rev);

void
fbe_eses_encl_get_side_id(fbe_edal_component_data_handle_t generalDataPtr, 
                                                          fbe_enclosure_types_t enclosure_type,
                                                           fbe_u8_t *side_id);

char * fbe_eses_debug_decode_slot_led_state( slot_led_flags_t  *driveLedDataPtr);

#endif /* FBE_ESES_ENCLOSURE_DEBUG_H */

/*************************
 * end file fbe_eses_enclosure_debug.h
 *************************/
