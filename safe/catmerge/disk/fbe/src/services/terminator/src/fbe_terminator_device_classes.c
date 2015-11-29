/**************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/*!**************************************************************************
 * @file fbe_terminator_device_classes.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an implementation of TCM classes representing main 
 *  Terminator devices - board, port, enclosure and drive.
 *
 * @version
 *   01/26/2009:  Alanov Alexander created
 * 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_terminator.h"
#include "terminator_port.h"
#include "terminator_fc_port.h"
#include "terminator_enclosure.h"
#include "terminator_fc_enclosure.h"
#include "terminator_drive.h"
#include "terminator_fc_drive.h"
#include "terminator_board.h"
#include "terminator_enum_conversions.h"
#include "fbe_terminator_class_management.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator_device_classes.h"
#include "fbe/fbe_terminator_api.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#define RETURN_FAILURE_ON_BAD_TCM_STATUS(status) if(status != TCM_STATUS_OK) { return FBE_STATUS_GENERIC_FAILURE; } 

static tcm_status_t initialize_board_accessors(tcm_reference_t class_handle);

static tcm_status_t initialize_port_accessors(tcm_reference_t class_handle);

static tcm_status_t initialize_enclosure_accessors(tcm_reference_t class_handle);

static tcm_status_t initialize_drive_accessors(tcm_reference_t class_handle);

static tcm_status_t terminator_fc_port_attribute_setter(fbe_terminator_device_ptr_t port_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value);

/* Initialization stuff */
/* function used to initialize class management */
fbe_status_t terminator_device_classes_initialize(void)
{
    tcm_status_t tcm_status;
    tcm_reference_t board_class_handle;
    tcm_reference_t port_class_handle;
    tcm_reference_t enclosure_class_handle;
    tcm_reference_t drive_class_handle;
    /* create classes for main Terminator devices */
    /* Board */
    tcm_status = fbe_terminator_class_management_class_create("Board", 
        terminator_board_allocator, NULL, &board_class_handle);
    RETURN_FAILURE_ON_BAD_TCM_STATUS(tcm_status)
    /* Port */
    tcm_status = fbe_terminator_class_management_class_create("Port", 
        terminator_port_allocator, NULL, &port_class_handle);
    RETURN_FAILURE_ON_BAD_TCM_STATUS(tcm_status)
    /* Enclosure */
    tcm_status = fbe_terminator_class_management_class_create("Enclosure", 
        terminator_enclosure_allocator, NULL, &enclosure_class_handle);
    RETURN_FAILURE_ON_BAD_TCM_STATUS(tcm_status);

    /* Drive */
    tcm_status = fbe_terminator_class_management_class_create("Drive", 
																terminator_drive_allocator, 
																terminator_drive_destroyer, 
																&drive_class_handle);

    RETURN_FAILURE_ON_BAD_TCM_STATUS(tcm_status)
    /* initialize accessors for main Terminator devices properties */
    /* Board */
    tcm_status = initialize_board_accessors(board_class_handle);
    RETURN_FAILURE_ON_BAD_TCM_STATUS(tcm_status)
    /* Port */
    tcm_status = initialize_port_accessors(port_class_handle);
    RETURN_FAILURE_ON_BAD_TCM_STATUS(tcm_status)
    /* Enclosure */
    tcm_status = initialize_enclosure_accessors(enclosure_class_handle);
    RETURN_FAILURE_ON_BAD_TCM_STATUS(tcm_status)
    /* Drive */
    tcm_status = initialize_drive_accessors(drive_class_handle);
    RETURN_FAILURE_ON_BAD_TCM_STATUS(tcm_status)
    
    return FBE_STATUS_OK;
}

static const char * board_attributes[] = {"Board_type", "Platform_type", "sp_id", "SN"};

static int number_of_board_attributes = sizeof(board_attributes)/sizeof(const char *);

static const char * port_attributes[] = {"Port_type", "IO_port_number",
                                        "Portal_number", "Backend_number",
                                        "Sas_address", "Diplex_address", "SN", "Serial_number",
                                        "sp_id", "module_id", "IOModule_type","Connect_class",
                                        "Port_role","Vendor_ID","Device_ID",
                                        "Hardware_bus_number","Hardware_bus_slot","Hardware_bus_function"};

static int number_of_port_attributes = sizeof(port_attributes)/sizeof(const char *);

static const char * enclosure_attributes[] = {"Enclosure_type", 
                                        "BE", "Backend_number",
                                        "Enc", "Enclosure_number",
                                        "Sas_address", "Diplex_address",
                                        "SN", "Serial_number", "UID",
                                        "Logical_parent_type", "connector_id"};

static int number_of_enclosure_attributes = sizeof(enclosure_attributes)/sizeof(const char *);

static const char * drive_attributes[] = {"Drive_type",
                                  "BE", "Backend_number",
                                  "Enc", "Enclosure_number",
                                  "Slot", "Slot_number",
                                  "Sas_address", "Diplex_address",
                                  "SN", "Serial_number",
                                  "PID", "Product_id",
                                  "BlockSize", "MaxLBA",
                                  "Size"}; /* Size is ignored */

static int number_of_drive_attributes = sizeof(drive_attributes)/sizeof(const char *);

