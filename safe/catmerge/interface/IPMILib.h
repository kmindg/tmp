#ifndef __IPMI_LIB__
#define __IPMI_LIB__

/**********************************************************************
 *      Company Confidential
 *          Copyright (c) EMC Corp, 2012
 *          All Rights Reserved
 *          Licensed material - Property of EMC Corp
 **********************************************************************/

/**********************************************************************
 *
 *  File Name: 
 *          IPMILib.h
 *
 *  Description:
 *          Function prototypes for the IPMI functions 
 *
 *  Revision History:
 *          12-Mar-15      Mark Tsombakos   Created
 *          
 **********************************************************************/

#ifdef CSX_BV_LOCATION_USER_SPACE
#include <stdlib.h>
#endif
#include "spid_types.h"
#include "ipmi.h"
#include "K10Assert.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define IPMI_BUS_ARBITRATION_TIMEOUT    5           // 5 second bus arb timeout
#define IPMI_1_SECOND_UNITS             10000000    // Scaling factor to calculate seconds from ticks
#define IPMILIB_DEFAULT_RETRY_COUNT     4           // Number of retry attempts for IPMI requests

// Wait time between retries (in milliseconds)
#define IPMILIB_DEFAULT_RETRY_WAIT_TIME IPMI_INTERFACE_REVIVE_INTERVAL_IN_MSECS

void IPMILibInitIPMIAttr(SPID_PLATFORM_INFO platformInfo,
                         SP_ID              spid);

typedef struct _IPMI_ATTR_REQUEST
{
    IPMI_COMMAND            command;
    char                    niceName[40];
    UCHAR                   netFn;
    UCHAR                   readCmd;
    UCHAR                   writeCmd;
    UCHAR                   readByteCount;
    UCHAR                   readParamByteCount;
    UCHAR                   writeByteCount;
    BOOL                    writeable;
    UCHAR                   args[32];
    BOOL                    valid;
} IPMI_ATTR_REQUEST, *PIPMI_ATTR_REQUEST;

extern IPMI_ATTR_REQUEST ipmiAttr[TOTAL_IPMI_COMMANDS];
extern UINT ipmiTraceDebugErrorFlag;
extern UINT ipmiTraceDebugVerboseFlag;

#ifndef CSX_BV_LOCATION_USER_SPACE
// Macros for kernel space start
#define ipmiLibTraceDebugVerbose    if (ipmiTraceDebugVerboseFlag) KvPrint
#define ipmiLibTraceError           if (ipmiTraceDebugErrorFlag) KvPrint
#if DBG
// Debug Tactic, used to detect Memory Leaks
extern ULONG ipmiDbgBytesAllocated;

#define ipmiLibAllocPool(m_data_ptr, m_data_type, m_pool, m_count)              \
    (m_data_ptr = (m_data_type *)                                               \
                    EmcpalAllocateUtilityPoolWithTag(m_pool, m_count, 'impi'),  \
     (ipmiDbgBytesAllocated += (m_data_ptr) ? m_count : 0));
#define ipmiLibFreePool(m_data_ptr)                                             \
    ((ipmiDbgBytesAllocated -= sizeof(*m_data_ptr)),                            \
                    EmcpalFreeUtilityPool(m_data_ptr))

#else
#define ipmiLibAllocPool(m_data_ptr, m_data_type, m_pool, m_count)              \
    m_data_ptr = (m_data_type *)                                                \
                    EmcpalAllocateUtilityPoolWithTag(m_pool, m_count, 'impi');
#define ipmiLibFreePool(m_data_ptr)                                             \
                    EmcpalFreeUtilityPool(m_data_ptr)
#endif
// Macros for kernel space end

#else
// Macros for user space start
#define ipmiLibTraceDebugVerbose
#define ipmiLibTraceError
#if DBG
// Debug Tactic, used to detect Memory Leaks
extern ULONG ipmiDbgBytesAllocated;

#define ipmiLibAllocPool(m_data_ptr, m_data_type, m_pool, m_count)  \
    (m_data_ptr = (m_data_type *)                                   \
                    malloc(m_count),                                \
     (ipmiDbgBytesAllocated += (m_data_ptr) ? m_count : 0));
#define ipmiLibFreePool(m_data_ptr)                                 \
    ((ipmiDbgBytesAllocated -= sizeof(*m_data_ptr)),                \
                    free(m_data_ptr))

