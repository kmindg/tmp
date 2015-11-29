#include "fbe_terminator_device_registry.h"
#include "fbe_terminator.h"
#include "terminator_port.h"
#include "terminator_fc_port.h"
#include "terminator_fc_enclosure.h"
#include "fbe_terminator_common.h"


/* Some useful TRACE macros */
#define TRACE_MISSING_ACCESSOR(attribute, device_class) terminator_trace(FBE_TRACE_LEVEL_ERROR,\
    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,\
    "%s: could not find TCM accessor for %s attribute of %s\n",\
    __FUNCTION__, attribute, device_class);

#define TRACE_SETTER_FAILURE(value, attribute, device_class) terminator_trace(FBE_TRACE_LEVEL_ERROR,\
    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,\
    "%s: could not set value %s for %s attribute of %s\n", __FUNCTION__, value, attribute, device_class);

#define TRACE_TCM_CLASS_MISSING(device_class)  terminator_trace(FBE_TRACE_LEVEL_ERROR,\
    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,\
    "%s: could not find TCM class for %s\n", __FUNCTION__, device_class);


fbe_status_t fbe_terminator_api_get_board_handle(fbe_terminator_api_device_handle_t * board_handle)
{
    tdr_device_handle_t board_handles[1];
    fbe_terminator_api_device_class_handle_t board_class_handle;
    fbe_status_t status;

    /* find class for board */
    status = fbe_terminator_api_find_device_class("Board", &board_class_handle);
    if(status != FBE_STATUS_OK)
    {
        TRACE_TCM_CLASS_MISSING("Board")
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* search for board */
    if( fbe_terminator_device_registry_enumerate_devices_by_type(
        board_handles,1, (tdr_device_type_t)(csx_ptrhld_t)board_class_handle) != TDR_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, "Could not find handle for a board\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    * board_handle = board_handles[0];
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_get_port_handle(fbe_u32_t port_number, fbe_terminator_api_device_handle_t *port_handle)
{
    fbe_terminator_api_device_handle_t *port_list;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_u32_t i, number_of_ports;
    fbe_u32_t current_port_number = -1;
    tdr_status_t status;
    fbe_terminator_api_device_class_handle_t class_handle;
    fbe_status_t api_status;
    fbe_port_type_t port_type;

    /* find class for port */
    api_status = fbe_terminator_api_find_device_class("Port", &class_handle);
    if(api_status != FBE_STATUS_OK)
    {
        TRACE_TCM_CLASS_MISSING("Port")
        return api_status;
    }

    /* get all the ports from device registry */
    number_of_ports = fbe_terminator_device_registry_get_device_count_by_type((tdr_device_type_t)(csx_ptrhld_t)class_handle);
    port_list = fbe_terminator_allocate_memory(number_of_ports * sizeof(tdr_device_handle_t));
    if (port_list == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(port_list, number_of_ports * sizeof(tdr_device_handle_t));

    status = fbe_terminator_device_registry_enumerate_devices_by_type(port_list,
                                                                      number_of_ports,
                                                                      (tdr_device_type_t)(csx_ptrhld_t)class_handle);
    if(status != TDR_STATUS_OK)
    {
    fbe_terminator_free_memory(port_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    for(i=0; i<number_of_ports; i++)
    {
        status = fbe_terminator_device_registry_get_device_ptr(port_list[i], &port_ptr);

        port_type = port_get_type((terminator_port_t *)port_ptr);
        if(port_type == FBE_PORT_TYPE_SAS_PMC)
            {
            current_port_number = sas_port_get_backend_number ((terminator_port_t *)port_ptr);
            }
        else if(port_type == FBE_PORT_TYPE_FC_PMC)
            {
            current_port_number = fc_port_get_backend_number ((terminator_port_t *)port_ptr);
            }
        else if(port_type == FBE_PORT_TYPE_ISCSI) /* TEMP HACK for ISCSI*/
            {
            current_port_number = fc_port_get_backend_number ((terminator_port_t *)port_ptr);
            }
         else if(port_type == FBE_PORT_TYPE_FCOE) /* TEMP HACK for FCOE*/
            {
            current_port_number = fc_port_get_backend_number ((terminator_port_t *)port_ptr);
            }
        
        if ( current_port_number == port_number )
        {
            *port_handle = port_list[i];
            fbe_terminator_free_memory(port_list);
            return FBE_STATUS_OK;
        }
    }
    fbe_terminator_free_memory(port_list);
    return FBE_STATUS_GENERIC_FAILURE;
}

/* obtain enclosure handle by port number and number of enclosure */
fbe_status_t fbe_terminator_api_get_enclosure_handle(fbe_u32_t port_number,
                                                     fbe_u32_t enclosure_number,
                                                     fbe_terminator_api_device_handle_t * enclosure_handle)
{
    fbe_terminator_api_device_handle_t port_handle;
    fbe_terminator_device_ptr_t port_ptr = NULL;
    terminator_enclosure_t * encl;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;

    /* find a port */
    status = fbe_terminator_api_get_port_handle(port_number, &port_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    encl = port_get_enclosure_by_enclosure_number((terminator_port_t *)port_ptr, enclosure_number);
    if(encl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: could not find an enclosure with id = %u on port %u\n",
            __FUNCTION__, enclosure_number, port_number);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_terminator_device_registry_get_device_handle((fbe_terminator_device_ptr_t)encl, enclosure_handle);
        return FBE_STATUS_OK;
    }
}

/* obtain drive handle by port number, number of enclosure and slot number*/
fbe_status_t fbe_terminator_api_get_drive_handle(fbe_u32_t port_number,
                                                 fbe_u32_t enclosure_number,
                                                 fbe_u32_t slot_number,
                                                 fbe_terminator_api_device_handle_t * drive_handle)
{
    fbe_terminator_api_device_handle_t encl_handle = 0;
    fbe_terminator_device_ptr_t encl_ptr, drive_ptr;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;

    if(drive_handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* find an enclosure */
    status = fbe_terminator_api_get_enclosure_handle(port_number, enclosure_number, &encl_handle);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl_handle, &encl_ptr);
    if(tdr_status == TDR_STATUS_OK && encl_ptr != NULL)
    {
        terminator_enclosure_t * encl = (terminator_enclosure_t *)encl_ptr;
        switch (encl->encl_protocol)
        {
            case FBE_ENCLOSURE_TYPE_FIBRE:
                status = fc_enclosure_get_device_ptr_by_slot(encl, slot_number, &drive_ptr);
            break;
            case FBE_ENCLOSURE_TYPE_SAS:
                status = sas_enclosure_get_device_ptr_by_slot(encl, slot_number, &drive_ptr);
            break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
            break;    
        }
        
        if((status == FBE_STATUS_OK)&&(drive_ptr != NULL))
        {
            fbe_terminator_device_registry_get_device_handle(drive_ptr, drive_handle);
            return FBE_STATUS_OK;
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    /* change to INFO since fbe_terminator_api_insert_sas_drive() use this function to make sure
     * no drive exists.  We expect this message for insert sas and sata drives.
     */
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, FBE_TRACE_MESSAGE_ID_INFO,
        "%s: could not find a drive with slot number = %u, enclosure number = %u and port number = %u\n",
        __FUNCTION__, slot_number, enclosure_number, port_number);

    return status;
}



/* obtain total device counts for boards, ports, enclosures, and drives*/
fbe_status_t fbe_terminator_api_count_terminator_devices(fbe_terminator_device_count_t *dev_counts)
{
    fbe_status_t status;
    fbe_terminator_api_device_class_handle_t class_handle;

    memset(dev_counts, 0, sizeof(*dev_counts));

    status = fbe_terminator_api_find_device_class("Board", &class_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: could not enumerate the board devices\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get all the ports from device registry */
    dev_counts->num_boards = fbe_terminator_device_registry_get_device_count_by_type((tdr_device_type_t)class_handle);

    /* find class for port */
    status = fbe_terminator_api_find_device_class("Port", &class_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: could not enumerate the port devices\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get all the ports from device registry */
    dev_counts->num_ports = fbe_terminator_device_registry_get_device_count_by_type(CSX_CAST_PTR_TO_PTRMAX(class_handle));

    /* find class for Enclosure */
    status = fbe_terminator_api_find_device_class("Enclosure", &class_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: could not enumerate the enclosure devices\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* get all the ports from device registry */
    dev_counts->num_enclosures = fbe_terminator_device_registry_get_device_count_by_type(CSX_CAST_PTR_TO_PTRMAX(class_handle));

    /* find class for Drive */
    status = fbe_terminator_api_find_device_class("Drive", &class_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: could not enumerate the drive devices\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get all the ports from device registry */
    dev_counts->num_drives = fbe_terminator_device_registry_get_device_count_by_type(CSX_CAST_PTR_TO_PTRMAX(class_handle));

    dev_counts->total_devices = 
        dev_counts->num_boards +
        dev_counts->num_ports +
        dev_counts->num_enclosures +
        dev_counts->num_drives; // no more logical drives

    return FBE_STATUS_OK;
}

