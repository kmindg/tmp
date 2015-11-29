#ifndef LRU_LIB_EXPORT_H
#define LRU_LIB_EXPORT_H
//***************************************************************************
// Copyright (C) EMC Corporation 2002
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// FILE:
//      LruLibExport.h
//
// DESCRIPTION:
//      This file contains exported function prototypes and
//      data structures.  Macros used for access to data structures
//      are also defined here
//
// NOTES:
//
// HISTORY:
//      10-Jun-02 Brian Parry -- Created.
//      14-Jun-03 Phil Leef - Removed enumerated data type to describe hardware platforms. 
//                              SPID is now the authoritative source for determining h/w.
//--
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//++
// Data Structures
//--

//++
// NAME:
//      LRU_REQUEST_DATA
//
// DESCRIPTION:
//      16-bit value used to pass request parameters
//      to and from the calling client.
//--
typedef unsigned short LRU_REQUEST_DATA, *PLRU_REQUEST_DATA;
//End LRU_REQUEST_DATA


//++
// End Data Structures
//--

//++
// Literals
//--

// API declaration
#define LRULIB_API CSX_MOD_EXPORT

// X1 LRU Register Offsets
#define LRU_ENCL_INFO_REG_OFFSET				0x00
#define LRU_ENCL_STAT_REG_OFFSET				0x01
    // bit masks into LRU_ENCL_STAT_REG_OFFSET
    #define LRU_ENCL_STAT_REG_MFG_MODE          0x01
#define LRU_PSA_STAT_REG_OFFSET					0x10
#define LRU_PSB_STAT_REG_OFFSET					0x11
#define LRU_POWER_DRIVE_CTRL_1_REG_OFFSET		0x12
#define LRU_POWER_DRIVE_CTRL_2_REG_OFFSET		0x13
#define LRU_DRV_INS_1_REG_OFFSET				0x20
#define LRU_DRV_INS_2_REG_OFFSET				0x21
#define LRU_DRV_FLT_1_REG_OFFSET				0x22
#define LRU_DRV_FLT_2_REG_OFFSET				0x23
#define LRU_DRV_FLT_OE_1_REG_OFFSET				0x24
#define LRU_DRV_FLT_OE_2_REG_OFFSET				0x25
#define LRU_DRV_BYP_REQ_1_REG_OFFSET			0x26
#define LRU_DRV_BYP_REQ_2_REG_OFFSET			0x27
#define LRU_DRV_PBC_ODIS_1_REG_OFFSET			0x28
#define LRU_DRV_PBC_ODIS_2_REG_OFFSET			0x29
#define LRU_DRV_PBC_CTRL_1_REG_OFFSET			0x2A
#define LRU_DRV_PBC_CTRL_2_REG_OFFSET			0x2B
#define LRU_TRUSTED_DRIVE_1_REG_OFFSET			0x2C
#define LRU_TRUSTED_DRIVE_2_REG_OFFSET			0x2D
#define LRU_UART_RESETS_REG_OFFSET				0x30
#define LRU_LRU_REV_REG_OFFSET					0x31
#define LRU_UART_INTERRUPTS_REG_OFFSET			0x32
#define LRU_CBL_PORT_STAT_REG_OFFSET			0x33
#define LRU_CBL_PORT_CTRL_REG_OFFSET			0x34
#define LRU_MISC_STAT_REG_OFFSET				0x35
    // bit masks into LRU_ENCL_STAT_REG_OFFSET
	#define LRU_MISC_STAT_REG_OTHER_SP_INS		0x04
#define LRU_VOLTAGE_MARGIN_REG_OFFSET			0x36

// Additional X1 Lite LRU Register Offsets.
#define LRU_ENCL_FE_INT_MASK_REG_OFFSET         0x02
#define LRU_ENCL_FE_CTRL_REG_OFFSET             0x03
#define LRU_MISC_FE_INT_STATUS_REG_OFFSET       0x39
#define LRU_MISC_BOARD_ID_REG_OFFSET            0x3A

//++
// End Literals
//-- 

//++
// Macros
//--

//++
// NAME:
//      LRU_REQ_SET_OFFSET
//
// DESCRIPTION:
//      Sets LRU register offset in a given request
//
// ARGUMENTS:
//      m_req_p - Pointer to LRU_REQUEST_DATA type
//      m_offset - Desired register offset
//--
#define LRU_REQ_SET_OFFSET(m_req_p, m_offset) (*m_req_p |= (m_offset << 8))

//++
// NAME:
//      LRU_REQ_GET_OFFSET
//
// DESCRIPTION:
//      Gets LRU register offset in a given request
//
// ARGUMENTS:
//      m_req_p - Pointer to LRU_REQUEST_DATA type
//--
#define LRU_REQ_GET_OFFSET(m_req_p) (UCHAR) ((*m_req_p & 0xFF00) >> 8)

//++
// NAME:
//      LRU_REQ_SET_DATA
//
// DESCRIPTION:
//      Sets data bits of LRU_REQUEST_DATA type
//
// ARGUMENTS:
//      m_req_p - Pointer to LRU_REQUEST_DATA type
//      m_data - Data bits to set.
//--
#define LRU_REQ_SET_DATA(m_req_p, m_data) (*m_req_p |= m_data)

//++
// NAME:
//      LRU_REQ_GET_DATA
//
// DESCRIPTION:
//      Gets data bits of LRU_REQUEST_DATA type
//
// ARGUMENTS:
//      m_req_p - Pointer to LRU_REQUEST_DATA type
//--
#define LRU_REQ_GET_DATA(m_req_p) (UCHAR)(*m_req_p & 0x00FF)

//++
// End Macros
//--

//++
// API Function prototypes
//--

LRULIB_API EMCPAL_STATUS
LruLibMapRegister(
                  IN  UCHAR   offset
                 );
// Map LRU register into NonPagedSystem space

LRULIB_API VOID
LruLibUnmapRegister(
                    IN UCHAR offset
                   );
// Unmap LRU register

LRULIB_API EMCPAL_STATUS
LruLibReadMappedRegister(
                         IN OUT PLRU_REQUEST_DATA pLruRequest
                        );
// Read from a previously mapped register

LRULIB_API EMCPAL_STATUS
LruLibWriteMappedRegister(
                          IN PLRU_REQUEST_DATA pLruRequest
                         );
// Write to a previously mapped register

LRULIB_API EMCPAL_STATUS
LruLibReadRegister(
                   IN OUT PLRU_REQUEST_DATA pLruRequest
                  );
// Read LRU register using dynamic mapping

LRULIB_API EMCPAL_STATUS
LruLibWriteRegister(
                    IN PLRU_REQUEST_DATA pLruRequest
                   );
// Write LRU register using dynamic mapping


//++
// End Function Prototypes
//--
#endif // LRU_LIB_EXPORT_H
//++
// End LruLibExport.h
//--
