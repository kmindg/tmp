/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-11
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_topology_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debug functions for topology Service.
.*
 * @version
 *   16-June-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#include "fbe/fbe_topology_interface.h"
#include "fbe_topology_debug.h"


char *fbe_topology_object_getStatusString (fbe_topology_object_status_t topoObjectStatus)
{
    char objectStatusStringTemp[25] = {'\0'};
    char *objectStatusString = &objectStatusStringTemp[0];

    switch (topoObjectStatus)
    {
        case FBE_TOPOLOGY_OBJECT_STATUS_NOT_EXIST:
            objectStatusString = "NotExist";
            break;
        case FBE_TOPOLOGY_OBJECT_STATUS_RESERVED:
            objectStatusString = "Reserved";
            break;
        case FBE_TOPOLOGY_OBJECT_STATUS_EXIST:
            objectStatusString = "Exist";
            break;
        case FBE_TOPOLOGY_OBJECT_STATUS_READY:
            objectStatusString = "Ready";
            break;
        default:
            sprintf(objectStatusString, "Undefined (0x%x)", topoObjectStatus); 
            break;
    }

    return objectStatusString;
}

//end of fbe_topology_debug.c
