#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator.h"
#include "terminator_enum_conversions.h"
#include "terminator_drive.h"
#include "fbe_terminator_device_registry.h"
#include "terminator_port.h"
#include "fbe_simulated_drive.h"


/* value of each device attribute should fit into 64-byte buffer */
#define TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN 64
static terminator_sp_id_t terminator_sp_id = TERMINATOR_SP_A;
static fbe_bool_t terminator_is_single_sp_system = FBE_FALSE;
static fbe_u16_t cmi_port_base = 0;
static char simulated_drive_server_name[32] = SIMULATED_DRIVE_DEFAULT_SERVER_NAME;
static fbe_u16_t simulated_drive_server_port = SIMULATED_DRIVE_DEFAULT_PORT_NUMBER;
extern fbe_terminator_io_mode_t terminator_io_mode;


fbe_status_t fbe_terminator_api_insert_board(fbe_terminator_board_info_t *board_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_api_device_class_handle_t board_class_handle;
    fbe_terminator_api_device_handle_t  board_handle;
    const char * board_type_string = NULL;
    const char * platform_type_string = NULL;

    /* find class for board */
    status = fbe_terminator_api_find_device_class("Board", &board_class_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* create a board */
    status = fbe_terminator_api_create_device_class_instance(board_class_handle, NULL, &board_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Convert platform type to board type */
    status = terminator_platform_type_to_board_type(board_info->platform_type, &board_info->board_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert platform type: %d to board type: %d\n", 
            __FUNCTION__, 
             board_info->platform_type,
             board_info->board_type);
        return status;
    }

    /* FIXME: This should be replaced when the int attr setter is available.
     * Convert the board type to string. */
    status = terminator_board_type_to_string(board_info->board_type, &board_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert board type %d to string\n", __FUNCTION__, board_info->board_type);
        return status;
    }
    /* set the attribute of this board */
    status = fbe_terminator_api_set_device_attribute(board_handle, "Board_type", board_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Convert the platform type to string. */
    status = terminator_platform_type_to_string(board_info->platform_type, &platform_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert platform type %d to string\n", __FUNCTION__, board_info->platform_type);
        return status;
    }
    /* set the attribute of this platform */
    status = fbe_terminator_api_set_device_attribute(board_handle, "Platform_type", platform_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Activate the device */
    status = fbe_terminator_api_activate_device(board_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not activate device\n", __FUNCTION__);
        return status;
    }
    return status;
}

static
fbe_status_t create_port_instance (fbe_port_type_t port_type,
                                   fbe_u32_t io_port_number,
                                   fbe_u32_t portal_number,
                                   fbe_u32_t backend_number,
                                   fbe_terminator_api_device_handle_t *port_handle)
{
    fbe_status_t status;
    fbe_terminator_api_device_class_handle_t port_class_handle;
    const char * port_type_string;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];

    /* find class for port */
    status = fbe_terminator_api_find_device_class("Port", &port_class_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

     /* convert port type to string */
    status = terminator_port_type_to_string(port_type, &port_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert port type %d to string\n", __FUNCTION__, port_type);
        return status;
    }    

    /* create a port */
    status = fbe_terminator_api_create_device_class_instance(port_class_handle, port_type_string, port_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    
    /* set port type */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "Port_type", port_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert io_port_number to string */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", io_port_number);
    /* set IO port number */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "IO_port_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert portal number to string */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", portal_number);
    /* set portal number */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "Portal_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
 
    /* convert backend number to string */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", backend_number);
    /* set backend number */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    return FBE_STATUS_OK;
}

static
fbe_status_t create_port_instance_extend (fbe_port_type_t port_type,
                                   fbe_u32_t io_port_number,
                                   fbe_u32_t portal_number,
                                   fbe_u32_t backend_number,
                                   fbe_port_connect_class_t connect_class,
                                   fbe_port_role_t port_role,
                                   fbe_terminator_api_device_handle_t *port_handle)
{
    fbe_status_t status;
    fbe_terminator_api_device_class_handle_t port_class_handle;
    const char * convert_string;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];

    /* find class for port */
    status = fbe_terminator_api_find_device_class("Port", &port_class_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

     /* convert port type to string */
    status = terminator_port_type_to_string(port_type, &convert_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert port type %d to string\n", __FUNCTION__, port_type);
        return status;
    }

    /* create a port */
    status = fbe_terminator_api_create_device_class_instance(port_class_handle, convert_string, port_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* set port type */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "Port_type", convert_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert io_port_number to string */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", io_port_number);
    /* set IO port number */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "IO_port_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert portal number to string */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", portal_number);
    /* set portal number */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "Portal_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert backend number to string */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", backend_number);
    /* set backend number */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert port connect class to string */
   status = terminator_port_connect_class_to_string(connect_class, &convert_string);
   if(status != FBE_STATUS_OK)
   {
       terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
           "%s: could not convert port connect class %d to string\n", __FUNCTION__, connect_class);
       return status;
   }
   /* set port connect class */
   status = fbe_terminator_api_set_device_attribute(*port_handle, "Connect_class", convert_string);
   if(status != FBE_STATUS_OK)
   {
       return status;
   }

    /* convert port role to string */
    status = terminator_port_role_to_string(port_role, &convert_string);
    if(status != FBE_STATUS_OK)
    {
      terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
          "%s: could not convert port role %d to string\n", __FUNCTION__, port_role);
      return status;
    }
    /* set port role */
    status = fbe_terminator_api_set_device_attribute(*port_handle, "Port_role", convert_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_insert_port (fbe_terminator_port_shim_backend_port_info_t *port_info,
                                                 fbe_terminator_api_device_handle_t *ret_port_handle)
{
    fbe_terminator_api_device_handle_t board_handle;
    fbe_terminator_api_device_handle_t port_handle;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];
    fbe_status_t status;

    *ret_port_handle = 0;

    /* get the board_handle */
    status = fbe_terminator_api_get_board_handle(&board_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    status = create_port_instance_extend (port_info->port_type,
                                   port_info->port_number,
                                   port_info->portal_number,
                                   port_info->backend_port_number,
                                   port_info->connect_class,
                                   port_info->port_role,
                                   &port_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* port type specific attributes setting */
    switch(port_info->port_type)
    {
    case FBE_PORT_TYPE_SAS_PMC:
        /* convert SAS address to string */
        memset(buffer, 0, sizeof(buffer));
        _snprintf(buffer, sizeof(buffer), "%llX", (unsigned long long)port_info->type_specific_info.sas_info.sas_address);
        /* set SAS address */
        status = fbe_terminator_api_set_device_attribute(port_handle, "Sas_address", buffer);
        if(status != FBE_STATUS_OK)
        {
            return status;
        }
        break;
    case FBE_PORT_TYPE_FC_PMC:
        /* convert diplex address to string */
        memset(buffer, 0, sizeof(buffer));
        _snprintf(buffer, sizeof(buffer), "%X", port_info->type_specific_info.fc_info.diplex_address);
        /* set diplex address */
        status = fbe_terminator_api_set_device_attribute(port_handle, "Diplex_address", buffer);
        if(status != FBE_STATUS_OK)
        {
            return status;
        }
        break;
    default:
        break;
	}

    /* Insert port */
    status = fbe_terminator_api_insert_device(board_handle, port_handle);
	if (status != FBE_STATUS_OK) {
		terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "SAS port insertion failed\n");
		return status;
	}

	/* Activate the device */
    status = fbe_terminator_api_activate_device(port_handle);
	if (status != FBE_STATUS_OK) {
		terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "SAS port activation failed\n");
		return status;
	}

    *ret_port_handle = port_handle;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_insert_sas_port (fbe_terminator_sas_port_info_t *port_info, 
                                                 fbe_terminator_api_device_handle_t *ret_port_handle)
{
    fbe_terminator_port_shim_backend_port_info_t shim_port_info;

    CHECK_AND_COPY_STRUCTURE(&(shim_port_info.type_specific_info.sas_info), port_info);
    shim_port_info.port_type = port_info->port_type;
    shim_port_info.port_number = port_info->io_port_number;
    shim_port_info.portal_number = port_info->portal_number;
    shim_port_info.backend_port_number = port_info->backend_number;
    shim_port_info.connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;
    shim_port_info.port_role = FBE_CPD_SHIM_PORT_ROLE_BE;//port_info->port_role; 
                                        /* Keeping this BE to avoid updateing all test scripts.*/

    return fbe_terminator_api_insert_port(&shim_port_info, ret_port_handle);
}

fbe_status_t fbe_terminator_api_insert_fc_port (fbe_terminator_fc_port_info_t *port_info, 
                                                 fbe_terminator_api_device_handle_t *ret_port_handle)
{
    fbe_terminator_port_shim_backend_port_info_t shim_port_info;

    CHECK_AND_COPY_STRUCTURE(&(shim_port_info.type_specific_info.fc_info), port_info);
    shim_port_info.port_type = port_info->port_type;
    shim_port_info.port_number = port_info->io_port_number;
    shim_port_info.portal_number = port_info->portal_number;
    shim_port_info.backend_port_number = port_info->backend_number;
    shim_port_info.connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FC;
    shim_port_info.port_role = port_info->port_role;//FBE_CPD_SHIM_PORT_ROLE_BE;

    return fbe_terminator_api_insert_port(&shim_port_info, ret_port_handle);
}

fbe_status_t fbe_terminator_api_insert_iscsi_port (fbe_terminator_iscsi_port_info_t *port_info, 
                                                 fbe_terminator_api_device_handle_t *ret_port_handle)
{
    fbe_terminator_port_shim_backend_port_info_t shim_port_info;

    CHECK_AND_COPY_STRUCTURE(&(shim_port_info.type_specific_info.sas_info), port_info);
    shim_port_info.port_type = port_info->port_type;
    shim_port_info.port_number = port_info->io_port_number;
    shim_port_info.portal_number = port_info->portal_number;
    shim_port_info.backend_port_number = port_info->backend_number;
    shim_port_info.connect_class = FBE_CPD_SHIM_CONNECT_CLASS_ISCSI;
    shim_port_info.port_role = port_info->port_role;//FBE_CPD_SHIM_PORT_ROLE_BE;

    return fbe_terminator_api_insert_port(&shim_port_info, ret_port_handle);
}

fbe_status_t fbe_terminator_api_insert_fcoe_port (fbe_terminator_fcoe_port_info_t *port_info, 
                                                 fbe_terminator_api_device_handle_t *ret_port_handle)
{
    fbe_terminator_port_shim_backend_port_info_t shim_port_info;

    CHECK_AND_COPY_STRUCTURE(&(shim_port_info.type_specific_info.sas_info), port_info);
    shim_port_info.port_type = port_info->port_type;
    shim_port_info.port_number = port_info->io_port_number;
    shim_port_info.portal_number = port_info->portal_number;
    shim_port_info.backend_port_number = port_info->backend_number;
    shim_port_info.connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FCOE;
    shim_port_info.port_role = port_info->port_role;

    return fbe_terminator_api_insert_port(&shim_port_info, ret_port_handle);
}


fbe_status_t fbe_terminator_api_insert_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *enclosure_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_api_device_class_handle_t enclosure_class_handle;
    const char * encl_type_string, * parent_type_string;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];
    tdr_device_ptr_t p_device_ptr;
    terminator_component_type_t parent_type;

    /* find class for enclosure */
    status = fbe_terminator_api_find_device_class("Enclosure", &enclosure_class_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    status = terminator_enclosure_type_to_string(FBE_ENCLOSURE_TYPE_SAS, &encl_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert enclosure type %d to string\n", __FUNCTION__, FBE_ENCLOSURE_TYPE_SAS);
        return status;
    }
    /* create an enclosure */
    status = fbe_terminator_api_create_device_class_instance(enclosure_class_handle, encl_type_string, enclosure_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* set device attributes */
    /* FIXME: This should be replaced when the int attr setter is available. */
    /* convert enclosure type to string */
    status = terminator_sas_enclosure_type_to_string(encl_info->encl_type, &encl_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert enclosure type %d to string\n", __FUNCTION__, encl_info->encl_type);
        return status;
    }
    /* set enclosure type */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "Enclosure_type", encl_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert UID to string. UID is a four-byte character array,
       and it is not guaranteed to be 0-terminated. */
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, encl_info->uid, sizeof(encl_info->uid));

    /* set UID */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "UID", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert enclosure number to string - OLD API does not pass it in attributes, calculate it manually! */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", encl_info->encl_number);
    /* set enclosure number */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "Enclosure_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
 
    /* convert backend number to string - OLD API does not pass it in attributes, use port_number instead */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", encl_info->backend_number);
    /* set backend number */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
 
    /* convert connector id to string - OLD API does not pass it in attributes, use port_number instead */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", encl_info->connector_id);
    /* set connector id */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "connector_id", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert SAS address to string */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%llX",
	      (unsigned long long)encl_info->sas_address);
    /* set SAS address */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "Sas_address", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    tdr_status = fbe_terminator_device_registry_get_device_ptr(parent_handle, &p_device_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    parent_type = base_component_get_component_type(p_device_ptr);
    status = terminator_component_type_to_string(parent_type, &parent_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert parent type %d to string\n", __FUNCTION__, parent_type);
        return status;
    }
    /* set parent type */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "Logical_parent_type", parent_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Insert enclosure */
    status = fbe_terminator_api_insert_device(parent_handle, *enclosure_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Insertion failed\n");
        return status;
    }

    /* Activate the device */
    status = fbe_terminator_api_activate_device(*enclosure_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Activation failed\n");
        return status;
    }
    return status;
}

fbe_status_t fbe_terminator_api_reinsert_sas_enclosure (fbe_terminator_api_device_handle_t port_handle,
                                                      fbe_terminator_api_device_handle_t enclosure_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];
    fbe_terminator_device_ptr_t port_ptr, enclosure_ptr;
    terminator_sas_port_info_t * info = NULL;

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if((info = base_component_get_attributes(&((terminator_port_t *)port_ptr)->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Backend number */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", info->backend_number);
    status = fbe_terminator_api_set_device_attribute(enclosure_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* FBE_FALSE indicates we don't need a new virtual phy for the enclosure */
    status = port_add_sas_enclosure(port_ptr, enclosure_ptr, FBE_FALSE);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Activate the enclosure */
    status = enclosure_activate(enclosure_ptr);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    return status;
}

fbe_status_t fbe_terminator_api_insert_fc_enclosure (fbe_terminator_api_device_handle_t port_handle,
                                                      fbe_terminator_fc_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *enclosure_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_api_device_class_handle_t enclosure_class_handle;
    const char * encl_type_string = NULL;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];

    /* find class for enclosure */
    status = fbe_terminator_api_find_device_class("Enclosure", &enclosure_class_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    status = terminator_enclosure_type_to_string(FBE_ENCLOSURE_TYPE_FIBRE, &encl_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert enclosure type %d to string\n", __FUNCTION__, FBE_ENCLOSURE_TYPE_FIBRE);
        return status;
    }

    /* create an enclosure */
    status = fbe_terminator_api_create_device_class_instance(enclosure_class_handle, encl_type_string, enclosure_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* set device attributes */
    /* FIXME: This should be replaced when the int attr setter is available.
    /* convert enclosure type to string */
    status = terminator_fc_enclosure_type_to_string(encl_info->encl_type, &encl_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert enclosure type %d to string\n", __FUNCTION__, encl_info->encl_type);
        return status;
    }
    /* set enclosure type */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "Enclosure_type", encl_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert UID to string. UID is a four-byte character array,
       and it is not guaranteed to be 0-terminated. */
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, encl_info->uid, sizeof(encl_info->uid));

    /* set UID */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "UID", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* convert enclosure number to string - OLD API does not pass it in attributes, calculate it manually! */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", encl_info->encl_number);
    /* set enclosure number */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "Enclosure_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
 
    /* convert backend number to string - OLD API does not pass it in attributes, use port_number instead */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", encl_info->backend_number);
    /* set backend number */
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
 
    /* /
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%llX", encl_info->diplex_address);
    
    status = fbe_terminator_api_set_device_attribute(*enclosure_handle, "diplex_address", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }*/

    /* Insert enclosure */
    status = fbe_terminator_api_insert_device(port_handle, *enclosure_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Insertion failed\n");
        return status;
    }

    /* Activate the device */
    status = fbe_terminator_api_activate_device(*enclosure_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Activation failed\n");
        return status;
    }
    return status;
}

fbe_status_t fbe_terminator_api_insert_sas_drive (fbe_terminator_api_device_handle_t enclosure_handle,
                                                  fbe_u32_t slot_number,
                                                  fbe_terminator_sas_drive_info_t *drive_info, 
                                                  fbe_terminator_api_device_handle_t *drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /*check if a drive exist on this slot*/
    status = fbe_terminator_api_get_drive_handle(drive_info->backend_number, 
                                                 drive_info->encl_number, 
                                                 slot_number,
                                                 drive_handle);
    if(status == FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: slot is used by another drive.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    drive_info->slot_number = slot_number;


   /*create drive*/
    status = fbe_terminator_api_force_create_sas_drive(drive_info, drive_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                         "Drive creation failed\n");
        return status;
    }
    /* Insert drive */
    status = fbe_terminator_api_insert_device(enclosure_handle, *drive_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                         "Drive insertion failed\n");
        return status;
    }

    /* Activate the device */
    status = fbe_terminator_api_activate_device(*drive_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                         "Drive activation failed\n");
        return status;
    }
    
    return status;
}


fbe_status_t fbe_terminator_api_insert_fc_drive (fbe_terminator_api_device_handle_t enclosure_handle,
                                                  fbe_u32_t slot_number,
                                                  fbe_terminator_fc_drive_info_t *drive_info, 
                                                  fbe_terminator_api_device_handle_t *drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /*check if a drive exist on this slot*/
    status = fbe_terminator_api_get_drive_handle(drive_info->backend_number, 
                                                 drive_info->encl_number, 
                                                 slot_number,
                                                 drive_handle);
    if(status == FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: slot is used by another drive.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    drive_info->slot_number = slot_number;
    /*create drive*/
    status = fbe_terminator_api_force_create_fc_drive(slot_number, drive_info, drive_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                         "Drive creation failed\n");
        return status;
    }
    /* Insert drive */
    status = fbe_terminator_api_insert_device(enclosure_handle, *drive_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                         "Drive insertion failed\n");
        return status;
    }

    /* Activate the device */
    status = fbe_terminator_api_activate_device(*drive_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                         "Drive activation failed\n");
        return status;
    }
    return status;
}