#else
#define ipmiLibAllocPool(m_data_ptr, m_data_type, m_pool, m_count)  \
    m_data_ptr = (m_data_type *)                                    \
                    malloc(m_count);
#define ipmiLibFreePool(m_data_ptr)                                 \
                    free(m_data_ptr)

#endif
// Macros for user space end
#endif

EMCPAL_STATUS
issueIPMIRequest(PIPMI_REQUEST  pIpmiRequest,
                 PIPMI_RESPONSE pIpmiResponse,
                 UCHAR          peerReq,
                 ULONG32        retryCount);

EMCPAL_STATUS
IPMIRawCmd(UCHAR            netfn,
           UCHAR            cmd,
           UCHAR            peerReq,
           PUCHAR           pInData,
           UCHAR            length,
           PIPMI_RESPONSE   pIPMIResponse);

EMCPAL_STATUS
IPMISetBMCAddr(IPMI_SET_BMC_ADDR_TYPE type,
               UCHAR addr);

#if 0
#ifdef C4_INTEGRATED

#define C4_IPMI_DEVICE_NAME         "/dev/ipmi0"

#define C4_IPMI_MAX_ADDR_SIZE       0x20
#define C4_IPMI_BMC_CHANNEL         0xf
#define C4_IPMI_NUM_CHANNELS        0x10

#define C4_IPMI_SYSTEM_INTERFACE_ADDR_TYPE      0x0c
#define C4_IPMI_IPMB_ADDR_TYPE                  0x01
#define C4_IPMI_IPMB_BROADCAST_ADDR_TYPE        0x41

#define C4_IPMI_RESPONSE_RECV_TYPE              1
#define C4_IPMI_ASYNC_EVENT_RECV_TYPE           2
#define C4_IPMI_CMD_RECV_TYPE                   3

typedef struct _C4_IPMI_ADDR 
{
    int     addr_type;
    short   channel;
    char    data [C4_IPMI_MAX_ADDR_SIZE];
}
C4_IPMI_ADDR, *PC4_IPMI_ADDR;

typedef struct _C4_IPMI_MSG 
{
    unsigned char       netfn;
    unsigned char       cmd;
    unsigned short      data_len;
    unsigned char       *data;
}
C4_IPMI_MSG, *PC4_IPMI_MSG;

typedef struct _C4_IPMI_REQ 
{
    unsigned char       *addr;
    unsigned int        addr_len;
    long                msgid;
    C4_IPMI_MSG         msg;
}
C4_IPMI_REQ, *PIPMI_REQ;

typedef struct _C4_IPMI_RECV 
{
    int             recv_type;
    unsigned char   *addr;
    unsigned int    addr_len;
    long            msgid;
    C4_IPMI_MSG     msg;
}
C4_IPMI_RECV, *PC4_IPMI_RECV;

typedef struct _C4_IPMI_SYSTEM_INTERFACE_ADDR 
{
    int             addr_type;
    short           channel;
    unsigned char   lun;
}
C4_IPMI_SYSTEM_INTERFACE_ADDR, *PC4_IPMI_SYSTEM_INTERFACE_ADDR;

#define C4_IPMI_IOC_MAGIC                   'i'
#define C4_IPMICTL_RECEIVE_MSG_TRUNC        _IOWR(IPMI_IOC_MAGIC, 11, struct _C4_IPMI_RECV)
#define C4_IPMICTL_RECEIVE_MSG              _IOWR(IPMI_IOC_MAGIC, 12, struct _C4_IPMI_RECV)
#define C4_IPMICTL_SEND_COMMAND             _IOR(IPMI_IOC_MAGIC, 13, struct _C4_IPMI_REQ)
#define C4_IPMICTL_SET_GETS_EVENTS_CMD      _IOR(IPMI_IOC_MAGIC, 16, int)
#define C4_IPMICTL_SET_MY_ADDRESS_CMD       _IOR(IPMI_IOC_MAGIC, 17, unsigned int)
#define C4_IPMICTL_GET_MY_ADDRESS_CMD       _IOR(IPMI_IOC_MAGIC, 18, unsigned int)
#define C4_IPMICTL_SET_MY_LUN_CMD           _IOR(IPMI_IOC_MAGIC, 19, unsigned int)
#define C4_IPMICTL_GET_MY_LUN_CMD           _IOR(IPMI_IOC_MAGIC, 20, unsigned int)


#endif /* C4_INTEGRATED - C4HW */
#endif // 0


#ifdef __cplusplus
}
#endif

#endif // __IPMI_LIB__
