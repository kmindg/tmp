#ifndef __SMBUS_DRIVER__
#define __SMBUS_DRIVER__

//***************************************************************************
// Copyright (C) Data General Corporation 1989-2011
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      smbus_driver.h
//
// Contents:
//      Definitions of the exported IOCTL codes and data structures
//      that form the interface between the SMBus driver and its higher-
//		level clients.
//
// Revision History:
//  5-Sept-01   Brian Parry    Created.
//  29-June-05  NQIU           Added timeout to the request structure.
//  15-Mar-06   Phil Leef      Exporting switch access
//--

#include "generic_types.h"
#include "EmcPAL_Ioctl.h"

//
// Exported constants
//

//++
// Description:
//      The device object pathnames of the SMBus device.
//      SMBus clients must open this device in order to send SMBus
//      the ioctl requests described below.
//--
#ifndef ALAMOSA_WINDOWS_ENV
#define SMBUS_BASE_DEVICE_NAME_ANSI  "SMBus"
#define SMBUS_NT_DEVICE_NAME_ANSI    "\\Device\\" SMBUS_BASE_DEVICE_NAME_ANSI
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE */
#define SMBUS_BASE_DEVICE_NAME	"SMBus"
#define SMBUS_NT_DEVICE_NAME	"\\Device\\" SMBUS_BASE_DEVICE_NAME
#define SMBUS_DOSDEVICES_NAME	"\\DosDevices\\" SMBUS_BASE_DEVICE_NAME
#define SMBUS_WIN32_DEVICE_NAME	"\\\\.\\" SMBUS_BASE_DEVICE_NAME

//++
// Driver specific status codes
//--
#define STATUS_SMB_DEV_ERR                          (EMCPAL_STATUS)0xA8000000 // Device error
#define STATUS_SMB_BUS_ERR                          (EMCPAL_STATUS)0xA8000001 // Bus collision / busy
#define STATUS_SMB_ARB_ERR                          (EMCPAL_STATUS)0xA8000002 // Bus arbitration error
#define STATUS_SMB_FAILED_ERR                       (EMCPAL_STATUS)0xA8000003 // Failed error (occurs when a 'kill' cmd is sent to terminate the transaction 
#define STATUS_SMB_TRANSACTION_IN_PROGRESS          (EMCPAL_STATUS)0xA8000004 // Host Controller is busy
#define STATUS_SMB_BYTE_DONE_COMPLETE               (EMCPAL_STATUS)0xA8000005 // A byte of data was transfered successfully
#define STATUS_SMB_TRANSACTION_COMPLETE             (EMCPAL_STATUS)0xA8000006 // The transaction has terminated by signalling the INTR bit. 
#define STATUS_SMB_DEVICE_NOT_PRESENT               (EMCPAL_STATUS)0xA8000007 // Device not physically inserted
#define STATUS_SMB_INVALID_BYTE_COUNT               (EMCPAL_STATUS)0xA8000008 // Invalid Byte Count specified
#define STATUS_SMB_DEVICE_NOT_VALID_FOR_PLATFORM    (EMCPAL_STATUS)0xA8000009 // Invalid Device for platform
#define STATUS_SMB_SELECTBITS_FAILED                (EMCPAL_STATUS)0xA800000A // Failed to set the SelectBits on the SuperIO Mux.
#define STATUS_SMB_IO_TIMEOUT                       (EMCPAL_STATUS)0xA800000B // IO Timeout error on bus.
#define STATUS_SMB_INSUFFICIENT_RESOURCES           (EMCPAL_STATUS)0xA800000C // An errorTs could not be allocated 
#define STATUS_SMB_DEVICE_POWERED_OFF               (EMCPAL_STATUS)0xA800000D // The device is powered OFF
#define STATUS_SMB_SENSOR_READING_UNAVAILABLE       (EMCPAL_STATUS)0xA800000E // IPMI Sensor reading unavailable

#define MAX_NUMBER_OF_SMB_ERRORS ((STATUS_SMB_DEVICE_POWERED_OFF & 0xFFFF) + 1)

/* move _SMB_DEVICE enum to smb_device_enum.h */
#include "smbus_device_enum.h"

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _SMB_DEVICE
 */
inline _SMB_DEVICE operator++( SMB_DEVICE &smbDev, int )
{
   return smbDev = (SMB_DEVICE)((int)smbDev + 1);
}
}
#endif



//*********************************************
    /* Maintain 'some' backwards compatability */
//*********************************************
#define CHASSIS_SERIAL_EEPROM   MIDPLANE_RESUME_PROM

#ifndef C4_INTEGRATED
typedef ULONG SMB_DEVICE_ADDRESS;
#else
typedef csx_u64_t SMB_DEVICE_ADDRESS;
#endif /* C4_INTEGRATED - C4BUG - seems suspicious */

