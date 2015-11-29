#ifndef SPM_API_H
#define SPM_API_H

/************************************************************************************
 * Copyright (C) EMC Corporation  2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws of the United States
 ***********************************************************************************/

/************************************************************************************
 *  spm_api.h
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This file is the header file that contains typedefs and defines used by
 *   other drivers to interface to the SPM driver.
 *
 *  HISTORY:
 *      16-Aug-2007     Joe Perry       Created.
 *
 ***********************************************************************************/

#define SPM_DEVICE_PATH_PREFIX               "\\Device\\"
#define SPM_DEVICE_PATH_PREFIX_LENGTH        sizeof(SPM_DEVICE_PATH_PREFIX)

#define SPM_DEVICE_LINK_PREFIX               "\\DosDevices\\"
#define SPM_DEVICE_LINK_PREFIX_LENGTH        sizeof(SPM_DEVICE_LINK_PREFIX)

#define SPM_SERIAL_NAME_PREFIX               "Serial"
#define SPM_SERIAL_NAME_PREFIX_LENGTH        sizeof(SPM_SERIAL_NAME_PREFIX)

#define SPM_COM_NAME_PREFIX                  "COM"
#define SPM_COM_NAME_PREFIX_LENGTH           sizeof(SPM_COM_NAME_PREFIX)

#define SPM_MPLXR_NAME_PREFIX               "MPLXR"
#define SPM_MPLXR_NAME_PREFIX_LENGTH        sizeof(SPM_MPLXR_NAME_PREFIX)

#define SPM_NT_MPLXR_DEVICE_NAME            SPM_DEVICE_PATH_PREFIX SPM_MPLXR_NAME_PREFIX

//  Define the register operations in the multiplexor chip.
#define MPLX_REGISTER_READ          0x80
#define MPLX_REGISTER_WRITE         0xC0


// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.  (Quoted from "/ddk/inc/devioctl.h".)

#define DEVICE_SPM		0x90FA		// unique but arbitrary  0x90FA = 37114.

//  The following control codes are for operations related to the HH FPGA.
#define IOCTL_SPM_GET_LED_BLINK_RATE    EMCPAL_IOCTL_CTL_CODE(DEVICE_SPM, 1, EMCPAL_IOCTL_METHOD_BUFFERED, \
                                                 EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_SPM_SET_LED_BLINK_RATE    EMCPAL_IOCTL_CTL_CODE(DEVICE_SPM, 2, EMCPAL_IOCTL_METHOD_BUFFERED, \
                                                 EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_SPM_READ_REGISTER         EMCPAL_IOCTL_CTL_CODE(DEVICE_SPM, 3, EMCPAL_IOCTL_METHOD_BUFFERED, \
                                                 EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_SPM_WRITE_REGISTER        EMCPAL_IOCTL_CTL_CODE(DEVICE_SPM, 4, EMCPAL_IOCTL_METHOD_BUFFERED, \
                                                 EMCPAL_IOCTL_FILE_ANY_ACCESS)
#define IOCTL_SPM_GET_PARAMETERS        EMCPAL_IOCTL_CTL_CODE(DEVICE_SPM, 5, EMCPAL_IOCTL_METHOD_BUFFERED, \
                                                 EMCPAL_IOCTL_FILE_ANY_ACCESS)

//  Following is used to build a register read or write request to a multiplexor.
//  It is also to hold the response to one of these requests, as currently both
//  the request and response have the same format.
typedef struct _MPLXR_REGISTER_BUFFER
{
    UINT_8              RegOp;
    UINT_8              RegAddr;
    UINT_8              Data;
    UINT_8              Checksum;
} MPLXR_REGISTER_BUFFER, *PMPLXR_REGISTER_BUFFER;

#define MPLXR_REQUEST_INIT(m_mplxr_req_p,m_mplxr_reg_op,m_mplxr_reg_addr,m_mplxr_data,m_mplxr_checksum)\
        ((m_mplxr_req_p)->RegOp     = (m_mplxr_reg_op),     \
         (m_mplxr_req_p)->RegAddr   = (m_mplxr_reg_addr),   \
         (m_mplxr_req_p)->Data      = (m_mplxr_data),       \
         (m_mplxr_req_p)->Checksum  = (m_mplxr_checksum))

/********************
 * Mplxr Devices
 ********************/
typedef enum _MPLXR_DEVICE
{
    MPLXR_DEVICE_INVALID,

    LOCAL_IOSLOT_0_MPLXR,
    LOCAL_IOSLOT_1_MPLXR,
    LOCAL_IOSLOT_2_MPLXR,
    LOCAL_IOSLOT_3_MPLXR,
    LOCAL_IOSLOT_4_MPLXR,
    LOCAL_IOSLOT_5_MPLXR,
    LOCAL_IOSLOT_6_MPLXR,
    LOCAL_IOSLOT_7_MPLXR,
    LOCAL_IOSLOT_8_MPLXR,
    LOCAL_IOSLOT_9_MPLXR,
    LOCAL_IOSLOT_10_MPLXR,
    LOCAL_IOSLOT_11_MPLXR,

    TOTAL_MPLXR_DEVICES,
} MPLXR_DEVICE;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _MPLXR_DEVICE
 */
inline _MPLXR_DEVICE operator++( MPLXR_DEVICE &mplxr_id, int )
{
   return mplxr_id = (MPLXR_DEVICE)((int)mplxr_id + 1);
}
}
#endif

#endif  //  SPM_API_H

//  End of file  -  spm_api.h
