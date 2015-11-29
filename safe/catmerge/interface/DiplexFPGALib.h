#ifndef __DIPLEX_FPGA_LIB__
#define __DIPLEX_FPGA_LIB__

/**********************************************************************
 *      Company Confidential
 *          Copyright (c) EMC Corp, 2003
 *          All Rights Reserved
 *          Licensed material - Property of EMC Corp
 **********************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          DiplexFPGALib.h
 *
 *  Description:
 *          Function prototypes for the Diplex FPGA functions 
 *          exported as a statically linked library.
 *
 *  Revision History:
 *          09-Jun-05       Phil Leef   Created
 *          
 **********************************************************************/

#include "spid_types.h"
#include "diplex_FPGA_micro.h"

#ifdef __cplusplus
extern "C"
{
#endif

void DiplexFPGALibInitDiplexFPGAAttr(SPID_PLATFORM_INFO platformInfo);

typedef struct _FPGA_ATTR_REQUEST
{
    FPGA_REG_BLOCK          regBlock;
    char                    niceName[40];
    UCHAR                   readOffset;
    ULONG                   readByteCount;
    UCHAR                   writeOffset;
    ULONG                   writeByteCount;
    BOOL                    writeable;
    BOOL                    valid;
} FPGA_ATTR_REQUEST, *PFPGA_ATTR_REQUEST;

extern FPGA_ATTR_REQUEST fpgaAttr[FPGA_TOTAL_BLOCK];

#ifdef __cplusplus
}
#endif

#endif // __DIPLEX_FPGA_LIB__