fbe_status_t fbe_terminator_api_force_create_fc_drive (fbe_u32_t slot_number, fbe_terminator_fc_drive_info_t *drive_info, fbe_terminator_api_device_handle_t *drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_api_device_class_handle_t drive_class_handle;
    const char * drive_type_string;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];
	terminator_drive_t * drive;

    /* find class for Drive */
    status = fbe_terminator_api_find_device_class("Drive", &drive_class_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    /* This is a special case for drive, convert FBE_DRIVE_TYPE_SAS to "FBE_DRIVE_TYPE_SAS" */
    if((status = terminator_fbe_drive_type_to_string(FBE_DRIVE_TYPE_FIBRE, &drive_type_string))
        != FBE_STATUS_OK)
    {
        return status;
    }
    /* create an enclosure */
    status = fbe_terminator_api_create_device_class_instance(drive_class_handle, drive_type_string, drive_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }


    tdr_status = fbe_terminator_device_registry_get_device_ptr(*drive_handle, (tdr_device_ptr_t *)&drive);
    if(tdr_status != TDR_STATUS_OK) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
	drive->capacity = drive_info->capacity;
	drive->block_size = drive_info->block_size;

    /* set device attributes */
    /* FIXME: This should be replaced when the int attr setter is available.
    /* Drive type */
    status = terminator_fc_drive_type_to_string(drive_info->drive_type, &drive_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert SAS drive type %d to string\n", __FUNCTION__, drive_info->drive_type);
        return status;
    }
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Drive_type", drive_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Diplex address */
    /*memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%llX", drive_info->diplex_address);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "diplex_address", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }*/

    /* Serial number */
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "SN", drive_info->drive_serial_number);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Backend number */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", drive_info->backend_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Enclosure number is not passed in drive_info (old API did not use it) */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", drive_info->encl_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Enc", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /*Slot number */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", slot_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Slot_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Product id */
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "PID", drive_info->product_id);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    return status;
}


