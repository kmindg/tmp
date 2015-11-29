/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 *  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 *  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ***************************************************************************/

#ifndef _HORNET_PCI_HAL_H_
#define	_HORNET_PCI_HAL_H_
 
#include "csx_ext.h"
#include "HornetInterface.h"


#ifdef __cplusplus 
struct _HORNET_TRANSPORT;

extern "C" 
{
#endif

    typedef enum _CB_AREA_TYPE
    {
        CB_LINK_AREA,
        CB_TRANSPORT_AREA,
        CB_CONTROL_AREA
    } CB_AREA_TYPE;

    CSX_GLOBAL csx_status_e HornetTransport_SendOutWindow(struct _HORNET_TRANSPORT *pTransport, CB_AREA_TYPE area);
#ifndef SIMMODE_ENV
    CSX_GLOBAL csx_status_e HornetTransport_DMAWrite(struct _HORNET_TRANSPORT *pTransport, csx_phys_address_t dst, csx_phys_address_t src, csx_size_t length);
    CSX_GLOBAL csx_status_e HornetTransport_DMAWriteList(struct _HORNET_TRANSPORT *pTransport, CSX_CONST hornet_sg_element_t *list, csx_nuint_t count);
#endif


#ifdef __cplusplus 
} // extern C
#endif

#endif // _HORNET_PCI_HAL_H_
