/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_enclosure_dbg_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains the utility functions for the
 *  sas enclosure.
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
#include "fbe_sas_enclosure_debug.h"
#include "fbe_base_enclosure_debug.h"
#include"sas_enclosure_private.h"


/*!**************************************************************
 * @fn fbe_sas_enclosure_print_sasEnclosureSMPAddress(
 *                         fbe_sas_address_t address, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print sasEnclosureSMPAddress variable of sas
 *  enclosure.
 *
 * @param address - sasEnclosureSMPAddress.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_sas_enclosure_print_sasEnclosureSMPAddress(fbe_sas_address_t address, 
                                                    fbe_trace_func_t trace_func, 
                                                    fbe_trace_context_t trace_context)
{
    LARGE_INTEGER sas_addr;

    sas_addr.QuadPart = address;
    trace_func(trace_context, "sasEnclosureSMPAddress: 0x%8.8x_0x%8.8x\n", sas_addr.HighPart,sas_addr.LowPart);
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_print_smp_address_generation_code(
 *                         fbe_sas_address_t address, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print smp_address_generation_code variable of sas
 *  enclosure.
 *
 * @param code - smp_address_generation_code.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_sas_enclosure_print_smp_address_generation_code(fbe_generation_code_t code, 
                                                         fbe_trace_func_t trace_func, 
                                                         fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "smpAdressGnerationCode: %llu\n",
	       (unsigned long long)code);
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_print_sasEnclosureType(
 *                         fbe_u8_t EnclosureType, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print sasEnclosureType variable of sas
 *  enclosure.
 *
 * @param EnclosureType - sasEnclosureType.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return  enclTypeStr - String for enclosure type
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
char * fbe_sas_enclosure_print_sasEnclosureType(fbe_u8_t EnclosureType, 
                                                fbe_trace_func_t trace_func, 
                                                fbe_trace_context_t trace_context)
{
    char * enclTypeStr = NULL;    

    switch(EnclosureType)
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
        case FBE_SAS_ENCLOSURE_TYPE_VIKING:
            enclTypeStr = (char *)("VIKING (UNKNOWN SXP)");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP :
            enclTypeStr = (char *)("VIKING_IOSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP :
            enclTypeStr = (char *)("VIKING_DRVSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE:
            enclTypeStr = (char *)("CAYENNE (UNKNOWN SXP)");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP :
            enclTypeStr = (char *)("CAYENNE_IOSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP :
        enclTypeStr = (char *)("CAYENNE_DRVSXP");
        break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA:
            enclTypeStr = (char *)("NAGA (UNKNOWN SXP)");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP :
            enclTypeStr = (char *)("NAGA_IOSXP");
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP :
            enclTypeStr = (char *)("NAGA_DRVSXP");
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
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO :
            enclTypeStr = (char *)("ANCHO");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN :
            enclTypeStr = (char *)("RAMHORN");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RHEA :
            enclTypeStr = (char *)("RHEA");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA :
            enclTypeStr = (char *)("MIRANDA");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO :
            enclTypeStr = (char *)("CALYPSO");  
            break;
        case FBE_SAS_ENCLOSURE_TYPE_INVALID :
            enclTypeStr = (char *)("INVALID");
            break;   
        case FBE_SAS_ENCLOSURE_TYPE_UNKNOWN :
            enclTypeStr = (char *)("UNKNOWN");
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
    
    return (enclTypeStr);
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_print_sasEnclosureProductType(
 *                         fbe_u8_t ProductType, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print sasEnclosureProductType variable of sas
 *  enclosure.
 *
 * @param ProductType - sasEnclosureProductType.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_sas_enclosure_print_sasEnclosureProductType(fbe_u8_t ProductType, 
                                                     fbe_trace_func_t trace_func, 
                                                     fbe_trace_context_t trace_context)
{

    trace_func(trace_context, "sasEnclosureProductType :");   

    switch(ProductType)
    {
    case FBE_SAS_ENCLOSURE_PRODUCT_TYPE_ESES :
      trace_func(trace_context, "%s\n", "ESES");    
        break;
    case FBE_SAS_ENCLOSURE_PRODUCT_TYPE_INVALID :
     trace_func(trace_context, "%s\n", "INVALID");        
        break;
    case FBE_SAS_ENCLOSURE_PRODUCT_TYPE_LAST :
    trace_func(trace_context, "%s\n", "LAST");
       break;
    default:
     trace_func(trace_context, "%s\n", "NA");       
        break;
    }

}

/*!**************************************************************
 * @fn fbe_sas_enclosure_print_ses_port_address(
 *                         fbe_sas_address_t portAddress, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print ses_port_address variable of sas
 *  enclosure.
 *
 * @param portAddress - ses_port_address.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_sas_enclosure_print_ses_port_address(fbe_sas_address_t portAddress, 
                                              fbe_trace_func_t trace_func, 
                                              fbe_trace_context_t trace_context)

{
    LARGE_INTEGER sas_addr;

    sas_addr.QuadPart = portAddress;
    trace_func(trace_context, "sesPortAddress: 0x%8.8x_0x%8.8x\n", sas_addr.HighPart,sas_addr.LowPart);
}

/*!**************************************************************
 * @fn fbe_sas_enclosure_print_ses_port_address_generation_code(
 *                         fbe_generation_code_t address, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print ses_port_address_generation_code variable of sas
 *  enclosure.
 *
 * @param address - ses_port_address_generation_code.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_sas_enclosure_print_ses_port_address_generation_code(fbe_generation_code_t address, 
                                                              fbe_trace_func_t trace_func, 
                                                              fbe_trace_context_t trace_context)
{

    trace_func(trace_context, "sesPortAddressGenerationCode: %llu\n",
	       (unsigned long long)address);

}
// End of file fbe_eses_enclosure_utils.c