fbe_status_t fbe_terminator_api_force_create_sas_drive (fbe_terminator_sas_drive_info_t *drive_info, fbe_terminator_api_device_handle_t *drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_api_device_class_handle_t drive_class_handle;
    const char * drive_type_string;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];
	terminator_drive_t * drive;

    /* find class for Drive */
    status = fbe_terminator_api_find_device_class("Drive", &drive_class_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    /* This is a special case for drive, convert FBE_DRIVE_TYPE_SAS to "FBE_DRIVE_TYPE_SAS" */
    if((status = terminator_fbe_drive_type_to_string(FBE_DRIVE_TYPE_SAS, &drive_type_string))
        != FBE_STATUS_OK)
    {
        return status;
    }
    /* create an enclosure */
    status = fbe_terminator_api_create_device_class_instance(drive_class_handle, drive_type_string, drive_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }


    tdr_status = fbe_terminator_device_registry_get_device_ptr(*drive_handle, (tdr_device_ptr_t *)&drive);
    if(tdr_status != TDR_STATUS_OK) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
	drive->capacity = drive_info->capacity;
	drive->block_size = drive_info->block_size;

    /* set device attributes */
    /* FIXME: This should be replaced when the int attr setter is available.
    /* Drive type */
    status = terminator_sas_drive_type_to_string(drive_info->drive_type, &drive_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert SAS drive type %d to string\n", __FUNCTION__, drive_info->drive_type);
        return status;
    }
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Drive_type", drive_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* SAS address */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%llX",
	      (unsigned long long)drive_info->sas_address);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Sas_address", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Serial number */
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "SN", drive_info->drive_serial_number);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Backend number */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", drive_info->backend_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Enclosure number is not passed in drive_info (old API did not use it) */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", drive_info->encl_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Enc", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /*Slot number */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", drive_info->slot_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Slot_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Product id */
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "PID", drive_info->product_id);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

   return status;
}