/* function used to initialize accessors of some device in more convenient way:
    - attributes are listed in array of given size;
    - the same accessors are used for all attributes (they should have a big switch inside)
    - IDs of attributes are assigned automatically as a number of attribute in array

*/
static tcm_status_t initialize_device_accessors(tcm_reference_t class_handle,
                    const char ** attributes, int number_of_attributes,
                    terminator_attribute_string_string_setter string_string_setter,
                    terminator_attribute_string_string_getter string_string_getter,
                    terminator_attribute_uint_string_setter   uint_string_setter,
                    terminator_attribute_uint_string_getter   uint_string_getter,
                    terminator_attribute_uint_uint_getter     uint_uint_getter)
{
    tcm_status_t tcm_status = TCM_STATUS_OK;
    int i = 0;
    for(i = 0; i < number_of_attributes; i++)
    {
        tcm_status = fbe_terminator_class_management_class_accessor_add(class_handle,
            attributes[i],
            i,
            string_string_setter,
            string_string_getter,
            uint_string_setter,
            uint_string_getter,
            uint_uint_getter);
        if(tcm_status != TCM_STATUS_OK)
        {
            return tcm_status;
        }
    }
    return tcm_status;
}

static tcm_status_t initialize_board_accessors(tcm_reference_t class_handle)
{
    tcm_status_t tcm_status;
    tcm_status = initialize_device_accessors(class_handle,
        board_attributes,
        number_of_board_attributes,
        terminator_board_attribute_setter,
        terminator_board_attribute_getter,
        NULL,
        NULL,
        NULL); /* last three are not used*/
    return tcm_status;
}

static tcm_status_t initialize_port_accessors(tcm_reference_t class_handle)
{
    tcm_status_t tcm_status;
    tcm_status = initialize_device_accessors(class_handle,
        port_attributes,
        number_of_port_attributes,
        terminator_port_attribute_setter,
        terminator_port_attribute_getter,
        NULL,
        NULL,
        NULL); /* last three are not used*/
    return tcm_status;
}

static tcm_status_t initialize_enclosure_accessors(tcm_reference_t class_handle)
{
    tcm_status_t tcm_status;
    tcm_status = initialize_device_accessors(class_handle,
        enclosure_attributes,
        number_of_enclosure_attributes,
        terminator_enclosure_attribute_setter,
        terminator_enclosure_attribute_getter,
        NULL,
        NULL,
        NULL); /* last three are not used*/
    return tcm_status;
}

static tcm_status_t initialize_drive_accessors(tcm_reference_t class_handle)
{
    tcm_status_t tcm_status;
    tcm_status = initialize_device_accessors(class_handle,
        drive_attributes,
        number_of_drive_attributes,
        terminator_drive_attribute_setter,
        terminator_drive_attribute_getter,
        NULL,
        NULL,
        NULL); /* last three are not used*/
    return tcm_status;
}
/* Allocators */

/* allocator for board */
tcm_status_t terminator_board_allocator(const char * type, tcm_reference_t * device)
{
    fbe_status_t status = terminator_create_board(device);
    /*TODO: should we do something with type? */
    if(status != FBE_STATUS_OK)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    else
    {
        return TCM_STATUS_OK;
    }
}


/* allocator for port */
tcm_status_t terminator_port_allocator(const char * type, tcm_reference_t * device)
{
    fbe_status_t status = terminator_create_port(type, device);
    fbe_port_type_t port_type;
   
    if(status != FBE_STATUS_OK)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    else
    {
        if(type == NULL || (!strcmp(type, "")))
        {
            /* default to the SAS Port */
            port_type = FBE_PORT_TYPE_SAS_PMC;
        }
        else
        {           
             /* convert string to appropriate port type */
             if((status = terminator_string_to_port_type(type, &port_type))
                  != FBE_STATUS_OK)
            {
                 return TCM_STATUS_GENERIC_FAILURE;
            }
        
        }
        /* set port type */
        status = port_set_type((terminator_port_t *)(*device), port_type);
        /* initialize protocol specific structures */
        status = terminator_port_initialize_protocol_specific_data((terminator_port_t *)(*device));
        return (status == FBE_STATUS_OK) ? TCM_STATUS_OK : TCM_STATUS_GENERIC_FAILURE;
    }
}


/* allocator for enclosure */
tcm_status_t terminator_enclosure_allocator(const char * type, tcm_reference_t * device)
{
    fbe_status_t status = terminator_create_enclosure(type, device);
    fbe_enclosure_type_t encl_type;
    
    /*TODO: should we do something with type? */
    if(status != FBE_STATUS_OK)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    else
    {
       
        if(type == NULL || (!strcmp(type, "")))
        {
            /* default to the SAS enclosure */
            encl_type = FBE_ENCLOSURE_TYPE_SAS;
        }
        else
        {           
             /* convert string to appropriate enclosure type */
             if((status = terminator_string_to_enclosure_type(type, &encl_type))
                  != FBE_STATUS_OK)
            {
                 return TCM_STATUS_GENERIC_FAILURE;
            }
        
        }
        /* set enclosure protocol */
        status = enclosure_set_protocol((terminator_enclosure_t *)(*device), encl_type);
        /* initialize protocol specific structures */
        status = terminator_enclosure_initialize_protocol_specific_data((terminator_enclosure_t *)(*device));
        return (status == FBE_STATUS_OK) ? TCM_STATUS_OK : TCM_STATUS_GENERIC_FAILURE;
        
    }
}
/* allocator for drive */
/* type must be specified:
   "FBE_DRIVE_TYPE_SAS" or NULL (for backward compatibility) - for SAS drive
   "FBE_DRIVE_TYPE_SATA" - for SATA drive 
   other types are not supported at this time
*/
tcm_status_t terminator_drive_allocator(const char * type, tcm_reference_t * device)
{
    fbe_status_t status = terminator_create_drive(device);
    if(status != FBE_STATUS_OK)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* initialize type-specific structures */
        fbe_drive_type_t drive_type;
        if(type == NULL || (!strcmp(type, "")))
        {
            /* default to the SAS drive */
            drive_type = FBE_DRIVE_TYPE_SAS;
        }
        else
        {
                /* convert string to appropriate drive type */
                if((status = terminator_string_to_fbe_drive_type(type, &drive_type))
                    != FBE_STATUS_OK)
                {
                    return TCM_STATUS_GENERIC_FAILURE;
                }
        
        }
        /* set drive protocol */
        drive_set_protocol((terminator_drive_t *)(*device), drive_type);
        /* initialize protocol specific structures */
        status = terminator_drive_initialize_protocol_specific_data((terminator_drive_t *)(*device));
        return (status == FBE_STATUS_OK) ? TCM_STATUS_OK : TCM_STATUS_GENERIC_FAILURE;
    }
}

