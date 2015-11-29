/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-2007
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * c4blkshim.h
 ***************************************************************************
 *
 * File Description:
 *
 *   Common data types used for registering device for blockshim access.
 *
 * Author:
 *   Tarek Haidar
 *
 * Revision History:
 *
 ***************************************************************************/

#ifndef __C4BLKSHIM_H__
#define __C4BLKSHIM_H__

#include "csx_ext.h"

#ifdef C4_INTEGRATED

#ifdef _C4BLKSHIM_
#define C4BLKSHIMAPI CSX_MOD_EXPORT
#else
#define C4BLKSHIMAPI CSX_MOD_IMPORT
#endif

#define C4_BLKSHIM_MAX_DEV_NAME_LEN   32

#define C4_BLKSHIM_GLOBAL_IF_NAME  "C4BLKSHIM:GlobalInterface"

#define C4_BLKSHIM_USE_PREALLOCATED_IO_MEMORY  0xFFFE
#define C4_BLKSHIM_USE_NEW_IO_MEMORY           0xFFFF


typedef struct _C4_BLKSHIM_DEVICE {
    char           		    device_name[C4_BLKSHIM_MAX_DEV_NAME_LEN];
    void                    * device_handle;
    unsigned int            dcall_levels_required;
    csx_u32_t               sector_size;
    csx_u32_t               dev_size_in_sectors;
    csx_u32_t               max_sectors_per_req;
    csx_status_e  (*device_read_write) (csx_p_device_handle_t,csx_p_dcall_body_t *);
    /* set by the block-shim */
    csx_virt_address_t      pBufmanShareMemVirtBase;
    csx_phys_address_t      pBufmanShareMemPhysBase;
    csx_size_t              shareMemSize;	
    int                     force_ordered_requests;
    unsigned int            dev_number;
    unsigned int            shared_io_memory_dev_number;
} C4_BLKSHIM_DEVICE;

#if defined(__cplusplus)
extern "C"
{
#endif

C4BLKSHIMAPI csx_status_e  c4blkshim_register_device(C4_BLKSHIM_DEVICE *devInfo);

#if defined(__cplusplus)
}
#endif

#endif /* C4_INTEGRATED - C4ARCH - blockshim_fakeport */
#endif // _C4BLKSHIM_H_
