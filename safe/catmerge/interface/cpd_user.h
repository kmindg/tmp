/***************************************************************************
 * Copyright (C) EMC Corp 1997 - 2015
 *
 * All rights reserved.
 * 
 * This software contains the intellectual property of EMC Corporation or
 * is licensed to EMC Corporation from third parties. Use of this software
 * and the intellectual property contained therein is expressly limited to
 * the terms and conditions of the License Agreement under which it is
 * provided by or on behalf of EMC
 **************************************************************************/

/***********************************************************************
 *  cpd_user.h
 ***********************************************************************
 *
 *  DESCRIPTION:
 *      This file contains definitions of "basic" structures used by the
 *      interface with the Multi-Protocol Dual-Mode OS Wrapper, used
 *      internally by the Multi-Protocol Dual-Mode Driver, AND used by
 *      some user-mode applications which may interface (directly or
 *      indirectly) with the Multi-Protocol Dual-Mode OS Wrapper.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      16 Feb 06   DCW     Created
 *      07 Feb 07   Chetan Loke  64bit Porting.
 ***********************************************************************/


/* Make sure we only include this file once */

#ifndef CPD_USER_H
#define CPD_USER_H


/***********************************************************************
 * INCLUDE FILES
 ***********************************************************************/
#include "cpd_generics.h"


#define CPD_IOCTL_HEADER_SIGNATURE "CLARiiON"
#define CPD_IOCTL_HEADER_SIGNATURE_LEN   8


#ifdef ALAMOSA_WINDOWS_ENV 

typedef struct CPD_IOCTL_HEADER_TAG{
    ULONG  HeaderLength;
    UCHAR  Signature[CPD_IOCTL_HEADER_SIGNATURE_LEN];
    ULONG  Timeout;
    ULONG  ControlCode;
    ULONG  ReturnCode;
    ULONG  Length;
}CPD_IOCTL_HEADER;
CPD_IOCTL_HEADER;

#define CPD_IOCTL_HEADER_SET_HEADER_LEN(m_ioctl_hdr, m_len) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->HeaderLength) = (m_len))
#define CPD_IOCTL_HEADER_SET_SIGNATURE(m_ioctl_hdr, m_str_ptr) \
          EmcpalCopyMemory(&(((CPD_IOCTL_HEADER*)m_ioctl_hdr)->Signature), (m_str_ptr),   \
                  CPD_IOCTL_HEADER_SIGNATURE_LEN)
#define CPD_IOCTL_HEADER_SET_TIMEOUT(m_ioctl_hdr, m_tov) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->Timeout) = (m_tov))
#define CPD_IOCTL_HEADER_SET_OPCODE(m_ioctl_hdr, m_opcode) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->ControlCode) = (m_opcode))
#define CPD_IOCTL_HEADER_SET_RETURN_CODE(m_ioctl_hdr, m_retcode) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->ReturnCode) = (m_retcode))
#define CPD_IOCTL_HEADER_SET_LEN(m_ioctl_hdr, m_len) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->Length) = (m_len))

#define CPD_IOCTL_HEADER_GET_HEADER_LEN(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->HeaderLength)
#define CPD_IOCTL_HEADER_GET_SIGNATURE(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->Signature)
#define CPD_IOCTL_HEADER_GET_TIMEOUT(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->Timeout)
#define CPD_IOCTL_HEADER_GET_OPCODE(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->ControlCode)
#define CPD_IOCTL_HEADER_GET_RETURN_CODE(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->ReturnCode)
#define CPD_IOCTL_HEADER_GET_LEN(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->Length)

#else

typedef struct CPD_IOCTL_HEADER_TAG
{
    UINT_32E header_length;
    CHAR     signature[CPD_IOCTL_HEADER_SIGNATURE_LEN];
    UINT_32E timeout_value;
    UINT_32E opcode;
    UINT_32E return_code;
    UINT_32E length;
} CPD_IOCTL_HEADER;

#define CPD_IOCTL_HEADER_SET_HEADER_LEN(m_ioctl_hdr, m_len) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->header_length) = (m_len))
#define CPD_IOCTL_HEADER_SET_SIGNATURE(m_ioctl_hdr, m_str_ptr) \
          EmcpalCopyMemory(&(((CPD_IOCTL_HEADER*)m_ioctl_hdr)->signature), (m_str_ptr),   \
                  CPD_IOCTL_HEADER_SIGNATURE_LEN)
#define CPD_IOCTL_HEADER_SET_TIMEOUT(m_ioctl_hdr, m_tov) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->timeout_value) = (m_tov))
#define CPD_IOCTL_HEADER_SET_OPCODE(m_ioctl_hdr, m_opcode) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->opcode) = (m_opcode))
#define CPD_IOCTL_HEADER_SET_RETURN_CODE(m_ioctl_hdr, m_retcode) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->return_code) = (m_retcode))
#define CPD_IOCTL_HEADER_SET_LEN(m_ioctl_hdr, m_len) \
          ((((CPD_IOCTL_HEADER*)m_ioctl_hdr)->length) = (m_len))