tcm_status_t terminator_drive_destroyer(const char *type, tcm_reference_t device)
{
	terminator_destroy_drive(device);
	return TCM_STATUS_OK;
}


/* Getters and Setters */
/* currently only "string-string accessors" are implemented */

/* setters and getters for board */
tcm_status_t terminator_board_attribute_setter(fbe_terminator_device_ptr_t board_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    fbe_board_type_t board_type;
    SPID_HW_TYPE platform_type;

    if(board_handle == NULL || attribute_name == NULL || attribute_value == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }

    if(!strcmp(attribute_name, "sp_id"))
    {
        return TCM_STATUS_OK; /* Do nothing */
    }

    if(!strcmp(attribute_name, "SN"))
    {
        return TCM_STATUS_OK; /* Also do nothing */
    }

    if(!strcmp(attribute_name, "Board_type"))
    {
    if(terminator_string_to_board_type(attribute_value, &board_type)
        != FBE_STATUS_OK )
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    if(set_board_type((terminator_board_t *)board_handle, board_type) != FBE_STATUS_OK)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    }

    if(!strcmp(attribute_name, "Platform_type"))
    {
        if(terminator_string_to_platform_type(attribute_value, &platform_type)
            != FBE_STATUS_OK )
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        if(set_platform_type((terminator_board_t *)board_handle, platform_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }

    return TCM_STATUS_OK;
}


tcm_status_t terminator_board_attribute_getter(fbe_terminator_device_ptr_t board_handle,
                                                    const char * attribute_name,
                                                    char * buffer,
                                                    tcm_uint_t buffer_len)
{
    /* not needed now and not implemented */
    return TCM_STATUS_NOT_IMPLEMENTED;
}


/* for SAS port */
static tcm_status_t terminator_sas_port_attribute_setter(fbe_terminator_device_ptr_t port_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    if(port_handle == NULL || attribute_name == NULL || attribute_value == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* ignored attributes */
    if(!strcmp(attribute_name, "Serial_number") || !strcmp(attribute_name, "SN")
        || !strcmp(attribute_name, "sp_id")
        || !strcmp(attribute_name, "module_id")
        || !strcmp(attribute_name, "IOModule_type")
        || !strcmp(attribute_name, "Alpa_address"))
    {
        return TCM_STATUS_OK;
    }
    /* port type */
    if(!strcmp(attribute_name, "Port_type"))
    {
        fbe_port_type_t port_type;
        if(terminator_string_to_port_type(attribute_value, &port_type)
            != FBE_STATUS_OK )
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(port_set_type((terminator_port_t *)port_handle, port_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }
    /* IO port number - currently for SAS port only!!! */
    if(!strcmp(attribute_name, "IO_port_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sas_port_set_io_port_number((terminator_port_t *)port_handle, (fbe_u32_t)val); 
        return TCM_STATUS_OK;
    }
    /* Portal number - currently for SAS port only!!! */
    if(!strcmp(attribute_name, "Portal_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sas_port_set_portal_number((terminator_port_t *)port_handle, (fbe_u32_t)val); 
        return TCM_STATUS_OK;
    }
    /* Backend number - currently for SAS port only!!! */
    if(!strcmp(attribute_name, "Backend_number"))
    {
        fbe_u64_t val;
        fbe_u32_t port_number;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        port_number = (fbe_u32_t)val;
        //base_component_assign_id(&(((terminator_port_t *)port_handle)->base), port_number);
        sas_port_set_backend_number((terminator_port_t *)port_handle, port_number);
        return TCM_STATUS_OK;
    }
    /* SAS address - for SAS port only */
    if(!strcmp(attribute_name, "Sas_address"))
    {
        fbe_u64_t val;
        if(terminator_hexadecimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sas_port_set_address((terminator_port_t *)port_handle, (fbe_sas_address_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Connect_class"))
    {
        fbe_port_connect_class_t port_connect_class;
        if(terminator_string_to_port_connect_class(attribute_value, &port_connect_class)
            != FBE_STATUS_OK )
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(port_set_connect_class((terminator_port_t *)port_handle, port_connect_class) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Port_role"))
    {
        fbe_port_role_t port_role;
        if(terminator_string_to_port_role(attribute_value, &port_role)
            != FBE_STATUS_OK )
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(port_set_role((terminator_port_t *)port_handle, port_role) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}


/* for FC port */
static tcm_status_t terminator_fc_port_attribute_setter(fbe_terminator_device_ptr_t port_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    if(port_handle == NULL || attribute_name == NULL || attribute_value == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* ignored attributes */
    if(!strcmp(attribute_name, "Serial_number") || !strcmp(attribute_name, "SN")
        || !strcmp(attribute_name, "sp_id")
        || !strcmp(attribute_name, "module_id")
        || !strcmp(attribute_name, "IOModule_type")
        || !strcmp(attribute_name, "Alpa_address"))
    {
        return TCM_STATUS_OK;
    }
    /* port type */
    if(!strcmp(attribute_name, "Port_type"))
    {
        fbe_port_type_t port_type;
        if(terminator_string_to_port_type(attribute_value, &port_type)
            != FBE_STATUS_OK )
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(port_set_type((terminator_port_t *)port_handle, port_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }
    /* IO port number - currently for SAS port only!!! */
    if(!strcmp(attribute_name, "IO_port_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        fc_port_set_io_port_number((terminator_port_t *)port_handle, (fbe_u32_t)val); 
        return TCM_STATUS_OK;
    }
    /* Portal number - currently for SAS port only!!! */
    if(!strcmp(attribute_name, "Portal_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        fc_port_set_portal_number((terminator_port_t *)port_handle, (fbe_u32_t)val); 
        return TCM_STATUS_OK;
    }
    /* Backend number - currently for SAS port only!!! */
    if(!strcmp(attribute_name, "Backend_number"))
    {
        fbe_u64_t val;
        fbe_u32_t port_number;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        port_number = (fbe_u32_t)val;
        //base_component_assign_id(&(((terminator_port_t *)port_handle)->base), port_number);
        fc_port_set_backend_number((terminator_port_t *)port_handle, port_number);
        return TCM_STATUS_OK;
    }    
    /* Diplex address */
    if(!strcmp(attribute_name, "Diplex_address"))
    {
        fbe_u64_t val;
        if(terminator_hexadecimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        fc_port_set_address((terminator_port_t *)port_handle, (fbe_diplex_address_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Connect_class"))
    {
        fbe_port_connect_class_t port_connect_class;
        if(terminator_string_to_port_connect_class(attribute_value, &port_connect_class)
            != FBE_STATUS_OK )
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        if(port_set_connect_class((terminator_port_t *)port_handle, port_connect_class) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Port_role"))
    {
        fbe_port_role_t port_role;
        if(terminator_string_to_port_role(attribute_value, &port_role)
            != FBE_STATUS_OK )
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        if(port_set_role((terminator_port_t *)port_handle, port_role) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

tcm_status_t terminator_port_attribute_setter(fbe_terminator_device_ptr_t port_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{

    terminator_port_t * port = (terminator_port_t *)port_handle;
    fbe_port_type_t port_type = port_get_type(port);
    switch(port_type)
    {
    case FBE_PORT_TYPE_SAS_PMC:
        return terminator_sas_port_attribute_setter(port_handle, attribute_name, attribute_value);
    case FBE_PORT_TYPE_ISCSI:/* TODO: Implement later*/
    case FBE_PORT_TYPE_FC_PMC:
    case FBE_PORT_TYPE_FCOE:
        return terminator_fc_port_attribute_setter(port_handle, attribute_name, attribute_value);
    default:
        return TCM_STATUS_NOT_IMPLEMENTED;
    }
}

tcm_status_t terminator_port_attribute_getter(fbe_terminator_device_ptr_t port_handle,
                                                    const char * attribute_name,
                                                    char * buffer,
                                                    tcm_uint_t buffer_len)
{
    /* not needed now and not implemented */
    return TCM_STATUS_GENERIC_FAILURE;
}


/* for SAS enclosure */
static tcm_status_t terminator_sas_enclosure_attribute_setter(fbe_terminator_device_ptr_t enclosure_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    terminator_enclosure_t * enclosure = (terminator_enclosure_t *)enclosure_handle;

    if(enclosure_handle == NULL || attribute_name == NULL || attribute_value == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* ignored attributes */
    if(!strcmp(attribute_name, "Serial_number") || !strcmp(attribute_name, "SN"))
    {
        return TCM_STATUS_OK;
    }
    /* SAS enclosure type */
    if(!strcmp(attribute_name, "Enclosure_type"))
    {
        fbe_sas_enclosure_type_t sas_encl_type;
        if(terminator_string_to_sas_enclosure_type(attribute_value, &sas_encl_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        if(sas_enclosure_set_enclosure_type(enclosure, sas_encl_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* Logical parent type */
    if(!strcmp(attribute_name, "Logical_parent_type"))
    {
        terminator_component_type_t logical_parent_type;
        if(terminator_string_to_component_type(attribute_value, &logical_parent_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        if(sas_enclosure_set_logical_parent_type(enclosure, logical_parent_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* Backend number */
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(sas_enclosure_set_backend_number(enclosure, (fbe_u32_t)val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* Connector id */
    if(!strcmp(attribute_name, "connector_id"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        if(sas_enclosure_set_connector_id(enclosure, (fbe_u32_t)val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* Enclosure number */
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(sas_enclosure_set_enclosure_number(enclosure, (fbe_u32_t)val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* SAS address */
    if(!strcmp(attribute_name, "Sas_address"))
    {
        fbe_u64_t val;
        if(terminator_hexadecimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(sas_enclosure_set_sas_address(enclosure, val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* Serial number - not implemented yet */
    if(!strcmp(attribute_name, "SN") || !strcmp(attribute_name, "Serial_number"))
    {
        /* There are no Serial Number getters and setters for enclosure object */
        return TCM_STATUS_NOT_IMPLEMENTED;
    }

    /* Unique ID */
    if(!strcmp(attribute_name, "UID"))
    {
		char value_buffer[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE]; /* clean up the input */
		
		fbe_zero_memory(value_buffer, FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE);
		strncpy(value_buffer, attribute_value, FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE-1);
        
		if(sas_enclosure_set_enclosure_uid(enclosure, 
            (fbe_u8_t *)value_buffer) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/* for FC enclosure */
static tcm_status_t terminator_fc_enclosure_attribute_setter(fbe_terminator_device_ptr_t enclosure_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    terminator_enclosure_t * enclosure = (terminator_enclosure_t *)enclosure_handle;

    if(enclosure_handle == NULL || attribute_name == NULL || attribute_value == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* ignored attributes */
    if(!strcmp(attribute_name, "Serial_number") || !strcmp(attribute_name, "SN"))
    {
        return TCM_STATUS_OK;
    }
    /* SAS enclosure type */
    if(!strcmp(attribute_name, "Enclosure_type"))
    {
        fbe_fc_enclosure_type_t fc_encl_type;
        if(terminator_string_to_fc_enclosure_type(attribute_value, &fc_encl_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        if(fc_enclosure_set_enclosure_type(enclosure, fc_encl_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* Backend number */
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(fc_enclosure_set_backend_number(enclosure, (fbe_u32_t)val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* Enclosure number */
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(fc_enclosure_set_enclosure_number(enclosure, (fbe_u32_t)val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }

    /* Diplex address */
   /* if(!strcmp(attribute_name, "Diplex_address"))
    {
        fbe_u64_t val;
        if(terminator_hexadecimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        if(fc_enclosure_set_diplex_address(enclosure, (fbe_u32_t)val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }*/

    /* Serial number - not implemented yet */
    if(!strcmp(attribute_name, "SN") || !strcmp(attribute_name, "Serial_number"))
    {
        /* There are no Serial Number getters and setters for enclosure object */
        return TCM_STATUS_NOT_IMPLEMENTED;
    }

    /* Unique ID */
    if(!strcmp(attribute_name, "UID"))
    {
        if(fc_enclosure_set_enclosure_uid(enclosure, 
            (fbe_u8_t *)attribute_value) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/*generic drive setter, checks enclosure's protocol and finds appropriate setter */
tcm_status_t terminator_enclosure_attribute_setter(fbe_terminator_device_ptr_t enclosure_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    terminator_enclosure_t * encl = (terminator_enclosure_t *)enclosure_handle;
    fbe_enclosure_type_t encl_type = enclosure_get_protocol(encl);
    switch(encl_type)
    {
    case FBE_ENCLOSURE_TYPE_SAS:
        return terminator_sas_enclosure_attribute_setter(enclosure_handle, attribute_name, attribute_value);
    case FBE_ENCLOSURE_TYPE_FIBRE:
        return terminator_fc_enclosure_attribute_setter(enclosure_handle, attribute_name, attribute_value);
    default:
        return TCM_STATUS_NOT_IMPLEMENTED;
    }
}

static tcm_status_t terminator_sas_enclosure_attribute_getter(fbe_terminator_device_ptr_t enclosure_handle,
                                                    const char * attribute_name,
                                                    char * buffer,
                                                    tcm_uint_t buffer_len)
{
    terminator_enclosure_t * enclosure = (terminator_enclosure_t *)enclosure_handle;
    memset(buffer, 0, buffer_len);
    if(enclosure_handle == NULL || attribute_name == NULL || buffer == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    if(!strcmp(attribute_name, "Enclosure_type"))
    {
        fbe_sas_enclosure_type_t sas_enclosure_type = sas_enclosure_get_enclosure_type(enclosure);
        const char * enclosure_type_string = NULL;
        if(terminator_sas_enclosure_type_to_string(sas_enclosure_type, &enclosure_type_string) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        _snprintf(buffer, buffer_len, "%s", enclosure_type_string);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u32_t val = sas_enclosure_get_backend_number(enclosure);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u32_t val = sas_enclosure_get_enclosure_number(enclosure);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Sas_address"))
    {
        fbe_u64_t val;
        val = sas_enclosure_get_sas_address(enclosure);
        _snprintf(buffer, buffer_len, "%llX", (unsigned long long)val);
        return TCM_STATUS_OK;
    }
    /* ignored attributes */
    if(!strcmp(attribute_name, "Serial_number") || !strcmp(attribute_name, "SN"))
    {
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

tcm_status_t terminator_enclosure_attribute_getter(fbe_terminator_device_ptr_t enclosure_handle,
                                                    const char * attribute_name,
                                                    char * attribute_value,
                                                    tcm_uint_t buffer_len)
{
    terminator_enclosure_t * enclosure = (terminator_enclosure_t *)enclosure_handle;
    fbe_enclosure_type_t enclosure_type = enclosure_get_protocol(enclosure);
    switch(enclosure_type)
    {
    case FBE_ENCLOSURE_TYPE_SAS:
        return terminator_sas_enclosure_attribute_getter(enclosure_handle, attribute_name, attribute_value, buffer_len);
    case FBE_ENCLOSURE_TYPE_FIBRE:

    default:
        return TCM_STATUS_NOT_IMPLEMENTED;
    }

}


/* for SAS drive */
static tcm_status_t terminator_sas_drive_attribute_setter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    if(drive_handle == NULL || attribute_name == NULL || attribute_value == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    if(!strcmp(attribute_name, "BlockSize"))
    {
        fbe_u64_t           val;
        fbe_block_size_t    block_size;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        block_size = (fbe_block_size_t)val;
        switch(block_size)
        {
            case 520:
            case 512:
                break;

            default:
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                "%s: Invalid value %d for %s\n", __FUNCTION__, block_size, attribute_name);
                return TCM_STATUS_GENERIC_FAILURE; 
        }
        drive_set_block_size(drive, block_size);
        return TCM_STATUS_OK;
    }
    if (!strcmp(attribute_name, "MaxLBA"))
    {
        fbe_u64_t   val;
        fbe_lba_t   capacity;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        capacity = (fbe_lba_t)val;
        drive_set_capacity(drive, capacity);
        return TCM_STATUS_OK;
    }
    if (!strcmp(attribute_name, "Size"))
    {
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Drive_type"))
    {
        fbe_sas_drive_type_t sas_drive_type;
        if(terminator_string_to_sas_drive_type(attribute_value, &sas_drive_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        sas_drive_set_drive_type(drive, sas_drive_type);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sas_drive_set_backend_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sas_drive_set_enclosure_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Slot") || !strcmp(attribute_name, "Slot_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sas_drive_set_slot_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Sas_address"))
    {
        fbe_u64_t val;
        if(terminator_hexadecimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sas_drive_set_sas_address(drive, val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "SN") || !strcmp(attribute_name, "Serial_number"))
    {
		char value_buffer[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE]; /* clean up the input */
		
		fbe_zero_memory(value_buffer, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
		strncpy(value_buffer, attribute_value, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE-1);
		
		sas_drive_set_serial_number(drive, value_buffer);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "PID") || !strcmp(attribute_name, "Product_id"))
    {
        char value_buffer[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1]; /* clean up the input */

        fbe_zero_memory(value_buffer, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1);
        strncpy(value_buffer, attribute_value, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);

        sas_drive_set_product_id(drive, value_buffer);
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/*for SATA drive */
static tcm_status_t terminator_sata_drive_attribute_setter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    if(drive_handle == NULL || attribute_name == NULL || attribute_value == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    if(!strcmp(attribute_name, "BlockSize"))
    {
        fbe_u64_t           val;
        fbe_block_size_t    block_size;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        block_size = (fbe_block_size_t)val;
        switch(block_size)
        {
            case 520:
            case 512:
                break;

            default:
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                "%s: Invalid value %d for %s\n", __FUNCTION__, block_size, attribute_name);
                return TCM_STATUS_GENERIC_FAILURE; 
        }
        drive_set_block_size(drive, block_size);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "MaxLBA"))
    {
        fbe_u64_t   val;
        fbe_lba_t   capacity;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        capacity = (fbe_lba_t)val;
        drive_set_capacity(drive, capacity);
        return TCM_STATUS_OK;
    }
    if (!strcmp(attribute_name, "Size"))
    {
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Drive_type"))
    {
        fbe_sata_drive_type_t sata_drive_type;
        if(terminator_string_to_sata_drive_type(attribute_value, &sata_drive_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        sata_drive_set_drive_type(drive, sata_drive_type);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sata_drive_set_backend_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sata_drive_set_enclosure_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Slot") || !strcmp(attribute_name, "Slot_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sata_drive_set_slot_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Sas_address"))
    {
        fbe_u64_t val;
        if(terminator_hexadecimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        sata_drive_set_sas_address(drive, val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "SN") || !strcmp(attribute_name, "Serial_number"))
    {
		char value_buffer[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE]; /* clean up the input */
		
		fbe_zero_memory(value_buffer, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
		strncpy(value_buffer, attribute_value, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE-1);
        sata_drive_set_serial_number(drive, value_buffer);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "PID") || !strcmp(attribute_name, "Product_id"))
    {
        char value_buffer[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1]; /* clean up the input */

        fbe_zero_memory(value_buffer, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1);
        strncpy(value_buffer, attribute_value, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);

        sata_drive_set_product_id(drive, value_buffer);
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/* for SAS drive */
static tcm_status_t terminator_fc_drive_attribute_setter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    if(drive_handle == NULL || attribute_name == NULL || attribute_value == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    if(!strcmp(attribute_name, "BlockSize")
        || !strcmp(attribute_name, "Size")
        || !strcmp(attribute_name, "MaxLBA"))
    {
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Drive_type"))
    {
        fbe_fc_drive_type_t fc_drive_type;
        if(terminator_string_to_fc_drive_type(attribute_value, &fc_drive_type) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        fc_drive_set_drive_type(drive, fc_drive_type);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        fc_drive_set_backend_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        fc_drive_set_enclosure_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Slot") || !strcmp(attribute_name, "Slot_number"))
    {
        fbe_u64_t val;
        if(terminator_decimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        fc_drive_set_slot_number(drive, (fbe_u32_t)val);
        return TCM_STATUS_OK;
    }
    /*if(!strcmp(attribute_name, "Diplex_address"))
    {
        fbe_u64_t val;
        if(terminator_hexadecimal_string_to_u64(attribute_value, &val) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE; 
        }
        fc_drive_set_diplex_address(drive,(fbe_u32_t)val);
        return TCM_STATUS_OK;
    }*/
    if(!strcmp(attribute_name, "SN") || !strcmp(attribute_name, "Serial_number"))
    {
        fc_drive_set_serial_number(drive, attribute_value);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "PID") || !strcmp(attribute_name, "Product_id"))
    {
        char value_buffer[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1]; /* clean up the input */

        fbe_zero_memory(value_buffer, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1);
        strncpy(value_buffer, attribute_value, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);

        fc_drive_set_product_id(drive, value_buffer);
        return TCM_STATUS_OK;
    }
    return TCM_STATUS_GENERIC_FAILURE;
}
/*generic drive setter, checks drive's protocol and finds appropriate setter */
tcm_status_t terminator_drive_attribute_setter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    fbe_drive_type_t drive_type = drive_get_protocol(drive);
    switch(drive_type)
    {
    case FBE_DRIVE_TYPE_SAS:
    case FBE_DRIVE_TYPE_SAS_FLASH_HE:
    case FBE_DRIVE_TYPE_SAS_FLASH_ME:
    case FBE_DRIVE_TYPE_SAS_FLASH_LE:
    case FBE_DRIVE_TYPE_SAS_FLASH_RI:
        return terminator_sas_drive_attribute_setter(drive_handle, attribute_name, attribute_value);
    case FBE_DRIVE_TYPE_SATA:
    case FBE_DRIVE_TYPE_SATA_FLASH_HE:
        return terminator_sata_drive_attribute_setter(drive_handle, attribute_name, attribute_value);
    case FBE_DRIVE_TYPE_FIBRE:
        return terminator_fc_drive_attribute_setter(drive_handle, attribute_name, attribute_value);     
    default:
        return TCM_STATUS_NOT_IMPLEMENTED;
    }
}

/* for SAS drive */
static tcm_status_t terminator_sas_drive_attribute_getter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    char * buffer, tcm_uint_t buffer_len)
{
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    memset(buffer, 0, buffer_len);
    if(drive_handle == NULL || attribute_name == NULL || buffer == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    if(!strcmp(attribute_name, "Drive_type"))
    {
        fbe_sas_drive_type_t sas_drive_type = sas_drive_get_drive_type(drive);
        const char * drive_type_string = NULL;
        if(terminator_sas_drive_type_to_string(sas_drive_type, &drive_type_string) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        _snprintf(buffer, buffer_len, "%s", drive_type_string);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u32_t val = sas_drive_get_backend_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u32_t val = sas_drive_get_enclosure_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Slot") || !strcmp(attribute_name, "Slot_number"))
    {
        fbe_u32_t val = sas_drive_get_slot_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Sas_address"))
    {
        fbe_u64_t val;
        val = sas_drive_get_sas_address(drive);
        _snprintf(buffer, buffer_len, "%llX", (unsigned long long)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "SN") || !strcmp(attribute_name, "Serial_number"))
    {
        fbe_u8_t * sn = sas_drive_get_serial_number(drive);
        if(sn != NULL)
        {
            _snprintf(buffer, buffer_len, "%s", sn);
            return TCM_STATUS_OK;
        }
        else
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }
    if(!strcmp(attribute_name, "PID") || !strcmp(attribute_name, "Product_id"))
    {
        fbe_u8_t * pid = sas_drive_get_product_id(drive);
        if(pid != NULL)
        {
            _snprintf(buffer, buffer_len, "%s", pid);
            return TCM_STATUS_OK;
        }
        else
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/*for SATA drive */
static tcm_status_t terminator_sata_drive_attribute_getter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    char * buffer, tcm_uint_t buffer_len)
{
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    memset(buffer, 0, buffer_len);
    if(drive_handle == NULL || attribute_name == NULL || buffer == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }

    if(!strcmp(attribute_name, "Drive_type"))
    {
        fbe_sata_drive_type_t sata_drive_type = sata_drive_get_drive_type(drive);
        const char * drive_type_string = NULL;
        if(terminator_sata_drive_type_to_string(sata_drive_type, &drive_type_string) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        _snprintf(buffer, buffer_len, "%s", drive_type_string);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u32_t val = sata_drive_get_backend_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u32_t val = sata_drive_get_enclosure_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Slot") || !strcmp(attribute_name, "Slot_number"))
    {
        fbe_u32_t val = sata_drive_get_slot_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Sas_address"))
    {
        fbe_u64_t val;
        val = sata_drive_get_sas_address(drive);
        _snprintf(buffer, buffer_len, "%llX", (unsigned long long)val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "SN") || !strcmp(attribute_name, "Serial_number"))
    {
        fbe_u8_t * sn = sata_drive_get_serial_number(drive);
        if(sn != NULL)
        {
            _snprintf(buffer, buffer_len, "%s", sn);
            return TCM_STATUS_OK;
        }
        else
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }
    if(!strcmp(attribute_name, "PID") || !strcmp(attribute_name, "Product_id"))
    {
        fbe_u8_t * pid = sata_drive_get_product_id(drive);
        if(pid != NULL)
        {
            _snprintf(buffer, buffer_len, "%s", pid);
            return TCM_STATUS_OK;
        }
        else
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}

/* for FC drive */
static tcm_status_t terminator_fc_drive_attribute_getter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    char * buffer, tcm_uint_t buffer_len)
{
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    memset(buffer, 0, buffer_len);
    if(drive_handle == NULL || attribute_name == NULL || buffer == NULL)
    {
        return TCM_STATUS_GENERIC_FAILURE;
    }
    if(!strcmp(attribute_name, "Drive_type"))
    {
        fbe_fc_drive_type_t fc_drive_type = fc_drive_get_drive_type(drive);
        const char * drive_type_string = NULL;
        if(terminator_fc_drive_type_to_string(fc_drive_type, &drive_type_string) != FBE_STATUS_OK)
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
        _snprintf(buffer, buffer_len, "%s", drive_type_string);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "BE") || !strcmp(attribute_name, "Backend_number"))
    {
        fbe_u32_t val = fc_drive_get_backend_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Enc") || !strcmp(attribute_name, "Enclosure_number"))
    {
        fbe_u32_t val = fc_drive_get_enclosure_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    if(!strcmp(attribute_name, "Slot") || !strcmp(attribute_name, "Slot_number"))
    {
        fbe_u32_t val = fc_drive_get_slot_number(drive);
        _snprintf(buffer, buffer_len, "%u", val);
        return TCM_STATUS_OK;
    }
    /*if(!strcmp(attribute_name, "Diplex_address"))
    {
        fbe_u64_t val;
        val = fc_drive_get_diplex_address(drive);
        _snprintf(buffer, buffer_len, "%llX", val);
        return TCM_STATUS_OK;
    }*/
    if(!strcmp(attribute_name, "SN") || !strcmp(attribute_name, "Serial_number"))
    {
        fbe_u8_t * sn = fc_drive_get_serial_number(drive);
        if(sn != NULL)
        {
            _snprintf(buffer, buffer_len, "%s", sn);
            return TCM_STATUS_OK;
        }
        else
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }
    if(!strcmp(attribute_name, "PID") || !strcmp(attribute_name, "Product_id"))
    {
        fbe_u8_t * pid = fc_drive_get_product_id(drive);
        if(pid != NULL)
        {
            _snprintf(buffer, buffer_len, "%s", pid);
            return TCM_STATUS_OK;
        }
        else
        {
            return TCM_STATUS_GENERIC_FAILURE;
        }
    }
    return TCM_STATUS_GENERIC_FAILURE;
}
tcm_status_t terminator_drive_attribute_getter(fbe_terminator_device_ptr_t drive_handle,
                                                    const char * attribute_name,
                                                    char * buffer,
                                                    tcm_uint_t buffer_len)
{
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    fbe_drive_type_t drive_type = drive_get_protocol(drive);
    switch(drive_type)
    {
    case FBE_DRIVE_TYPE_SAS:
    case FBE_DRIVE_TYPE_SAS_FLASH_HE:
    case FBE_DRIVE_TYPE_SAS_FLASH_ME:
    case FBE_DRIVE_TYPE_SAS_FLASH_LE:
    case FBE_DRIVE_TYPE_SAS_FLASH_RI:         
        return terminator_sas_drive_attribute_getter(drive_handle, attribute_name, buffer, buffer_len);
    case FBE_DRIVE_TYPE_SATA:
        case FBE_DRIVE_TYPE_SATA_FLASH_HE: 
        return terminator_sata_drive_attribute_getter(drive_handle, attribute_name, buffer, buffer_len);
    case FBE_DRIVE_TYPE_FIBRE:
        return terminator_fc_drive_attribute_getter(drive_handle, attribute_name, buffer, buffer_len);    
    default:
        return TCM_STATUS_NOT_IMPLEMENTED;
    }
}

/* Convenient (and slower) way to invoke an accessor (requires many lookups) */
tcm_status_t terminator_set_device_attribute(fbe_terminator_device_ptr_t device_handle,
                                             const char * attribute_name,
                                             const char * attribute_value)
{
    terminator_component_type_t device_type;
    const char * class_name = NULL;
    tcm_reference_t class_handle = NULL;
    terminator_attribute_accessor_t * attribute_accessor = NULL;
    tcm_status_t tcm_status;
    /* Get device type */
    device_type = base_component_get_component_type((base_component_t*)device_handle);
    /* Get device class name */
    switch(device_type)
    {
    case TERMINATOR_COMPONENT_TYPE_BOARD:
        class_name = "Board";
        break;
    case TERMINATOR_COMPONENT_TYPE_PORT:
        class_name = "Port";
        break;
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        class_name = "Enclosure";
        break;
    case TERMINATOR_COMPONENT_TYPE_DRIVE:
        class_name = "Drive";
        break;
    default:
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* Get class handle for given device class name */
    if((tcm_status = fbe_terminator_class_management_class_find(class_name, &class_handle))
        != TCM_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "%s: could not find TCM class for %s\n", __FUNCTION__, class_name);
        return tcm_status;
    }
    /* Get accessor for given attribute name */
    if((tcm_status = fbe_terminator_class_management_class_accessor_string_lookup(class_handle,
        attribute_name, &attribute_accessor)) != TCM_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not find TCM accessors for %s attribute of %s\n",
            __FUNCTION__, attribute_name, class_name);
        return tcm_status;
    }
    /* Check that string-string accessor exists */
    if(attribute_accessor->string_string_setter == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: string_string_setter for %s attribute of %s is not defined\n",
            __FUNCTION__, attribute_name, class_name);
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* Invoke accessor for given attribute name and value */
    if((tcm_status = attribute_accessor->string_string_setter(device_handle,
        attribute_name, attribute_value)) != TCM_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not set value %s for %s attribute of %s\n", 
            __FUNCTION__, attribute_value, attribute_name, class_name);
        return tcm_status;
    }
    return TCM_STATUS_OK;
}

/* Convenient (and slower) way to invoke an accessor (requires many lookups) */
tcm_status_t terminator_get_device_attribute(fbe_terminator_device_ptr_t device_handle,
                                                    const char * attribute_name,
                                                    char * attribute_value_buffer,
                                                    fbe_u32_t buffer_length)
{
    terminator_component_type_t device_type;
    const char * class_name = NULL;
    tcm_reference_t class_handle = NULL;
    terminator_attribute_accessor_t * attribute_accessor = NULL;
    tcm_status_t tcm_status;
    /* Get device type */
    device_type = base_component_get_component_type((base_component_t*)device_handle);
    /* Get device class name */
    switch(device_type)
    {
    case TERMINATOR_COMPONENT_TYPE_BOARD:
        class_name = "Board";
        break;
    case TERMINATOR_COMPONENT_TYPE_PORT:
        class_name = "Port";
        break;
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        class_name = "Enclosure";
        break;
    case TERMINATOR_COMPONENT_TYPE_DRIVE:
        class_name = "Drive";
        break;
    default:
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* Get class handle for given device class name */
    if((tcm_status = fbe_terminator_class_management_class_find(class_name, &class_handle))
        != TCM_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "%s: could not find TCM class for %s\n", __FUNCTION__, class_name);
        return tcm_status;
    }
    /* Get accessor for given attribute name */
    if((tcm_status = fbe_terminator_class_management_class_accessor_string_lookup(class_handle,
        attribute_name, &attribute_accessor)) != TCM_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not find TCM accessors for %s attribute of %s\n",
            __FUNCTION__, attribute_name, class_name);
        return tcm_status;
    }
    /* Check that string-string accessor exists */
    if(attribute_accessor->string_string_getter == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: string_string_setter for %s attribute of %s is not defined\n",
            __FUNCTION__, attribute_name, class_name);
        return TCM_STATUS_GENERIC_FAILURE;
    }
    /* Invoke accessor for given attribute name and value */
    if((tcm_status = attribute_accessor->string_string_getter(device_handle,
        attribute_name, attribute_value_buffer, buffer_length)) != TCM_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not get of %s attribute of %s\n", 
            __FUNCTION__, attribute_name, class_name);
        return tcm_status;
    }
    return TCM_STATUS_OK;
}