fbe_status_t fbe_terminator_api_insert_sata_drive(fbe_terminator_api_device_handle_t enclosure_handle,
                                                  fbe_u32_t slot_number,
                                                  fbe_terminator_sata_drive_info_t *drive_info, 
                                                  fbe_terminator_api_device_handle_t *drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /*check if a drive exist on this slot*/
    status = fbe_terminator_api_get_drive_handle(drive_info->backend_number, 
                                                 drive_info->encl_number, 
                                                 slot_number,
                                                 drive_handle);
    if(status == FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: slot is used by another drive.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    drive_info->slot_number = slot_number;

    /*create the drive*/
    status = fbe_terminator_api_force_create_sata_drive(drive_info, drive_handle);

    /* Insert drive */
    status = fbe_terminator_api_insert_device(enclosure_handle, *drive_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                         "Drive insertion failed\n");
        return status;
    }

    /* Activate the device */
    status = fbe_terminator_api_activate_device(*drive_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                         "Drive activation failed\n");
        return status;
    }
   
    return status;
}

fbe_status_t fbe_terminator_api_force_create_sata_drive (fbe_terminator_sata_drive_info_t *drive_info, fbe_terminator_api_device_handle_t *drive_handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    fbe_terminator_api_device_class_handle_t drive_class_handle;
    const char * drive_type_string;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];
	terminator_drive_t * drive;


    /* find class for Drive */
    status = fbe_terminator_api_find_device_class("Drive", &drive_class_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    /* This is a special case for drive, convert FBE_DRIVE_TYPE_SATA to "FBE_DRIVE_TYPE_SATA" */
    if((status = terminator_fbe_drive_type_to_string(FBE_DRIVE_TYPE_SATA, &drive_type_string))
        != FBE_STATUS_OK)
    {
        return status;
    }
    /* create an enclosure */
    status = fbe_terminator_api_create_device_class_instance(drive_class_handle, drive_type_string, drive_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    tdr_status = fbe_terminator_device_registry_get_device_ptr(*drive_handle, (tdr_device_ptr_t *)&drive);
    if(tdr_status != TDR_STATUS_OK) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
	drive->capacity = drive_info->capacity;
	drive->block_size = drive_info->block_size;

    /* set device attributes */
    /* FIXME: This should be replaced when the int attr setter is available.
    /* Drive type */
    status = terminator_sata_drive_type_to_string(drive_info->drive_type, &drive_type_string);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: could not convert SAS drive type %d to string\n", __FUNCTION__, drive_info->drive_type);
        return status;
    }
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Drive_type", drive_type_string);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }


    /* SAS address */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%llX",
	      (unsigned long long)drive_info->sas_address);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Sas_address", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Serial number */
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "SN", drive_info->drive_serial_number);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Backend number */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", drive_info->backend_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Enclosure number is not passed in drive_info (old API did not use it) */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", drive_info->encl_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Enc", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /*Slot number */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", drive_info->slot_number);
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "Slot_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Product id */
    status = fbe_terminator_api_set_device_attribute(*drive_handle, "PID", drive_info->product_id);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    return status;
}