#define CPD_IOCTL_HEADER_GET_HEADER_LEN(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->header_length)
#define CPD_IOCTL_HEADER_GET_SIGNATURE(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->signature)
#define CPD_IOCTL_HEADER_GET_TIMEOUT(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->timeout_value)
#define CPD_IOCTL_HEADER_GET_OPCODE(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->opcode)
#define CPD_IOCTL_HEADER_GET_RETURN_CODE(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->return_code)
#define CPD_IOCTL_HEADER_GET_LEN(m_ioctl_hdr) \
          (((CPD_IOCTL_HEADER*)m_ioctl_hdr)->length)


#endif /* ALAMOSA_WINDOWS_ENV - ARCH - scsi */

#define CPD_IOCTL_HEADER_INIT(m_ioctl_hdr, m_opcode, m_tov, m_length)   \
            CPD_IOCTL_HEADER_SET_HEADER_LEN((m_ioctl_hdr), sizeof(CPD_IOCTL_HEADER));\
            CPD_IOCTL_HEADER_SET_SIGNATURE((m_ioctl_hdr), CPD_IOCTL_HEADER_SIGNATURE);\
            CPD_IOCTL_HEADER_SET_OPCODE((m_ioctl_hdr), (m_opcode));\
            CPD_IOCTL_HEADER_SET_TIMEOUT((m_ioctl_hdr), (m_tov));\
            CPD_IOCTL_HEADER_SET_RETURN_CODE((m_ioctl_hdr), 0);\
            CPD_IOCTL_HEADER_SET_LEN((m_ioctl_hdr), (m_length));

/*
 *  IOCTL definition
 *
 */
#define CPD_IOCTL_DOWNLOAD_BUFFER               0xA1000001
#define CPD_IOCTL_GET_CONFIG                    0xA1000002
#define CPD_IOCTL_GET_NAME                      0xA1000003
#define CPD_IOCTL_SET_NAME                      0xA1000004
#define CPD_IOCTL_GET_ADDRESS                   0xA1000005
#define CPD_IOCTL_SET_ADDRESS                   0xA1000006
#define CPD_IOCTL_GET_ATTRIBUTE                 0xA1000007
#define CPD_IOCTL_SET_ATTRIBUTE                 0xA1000008
#define CPD_IOCTL_AUTHENTICATE                  0xA1000009
#define CPD_IOCTL_GET_PORT_INFO                 0xA100000A
#define CPD_IOCTL_GET_CAPABILITIES              0xA100000B
#define CPD_IOCTL_SET_CAPABILITIES              0xA100000C
#define CPD_IOCTL_GET_STATISTICS                0xA100000D
#define CPD_IOCTL_ADD_DEVICE                    0xA100000E
#define CPD_IOCTL_REMOVE_DEVICE                 0xA100000F
#define CPD_IOCTL_unused_10                     0xA1000010
#define CPD_IOCTL_RETURN_TARGET_INFO            0xA1000011
#define CPD_IOCTL_REGISTER_CALLBACKS            0xA1000012
#define CPD_IOCTL_CALL_FUNCTION_IN_ISR_CONTEXT  0xA1000013
#define CPD_IOCTL_GET_HARDWARE_INFO             0xA1000014
#define CPD_IOCTL_PROCESS_IO                    0xA1000015
#define CPD_IOCTL_DECREMENT_TARGET_ASYNC_EVENT  0xA1000016
#define CPD_IOCTL_GET_MEDIA_INTERFACE_INFO      0xA1000017 
#define CPD_IOCTL_QUERY_TARGET_DEVICE_DATA      0xA1000018
#define CPD_IOCTL_GET_TARGET_INFO               0xA1000019
#define CPD_IOCTL_SET_PORT                      0xA100001A
#define CPD_IOCTL_LUN_ABORTED                   0xA100001B
#define CPD_IOCTL_RESET_DRIVER                  0xA100001C
#define CPD_IOCTL_unused_1D                     0xA100001D
#define CPD_IOCTL_unused_1E                     0xA100001E
#define CPD_IOCTL_unused_1F                     0xA100001F
#define CPD_IOCTL_INITIATOR_LOOP_COMMAND        0xA1000020
#define CPD_IOCTL_RESET_DEVICE                  0xA1000021
#define CPD_IOCTL_RESET_BUS                     0xA1000022
#define CPD_IOCTL_ABORT_IO                      0xA1000023
#define CPD_IOCTL_REGISTER_REDIRECTION          0xA1000024
#define CPD_IOCTL_GET_REGISTERED_DEVICE_LIST    0xA1000025
#define CPD_IOCTL_GET_DEVICE_ADDRESS            0xA1000026
#define CPD_IOCTL_ERROR_INSERTION               0xA1000027
#define CPD_IOCTL_GET_DRIVE_CAPABILITY          0xA1000028
#define CPD_IOCTL_MGMT_PASS_THRU                0xA1000029
#define CPD_IOCTL_SET_LOG_ID_MASK               0xA1000030
#define CPD_IOCTL_FAULT_INSERTION               0xA1000031
#define CPD_IOCTL_SET_SYSTEM_DEVICES            0xA1000032
#define CPD_IOCTL_GET_DEBUG_DUMP                0xA1000033
#define CPD_IOCTL_GPIO_CONTROL                  0xA1000034
#define CPD_IOCTL_ENC_KEY_MGMT                  0xA1000035
#define CPD_IOCTL_GET_PORTAL_INFO               0xA1000036
#define CPD_IOCTL_ENC_SELF_TEST                 0xA1000037
#define CPD_IOCTL_ENC_ALG_TEST                  0xA1000038

