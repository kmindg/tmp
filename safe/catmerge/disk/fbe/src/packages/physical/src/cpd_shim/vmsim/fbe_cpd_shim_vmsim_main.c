#include "fbe_terminator_miniport_interface.h"
#include "fbe/fbe_terminator_kernel_api.h"

fbe_status_t
cpd_shim_initialize_miniport_interface(fbe_terminator_get_interface_t* terminator_ifc)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    EMCPAL_STATUS nt_status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_DEVICE_OBJECT pDeviceObject = NULL;
    PEMCPAL_FILE_OBJECT pFileObject = NULL;

    // Form the NT device name for the Terminator, and retrieve a pointer to the
    // Terminator device object.
    nt_status = EmcpalExtDeviceOpen( FBE_TERMINATOR_DEVICE_NAME,
                                       FILE_ALL_ACCESS,
                                       &pFileObject,
                                       &pDeviceObject );

    if (EMCPAL_IS_SUCCESS(nt_status)) {
        PEMCPAL_IRP pIrp = NULL;

        pIrp = EmcpalExtIrpBuildIoctl(FBE_TERMINATOR_GET_INTERFACE_IOCTL,
            pDeviceObject, NULL, 0, terminator_ifc, sizeof(fbe_terminator_get_interface_t),
            FALSE, NULL, NULL);
        if (pIrp != NULL) {
            // Send the request to the Terminator.  The Terminator will always complete
            // the handshake within the context of the request.
            nt_status = EmcpalExtIrpSendAsync( pDeviceObject, pIrp );

            // NOTE: The Terminator does not queue a handshake request for further processing.
            // The Terminator always returns STATUS_SUCCESS for a handshake request.  If any
            // other return value is returned, attempt to catch it here.
            if (nt_status != EMCPAL_STATUS_SUCCESS)
            {
                KvPrint( "%s Could not handshake with the Terminator. 0x%x\n",
                    __FUNCTION__, nt_status);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                status = FBE_STATUS_OK;
            }
        }
        else
        {
            // We cannot allocate an IRP to handshake with the Terminator.
            status = FBE_STATUS_INSUFFICIENT_RESOURCES;

            KvPrint( "%s Could not allocate handshake IRP. 0x%x\n",
                    __FUNCTION__, nt_status);
        }

        EmcpalExtDeviceClose(pFileObject );
    }

    return status;
}

fbe_status_t
fbe_cpd_shim_init(void)
{
    fbe_status_t status = FBE_STATUS_INVALID;

    fbe_terminator_get_interface_t terminator_ifc = {0};

    // Get terminator miniport interface
    status = cpd_shim_initialize_miniport_interface(&terminator_ifc);
    if ( FBE_STATUS_OK != status )
    {
        KvPrint( "%s Could not initialize cpd shim miniport 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_cpd_shim_sim_set_terminator_miniport_pointers(&terminator_ifc.miniport_api);
    if ( FBE_STATUS_OK != status )
    {
        KvPrint( "%s Could not initialize cpd shim sim 0x%x\n",
                __FUNCTION__, status);
        return status;
    }

    return status;
}