fbe_status_t fbe_terminator_api_init_with_sp_id(terminator_sp_id_t sp_id)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
	
    terminator_sp_id = sp_id;
    status = fbe_terminator_api_init();
    return status;
}

fbe_status_t fbe_terminator_api_set_sp_id(terminator_sp_id_t sp_id)
{
	terminator_sp_id = sp_id;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_get_sp_id(terminator_sp_id_t *sp_id)
{
	*sp_id = terminator_sp_id;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_set_single_sp(fbe_bool_t is_single)
{
    terminator_is_single_sp_system = is_single;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_is_single_sp_system(fbe_bool_t *is_single)
{
    *is_single = terminator_is_single_sp_system;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_set_cmi_port_base(fbe_u16_t port_base)
{
    cmi_port_base = port_base;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_get_cmi_port_base(fbe_u16_t *port_base)
{
    *port_base = cmi_port_base;
    return FBE_STATUS_OK;
}


fbe_status_t fbe_terminator_api_set_simulated_drive_server_name(const char* server_name)
{
    fbe_zero_memory(simulated_drive_server_name, sizeof(simulated_drive_server_name));
    strncpy(simulated_drive_server_name, server_name, sizeof(simulated_drive_server_name) - 1);
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "set simulated drive server name to [%s]\n", simulated_drive_server_name);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_get_simulated_drive_server_name(char **simulated_server_name_pp)
{
    *simulated_server_name_pp = simulated_drive_server_name;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_set_simulated_drive_server_port(fbe_u16_t port)
{
    simulated_drive_server_port = port;
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "set simulated drive server port to [%d]\n", simulated_drive_server_port);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_get_simulated_drive_server_port(fbe_u16_t *port)
{
    *port = simulated_drive_server_port;
    return FBE_STATUS_OK;
}