/* CPD_DBG IOCTL */
#define CPD_IOCTL_DBG_SET                       0xA1010001
#define CPD_IOCTL_DBG_GET                       0xA1010002
#define CPD_IOCTL_DBG_CLEAR                     0xA1010003
#define CPD_IOCTL_PRINT_DATA                    0xA1010004
#define CPD_IOCTL_DRIVER_NAME                   0xA1010005


/***********************************************************************
 * CPD_HARDWARE_INFO structure
 ***********************************************************************/

typedef struct CPD_HARDWARE_INFO_TAG
{
    UINT_32E    vendor;             // vendor ID
    UINT_32E    device;             // device ID
    UINT_32E    bus;                // I/O bus number
    UINT_32E    slot;               // I/O bus slot number
    UINT_32E    function;           // I/O functions number within device
    UINT_32E    pci_offset;         // For Hilda CNA port only
    UINT_32E    hw_major_rev;       // hardware major rev
    UINT_32E    hw_minor_rev;       // hardware minor rev
    UINT_32E    firmware_rev_1;     // Firmware rev - 4 levels
    UINT_32E    firmware_rev_2;
    UINT_32E    firmware_rev_3;
    UINT_32E    firmware_rev_4;
} CPD_HARDWARE_INFO, * PCPD_HARDWARE_INFO;


#pragma pack(4)
typedef struct S_CPD_HARDWARE_INFO_TAG
{
    CPD_IOCTL_HEADER    ioctl_hdr;
    CPD_HARDWARE_INFO   param;
} S_CPD_HARDWARE_INFO, * PS_CPD_HARDWARE_INFO;
#pragma pack()

/***********************************************************************
 * CPD_MEDIA_INTERFACE_INFO structure
 ***********************************************************************/
#pragma pack(4)
typedef struct S_CPD_MEDIA_INTERFACE_INFO_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_MEDIA_INTERFACE_INFO   param;
} S_CPD_MEDIA_INTERFACE_INFO, * PS_CPD_MEDIA_INTERFACE_INFO;
#pragma pack()

/***********************************************************************
 * CPD_PORT_PROPERTIES structures
 ***********************************************************************/

typedef struct CPD_PORT_PROPERTIES_TAG
{
    BITS_32E        flags;
    UINT_16E        portal_nr;
    UINT_16E        user_port_nr;
} CPD_PORT_PROPERTIES, * PCPD_PORT_PROPERTIES;

#pragma pack(4)
typedef struct S_CPD_PORT_PROPERTIES_TAG
{
    CPD_IOCTL_HEADER        ioctl_hdr;
    CPD_PORT_PROPERTIES     param;
} S_CPD_PORT_PROPERTIES,  * PS_CPD_PORT_PROPERTIES;
#pragma pack()

/* CPD_PORT_PROPERTIES flags bit definitions */
        // Set user port number in driver display name for ktrace logs
#define CPD_PORT_SET_USER_PORT_NUMBER           0x00000001
#define CPD_PORT_DISABLE_TX                     0x00000002
#define CPD_PORT_ENABLE_TX                      0x00000004

/***********************************************************************
 * xx_ADDRESS_PROPERTIES structures
 ***********************************************************************/

typedef struct FC_ADDRESS_PROPERTIES_TAG
{
    UINT_8E         preferred_loop_id;
    UINT_8E         acquired_loop_id;
    FC_ADDRESS_TYPE address_type;
} FC_ADDRESS_PROPERTIES, * PFC_ADDRESS_PROPERTIES;


typedef struct ISCSI_ADDRESS_PROPERTIES_TAG
{
    UINT_16E        portal_group_tag;
    CPD_IP_ADDRESS  subnet_mask;
    CPD_IP_ADDRESS  gateway_ip_address;
    UINT_8E         mac_address[CPD_MAC_ADDR_LEN];
    BITS_32E        flags;
} ISCSI_ADDRESS_PROPERTIES, * PISCSI_ADDRESS_PROPERTIES;

/* ISCSI_ADDRESS_PROPERTIES flags bit definitions */

        // Use default MAC (SET), or MAC address specified is default (GET)
#define ISCSI_ADDR_DEFAULT_MAC                  0x00000001
        // MAC address returned on GET is not valid (port not initialized?)
#define ISCSI_ADDR_MAC_NOT_VALID                0x00000002
        // Delete previous portal definition for this miniport_portal_context
        // ************ OBSOLETE FLAG ************
#define ISCSI_ADDR_DELETE_PORTAL_PROPERTIES     0x00000004

typedef struct SAS_ADDRESS_PROPERTIES_TAG
{
    UINT_8E         unused;
} SAS_ADDRESS_PROPERTIES, * PSAS_ADDRESS_PROPERTIES;