//Special SMB_DEVICE_ADDRESS
#define SMB_INVALID_DEVICE_ADDRESS 0xFFFFFFFF
//++
// Exported structures
//--.
#if defined(__cplusplus)
#pragma warning( push )
#pragma warning( disable : 4200 )
#endif
typedef struct _SMB_REQUEST
{
    SMB_DEVICE          smbus_device;   //The SMBUS device as defined above
    SMB_DEVICE_ADDRESS  smbus_address;  // I2C address of the device
    #ifndef C4_INTEGRATED  /* to make sure sturct size is the same in kernel and user space */
    ULONG               offset;         // Offset into device
    ULONG               byte_count;     // Number of bytes to read/write
    PUCHAR              buffer;         // Data to read/write
    ULONG               time_out;       // Caller specified timeout value

    struct _SMB_REQUEST *next_ptr;
    #else
    csx_u64_t           offset;         // Offset into device
    csx_u64_t           byte_count;     // Number of bytes to read/write
    csx_u8_t*           buffer;      /* unsigned char * */   // Data to read/write
    csx_u64_t           time_out;       // Caller specified timeout value

    csx_u64_t           next_ptr;  /* struct _SMB_REQUEST * */
    #endif /* C4_INTEGRATED - C4BUG - seems suspicious */

    /********************************************
     **** Maintain as last element of struct ****
     ********************************************/
    UCHAR       user_input_buffer[0];    // Input buffer to be used for IOCTL from user-space

} SMB_REQUEST, *PSMB_REQUEST;
#if defined(__cplusplus)
#pragma warning( pop ) 
#endif
//++
// Macros exported to calling drivers
//--

//++
// Name: 
//     SMB_REQUEST_INIT
//
// Description:
//     Initializes an SMB_REQUEST structure
//     based on the given arguments and default
//     values.
//
//     Note that for now the address is set to invalid.  This
//     Will be filled in when the request is processed.
//--
#ifndef C4_INTEGRATED
#define SMB_REQUEST_INIT(m_smb_req_p,m_device,m_offset,m_count,m_buff_p,m_timeout) \
    ((m_smb_req_p)->smbus_device    = (m_device),                   \
     (m_smb_req_p)->smbus_address   = SMB_INVALID_DEVICE_ADDRESS,   \
     (m_smb_req_p)->offset          = (m_offset),                   \
     (m_smb_req_p)->byte_count      = (m_count),                    \
     (m_smb_req_p)->buffer          = (m_buff_p),                   \
     (m_smb_req_p)->time_out        = (m_timeout),                  \
     (m_smb_req_p)->next_ptr        = NULL)
#else
#define SMB_REQUEST_INIT(m_smb_req_p,m_device,m_offset,m_count,m_buff_p,m_timeout) \
    ((m_smb_req_p)->smbus_device    = (m_device),                   \
     (m_smb_req_p)->smbus_address   = SMB_INVALID_DEVICE_ADDRESS,   \
     (m_smb_req_p)->offset          = (csx_u64_t) (m_offset),       \
     (m_smb_req_p)->byte_count      = (csx_u64_t) (m_count),        \
     (m_smb_req_p)->buffer          = (csx_u8_t*) (m_buff_p),       \
     (m_smb_req_p)->time_out        = (csx_u64_t)(m_timeout),       \
     (m_smb_req_p)->next_ptr        = (csx_u64_t) NULL)
#endif /* C4_INTEGRATED - C4BUG - seems suspicious */


//++
// Name: 
//     SMB_REQUEST_ADD_TO_CHAIN
//
// Description:
//     Add a new request to the given chain    
//--
#ifndef C4_INTEGRATED
#define SMB_REQUEST_ADD_TO_CHAIN(m_head_p, m_new_p)\
{                                               \
    struct _SMB_REQUEST* m_cur_p = (m_head_p);  \
    while (m_cur_p)                             \
    {                                           \
        if (m_cur_p->next_ptr == NULL)          \
        {                                       \
            m_cur_p->next_ptr = (m_new_p);      \
            break;                              \
        }                                       \
        m_cur_p = m_cur_p->next_ptr;            \
    }                                           \
}
#else
#define SMB_REQUEST_ADD_TO_CHAIN(m_head_p, m_new_p)         \
{                                                           \
    struct _SMB_REQUEST* m_cur_p = (m_head_p);              \
    while (m_cur_p)                                         \
    {                                                       \
        if (m_cur_p->next_ptr == 0)                         \
        {                                                   \
          m_cur_p->next_ptr = (csx_u64_t)(m_new_p);         \
            break;                                          \
        }                                                   \
        m_cur_p = (struct _SMB_REQUEST *)m_cur_p->next_ptr; \
    }                                                       \
}
#endif /* C4_INTEGRATED - C4BUG - seems suspicious */


//++
// Exported IOCTL codes
//--
#define SMBUS_IOCTL_BASE                    0xC00
#define SMBUS_IOCTL_READ_OFFSET             0x000
#define SMBUS_IOCTL_WRITE_OFFSET            0x001
#define SMBUS_IOCTL_REPORT_DEVICE_OFFSET    0x002

#define SMBUS_IOCTL_CODE(m_offset) (SMBUS_IOCTL_BASE + m_offset)

#define IOCTL_SMBUS_READ   \
EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, SMBUS_IOCTL_CODE(SMBUS_IOCTL_READ_OFFSET), EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_SMBUS_WRITE  \
EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, SMBUS_IOCTL_CODE(SMBUS_IOCTL_WRITE_OFFSET), EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define SMBUS_IOCTL_REPORT_DEVICE  \
EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, SMBUS_IOCTL_CODE(SMBUS_IOCTL_REPORT_DEVICE_OFFSET), EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#endif //__SMBUS_DRIVER__
