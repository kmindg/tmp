/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_dbg_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains the utility functions used by the 
 *  ESES enclosure object.
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   02-May-2009 Dipak Patel - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_eses_enclosure_config.h"
#include "edal_eses_enclosure_data.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_library_interface.h"

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_drivePowerCyclePollCount(
 *                         fbe_u32_t count, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print drivePowerCyclePollCount variable of eses
 *  enclosure.
 *
 * @param count - drivePowerCyclePollCount.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_drivePowerCyclePollCount(fbe_u32_t count,
                                                                fbe_trace_func_t trace_func, 
                                                                fbe_trace_context_t trace_context)
{    
     trace_func(trace_context, "drivePowerCyclePollCount: %d\n", count);     
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_enclosure_type(
 *                         char * enclosure_type, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print enclosure type
 *
 * @param enclosure_type.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   02/08/2011:  Created. Rashmi Sawale.
 *
 ****************************************************************/
void fbe_eses_enclosure_print_enclosure_type(fbe_u8_t enclosure_type, 
                                                   fbe_trace_func_t trace_func, 
                                                   fbe_trace_context_t trace_context)
{
    char * enclTypeStr = NULL;    

    switch(enclosure_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_BULLET :
            enclTypeStr = (char *)("BULLET"); 
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIPER :
            enclTypeStr = (char *)("VIPER");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM :
            enclTypeStr = (char *)("MAGNUM"); 
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL :
            enclTypeStr = (char *)("CITADEL");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER :
            enclTypeStr = (char *)("BUNKER");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER :
            enclTypeStr = (char *)("DERRINGER");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO :
            enclTypeStr = (char *)("TABASCO");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM :
            enclTypeStr = (char *)("VOYAGER_ICM");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE :
            enclTypeStr = (char *)("VOYAGER_EE");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK :
            enclTypeStr = (char *)("FALLBACK");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD :
            enclTypeStr = (char *)("BOXWOOD");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_KNOT :
            enclTypeStr = (char *)("KNOT");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE :
            enclTypeStr = (char *)("PINECONE");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW :
            enclTypeStr = (char *)("STEELJAW");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN :
            enclTypeStr = (char *)("RAMHORN");  
            break;		
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO :
            enclTypeStr = (char *)("ANCHO");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO :
            enclTypeStr = (char *)("CALYPSO");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RHEA :
            enclTypeStr = (char *)("RHEA");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA :
            enclTypeStr = (char *)("MIRANDA");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_INVALID :
            enclTypeStr = (char *)("INVALID");
            break;   
        case FBE_SAS_ENCLOSURE_TYPE_UNKNOWN :
            enclTypeStr = (char *)("UNKNOWN");
            break;   
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP :
            enclTypeStr = (char *)("VIKING_IOSXP");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP :
            enclTypeStr = (char *)("VIKING_DRVSXP");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP :
            enclTypeStr = (char *)("CAYENNE_IOSXP");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP :
            enclTypeStr = (char *)("CAYENNE_DRVSXP");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP :
            enclTypeStr = (char *)("NAGA_IOSXP");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP :
            enclTypeStr = (char *)("NAGA_DRVSXP");  
            break;
        default:
            enclTypeStr = (char *)("NA");  
        break;
    }

    if(trace_func)
    {
        trace_func(trace_context, "sasEnclosureType :");
        trace_func(trace_context, "%s\n", enclTypeStr);
    }
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_fw_activating_status(
 *                         fbe_bool_t inform_fw_activating, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print fw activating related info
 *
 * @param inform_fw_activating.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_fw_activating_status(fbe_bool_t inform_fw_activating, 
                                                   fbe_trace_func_t trace_func, 
                                                   fbe_trace_context_t trace_context)
{
     trace_func(trace_context, "notify upstream enclosure fw activating fw: %d\n", inform_fw_activating);
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_primary_port_entire_connector_index(
 *                         fbe_u8_t port, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print primary_port_entire_connector_index variable of eses
 *  enclosure.
 *
 * @param port - primary_port_entire_connector_index.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_primary_port_entire_connector_index(fbe_u8_t port, 
                                                                            fbe_trace_func_t trace_func, 
                                                                            fbe_trace_context_t trace_context)
{
     trace_func(trace_context, "primaryPortEntireConnectorIndex: %d\n", port);
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_encl_serial_number(
 *                         char * serialNo, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print encl_serial_number variable of eses
 *  enclosure.
 *
 * @param serialNo - char pointer to the encl_serial_number.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_encl_serial_number(char * serialNo, 
                                                        fbe_trace_func_t trace_func, 
                                                         fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "emcSerialNumber: 0x%s\n", serialNo);
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_enclosureConfigBeingUpdated(
 *                         fbe_bool_t enclConfUpdated, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print enclosureConfigBeingUpdated variable of eses
 *  enclosure.
 *
 * @param enclConfUpdated - Bool Value to enclosureConfigBeingUpdated.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_enclosureConfigBeingUpdated(fbe_bool_t enclConfUpdated, 
                                                                    fbe_trace_func_t trace_func, 
                                                                    fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "enclosureConfigBeingUpdated: %s\n", (enclConfUpdated == TRUE)? "YES" : "NO");
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_outstanding_write(
 *                         fbe_u8_t outStandingWrite, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print outstanding_write variable of eses
 *  enclosure.
 *
 * @param outStandingWrite - outstanding_write.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_outstanding_write(fbe_u8_t outStandingWrite, 
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "outstandingWrite: %d\n", outStandingWrite);
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_emc_encl_ctrl_outstanding_write(
 *                         fbe_u8_t outStandingWrite, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print EMC encl control outstanding write.
 *
 * @param outStandingWrite - outstanding_write.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   20-May-2009:  PHE - Created.
 *
 ****************************************************************/
void fbe_eses_enclosure_print_emc_encl_ctrl_outstanding_write(fbe_u8_t outStandingWrite, 
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "emcEnclCtrlOutstandingWrite: %d\n", outStandingWrite);
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_mode_select_needed(
 *                         fbe_u8_t mode_select_needed, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print mode_select_needed variable of eses
 *  enclosure.
 *
 * @param mode_select_needed - mode_select_needed.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_mode_select_needed(fbe_u8_t mode_select_needed, 
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "modeSelectNeeded: %s\n", (mode_select_needed == TRUE)? "YES" : "NO");
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_test_mode_status(
 *                         fbe_u8_t test_mode_status, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print test_mode_status variable of eses
 *  enclosure.
 *
 * @param test_mode_status - test_mode_status.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_test_mode_status(fbe_u8_t test_mode_status, 
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)
{
     trace_func(trace_context, "testModeStatus: %s\n", (test_mode_status == TRUE)? "ON" : "OFF");
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_test_mode_rqstd(
 *                         fbe_u8_t test_mode_rqstd, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print test_mode_rqstd variable of eses
 *  enclosure.
 *
 * @param test_mode_rqstd - test_mode_rqstd.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_test_mode_rqstd(fbe_u8_t test_mode_rqstd, 
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "testModeRqstd: %s\n", (test_mode_rqstd == TRUE)? "ON" : "OFF");
}
// End of file fbe_eses_enclosure_utils.c


/*!*************************************************************************
 *                         @fn fbe_eses_debug_getSpecificDriveData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get Specific Drive/Slot component data for 
 *      physical package debugger extension.
 *
 *  PARAMETERS:
 *      componentTypeDataPtr - pointer to drive component;
 *      index - index of element to be printed
 *      enclosure_type - enclosure type
 *      driveStatPtr - pointer to drive data
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    05/19/2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
void
fbe_eses_debug_getSpecificDriveData(fbe_edal_component_data_handle_t generalDataPtr,
                                                          fbe_u32_t index,
                                                          fbe_enclosure_types_t enclosure_type,
                                                          slot_flags_t *driveStatPtr)
{
    fbe_edal_status_t           status;

    status = fbe_edal_getBool_direct(generalDataPtr,
                            enclosure_type,
                            FBE_ENCL_COMP_INSERTED,
                            FBE_ENCL_DRIVE_SLOT,
                            &(driveStatPtr[index].driveInserted));

    //trace_func(trace_context,"\ndriveInserted:%d",driveStatPtr[index].driveInserted);

    status = fbe_edal_getBool_direct(generalDataPtr,
	  	                enclosure_type,
                                FBE_ENCL_COMP_FAULTED,
                                FBE_ENCL_DRIVE_SLOT,
                                &(driveStatPtr[index].driveFaulted));

    //trace_func(trace_context," driveFaulted:%d",driveStatPtr[index].driveFaulted); // debug only

    status = fbe_edal_getBool_direct(generalDataPtr,
	  	                        enclosure_type,
                                FBE_ENCL_COMP_POWERED_OFF,
                                FBE_ENCL_DRIVE_SLOT,
                                &(driveStatPtr[index].drivePoweredOff));
    //trace_func(trace_context," drivePoweredOff:%d",driveStatPtr[index].drivePoweredOff); // debug only

    status = fbe_edal_getBool_direct(generalDataPtr,
                            enclosure_type,
                            FBE_ENCL_DRIVE_BYPASSED,
                            FBE_ENCL_DRIVE_SLOT,
                            &(driveStatPtr[index].driveBypassed));
   
     status = fbe_edal_getBool_direct(generalDataPtr,
                            enclosure_type,
                            FBE_ENCL_DRIVE_LOGGED_IN,
                            FBE_ENCL_DRIVE_SLOT,
                            &(driveStatPtr[index].driveLoggedIn));

     
     
    //trace_func(trace_context," drivePoweredOff:%d",driveStatPtr[index].drivePoweredOff); // debug only

   
}

/*!*************************************************************************
 * @fn fbe_eses_debug_decode_slot_state
 **************************************************************************
 *
 *  @brief
 *      This function retun drive stat into text format.
 *
 *  @param   slot_flags_t  *driveDataPtr
 *
 *  @return    char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-Mar-2009: Dipak Patel- created.
 *
 **************************************************************************/
char * fbe_eses_debug_decode_slot_state( slot_flags_t  *driveDataPtr)
{
   char *string = NULL;
   
   if(driveDataPtr->driveInserted == FALSE)
   {
      string = "REM";
   }
   else if((driveDataPtr->driveInserted  == TRUE) &&
           (driveDataPtr->drivePoweredOff== TRUE))
   {
      string = "OFF";
   }
   else if((driveDataPtr->driveInserted  == TRUE  ) &&
    (driveDataPtr->driveBypassed == TRUE) )

   {
      string = "BYP" ;
   }
   else if((driveDataPtr->driveInserted  == TRUE  ) && 
    ((driveDataPtr->driveBypassed == FALSE  ) &&
    (driveDataPtr->driveLoggedIn== FALSE ) ) )

   {
      string = "NRY" ;
   }
   else if((driveDataPtr->driveInserted == TRUE) &&
           (driveDataPtr->driveFaulted == FALSE)&&
           (driveDataPtr->drivePoweredOff == FALSE)&&
           (driveDataPtr->driveBypassed == FALSE))
   {
      string = "RDY";
   }
   else if((driveDataPtr->driveInserted == TRUE) &&
           ((driveDataPtr->drivePoweredOff == FALSE)&&
           (driveDataPtr->driveFaulted == TRUE)))
          
          
   {
      string = "FLT";
   }
   else{
      string = "N/A" ;
    }
   return(string);
}
/*!*************************************************************************
 * @fn fbe_eses_debug_decode_slot_Led_state
 **************************************************************************
 *
 *  @brief
 *      This function retun drive Led into text format.
 *
 *  @param   slot_flags_t  *driveDataPtr
 *
 *  @return    char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-Mar-2009: Dipak Patel- created.
 *
 **************************************************************************/
char * fbe_eses_debug_decode_slot_Led_state( fbe_led_status_t *pEnclDriveFaultLeds)
{
   char *string = NULL;
   
   if(*pEnclDriveFaultLeds == FBE_LED_STATUS_OFF)
   {
      string = "OFF " ;
   }
   else if(*pEnclDriveFaultLeds == FBE_LED_STATUS_ON)
   {
      string = "FLT ";
   }
   else if(*pEnclDriveFaultLeds == FBE_LED_STATUS_MARKED)

   {
      string = "MRK " ;
   }
   else{
      string = "--- ";
    }
   return(string);
}
/*!*************************************************************************
 *                         @fn fbe_eses_getFwRevs
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data for each firmware component.
 *
 *  PARAMETERS:
 *      componentTypeDataPtr - pointer to the enclosure components
 *
 *  HISTORY:
 *   05/19/2009: Nayana Chaudhari - Created
 *   04/29/2010: NCHIU - modified to retrieve fw info from associated component.
 *
 **************************************************************************/
void
fbe_eses_getFwRevs(fbe_edal_component_data_handle_t generalDataPtr,
                   fbe_enclosure_types_t enclosure_type,
                   fbe_enclosure_component_types_t comp_type, 
                   fbe_base_enclosure_attributes_t comp_attr,
                                              char *fw_rev)
{
   fbe_edal_fw_info_t fw_info;
    fbe_edal_status_t      status;
                  
   status = fbe_edal_getStr_direct(generalDataPtr,
                                    enclosure_type,
                                    comp_attr, 
                                    comp_type,
                                    sizeof(fbe_edal_fw_info_t),
                                    (char *)&fw_info);
   if (fw_info.valid)
   {
       strncpy(fw_rev, fw_info.fwRevision, FBE_EDAL_FW_REV_STR_SIZE);
   }
   else
   {
       strncpy(fw_rev, "INVLD", FBE_EDAL_FW_REV_STR_SIZE);
   }
   
}
/*!*************************************************************************
 *                         @fn fbe_eses_encl_get_side_id
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get side_id.
 *
 *  PARAMETERS:
 *      generalDataPtr - pointer to the enclosure components
 *      enclosure_type 
 *
 *  HISTORY:
 *   11/03/2009: - Created
 *
 **************************************************************************/
void
fbe_eses_encl_get_side_id(fbe_edal_component_data_handle_t generalDataPtr, 
                                                          fbe_enclosure_types_t enclosure_type,
                                                          fbe_u8_t *side_id)
{
 fbe_edal_status_t           status;

    status = fbe_edal_getU8_direct(generalDataPtr,
                                   enclosure_type,
                                   FBE_ENCL_COMP_SIDE_ID,
                                   FBE_ENCL_ENCLOSURE,
                                   side_id);
}
/*!*************************************************************************
* @fn fbe_eses_debug_decode_slot_led_state
**************************************************************************
*
*  @brief
*      This function retun drive stat into text format.
*
*  @param   slot_led_flags_t  *driveDataPtr
*
*  @return    char * - string
*
*  NOTES:
*
*  HISTORY:
*    23-July-2010: Sujay Ranjan- created.
*
**************************************************************************/
char * fbe_eses_debug_decode_slot_led_state( slot_led_flags_t  *driveLedDataPtr)
{
    char *string = NULL;

    if(driveLedDataPtr->driveInserted == FALSE)
    {
        string = "---";
    }
    else if(driveLedDataPtr->driveLedFaulted == TRUE)
    {
        string = "FLT";
    }
    else if(driveLedDataPtr->driveLedMarked == TRUE)

    {
        string = "MAR" ;
    }
    else{
        string = "OFF" ;
    }
    return(string);
}