/* Use default MAC (SET), or MAC address specified is default (GET) */
#define FCOE_ADDR_DEFAULT_MAC                  0x00000001

/* MAC address returned on GET is not valid (port not initialized?) */
#define FCOE_ADDR_MAC_NOT_VALID                0x00000002

/* MAC address returned on GET is not valid (port not initialized?) */
#define FCOE_ADDR_VLAN_AUTO_CONFIG             0x00000004

typedef struct FCOE_ADDRESS_PROPERTIES_TAG
{
    UINT_8E     mac_address[CPD_MAC_ADDR_LEN];
    UINT_8E     enode_mac_address[CPD_MAC_ADDR_LEN];
    BITS_32E    flags;
} FCOE_ADDRESS_PROPERTIES, * PFCOE_ADDRESS_PROPERTIES;

typedef struct CPD_ADDRESS_PROPERTIES_TAG
{
    UINT_16E        portal_nr;
    void          * portal_context;
    void          * miniport_portal_context;
    BITS_32E        flags;
    CPD_ADDRESS     address;
    union
    {
        FC_ADDRESS_PROPERTIES       fc;
        ISCSI_ADDRESS_PROPERTIES    iscsi;
        SAS_ADDRESS_PROPERTIES      sas;
        FCOE_ADDRESS_PROPERTIES     fcoe;
    } properties;
} CPD_ADDRESS_PROPERTIES, * PCPD_ADDRESS_PROPERTIES;

#pragma pack(4)
typedef struct S_CPD_ADDRESS_PROPERTIES_TAG
{
    CPD_IOCTL_HEADER          ioctl_hdr;
    CPD_ADDRESS_PROPERTIES  param;
} S_CPD_ADDRESS_PROPERTIES, * PS_CPD_ADDRESS_PROPERTIES;
#pragma pack()

/* CPD_ADDRESS_PROPERTIES flags bit definitions */
        // Do NOT set address, only set portal_context and
        //   return miniport_portal_context
#define CPD_ADDRESS_SET_PORTAL_CONTEXT_ONLY     0x00000001
        // For GET_ADDRESS return the portal_context previously assigned to
        //   this portal; if none was previously assigned, return an error
#define CPD_ADDRESS_GET_PORTAL_CONTEXT          0x00000002
        // Delete previous portal definition for this miniport_portal_context
#define CPD_ADDRESS_DELETE_PORTAL               0x00000004
        // Do NOT initialize controller as a result of this SET_ADDRESS
#define CPD_ADDRESS_DO_NOT_INITIALIZE           0x00000008

/***********************************************************************
 * xx_NAME structures
 ***********************************************************************/

#pragma pack(4)
typedef struct S_CPD_NAME_TAG
{
    CPD_IOCTL_HEADER   ioctl_hdr;
    CPD_NAME         param;
} S_CPD_NAME, * PS_CPD_NAME;
#pragma pack()
 
/***********************************************************************
 * CPD_AUTHENTICATE structure
 ***********************************************************************/
#pragma pack(4)
typedef struct S_CPD_AUTHENTICATE_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_AUTHENTICATE    param;
} S_CPD_AUTHENTICATE, * PS_CPD_AUTHENTICATE;
#pragma pack()


/***********************************************************************
 * CPD_RESET_DRIVER structure
 ***********************************************************************/

typedef enum CPD_RESET_DRIVER_TYPE_TAG
{
    CPD_RESET_DRIVER_RESET                  = 0x00000001,
    CPD_RESET_BUS_RESET                     = 0x00000002,
    CPD_RESET_DRIVER_SHUTDOWN               = 0x00000003,
    CPD_RESET_CTLR_RESET                    = 0x00000004,
    // Don't complete IOCTL until CTLR_RESET is complete
    CPD_RESET_CTLR_RESET_PEND_COMPLETION    = 0x00000005    
} CPD_RESET_DRIVER_TYPE;

#pragma pack(4)
typedef struct CPD_RESET_DRIVER_TAG
{
    CPD_RESET_DRIVER_TYPE       type;
} CPD_RESET_DRIVER, * PCPD_RESET_DRIVER;

typedef struct S_CPD_RESET_DRIVER_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_RESET_DRIVER    param;
} S_CPD_RESET_DRIVER, * PS_CPD_RESET_DRIVER;
#pragma pack()


/***********************************************************************
 * CPD_FAULT_ENTRY/INSERTION structures
 ***********************************************************************/

#pragma pack(4)
typedef struct S_CPD_FAULT_INSERTION_TAG
{
    CPD_IOCTL_HEADER       ioctl_hdr;
    CPD_FAULT_INSERTION  param;
} S_CPD_FAULT_INSERTION, *PS_CPD_FAULT_INSERTION;
#pragma pack()


/***********************************************************************
 * xx_CAPABILITIES structures
 ***********************************************************************/

typedef struct FC_CAPABILITIES_TAG
{
    UINT_16E            retries_before_abort;
    UINT_16E            aborts_before_reset;
    BITS_32E            bits;
} FC_CAPABILITIES, * PFC_CAPABILITIES;

/* FC_CAPABILITIES bits bit definitions */
#define FC_MIXED_SG                             0x00000001
#define FC_RECOVERY_CONFIG                      0x00000002
#define FC_DETAILED_LOGGING                     0x00000004

/* ISCSI_CAPABILITIES ip_formats_supported bit definitions */
    /* see CPD_IP_ADDRESS format bit definitions */

/* ISCSI_CAPABILITIES authentication_mechanisms bit definitions */
    /* see cpd_generics.h */

/* ISCSI_CAPABILITIES encryption_mechanisms bit definitions */
        // Support for IPsec
#define CPD_ENC_IPSEC                           0x00000001

/* ISCSI_CAPABILITIES misc bit definitions */
        // Support for VLANs (virtual LANs)
        // Support for configurable TCP Window Sizes
#define CPD_CAP_VLAN                            0x00000080
#define CPD_CAP_WS_SET_INDEPENDENTLY            0x00000100
#define CPD_CAP_WS_SET_LINKED                   0x00000200


/* SAS_CAPABILITIES misc bit definitions */
        // 
#define CPD_CAP_CONTINUE_AWT                    0x00010000
#define CPD_CAP_BCAST_ASYNC_EVENT               0x00020000
#define CPD_CAP_READY_LED_MEANING               0x00040000

/* MISC capablities bit definitions */
#define CPD_CAP_RESET_CONTROLLER                0x00000002
#define CPD_CAP_VIRTUAL_ADDRESSES_REQUIRED      0x00000004
#define CPD_CAP_MULTIPLE_ADDRESS_NAME_PAIRS     0x00000008
#define CPD_CAP_INDEPENDENT_PORTS               0x00000010
#define CPD_CAP_MULTIPLE_ADDRESSES              0x00000020
#define CPD_CAP_DELETE_PORTAL_DISABLES_NAME     0x00000040
#define CPD_CAP_SFP_CAPABLE                     0x00000080
#define CPD_CAP_MP_CAPABLE                      0x00000100
#define CPD_CAP_ENCRYPTION                      0x00000200
#define CPD_CAP_VIRTUALIZATION_SUPPORT          0x00000400


/*The CPD_CAP_DEFAULT is set where upper layer drivers (such as 
* SCSITarg etc) wants to use the default values supported by 
* underlying iSCSI chip.The CPD_CAP_DEFAULT is applicable to tcp_mtu, 
* authentication_mechanisms & encryption_mechanisms fields in 
*iSCSI_CAPABILITIES.*/
#define CPD_CAP_DEFAULT                         0xFFFFFFFF
/*The CPD_CAP_UNCHANGED is set where upper layer drivers(such as 
* SCSITarg etc)wants to use the existing values set in underlying 
* iSCSI chip's nvram.The CPD_CAP_UNCHANGED is applicable to tcp_mtu, 
* authentication_mechanisms & encryption_mechanisms fields
* in iSCSI_CAPABILITIES.*/
#define CPD_CAP_UNCHANGED                       0xFFFFFFFE

typedef struct ISCSI_CAPABILITIES_TAG
{
    //UINT_16E            max_nr_ips;
    //UINT_16E            max_nr_ips_per_port;
    //#ifdef IPV6_SUPPORTED
    UINT_8E             nr_ipv4;    // max IPv4 addresses on a virtual port
    UINT_8E             nr_ipv6;    // max IPv6 addresses on a virtual port
    UINT_16E            nr_vports;  // num of virtual ports on a physical port
    //#endif
    UINT_16E            max_nr_macs_per_port;
    UINT_16E            max_connections_per_session;
    UINT_32E            min_tcp_mtu;
    UINT_32E            tcp_mtu;
    UINT_32E            min_tcp_window_size;
    UINT_32E            max_tcp_window_size;
    UINT_32E            host_tcp_window_size;
    UINT_32E            app_tcp_window_size;
    BITS_32E            ip_formats_supported;
    BITS_32E            authentication_mechanisms;
    BITS_32E            encryption_mechanisms;
    BITS_32E            flow_control;
    BITS_32E            misc;
    UINT_32E            edc_mode;
} ISCSI_CAPABILITIES, * PISCSI_CAPABILITIES;

typedef struct SAS_CAPABILITIES_TAG
{
    UINT_32E            it_nexus_loss_time;
    UINT_32E            initiator_response_timeout;
    UINT_32E            reject_to_open_limit;
    BITS_32E            misc;
} SAS_CAPABILITIES, * PSAS_CAPABILITIES;

typedef struct FCOE_CAPABILITIES_TAG
{
    UINT_32E            minimum_mtu;
    UINT_32E            maximum_mtu;
    BITS_32E            misc;
} FCOE_CAPABILITIES, * PFCOE_CAPABILITIES;

typedef struct CPD_CAPABILITIES_TAG
{
    UINT_32E            max_transfer_length;
    UINT_16E            max_sg_entries;
    UINT_16E            sg_length;
    UINT_16E            nr_portals;
    UINT_16E            portal_nr;
    BITS_32E            link_speed;
    BITS_32E            misc;
    CPD_SGL_SUPPORT     sgl_support;
    union
    {
        FC_CAPABILITIES     fc;
        ISCSI_CAPABILITIES  iscsi;
        SAS_CAPABILITIES    sas;
        FCOE_CAPABILITIES   fcoe;
    } capabilities;
} CPD_CAPABILITIES, * PCPD_CAPABILITIES;

#pragma pack(4)
typedef struct S_CPD_CAPABILITIES_TAG
{
    CPD_IOCTL_HEADER      ioctl_hdr;
    CPD_CAPABILITIES    param;
} S_CPD_CAPABILITIES, * PS_CPD_CAPABILITIES;
#pragma pack()
 
/***********************************************************************
 * CPD_CONFIG structure
 ***********************************************************************/

typedef enum CPD_PASS_THRU_ERR_TAG
{
    CPD_CONFIG_NO_ERROR                         = 0,
    CPD_CONFIG_TRUNCATED_BY_MINIPORT,
    CPD_CONFIG_BUF_TOO_SMALL,
    CPD_CONFIG_TRUNC_AND_BUF_TOO_SMALL,
    CPD_CONFIG_SYNTAX_ERROR
} CPD_PASS_THRU_ERR;

#define CPD_CONFIG_FORMAT_VERSION               3
// Rev 2 adds additional_continue_ccbs field and moves format version to top
// Rev 3 adds portal_nr field
#define CPD_CONFIG_INTERFACE_REVISION           1
#define CPD_CONFIG_TCD_MIN_ACCEPT_CCBS          65

typedef struct CPD_CONFIG_TAG
{
    UINT_16E            fmt_ver_major;          // currently 3
    UINT_16E            fmt_flags;              // unused
    CPD_CONNECT_CLASS   connect_class;
    UINT_32E            required_accept_ccbs;   // max number of exchanges
    UINT_16E            portal_nr;              // virtual port number
    BOOL                logins_active;          // miniport initiates logins
    UINT_16E            cdb_total_max;          // max concurrent CDBs
    UINT_16E            cdb_inbound_max;        // max target CDBs
    UINT_16E            k10_drives;             // back-end disks
    UINT_16E            k10_tgts_more;          // non-disk target devices
    UINT_16E            k10_initiators;         // initiator and dual-mode dev.
    CHAR              * pass_thru_buf;
    UINT_16E            pass_thru_buf_len;      // # bytes in pass_thru_buf
    CPD_PASS_THRU_ERR   pass_thru_str_err;
    UINT_16E            interface_rev;          // currently 1
    CHAR              * display_name;           // nice name of port
    UINT_32E            additional_continue_ccbs;   // max number of exchanges
    UINT_32E            max_infrastructure_devices; // max number of infrastructure devices
    BITS_32E            available_speeds;
    BITS_32E            mp_enabled : 1;
    CPD_PROC_MASK       active_proc_mask;
} CPD_CONFIG, * PCPD_CONFIG;

/* Revision 1 of CPD_CONFIG structure - temporary until transition
 * can be mode with all drivers
 */
typedef struct CPD_CONFIG_1_TAG
{
    CPD_CONNECT_CLASS   connect_class;
    UINT_32E            required_accept_ccbs;   // max number of exchanges
    UINT_16E            fmt_ver_major;          // currently 1
    UINT_16E            fmt_flags;              // unused
    BOOL                logins_active;          // miniport initiates logins
    UINT_16E            cdb_total_max;          // max concurrent CDBs
    UINT_16E            cdb_inbound_max;        // max target CDBs
    UINT_16E            k10_drives;             // back-end disks
    UINT_16E            k10_tgts_more;          // non-disk target devices
    UINT_16E            k10_initiators;         // initiator and dual-mode dev.
    CHAR              * pass_thru_buf;
    UINT_16E            pass_thru_buf_len;      // # bytes in pass_thru_buf
    CPD_PASS_THRU_ERR   pass_thru_str_err;
    UINT_16E            interface_rev;          // currently 1
    CHAR              * display_name;           // nice name of port
} CPD_CONFIG_1, * PCPD_CONFIG_1;

#pragma pack(4)
typedef struct S_CPD_CONFIG_TAG
{
    CPD_IOCTL_HEADER   ioctl_hdr;
    CPD_CONFIG       param;
} S_CPD_CONFIG, * PS_CPD_CONFIG;
#pragma pack()

/*************************** CPDCLI *******************************/
/*!
 * \enum    CPD_DBG FLAG
 */
typedef enum
{
    CPD_DBG_FIELD_ALL = 0,
    WSK_API_DEVICE_LOGGING_ENABLED ,
    CPD_DBG_FIELD_MAX,

}CPD_DBG_FLAG_TYPE;

/*
 * CPD_PRINT_FIELD_TYPE
 */
typedef enum 
{
    CPD_PRINT_FIELD_ALL = 0,
    CPD_PRINT_DATA_ATD,
    CPD_PRINT_FIELD_MAX
}CPD_PRINT_FIELD_TYPE;

/* For our operation, we need get a few data from CPD,
 * like display name or driver name, we use the biggest memory for this.
 * */
#define CPD_DBG_DATA_BUF_SIZE 32
typedef struct 
{
    UINT_32E    op;     
    UINT_32E    dbg_field;
    union{
        UINT_32E    value;
        CHAR        data[CPD_DBG_DATA_BUF_SIZE];
    };
    UINT_16E    portal_nr;
}CPD_DBG_CTRL;

#pragma pack(4)
typedef struct
{
    CPD_IOCTL_HEADER    ioctl_hdr;
    CPD_DBG_CTRL        param;
}S_CPD_DBG_CTRL;
#pragma pack()

/* For output the ATD data information, we need more than 4K memory to storage 
 * string data. It also is limited by IOCTL, at least 5K we can work.
 **/
#define CPD_DBG_PRINT_BUF_SIZE (5*1024)

/*
 * PRINT data 
 */
typedef struct
{
    UINT_32E op;
    UINT_32E portal_nr;
    UINT_64E kernel_addr;

    UINT_8E  data[CPD_DBG_PRINT_BUF_SIZE];
}CPD_DBG_PRINT;

#pragma pack(4)
/*
 * For print related data structure data info
 */
typedef struct
{
    CPD_IOCTL_HEADER    ioctl_hdr;
    CPD_DBG_PRINT       param;
}S_CPD_DBG_PRINT;
#pragma pack()
/************************* End CPDCLI *********************************/

/***********************************************************************
 * xx_PORT_INFORMATION structure
 ***********************************************************************/

typedef struct FC_PORT_INFORMATION_TAG
{
    FC_PORT_STATE   port_state;
    FC_CONNECT_TYPE connect_type;
    FC_NAME         fabric_wwn;
} FC_PORT_INFORMATION, * PFC_PORT_INFORMATION;

typedef struct ISCSI_PORT_INFORMATION_TAG
{
    UINT_32E   tcp_mtu;
    UINT_32E   host_tcp_window_size;
    UINT_32E   app_tcp_window_size;
    UINT_32E   desired_edc_mode;
    BITS_32E   flow_control;
    BITS_32E   misc;
} ISCSI_PORT_INFORMATION, * PISCSI_PORT_INFORMATION;

typedef enum 
{
    CPD_LINK_NO_CHANGE = 0,
    CPD_LINK_BROADCAST_CHANGE,
    CPD_LINK_BROADCAST_SES,
    CPD_LINK_DISCOVERY_STARTED,
    CPD_LINK_DISCOVERY_COMPLETED,
    CPD_LINK_DISCOVERY_CHANGES_COMPLETED,
    CPD_LINK_DEGRADED,
    CPD_LINK_ALL_LANES_UP
} SAS_CHANGE_TYPE;

typedef struct SAS_PORT_INFORMATION_TAG
{
    BITS_32E   phy_map;
    BITS_32E   phy_polarity_map;
    UINT_8E    nr_phys;
    UINT_8E    nr_phys_enabled;
    UINT_8E    nr_phys_up;
    SAS_CHANGE_TYPE change_type;
} SAS_PORT_INFORMATION, * PSAS_PORT_INFORMATION;

typedef enum 
{   FCOE_LS_INFO_INIT = 0,
    FCOE_LS_INFO_NO_ADD_INFO,
    FCOE_LS_INFO_NO_FCF
} FCOE_LINK_STATE_INFO;

#pragma pack(4)
typedef struct FCOE_PORT_INFORMATION_TAG
{
    FCOE_PORT_STATE         port_state;
    FCOE_CONNECT_TYPE       connect_type;
    UINT_32E                current_mtu;
    FCOE_NAME               fabric_wwn;
    FCOE_LINK_STATE_INFO    link_state_info;
} FCOE_PORT_INFORMATION, *PFCOE_PORT_INFORMATION;
#pragma pack()

#define CPD_PORT_INFO_VIRTUALIZATION_ENABLED 0x00000001
#define CPD_PORT_INFO_FAULTED                0x00000002

typedef struct CPD_PORT_INFORMATION_TAG
{
    UINT_16E        portal_nr;
    CPD_LINK_STATUS link_status;
    BITS_32E        link_speed;
    CPD_ENC_MODE    encryption_mode;    
    BITS_32E        failover_flags;
    union
    {
        FC_PORT_INFORMATION    fc;
        ISCSI_PORT_INFORMATION iscsi;
        SAS_PORT_INFORMATION   sas;
        FCOE_PORT_INFORMATION  fcoe; 
    } port_info;
} CPD_PORT_INFORMATION, * PCPD_PORT_INFORMATION;

#pragma pack(4)
typedef struct S_CPD_PORT_INFORMATION_TAG
{
    CPD_IOCTL_HEADER        ioctl_hdr;
    CPD_PORT_INFORMATION    param;
} S_CPD_PORT_INFORMATION, * PS_CPD_PORT_INFORMATION;
#pragma pack()

/* CPD_PORTAL_INFO flags bit definitions */
#define CPD_PORTAL_INFO_VIRTUALIZATION_ACTIVE 0x00000001

typedef struct CPD_PORTAL_INFORMATION_TAG
{
    /* The caller has to set this field */
    UINT_16E portal_nr;

    /* The following portal-specific information
     * is looked up using the portal_nr value
     */
    BITS_32E flags; /* See CPD_PORTAL_INFO flags bit definitions above */
} CPD_PORTAL_INFORMATION, * PCPD_PORTAL_INFORMATION;

#pragma pack(4)
typedef struct S_CPD_PORTAL_INFORMATION_TAG
{
    CPD_IOCTL_HEADER        ioctl_hdr;
    CPD_PORTAL_INFORMATION  param;
} S_CPD_PORTAL_INFORMATION, * PS_CPD_PORTAL_INFORMATION;
#pragma pack()

/***********************************************************************
 * CPD_STATISTICS structure
 ***********************************************************************/

typedef struct CPD_STATISTICS_TAG
{
    BITS_32E            flags;
    BITS_32E            statistics_type;
    UINT_16E            portal_nr;
    UINT_16E            element_index;
    void              * miniport_login_context;
    UINT_32E            login_qualifier;
    union
    {
        FC_STATISTICS_LESB          fc_lesb;
        CPD_STATISTICS_INTERFACES   mib2_interfaces;
        CPD_STATISTICS_IP           mib2_ip;
        CPD_STATISTICS_TCP          mib2_tcp;
        ISCSI_STATISTICS_INSTANCE   iscsi_instance;
        ISCSI_STATISTICS_PORTAL     iscsi_portal;
        ISCSI_STATISTICS_NODE       iscsi_node;
        ISCSI_STATISTICS_SESSION    iscsi_session;
        ISCSI_STATISTICS_CONNECTION iscsi_connection;
        SAS_PHY_INFO                sas_phy;
    } stats_block;
} CPD_STATISTICS, * PCPD_STATISTICS;

#pragma pack(4)
typedef struct S_CPD_STATISTICS_TAG
{
    CPD_IOCTL_HEADER  ioctl_hdr;
    CPD_STATISTICS    param;
} S_CPD_STATISTICS, * PS_CPD_STATISTICS;
#pragma pack()

/* CPD_STATISTICS flags bit definitions */
#define CPD_CLEAR_STATS                     0x00000001
#define CPD_STATS_SEND_TEST_DATA            0x80000000

/***********************************************************************
 * CPD_GPIO_CONTROL structure
 ***********************************************************************/

typedef struct CPD_GPIO_CONTROL_TAG
{
    UINT_16E            portal_nr;
    CPD_GPIO_OPERATION  gpio_op;
    BITS_32E            pin_mask;           // specify pins affected
    BITS_32E            pin_values;
    void *              callback_handle;    // specify callback (for reads)
} CPD_GPIO_CONTROL, * PCPD_GPIO_CONTROL;

#pragma pack(4)
typedef struct S_CPD_GPIO_CONTROL
{
    CPD_IOCTL_HEADER        ioctl_hdr;
    CPD_GPIO_CONTROL        param;
} S_CPD_GPIO_CONTROL, * PS_CPD_GPIO_CONTROL;
#pragma pack()

/***********************************************************************
 * CPD_IOCTL_RESET_DEVICE structure
 ***********************************************************************/

// Defined in cpd_generics.h
// indicates (for SAS) to reset device indicated by miniport_login_context;
// otherwise, reset the device attached to "phy_nr" of device indicated by
//   miniport_login_context
// #define CPD_RESET_SPECIFIED_DEVICE     0xFF

typedef struct CPD_RESET_DEVICE_TAG
{
    UINT_16E   portal_nr;
    UINT_8E    phy_nr;  // CPD_RESET_SPECIFIED_DEVICE if
                        // mlc points to device to reset
    void     * miniport_login_context;

} CPD_RESET_DEVICE, * PCPD_RESET_DEVICE;
#pragma pack(4)
typedef struct S_CPD_RESET_DEVICE_TAG
{
    CPD_IOCTL_HEADER   ioctl_hdr;
    CPD_RESET_DEVICE param;  // miniport_login_context
} S_CPD_RESET_DEVICE, * PS_CPD_RESET_DEVICE;
#pragma pack()


/***********************************************************************
 * CPD_SET_LOG_ID_MASK structure
 ***********************************************************************/

#define CPD_SET_LOG_ID_MASK_REVISION     1

typedef enum CPD_IOCTL_SET_LOG_ID_MASK_OP_TAG
{
    CPD_LOG_SET_MASK          = 0x00000001,
    CPD_LOG_SET_DEFAULT       = 0x00000002,
    CPD_LOG_ADD_MASK          = 0x00000003,
    CPD_LOG_CLEAR_MASK        = 0x00000004,
    CPD_LOG_GET_MASK          = 0x00000005
} CPD_IOCTL_SET_LOG_ID_MASK_OP;

typedef struct CPD_SET_LOG_ID_MASK_TAG
{
    UINT_16E                     portal_nr;    
    CPD_IOCTL_SET_LOG_ID_MASK_OP op;
    BITS_32E                     mask;
} CPD_SET_LOG_ID_MASK, * PCPD_SET_LOG_ID_MASK;

#pragma pack(4)
typedef struct S_CPD_SET_LOG_ID_MASK_TAG
{
    CPD_IOCTL_HEADER       ioctl_hdr;
    CPD_SET_LOG_ID_MASK  param;
} S_CPD_SET_LOG_ID_MASK, *PS_CPD_SET_LOG_ID_MASK;
#pragma pack()


#endif  /* CPD_USER_H */
