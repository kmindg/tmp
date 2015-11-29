/***************************************************************************
 * Copyright (C) EMC Corp 1997 - 2014
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
 *  cpd_generics.h
 ***********************************************************************
 *
 *  DESCRIPTION:
 *      This file contains definitions of "basic" structures used by the
 *      interface with the Multi-Protocol Dual-Mode OS Wrapper, and used
 *      internally by the Multi-Protocol Dual-Mode Driver.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      27 Mar 03   DCW     Created
 *
 ***********************************************************************/


/* Make sure we only include this file once */

#ifndef CPD_GENERICS_H
#define CPD_GENERICS_H


/***********************************************************************
 * INCLUDE FILES
 ***********************************************************************/

#ifndef UEFI
#include "generic_types.h"
#else
#include "cpd_uefi_types.h"
#endif

/***************************************************************
 * MACROS
 ***************************************************************/

#if defined(ALAMOSA_WINDOWS_ENV) || defined(UEFI)
#define CPD_COMPILE_TIME_ASSERT(id, condition)  \
        typedef char __C_ASSERT_ ## id ## __[(condition)?1:-1]
#else
#define CPD_COMPILE_TIME_ASSERT(id, condition)   \
        CSX_ASSERT_AT_COMPILE_TIME_GLOBAL_SCOPE(condition)
#endif /* ALAMOSA_WINDOWS_ENV - COLLAPSE - use CSX compile time asserts */


/***************************************************************
 * TYPEDEFS
 ***************************************************************/

/* Add a signed character type not included in generics.h */

typedef signed char INT_8E;

/***********************************************************************
 * CPD_IP_ADDRESS structure
 ***********************************************************************/

#define CPD_IP_ADDRESS_V4_LEN 4
#define CPD_IP_ADDRESS_V6_LEN 16
#define CPD_IP_ADDRESS_FQDN_LEN 256

/* The maximum number of IP addresses supported per Physical Port*/
#define CPD_MAX_IP_ADDR_PER_PORT  64 // 32 VLANS * 2 (1 IPv4, 1 IPv6) IPs per VLAN


/* CPD_IP_ADDRESS format bit definitions */
#define CPD_IP_V4                               0x00000001
#define CPD_IP_V6                               0x00000002
#define CPD_IP_FQDN                             0x00000004


/* Flag bit definition */
#define CPD_NAME_INITIATOR            0x00000001
#define CPD_NAME_TARGET               0x00000002
#define CPD_NAME_SET_ALIAS_ONLY       0x00000004
#define CPD_NAME_DELETE_PORTAL        0x00000008



typedef struct CPD_IP_ADDRESS_TAG
{
    BITS_32E        format;
    union
    {
        UINT_8E         v4[CPD_IP_ADDRESS_V4_LEN];
        UINT_8E         v6[CPD_IP_ADDRESS_V6_LEN];
    } ip;
} CPD_IP_ADDRESS, * PCPD_IP_ADDRESS;


typedef struct CPD_IP_ADDRESS_EXT_TAG
{
    BITS_32E        format;
    union
    {
        UINT_8E         v4[CPD_IP_ADDRESS_V4_LEN];
        UINT_8E         v6[CPD_IP_ADDRESS_V6_LEN];
        CHAR            fqdn[CPD_IP_ADDRESS_FQDN_LEN];
    } ip;
} CPD_IP_ADDRESS_EXT, * PCPD_IP_ADDRESS_EXT;



/***********************************************************************
 * xx_ADDRESS structure
 ***********************************************************************/

typedef struct ISCSI_ADDRESS_TAG
{
    CPD_IP_ADDRESS      ip_address;
    UINT_16E            tcp_port_number;
    UINT_32E            vlan_id;
} ISCSI_ADDRESS, * PISCSI_ADDRESS;

typedef struct ISCSI_ADDRESS_EXT_TAG
{
    CPD_IP_ADDRESS_EXT  ip_address;
    UINT_16E            tcp_port_number;
    UINT_32E            vlan_id;
} ISCSI_ADDRESS_EXT, * PISCSI_ADDRESS_EXT;

typedef union SAS_LOCATOR_TAG
{
    UINT_32E            locator;
    struct
    {
        UINT_8E         phy_nr;  // attached expander's PHY nr for this device
        UINT_8E         exp_nr;  // attached expander's discovery order among
                                 // expanders at same depth
        UINT_8E         depth;   // nr expanders between device and controller
        UINT_8E         encl;    // enclosure ID for this device        
    } s;
} SAS_LOCATOR, * PSAS_LOCATOR;


/*             ADDRESS                  */
typedef struct FCOE_ADDRESS_TAG
{
    UINT_32E         vlan_tag;
    UINT_32E         address_id;
} FCOE_ADDRESS;

typedef struct CPD_ADDRESS_TAG
{
    union
    {
        UINT_32E        fc;
        ISCSI_ADDRESS   iscsi;
        SAS_LOCATOR     sas;
        FCOE_ADDRESS    fcoe;
    } address;
} CPD_ADDRESS, * PCPD_ADDRESS;


/***********************************************************************
 * xx_NAME structures
 ***********************************************************************/
#pragma pack(4)
typedef struct FC_NAME_TAG
{
    UINT_64E        node_name;
    UINT_64E        port_name;
} FC_NAME, * PFC_NAME;
#pragma pack()

#define ISCSI_NAME_NAME_MAX_SIZE                256
#define ISCSI_NAME_ALIAS_MAX_SIZE               256

typedef struct ISCSI_NAME_TAG
{
    CHAR            name[ISCSI_NAME_NAME_MAX_SIZE];
    CHAR            alias[ISCSI_NAME_ALIAS_MAX_SIZE];
} ISCSI_NAME, * PISCSI_NAME;

typedef struct SAS_NAME_TAG
{
    UINT_64E        device_name;
    UINT_64E        sas_address;
} SAS_NAME, * PSAS_NAME;

typedef struct FCOE_NAME_TAG
{
    UINT_64E        node_name;
    UINT_64E        port_name;
} FCOE_NAME, * PFCOE_NAME;

typedef struct CPD_NAME_TAG
{
    union
    {
        FC_NAME         fc;
        ISCSI_NAME      iscsi;
        SAS_NAME        sas;
        FCOE_NAME       fcoe;
    } name;
    UINT_16E    portal_nr;
    BITS_32E    flags;
} CPD_NAME, * PCPD_NAME;


/***********************************************************************
 * CPD_FAULT_ENTRY/INSERTION structures
 ***********************************************************************/

#define CPD_FAULT_DESCR_SIZE          32
#define CPD_FAULT_PARAM_UNUSED        (((UINT_64E)0)-1)

/* Define status for use with CPD_FAULT_ENTRY */

#define CPD_FAULT_ENABLED             1
#define CPD_FAULT_TRIGGERED           2
#define CPD_FAULT_COMPLETED           3
#define CPD_FAULT_VERIFIED            4
#define CPD_FAULT_STOPPED             5

/* Define commands for use with CPD_FAULT_ENTRY */

#define CPD_FAULT_START               11
#define CPD_FAULT_STOP                12


typedef struct CPD_FAULT_ENTRY_TAG
{
    UINT_8E    module_index;    // CPD_FAULT_DEFAULT, unless scripting
                                // to allow more static fault_index

    UINT_8E    fault_index;     // index within miniport's fauilt table;
                                // or within module's fault table if
                                // module_index specified.

    UINT_8E    command;         // CPD_FAULT_XXX
                                // COMMANDS (write-only):
                                //   START - starts fault insertion for this
                                //           fault 
                                //   STOP  - stops fault insertion for this
                                //           fault 

    UINT_8E    status;          // CPD_FAULT_XXX
                                //   ENABLED   - fault has been started and
                                //               is active  
                                //   TRIGGERED - fault has been inserted at
                                //               least once, and is still
                                //               active.
                                //   COMPLETED - fault has been inserted
                                //                number of requested times
                                //                and is inactive.
                                //   VERIFIED  - fault has been inserted AND
                                //               verified the requested amount
                                //   STOPPED   - fault has been stopped and
                                //               is inactive

    UINT_16E   modulus;         // fault inserted every modulus occurences of
                                // base event (0 and 1 are equivalent)
    
    UINT_16E   modulus_down_count;  // number of base events remaining until
                                    // next insertion
    
    UINT_16E   down_count;      // number of times to insert fault (on reads,
                                // number of insertions remaining)

    UINT_16E   ver_down_count;  // number of faults inserted for which a
                                // verification event has not been received.

    CHAR      (* name)[CPD_FAULT_DESCR_SIZE];   // display name of fault

    UINT_64E   parameter;       // set to default by miniport on GET -
                                // CPD_FAULT_PARAM_UNUSED if not used.
                                // written with value to be use by UL
    
} CPD_FAULT_ENTRY, *PCPD_FAULT_ENTRY;

/* Define revision of CPD_FAULT_INSERTION struct */

#define CPD_FAULT_VERSION             0x01

/* Define commands for use with CPD_FAULT_INSERTION */

#define CPD_FAULT_GET_NR              21
#define CPD_FAULT_GET                 22
#define CPD_FAULT_GET_NAMES           23
#define CPD_FAULT_MODIFY              24
#define CPD_FAULT_DISABLE             25
#define CPD_FAULT_REENABLE            26
#define CPD_FAULT_REINITIALIZE        27

/* Define values used for the status field */
#define CPD_FAULT_INSERTION_DISABLED  FALSE
#define CPD_FAULT_INSERTION_ENABLED   TRUE

typedef struct CPD_FAULT_INSERTION_TAG
{
    UINT_8E           version;  // UL sets to CPD_FAULT_VERSION,
                                // miniport checks

    UINT_8E           command;  // UL sets to CPD_FAULT_XXX
                                // COMMANDS (write-only):
                                //   GET_NR       - returns nr faults in
                                //                  miniport's table. 
                                //   GET          - returns nr_fault_entries
                                //                  table entries from table;
                                //                  name_ptr points to kernel
                                //                  memory strings. 
                                //   GET_NAMES     - same as GET but name_ptr
                                //                  points to memory allocated
                                //                  within IOCTL payload. 
                                //   MODIFY       - modifies miniport table per
                                //                  attached entires.
                                //   DISABLE      - disables all faults while
                                //                  maintaining state.
                                //   REENABLE     - enable all previous active
                                //                  faults disabled by DISABLE
                                //                  command. 
                                //   REINITIALIZE - disables all faults,
                                //                  clearing all state.
    
    UINT_16E          portal_nr;    

    UINT_8E           status;   // indicates status of fault insertion -
                                // CPD_FAULT_INSERTION_DISABLED or
                                // CPD_FAULT_INSERTION_ENABLED 
    
    UINT_8E           nr_fault_entries;
                                // UL sets for GET, GET_NAMES, MODIFY;
                                // minport returns for GET_NR, GET, GET_NAMES,
                                //   MODIFY

    union
    {
        CPD_FAULT_ENTRY * table_ptr;
                                // pointer to array of fault entries used for
                                // MODIFY, GET;
                                // UL allocates space for table
                                // (nr_fault_entries * sizeof(CPD_FAULT_ENTRY);
                                // UL fills in for MODIFY;
                                // if NULL, table immediately follows this
                                // FAULT_INSERTION structure
        CHAR (* names_ptr)[][CPD_FAULT_DESCR_SIZE];
                                // pointer to array of strings used for
                                // GET_NAMES;
                                // UL allocates space for array
                                // (nr_fault_entries * CPD_FAULT_DESCR_SIZE);
                                // if NULL, array immediately follows this
                                // FAULT_INSERTION structure
    } u;

} CPD_FAULT_INSERTION, *PCPD_FAULT_INSERTION;

/*----------------------------------------------------------------------------*/


/* Defines miniport's SGL support. A disparate SGL is considered to be an SGL
 * that requires skipping of the meta-data located between user-data blocks. */

typedef enum CPD_SGL_SUPPORT_TAG
{
    // Meta-data skipping is not supported
    //  (iSCSI QLogic)
    CPD_DISPARATE_SGL_NOT_SUPPORTED = 0x01,
    // Meta-data skipping is supported with a performance penalty
    //  (iSCSI Broadcom)
    CPD_DISPARATE_SGL_NOT_PREFERRED,
    // Meta-data skipping is supported
    //  (FC, FCoE, SAS)
    CPD_DISPARATE_SGL_SUPPORTED
} CPD_SGL_SUPPORT;


/***********************************************************************
 * STRUCTURE DEFINES
 ***********************************************************************/


#define CPD_MAC_ADDR_LEN 6
#define CPD_CTLR_ERROR_INFO_SIZE 2
#define CPD_DRIVER_FAILURE_INFO_SIZE 3


/***********************************************************************
 * ACCEPT_CCB Structure enums
 ***********************************************************************/

typedef enum CPD_TAG_TYPE_TAG
{
    // Note TCD uses bool tests for untagged - so must be 0
    CPD_UNTAGGED                        = 0x00,

    // These values are taken from the SAM spec.
    CPD_SIMPLE_QUEUE_TAG                = 0x20,
    CPD_HEAD_OF_QUEUE_TAG               = 0x21,
    CPD_ORDERED_QUEUE_TAG               = 0x22,
    CPD_ACA_QUEUE_TAG                   = 0x2f,

    // These values below are NOT from the SAM spec. (They are randomly chosen. :-)
    CPD_SATA_NCQ_TAG                    = 0x80,
    CPD_SATA_DMA_UNTAGGED               = 0x81,
    CPD_SATA_PIO_UNTAGGED               = 0x82,
    CPD_SATA_NON_DATA                   = 0x83

} CPD_TAG_TYPE;

typedef enum CPD_ACF_TAG
{
    CPD_ACF_NO_DISCONNECT               = 1,
#ifndef C4_INTEGRATED
     CPD_ACF_CONTINGENT_ALLEGIANCE
#else
    CPD_ACF_CONTINGENT_ALLEGIANCE,
    CPD_ACF_FAKEPORT                    = 4,
    CPD_ACF_BLKSHIM                     = 8
#endif /* C4_INTEGRATED - C4ARCH - blockshim_fakeport */


} CPD_ACF;


/***********************************************************************
 * xx_ADDRESS_PROPERTIES structures
 ***********************************************************************/

typedef enum FC_ADDRESS_TYPE_TAG
{
    FC_ADDRESS_SOFT             = 0x01500001,
    FC_ADDRESS_PREFERRED,
    FC_ADDRESS_HARD
} FC_ADDRESS_TYPE;


/***********************************************************************
 * CPD TARGET INFO STRUCTURES
 ***********************************************************************/

#define CPD_POLL_FIELD_INVALID                                  0x00000001
#define CPD_POLL_FIELD_VALID                                    0x00000002
#define CPD_POLL_FIELD_RLS_BASELINE                             0000000004

#define CPD_POLL_TYPE_RPSC                                      0x00000001
#define CPD_POLL_TYPE_RLS                                       0x00000002
#define CPD_POLL_TYPE_WWN                                       0x00000004


#define CPD_POLL_MAX_RPSC_ENTRIES                               0x2
/* Some data types do not require additional external commands
 * create a mask for these
 */
#define CPD_POLL_TYPE_EXT_CMD_MASK              (CPD_POLL_TYPE_RLS | \
                                                 CPD_POLL_TYPE_RPSC )

#define CPD_POLL_NO_RETRY                                       0x80000000
#define CPD_POLL_RETRY_ERROR                                    0x40000000
#define CPD_POLL_RESET_RETRY                                    0x20000000


#define CPD_POLL_RC_UNSUPPORTED_FUNCTION                        0x00000001
#define CPD_POLL_RC_INCOMPLETE                                  0x00000002
#define CPD_POLL_RC_REJECTED                                    0x00000004
#define CPD_POLL_RC_LINK_UNAVAILABLE                            0x00000008
#define CPD_POLL_RC_NOT_LOGGED_IN                               0x00000010
#define CPD_POLL_RC_FAILED_TIMEOUT                              0x00000020
#define CPD_POLL_RC_FAILED_SELECTION_TIMEOUT                    0x00000040
#define CPD_POLL_RC_FRAME_CREDIT_TIMEOUT                        0x00000080
#define CPD_POLL_RC_INVALID_RESPONSE                            0x00000100
#define CPD_POLL_RC_INTERNAL_ERROR                              0x00000200
#define CPD_POLL_RC_NO_ERROR                                    0x80000000

#define CPD_LS_RPSC_1G_CAP                                      0x80000000
#define CPD_LS_RPSC_2G_CAP                                      0x40000000
#define CPD_LS_RPSC_4G_CAP                                      0x20000000
#define CPD_LS_RPSC_10G_CAP                                     0x10000000
#define CPD_LS_RPSC_8G_CAP                                      0x08000000
#define CPD_LS_RPSC_16G_CAP                                     0x04000000
#define CPD_LS_RPSC_UNKNOWN                                     0x00010000

#define CPD_LS_RPSC_1G_OPER                                     0x00008000
#define CPD_LS_RPSC_2G_OPER                                     0x00004000
#define CPD_LS_RPSC_4G_OPER                                     0x00002000
#define CPD_LS_RPSC_10G_OPER                                    0x00001000
#define CPD_LS_RPSC_8G_OPER                                     0x00000800
#define CPD_LS_RPSC_16G_OPER                            `       0x00000400
#define CPD_LS_RPSC_UNKNOWN_OPER                                0x00000002
#define CPD_LS_RPSC_NOT_ESTABLISHED_OPER                        0x00000001


typedef enum CPD_REMOTE_TARGET_ACTION_TAG
{
    CPD_CONFIG_DATA_POLLING          = 0x00000001,
    CPD_GET_SUPPORTED_FUNCTIONS ,
    CPD_GET_TARGET_DATA ,
    CPD_INITIATE_TARGET_POLL ,
} CPD_REMOTE_TARGET_ACTION;

typedef struct CPD_TARGET_INFO_POLL_CONFIG_TAG
{
        BITS_32E flags;
        BITS_32E invalid_rc;
        void * miniport_login_context;
        BITS_32E type;
        UINT_32E timeout_value;
} CPD_TARGET_INFO_POLL_CONFIG;

typedef struct CPD_RLS_TAG
{
        BITS_32E flags;
        BITS_32E invalid_rc;
        void * miniport_login_context;
        BITS_32E type;
        UINT_32E address;
//      FC_FCP_LESB fc_lesb;
} CPD_RLS;

typedef struct CPD_RPSC_TAG
{
        BITS_32E flags;
        BITS_32E invalid_rc;
        void * miniport_login_context;
        BITS_32E type;
        UINT_32E address;
        UINT_32E entries;
        BITS_32E port_capabilities[2];
        /* Additional contiguous buffer space should be allocated
         * for the port speed entries.
         */
} CPD_RPSC;


    // iSCSI duplex settings
#define ISCSI_DUPLEX_AUTO_DETECT                0x00000004
#define ISCSI_DUPLEX_FULL                       0x00000002
#define ISCSI_DUPLEX_HALF                       0x00000001


/***********************************************************************
 * CPD_CONFIG structure
 ***********************************************************************/

typedef enum CPD_CONNECT_CLASS_TAG
{
    CPD_CC_PARALLEL_SCSI                        = 0x00000001,
    CPD_CC_FIBRE_CHANNEL,
    CPD_CC_INTERNET_SCSI,
    CPD_CC_SAS,
    CPD_CC_FCOE
} CPD_CONNECT_CLASS;


/***********************************************************************
 * CPD_LUN structure
 ***********************************************************************/

typedef enum CPD_LUN_ACCESS_MODE_TAG
{
    CPD_AMODE_PERIPHERAL_DEVICE         = 0x00000000,
    CPD_AMODE_FLAT_SPACE,
    CPD_AMODE_LOGICAL_UNIT,
    CPD_AMODE_EXTENDED_LOGICAL_UNIT,
    CPD_AMODE_RAW
} CPD_LUN_ACCESS_MODE;

#pragma pack(4)

typedef struct CPD_LUN_TAG
{
    UINT_64E             lun;            // LUN number
    CPD_LUN_ACCESS_MODE access_mode;
} CPD_LUN, * PCPD_LUN;

#pragma pack()


/***********************************************************************
 * CPD_PATH structure
 ***********************************************************************/

typedef struct CPD_PATH_TAG
{
    UINT_16E        portal_nr;
    void *          login_context;
    UINT_32E        login_qualifier;
    CPD_LUN         lun;
    UINT_32E        tag_id;
} CPD_PATH, * PCPD_PATH;


/***********************************************************************
 * xx_PORT_INFORMATION structure
 ***********************************************************************/

typedef enum CPD_LINK_STATUS_TAG
{
    CPD_LS_LINK_DOWN                    = 0x00000000,
    CPD_LS_LINK_UP,
    CPD_LS_LINK_NOT_INITIALIZED
} CPD_LINK_STATUS;


typedef enum FC_PORT_STATE_TAG
{
    FC_PS_OFFLINE                       = 0x00000000,
    FC_PS_PENDING,
    FC_PS_POINT_TO_POINT,
    FC_PS_NONPARTICIPATING_ARB_LOOP,
    FC_PS_PARTICIPATING_ARB_LOOP,
    FC_PS_FAULTED
} FC_PORT_STATE;


typedef enum FC_CONNECT_TYPE_TAG
{
    FC_CT_POINT_TO_POINT                = 0x00000001,
    FC_CT_PRIVATE_LOOP                  = 0x00000002,
    FC_CT_OFFLINE                       = 0x00000005,
    FC_CT_NON_PARTICIPATING             = 0x00000006,
/* SWITCH_PRESENT is a bit setting, and not a connect type in and of itself */
    FC_CT_SWITCH_PRESENT                = 0x00000010,
    FC_CT_SWITCH_PTP                    =
                                (FC_CT_SWITCH_PRESENT | FC_CT_POINT_TO_POINT),
    FC_CT_PUBLIC_LOOP                   =
                                (FC_CT_SWITCH_PRESENT | FC_CT_PRIVATE_LOOP)
} FC_CONNECT_TYPE;

typedef enum FCOE_PORT_STATE_TAG
{
    FCOE_PS_OFFLINE                       = 0x00000000,
    FCOE_PS_PENDING,
    FCOE_PS_POINT_TO_POINT,
    FCOE_PS_NONPARTICIPATING_ARB_LOOP,
    FCOE_PS_PARTICIPATING_ARB_LOOP,
    FCOE_PS_FAULTED
} FCOE_PORT_STATE;

typedef enum FCOE_CONNECT_TYPE_TAG
{
    FCOE_CT_POINT_TO_POINT                = 0x00000001,
    FCOE_CT_PRIVATE_LOOP                  = 0x00000002,
    FCOE_CT_OFFLINE                       = 0x00000005,
    FCOE_CT_NON_PARTICIPATING             = 0x00000006,
/* SWITCH_PRESENT is a bit setting, and not a connect type in and of itself */
    FCOE_CT_SWITCH_PRESENT                = 0x00000010,
    FCOE_CT_SWITCH_PTP                    =
                                (FCOE_CT_SWITCH_PRESENT | FCOE_CT_POINT_TO_POINT),
    FCOE_CT_PUBLIC_LOOP                   =
                                (FCOE_CT_SWITCH_PRESENT | FCOE_CT_PRIVATE_LOOP)
} FCOE_CONNECT_TYPE;

/***********************************************************************
 * CPD_IOCTL_RESET_DEVICE defines
 ***********************************************************************/

// indicates (for SAS) to reset device indicated by miniport_login_context;
// otherwise, reset the device attached to "phy_nr" of device indicated by
//   miniport_login_context
#define CPD_RESET_SPECIFIED_DEVICE     0xFF


/***********************************************************************
 * CPD_STATISTICS structure
 ***********************************************************************/

/* CPD_STATISTICS statistics_type bit definitions */
#define CPD_STATS_FC_LESB                   0x00000001
#define CPD_STATS_MIB2_INTERFACES           0x00000002
#define CPD_STATS_MIB2_IP                   0x00000004
#define CPD_STATS_MIB2_TCP                  0x00000008
#define CPD_STATS_ISCSI_INSTANCE            0x00000010
#define CPD_STATS_ISCSI_PORTAL              0x00000020
#define CPD_STATS_ISCSI_NODE                0x00000040
#define CPD_STATS_ISCSI_SESSION             0x00000080
#define CPD_STATS_ISCSI_CONNECTION          0x00000100
#define CPD_STATS_SAS_PHY                   0x00000200

#define CPD_STATS_RETURN_SUPPORTED_STATS    0x80000000


typedef struct FC_STATISTICS_LESB_TAG
{
    struct
    {
        unsigned int        link_failure_count      : 1;
        unsigned int        loss_of_sync_count      : 1;
        unsigned int        loss_of_signal_count    : 1;
        unsigned int        prim_seq_error_count    : 1;
        unsigned int        invalid_xmit_word_count : 1;
        unsigned int        invalid_crc_count       : 1;
    } valid;
    UINT_32E        link_failure_count;
    UINT_32E        loss_of_sync_count;
    UINT_32E        loss_of_signal_count;
    UINT_32E        prim_seq_error_count;
    UINT_32E        invalid_xmit_word_count;
    UINT_32E        invalid_crc_count;
} FC_STATISTICS_LESB, * PFC_STATISTICS_LESB;


typedef struct CPD_STATISTICS_INTERFACES_TAG
{
    struct
    {
        unsigned int        in_octets               : 1;
        unsigned int        in_ucast_pkts           : 1;
        unsigned int        in_nucast_pkts          : 1;
        unsigned int        in_discards             : 1;
        unsigned int        in_errors               : 1;
        unsigned int        in_unknown_protos       : 1;
        unsigned int        out_octets              : 1;
        unsigned int        out_ucast_pkts          : 1;
        unsigned int        out_nucast_pkts         : 1;
        unsigned int        out_discards            : 1;
        unsigned int        out_errors              : 1;
        unsigned int        out_qlen                : 1;
    } valid;
    UINT_64         in_octets;
    UINT_64         in_ucast_pkts;
    UINT_64         in_nucast_pkts;
    UINT_64         in_discards;
    UINT_64         in_errors;
    UINT_64         in_unknown_protos;
    UINT_64         out_octets;
    UINT_64         out_ucast_pkts;
    UINT_64         out_nucast_pkts;
    UINT_64         out_discards;
    UINT_64         out_errors;
    UINT_32E        out_qlen;
} CPD_STATISTICS_INTERFACES, * PCPD_STATISTICS_INTERFACES;


typedef struct CPD_STATISTICS_IP_TAG
{
    struct
    {
        unsigned int        in_receives             : 1;
        unsigned int        in_hdr_errors           : 1;
        unsigned int        in_addr_errors          : 1;
        unsigned int        forw_datagrams          : 1;
        unsigned int        in_unknown_protos       : 1;
        unsigned int        in_discards             : 1;
        unsigned int        in_delivers             : 1;
        unsigned int        out_requests            : 1;
        unsigned int        out_discards            : 1;
        unsigned int        out_no_routes           : 1;
        unsigned int        reasm_timeout           : 1;
        unsigned int        reasm_reqds             : 1;
        unsigned int        reasm_oks               : 1;
        unsigned int        reasm_fails             : 1;
        unsigned int        frag_oks                : 1;
        unsigned int        frag_fails              : 1;
        unsigned int        frag_creates            : 1;
    } valid;
    UINT_64         in_receives;
    UINT_64         in_hdr_errors;
    UINT_64         in_addr_errors;
    UINT_64         forw_datagrams;
    UINT_64         in_unknown_protos;
    UINT_64         in_discards;
    UINT_64         in_delivers;
    UINT_64         out_requests;
    UINT_64         out_discards;
    UINT_64         out_no_routes;
    UINT_32E        reasm_timeout;
    UINT_64         reasm_reqds;
    UINT_64         reasm_oks;
    UINT_64         reasm_fails;
    UINT_64         frag_oks;
    UINT_64         frag_fails;
    UINT_64         frag_creates;
} CPD_STATISTICS_IP, * PCPD_STATISTICS_IP;


typedef struct CPD_STATISTICS_TCP_TAG
{
    struct
    {
        unsigned int        attempt_fails           : 1;
        unsigned int        estab_resets            : 1;
        unsigned int        curr_estab              : 1;
        unsigned int        in_segs                 : 1;
        unsigned int        out_segs                : 1;
        unsigned int        retrans_segs            : 1;
        unsigned int        in_errs                 : 1;
        unsigned int        out_rsts                : 1;
    } valid;
    UINT_32E        attempt_fails;
    UINT_32E        estab_resets;
    UINT_32E        curr_estab;
    UINT_64         in_segs;
    UINT_64         out_segs;
    UINT_64         retrans_segs;
    UINT_64         in_errs;
    UINT_64         out_rsts;
} CPD_STATISTICS_TCP, * PCPD_STATISTICS_TCP;


typedef struct ISCSI_STATISTICS_INSTANCE_TAG
{
    struct
    {
        unsigned int        version_min             : 1;
        unsigned int        version_max             : 1;
        unsigned int        portal_number           : 1;
        unsigned int        node_number             : 1;
        unsigned int        session_number          : 1;
        unsigned int        ssn_failures            : 1;
        unsigned int        last_ssn_rmt_node_name  : 1;
        unsigned int        ssn_digest_errors       : 1;
        unsigned int        ssn_cxn_timeout_errors  : 1;
        unsigned int        ssn_format_errors       : 1;
    } valid;
    UINT_8E         version_min;
    UINT_8E         version_max;
    UINT_32E        portal_number;
    UINT_32E        node_number;
    UINT_32E        session_number;
    UINT_32E        ssn_failures;
    CPD_NAME *      last_ssn_rmt_node_name;
    UINT_32E        ssn_digest_errors;
    UINT_32E        ssn_cxn_timeout_errors;
    UINT_32E        ssn_format_errors;
} ISCSI_STATISTICS_INSTANCE, * PISCSI_STATISTICS_INSTANCE;


// roles bit definitions
#define CPD_STATS_ROLES_TARGET                  0x80000000
#define CPD_STATS_ROLES_INITIATOR               0x40000000

// Login qualifier field is unused for Fibre Channel
// because establishing new login
#define CPD_NO_LOGIN_QUALIFIER                  0

typedef enum CPD_STATS_DIGEST_TYPE_TAG
{
    CPD_STATS_DIGEST_NONE                       = 0x00000001,
    CPD_STATS_DIGEST_OTHER                      = 0x00000002,
    CPD_STATS_DIGEST_NO_DIGEST                  = 0x00000003,
    CPD_STATS_DIGEST_CRC32C                     = 0x00000004
} CPD_STATS_DIGEST_TYPE;

typedef struct ISCSI_STATISTICS_PORTAL_TAG
{
    struct
    {
        unsigned int        roles                   : 1;
        unsigned int        max_recv_data_seg_length : 1;
        unsigned int        primary_hdr_digest      : 1;
        unsigned int        primary_data_digest     : 1;
        unsigned int        secondary_hdr_digest    : 1;
        unsigned int        secondary_data_digest   : 1;
        unsigned int        recv_marker             : 1;
    } valid;
    BITS_32E        roles;
    UINT_32E        max_recv_data_seg_length;
    CPD_STATS_DIGEST_TYPE   primary_hdr_digest;
    CPD_STATS_DIGEST_TYPE   primary_data_digest;
    CPD_STATS_DIGEST_TYPE   secondary_hdr_digest;
    CPD_STATS_DIGEST_TYPE   secondary_data_digest;
    BOOL            recv_marker;
} ISCSI_STATISTICS_PORTAL, * PISCSI_STATISTICS_PORTAL;


typedef struct ISCSI_STATISTICS_NODE_TAG
{
    struct
    {
        unsigned int        name                    : 1;
        unsigned int        roles                   : 1;
        unsigned int        initial_r2t             : 1;
        unsigned int        immediate_data          : 1;
        unsigned int        max_outstanding_r2t     : 1;
        unsigned int        first_burst_length      : 1;
        unsigned int        max_burst_length        : 1;
        unsigned int        max_connections         : 1;
        unsigned int        data_sequence_in_order  : 1;
        unsigned int        data_pdu_in_order       : 1;
        unsigned int        default_time2wait       : 1;
        unsigned int        default_time2retain     : 1;
        unsigned int        error_recovery_level    : 1;
    } valid;
    CPD_NAME *      name;
    BITS_32E        roles;
    BOOL            initial_r2t;
    BOOL            immediate_data;
    UINT_16E        max_outstanding_r2t;
    UINT_32E        first_burst_length;
    UINT_32E        max_burst_length;
    UINT_32E        max_connections;
    BOOL            data_sequence_in_order;
    BOOL            data_pdu_in_order;
    UINT_16E        default_time2wait;
    UINT_16E        default_time2retain;
    UINT_8E         error_recovery_level;
} ISCSI_STATISTICS_NODE, * PISCSI_STATISTICS_NODE;


typedef enum CPD_STATS_DIRECTION_TAG
{
    CPD_STATS_DIRECTION_INBOUND          = 0x00000001,
    CPD_STATS_DIRECTION_OUTBOUND         = 0x00000002
} CPD_STATS_DIRECTION;

typedef enum CPD_STATS_SESSION_TYPE_TAG
{
    CPD_STATS_SESSION_TYPE_NORMAL        = 0x00000001,
    CPD_STATS_SESSION_TYPE_DISCOVERY     = 0x00000002
} CPD_STATS_SESSION_TYPE;

typedef struct ISCSI_STATISTICS_SESSION_TAG
{
    struct
    {
        unsigned int        direction               : 1;
        unsigned int        tsih                    : 1;
        unsigned int        isid                    : 1;
        unsigned int        initial_r2t             : 1;
        unsigned int        immediate_data          : 1;
        unsigned int        type                    : 1;
        unsigned int        max_outstanding_r2t     : 1;
        unsigned int        first_burst_length      : 1;
        unsigned int        max_burst_length        : 1;
        unsigned int        connection_number       : 1;
        unsigned int        data_sequence_in_order  : 1;
        unsigned int        data_pdu_in_order       : 1;
        unsigned int        error_recovery_level    : 1;
        unsigned int        cmd_pdus                : 1;
        unsigned int        rsp_pdus                : 1;
        unsigned int        tx_data_octets          : 1;
        unsigned int        rx_data_octets          : 1;
    } valid;
    CPD_STATS_DIRECTION     direction;
    UINT_32E        tsih;
    UINT_64         isid;
    UINT_32E        initial_r2t;
    UINT_32E        immediate_data;
    CPD_STATS_SESSION_TYPE  type;
    UINT_32E        max_outstanding_r2t;
    UINT_32E        first_burst_length;
    UINT_32E        max_burst_length;
    UINT_32E        connection_number;
    UINT_32E        data_sequence_in_order;
    UINT_32E        data_pdu_in_order;
    UINT_32E        error_recovery_level;
    UINT_32E        cmd_pdus;
    UINT_32E        rsp_pdus;
    UINT_64         tx_data_octets;
    UINT_64         rx_data_octets;
} ISCSI_STATISTICS_SESSION, * PISCSI_STATISTICS_SESSION;


typedef enum CPD_STATS_CONNECTION_STATE_TAG
{
    CPD_STATS_CONNECTION_STATE_LOGIN     = 0x00000001,
    CPD_STATS_CONNECTION_STATE_FULL      = 0x00000002,
    CPD_STATS_CONNECTION_STATE_LOGOUT    = 0x00000003
} CPD_STATS_CONNECTION_STATE;

typedef struct ISCSI_STATISTICS_CONNECTION_TAG
{
    struct
    {
        unsigned int        cid                     : 1;
        unsigned int        state                   : 1;
        unsigned int        remote_port             : 1;
        unsigned int        max_recv_data_seg_length : 1;
        unsigned int        max_xmit_data_seg_length : 1;
        unsigned int        header_integrity        : 1;
        unsigned int        data_integrity          : 1;
        unsigned int        recv_marker             : 1;
        unsigned int        send_marker             : 1;
        unsigned int        version_active          : 1;
    } valid;
    UINT_16E        cid;
    CPD_STATS_CONNECTION_STATE  state;
    UINT_32E        remote_port;
    UINT_32E        max_recv_data_seg_length;
    UINT_32E        max_xmit_data_seg_length;
    CPD_STATS_DIGEST_TYPE   header_integrity;
    CPD_STATS_DIGEST_TYPE   data_integrity;
    BOOL            recv_marker;
    BOOL            send_marker;
    UINT_8E         version_active;
} ISCSI_STATISTICS_CONNECTION, * PISCSI_STATISTICS_CONNECTION;


typedef struct SAS_PHY_INFO_TAG
{
    struct
    {
        BITS_32E     phy_id :1;
        BITS_32E     invalid_dwords :1;
        BITS_32E     running_disparity_dwords :1;
        BITS_32E     loss_of_dword_sync :1;
        BITS_32E     phy_reset_errors :1;
        BITS_32E     crc_errors :1;
        BITS_32E     code_violation_errors :1;
        BITS_32E     probations :1;
    } valid;

    UINT_8E     phy_id;
    UINT_64E    invalid_dwords;
    UINT_64E    running_disparity_dwords;
    UINT_64E    loss_of_dword_sync;
    UINT_64E    phy_reset_errors;
    UINT_64E    crc_errors;
    UINT_64E    code_violation_errors;
    UINT_64E    probations;

} SAS_PHY_INFO, *PSAS_PHY_INFO;


/***********************************************************************
 * CPD_DMRB structure
 ***********************************************************************/

// DMRB functions
typedef enum CPD_DMRB_FUNCTION_TAG
{
/* Function codes for use with CPD_DMRB */
    CPD_LOGIN_DEVICE                    = 1,
    CPD_INITIATOR_NO_DATA,
    CPD_INITIATOR_DATA_IN,
    CPD_INITIATOR_DATA_OUT,
    CPD_TARGET_COMMAND,
    CPD_TARGET_DATA_OUT,
    CPD_TARGET_DATA_IN                  = 0x08,
    CPD_TARGET_STATUS                   = 0x10,
    CPD_TARGET_DATA_IN_AND_STATUS       =
                                (CPD_TARGET_DATA_IN | CPD_TARGET_STATUS)
} CPD_DMRB_FUNCTION, * PCPD_DMRB_FUNCTION;

/* CPD_DMRB flags bit definitions */
#define CPD_DMRB_FLAGS_PHYS_SGL                     0x00000001
#define CPD_DMRB_FLAGS_SKIP_SECTOR_OVERHEAD_BYTES   0x00000002
#define CPD_DMRB_FLAGS_SGL_DESCRIPTOR               0x00000004
#define CPD_DMRB_FLAGS_DISABLE_MAX_XFER_LEN_CHK     0x00000008
#define CPD_DMRB_FLAGS_ABORTED_BY_UL                0x00000100
#define CPD_DMRB_FLAGS_INITIATOR                    0x00010000
#define CPD_DMRB_FLAGS_TARGET                       0x00020000
#define CPD_DMRB_FLAGS_QUICK_RECOVERY               0x00040000
#define CPD_DMRB_FLAGS_SATA_CMD                     0x00080000
#define CPD_DMRB_FLAGS_USE_ECB_ENCRYPTION           0x01000000
#define CPD_DMRB_FLAGS_USE_CB_ARG2_AS_BLK_HI        0x02000000  // enc IV high

// DMRB statuses
typedef enum CPD_DMRB_STATUS_TAG
{
    // Immediate or completion return values
    CPD_STATUS_GOOD                     = 0x000E0001,
    CPD_STATUS_PENDING                  = 0x000E0002,
    CPD_STATUS_INVALID_REQUEST          = 0x000E0003,
    CPD_STATUS_INSUFFICIENT_RESOURCES   = 0x000E0004,
    CPD_STATUS_UNKNOWN_DEVICE           = 0x000E0005,
    CPD_STATUS_DEVICE_NOT_FOUND         = 0x000E0006,
    CPD_STATUS_DEVICE_NOT_LOGGED_IN     = 0x000E0007,
    CPD_STATUS_DEVICE_BUSY              = 0x000E0008,
    CPD_STATUS_BAD_REPLY                = 0x000E0009,
    CPD_STATUS_TIMEOUT                  = 0x000E000A,
    CPD_STATUS_DEVICE_NOT_RESPONDING    = 0x000E000B,
    CPD_STATUS_LINK_FAIL                = 0x000E000C,
    CPD_STATUS_DATA_OVERRUN             = 0x000E000D,
    CPD_STATUS_DATA_UNDERRUN            = 0x000E000E,
    CPD_STATUS_STATUS_OVERRUN           = 0x000E000F,
    CPD_STATUS_ABORTED_BY_UPPER_LEVEL   = 0x000E0010,
    CPD_STATUS_ABORTED_BY_DEVICE        = 0x000E0011,
    CPD_STATUS_UNSUPPORTED              = 0x000E0012,
    CPD_STATUS_SCSI_STATUS              = 0x000E0013,
    CPD_STATUS_CHECK_CONDITION          = 0x000E0014,
    CPD_STATUS_LOGIN_IN_PROGRESS        = 0x000E0015,
    CPD_STATUS_GOOD_NOTIFY              = 0x000E0016,
    CPD_STATUS_SATA_NCQ_ERROR           = 0x000E0017,
    CPD_STATUS_INCIDENTAL_ABORT         = 0x000E0018,
    CPD_STATUS_SATA_REJECTED_NCQ_MODE   = 0x000E0019,
    CPD_STATUS_FAILED                   = 0x000E001A,
    CPD_STATUS_ENCRYPTION_NOT_ENABLED   = 0x000E001B,
    CPD_STATUS_ENCRYPTION_KEY_NOT_FOUND = 0x000E001C,
    CPD_STATUS_ENCRYPTION_KEY_ERROR     = 0x000E001D,
    CPD_STATUS_ENCRYPTION_ERROR         = 0x000E001E,
    CPD_STATUS_ENCRYPTION_KEY_INV_UNWRAP = 0x000E001F,

    // Statuses specific to DMRB.recovery_status
    CPD_STATUS_NO_RECOVERY_INFORMATION  = 0x000EE000,
    CPD_STATUS_NO_RECOVERY_PERFORMED    = 0x000EE001

} CPD_DMRB_STATUS, * PCPD_DMRB_STATUS;


/***********************************************************************
 * CPD_EVENT_INFO structure
 * This is the structure passed on all asynchronous events.
 ***********************************************************************/

typedef enum CPD_EVENT_TYPE_TAG
{
    // Asynchronous events
    CPD_EVENT_LINK_DOWN                     = 0x00041001,
    CPD_EVENT_LINK_UP,
    CPD_EVENT_LINK_STATE_CHANGE,
    CPD_EVENT_LOGIN,
    CPD_EVENT_LOGOUT,
    CPD_EVENT_LOGIN_FAILED,
    CPD_EVENT_unused_41007,
    CPD_EVENT_DEVICE_FAILED,
    CPD_EVENT_NEW_DMRB,
    CPD_EVENT_DOWNLOAD_COMPLETE,
    CPD_EVENT_CLEAR_ACA,
    CPD_EVENT_SFP_CONDITION,
    CPD_EVENT_REMOTE_TARGET_DATA_POLL_COMPLETE,
    CPD_EVENT_GET_TARGET_INFO_COMPLETE,
    CPD_EVENT_PORT_FAILURE,
    CPD_EVENT_SATA_NCQ_ERROR,
    CPD_EVENT_FAULT_INSERTION_COMPLETE,
    CPD_EVENT_OOO_FRAME_RECEIVED,
    CPD_EVENT_LIPF_RECEIVED,
    CPD_EVENT_CREDIT_ERROR,
    CPD_EVENT_XLS_FAILED,
    CPD_EVENT_GPIO_COMPLETE,

    // Abort events which require outstanding operations to be aborted.
    CPD_ABORT_INITIATOR                     = 0x00042001,
    CPD_ABORT_INITIATOR_LUN,
    CPD_ABORT_TAG,
    CPD_ABORT_LUN_ALL_INITIATORS,
    CPD_ABORT_GTPRLO,
    CPD_ABORT_DEVICE_RESET,
    CPD_ABORT_LOGICAL_UNIT_RESET,

    // Request events which require class driver to return information
    // (synchronously or asynchronously)
    CPD_REQUEST_TARGET_INFO                 = 0x00043001,
    CPD_REQUEST_GET_AFFECTED_INITIATORS,
    CPD_REQUEST_AUTHENTICATION,
    CPD_REQUEST_LOGIN,

    // Asynchronous events to class driver supporting CM interface
    CPD_CM_LOOP_NEEDS_DISCOVERY             = 0x00044001,
    CPD_CM_SUSPEND_IO,
    CPD_CM_RESUME_IO,
    CPD_CM_LINK_HUNG,
    CPD_CM_TEST_LOOP,
    CPD_CM_LOOP_PHY_NOTIFY,

    // Encryption events notifying upper-level of completions
    CPD_EVENT_ENC_ENABLED                   = 0x00045001,
    CPD_EVENT_ENC_KEY_SET,
    CPD_EVENT_ENC_KEY_INVALIDATED,
    CPD_EVENT_ENC_SELF_TEST,
    CPD_EVENT_ENC_ALL_KEYS_FLUSHED,
    CPD_EVENT_ENC_MODE_MODIFIED,
    CPD_EVENT_ENC_ALG_TEST

} CPD_EVENT_TYPE;

#define CPD_LOOPMAP_ALL                     0xFFFFFFFF
#define CPD_LOOPMAP_NONE                    0xFFFFFFFF

/***********************************************************************
 * CPD_SFP_CONDITION enum
 ***********************************************************************/
typedef enum CPD_SFP_CONDITION_TYPE_TAG
{
    CPD_SFP_CONDITION_GOOD                  = 0x00000001,
    CPD_SFP_CONDITION_INSERTED,
    CPD_SFP_CONDITION_REMOVED,
    CPD_SFP_CONDITION_FAULT,
    CPD_SFP_CONDITION_WARNING,
    CPD_SFP_CONDITION_INFO,
    CPD_SFP_CONDITION_UNSUPPORTED
} CPD_SFP_CONDITION_TYPE;

typedef enum CPD_SFP_CONDITION_SUB_TYPE_TAG
{
    CPD_SFP_NONE                   = 0x00000000,

    /*
     * INFO
     */
    CPD_SFP_INFO_SPD_LEN_AVAIL,
    CPD_SFP_INFO_RECHECK_MII,
    CPD_SFP_INFO_BAD_EMC_CHKSUM,

    /*
     * CPD_SFP HW Faults
     */
    CPD_SFP_BAD_CHKSUM,
    CPD_SFP_BAD_I2C,
    CPD_SFP_DEV_ERR, //relates to STATUS_SMB_DEV_ERR

    /*
     * CPD_SFP_CONDITION_BAD_DIAGS Warnings
     */
    CPD_SFP_DIAG_TXFAULT           = 0x00001000,
    CPD_SFP_DIAG_TEMP_HI_ALARM,
    CPD_SFP_DIAG_TEMP_LO_ALARM,
    CPD_SFP_DIAG_VCC_HI_ALARM,
    CPD_SFP_DIAG_VCC_LO_ALARM,
    CPD_SFP_DIAG_TX_BIAS_HI_ALARM,
    CPD_SFP_DIAG_TX_BIAS_LO_ALARM,
    CPD_SFP_DIAG_TX_POWER_HI_ALARM,
    CPD_SFP_DIAG_TX_POWER_LO_ALARM,
    CPD_SFP_DIAG_RX_POWER_HI_ALARM,
    CPD_SFP_DIAG_RX_POWER_LO_ALARM,
    CPD_SFP_DIAG_TEMP_HI_WARNING,
    CPD_SFP_DIAG_TEMP_LO_WARNING,
    CPD_SFP_DIAG_VCC_HI_WARNING,
    CPD_SFP_DIAG_VCC_LO_WARNING,
    CPD_SFP_DIAG_TX_BIAS_HI_WARNING,
    CPD_SFP_DIAG_TX_BIAS_LO_WARNING,
    CPD_SFP_DIAG_TX_POWER_HI_WARNING,
    CPD_SFP_DIAG_TX_POWER_LO_WARNING,
    CPD_SFP_DIAG_RX_POWER_HI_WARNING,
    CPD_SFP_DIAG_RX_POWER_LO_WARNING,

    /*
     * CPD_SFP_CONDITION_UNQUALIFIED Faults
     */
    CPD_SFP_UNQUAL_OPT_NOT_4G     = 0x00002000,
    CPD_SFP_UNQUAL_COP_AUTO,
    CPD_SFP_UNQUAL_COP_SFP_SPEED,
    CPD_SFP_UNQUAL_SPEED_EXCEED_SFP,
    CPD_SFP_UNQUAL_PART,
    CPD_SFP_UNKNOWN_TYPE,
    CPD_SFP_SPEED_MISMATCH,
    CPD_SFP_EDC_MODE_MISMATCH,
    CPD_SFP_SAS_SPECL_ERROR

} CPD_SFP_CONDITION_SUB_TYPE;



/***********************************************************************
 * CPD_MEDIA_INTERFACE_INFO_TYPE enum
 ***********************************************************************/

typedef enum CPD_MEDIA_INTERFACE_INFO_TYPE_TAG
{
    CPD_MII_TYPE_ALL                  = 0x00000001,
    CPD_MII_TYPE_HIGHEST,
    CPD_MII_TYPE_LAST
} CPD_MEDIA_INTERFACE_INFO_TYPE;

/***********************************************************************
 * CPD_MII_MEDIA__TYPE enum
 ***********************************************************************/

typedef enum CPD_MII_MEDIA_TYPE_TAG
{
    CPD_SFP_COPPER                  = 0x00000001, /* copper SFP*/
    CPD_SFP_OPTICAL,                              /* optical SFP*/
    CPD_SFP_NAS_COPPER,                           /* copper SFP of NAS FE*/
    CPD_SFP_UNKNOWN,                              /* unknown */
    CPD_SFP_MINI_SAS_HD

} CPD_MII_MEDIA_TYPE;

typedef enum CPD_SFP_MODULE_TYPE_TAG
{
    CPD_SFP_MOD_TYPE_10GBASE_LRM = 1,
    CPD_SFP_MOD_TYPE_10GBASE_LR,
    CPD_SFP_MOD_TYPE_10GBASE_SR,
    CPD_SFP_MOD_TYPE_10G_COPPER_PASSIVE,
    CPD_SFP_MOD_TYPE_10G_COPPER_ACTIVE,
    CPD_SFP_MOD_TYPE_10G_COPPER_PASSIVE_LEGACY,
    CPD_SFP_MOD_TYPE_4G_FC,
    CPD_SFP_MOD_TYPE_8G_FC,
    CPD_SFP_MOD_TYPE_16G_FC,
    CPD_SFP_MOD_TYPE_UNKNOWN = 0xFF //MUST BE LAST ONE
} CPD_SFP_MODULE_TYPE;


/***********************************************************************
 * SFP Checksum status values enum
 ***********************************************************************/

typedef enum CPD_SFP_CHECK_SUM_STATUS_TAG
{
    CPD_SFP_CHECK_SUM_INPROGRESS   = 0x00000000,
    CPD_SFP_CHECK_SUM_GOOD         = 0x00000001,
    CPD_SFP_CHECK_SUM_BAD          = 0x00000002,
    CPD_SFP_CHECK_SUM_NOT_STARTED  = 0x00000003
} CPD_SFP_CHECK_SUM_STATUS;


/***********************************************************************
 * CPD_MEDIA_INTERFACE_INFO structure
 ***********************************************************************/

typedef struct CPD_MEDIA_INTERFACE_INFO_TAG
{
    UINT_16E                       portal_nr;
    CPD_SFP_CONDITION_TYPE         condition;          // SFP condition
    CPD_SFP_CONDITION_SUB_TYPE     sub_condition;      // SFP subcondition
    CPD_MEDIA_INTERFACE_INFO_TYPE  mii_type;           // MII Type (ALL, HIGHEST, LAST)

    struct
    {
        unsigned int        cable_length            : 1;
        unsigned int        speeds                  : 1;
        unsigned int        hw_type                 : 1;
        unsigned int        mii_media_type          : 1;
        unsigned int        emc_info                : 1;
        unsigned int        vendor_info             : 1;
    } valid;

    UINT_8E                        cable_length;       // Cable length in meters
    BITS_32E                       speeds;             // Speeds
    UINT_32E                       hw_type;            // IO module flavor

     /* 0 - Inprogress , 1 - good , 2 - failed */
    CPD_SFP_CHECK_SUM_STATUS       checksum_status;

    /* Media Type Copper, Optical, NAS Copper */
    CPD_MII_MEDIA_TYPE             mii_media_type;

    /* EMC Info */
    CHAR*                          emc_part_number;
    CHAR*                          emc_part_revision;
    CHAR*                          emc_serial_number;

    /* Vendor Info */
    CHAR*                          vendor_part_number;
    CHAR*                          vendor_part_revision;
    CHAR*                          vendor_serial_number;

} CPD_MEDIA_INTERFACE_INFO, * PCPD_MEDIA_INTERFACE_INFO;


/***********************************************************************
 * CPD_GPIO_CONTROL structure
 ***********************************************************************/

typedef enum CPD_GPIO_OPERATION_TAG
{
    CPD_GPIO_OPERATION_NULL             = 0x00000000,
    CPD_GPIO_GET_READ_PINS              = 0x00000001,
    CPD_GPIO_GET_WRITE_PINS             = 0x00000002,
    CPD_GPIO_CONFIG_READ_PINS           = 0x00000003,
    CPD_GPIO_CONFIG_WRITE_PINS          = 0x00000004,
    CPD_GPIO_READ                       = 0x00000005,
    CPD_GPIO_WRITE                      = 0x00000006,
    CPD_GPIO_BLINK                      = 0x00000007,

    CPD_GPIO_PENDING                    = 0x00004000,
    CPD_GPIO_GET_READ_PINS_PENDING      = CPD_GPIO_GET_READ_PINS |
                                              CPD_GPIO_PENDING,
    CPD_GPIO_GET_WRITE_PINS_PENDING     = CPD_GPIO_GET_WRITE_PINS |
                                              CPD_GPIO_PENDING,
    CPD_GPIO_CONFIG_READ_PINS_PENDING   = CPD_GPIO_CONFIG_READ_PINS |
                                              CPD_GPIO_PENDING,
    CPD_GPIO_CONFIG_WRITE_PINS_PENDING  = CPD_GPIO_CONFIG_WRITE_PINS |
                                              CPD_GPIO_PENDING,
    CPD_GPIO_READ_PENDING               = CPD_GPIO_READ |
                                              CPD_GPIO_PENDING,
    CPD_GPIO_WRITE_PENDING              = CPD_GPIO_WRITE |
                                              CPD_GPIO_PENDING,
    CPD_GPIO_BLINK_PENDING              = CPD_GPIO_BLINK |
                                              CPD_GPIO_PENDING,

    CPD_GPIO_COMPLETE                   = 0x00008000,
    CPD_GPIO_GET_READ_PINS_COMPLETE     = CPD_GPIO_GET_READ_PINS |
                                              CPD_GPIO_COMPLETE,
    CPD_GPIO_GET_WRITE_PINS_COMPLETE    = CPD_GPIO_GET_WRITE_PINS |
                                              CPD_GPIO_COMPLETE,
    CPD_GPIO_CONFIG_READ_PINS_COMPLETE  = CPD_GPIO_CONFIG_READ_PINS |
                                              CPD_GPIO_COMPLETE,
    CPD_GPIO_CONFIG_WRITE_PINS_COMPLETE = CPD_GPIO_CONFIG_WRITE_PINS |
                                              CPD_GPIO_COMPLETE,
    CPD_GPIO_READ_COMPLETE              = CPD_GPIO_READ |
                                              CPD_GPIO_COMPLETE,
    CPD_GPIO_WRITE_COMPLETE             = CPD_GPIO_WRITE |
                                              CPD_GPIO_COMPLETE,
    CPD_GPIO_BLINK_COMPLETE             = CPD_GPIO_BLINK |
                                              CPD_GPIO_COMPLETE
} CPD_GPIO_OPERATION;


/***********************************************************************
 * CPD_LOGIN_REASON enum
 ***********************************************************************/

typedef enum CPD_LOGIN_REASON_TAG
{
    CPD_LOGIN_NULL                                  = 0x0,

    /* CPD_REQUEST_LOGIN reject_reason */

    CPD_LOGIN_ACCEPT                                = 0x01200001,
    CPD_LOGIN_RESOURCES_UNAVAILABLE,
    CPD_LOGIN_UNABLE_TO_PROCESS,
    CPD_LOGIN_BAD_INITIATOR,
    CPD_LOGIN_NAME_LOGGED_IN,


    /* CPD_EVENT_LOGOUT logout_reason */

    CPD_LOGIN_UPPER_LEVEL_REQUEST                   = 0x01200101,
    CPD_LOGIN_LINK_DOWN,
    CPD_LOGIN_TOPOLOGY_CHANGE,
    CPD_LOGIN_INTERNAL_STATE_CHANGE,
    CPD_LOGIN_GLOBAL_REQUEST,
    CPD_LOGIN_NOT_ONLINE,
    CPD_LOGIN_EXPANDER_UNSUPPORTED,
    CPD_LOGIN_PARENT_REMOVED,
    CPD_LOGIN_STALE_DEVICE,
    CPD_LOGIN_DEVICE_RESET,

    /* CPD_EVENT_LOGIN_FAILED login_failure_reason */

    CPD_LOGIN_REQUIRED_SERVICES_NOT_AVAILABLE       = 0x01200201,
    CPD_LOGIN_NO_INITIATOR_FOUND,
    CPD_LOGIN_UNSUPPORTED_TOPOLOGY,
    CPD_LOGIN_TASK_MANAGEMENT_FAILED,
    CPD_LOGIN_CONTROLLER_ADD_ERROR,
    CPD_LOGIN_EXCEEDS_LIMITS,
    CPD_LOGIN_MIXED_COMPLIANCE,
    CPD_LOGIN_INVALID_COMPLIANCE,
    CPD_LOGIN_UNSUPPORTED_DEVICE,
    CPD_LOGIN_SATA_RESOURCE_BUSY,
    CPD_LOGIN_DUPLICATE_ADDRESS,


    /* CPD_EVENT_DEVICE_FAILED device_failure_reason */

    CPD_LOGIN_DEVICE_TIMEOUT                        = 0x01200301,


    /* No reason available */

    CPD_LOGIN_NO_INFO_AVAILABLE                     = 0x012003FF,


    /* Logout reason classes */

    CPD_LOGIN_CLASS_DEFINITIONS_BEGIN               = 0x01200400,

    CPD_LOGIN_INTERNAL_CTLR_ERROR_CLASS             = 0x01200400,
    CPD_LOGIN_TRANSPORT_ERROR_CLASS                 = 0x01200500,
    CPD_LOGIN_SCSI_PROTOCOL_ERROR_CLASS             = 0x01200600,
    CPD_LOGIN_PROTOCOL_ERROR_CLASS                  = 0x01200700,
    CPD_LOGIN_REMOTE_INITIATED_CLASS                = 0x01200800,
    CPD_LOGIN_NEGOTIATION_FAILURE_CLASS             = 0x01200900,
    CPD_LOGIN_INITIATOR_AUTHENTICATION_FAILURE_CLASS = 0x01200A00,
    CPD_LOGIN_TARGET_AUTHENTICATION_FAILURE_CLASS   = 0x01200B00,


/****** All definitions from this point on may be "abstracted" to a
 ****** class error for ease of use by upper-level.
 ******/

        // CTLR internal errors

    CPD_LOGIN_DRIVER_RESET
               = (CPD_LOGIN_INTERNAL_CTLR_ERROR_CLASS + 1),     // 01200401
    CPD_LOGIN_SOCK_ERR_ALLOC_FAIL,
    CPD_LOGIN_SOCK_ERR_BIND_NOT_INITIATED,
    CPD_LOGIN_SOCK_ERR_BIND_NOT_COMPLETED,
    CPD_LOGIN_INT_INSUFF_SOCKET_RESOURCES,
    CPD_LOGIN_INTERNAL_CONN_INIT_FAILED,
    CPD_LOGIN_INTERNAL_INIT_ERR_ON_RECONN,
    CPD_LOGIN_CTLR_OUT_OF_RP,
    CPD_LOGIN_CTLR_OUT_OF_NEGOTIATE_RES,
    CPD_LOGIN_CTLR_SENDTGTS_NO_RESOURCE,
    CPD_LOGIN_CTLR_LOGIN_PDU_NO_RESOURCE,
    CPD_LOGIN_CTLR_LOGIN_REPLY_NO_RESOURCE,
    CPD_LOGIN_CTLR_IMM_NOTIFY_NOT_SENT,
    CPD_LOGIN_CTLR_CHAP_DATA_NO_RESOURCE,


    CPD_LOGIN_CTLR_LOGIN_REDIR_NO_RESOURCE
           = (CPD_LOGIN_INTERNAL_CTLR_ERROR_CLASS + 0x0010),    // 01200410
    CPD_LOGIN_CTLR_TGT_ADDDRESS_STRING_NO_RESOURCE,
    CPD_LOGIN_CTLR_OUT_OF_RESOURCES,
    CPD_LOGIN_CTLR_UNEXPECTED_INTERNAL_EVENT,
    CPD_LOGIN_CTLR_UNEXPECTED_INTERNAL_CONN_FAIL,
    CPD_LOGIN_DMA_ERROR,
    CPD_LOGIN_CONN_ABORTED_BY_CTLR_FIRMWARE,
    CPD_LOGIN_CONN_CLOSED_MAILBOX_TIMEOUT,
    CPD_LOGIN_DEVICE_NOT_TARGET_ENTRY,
    CPD_LOGIN_DEVICE_NOT_CONFIGURED,
    CPD_LOGIN_DUPLICATE_DEVICE_CONFIGURED,
    CPD_LOGIN_UNABLE_TO_DROP_PAYLOAD,
    CPD_LOGIN_INITIATOR_MODE_UNSUPPORTED,
    CPD_LOGIN_SET_PEER_ADDRESS_FAILED,
    CPD_LOGIN_SET_NEGOTIATED_PARAMS_FAILED,

    CPD_LOGIN_CTLR_FATAL_HARDWARE_ERROR
           = (CPD_LOGIN_INTERNAL_CTLR_ERROR_CLASS + 0x0030),    // 01200430
    CPD_LOGIN_CTLR_SDRAM_PARITY_ERROR,
    CPD_LOGIN_CTLR_NULL_MCB_PTR,
    CPD_LOGIN_CTLR_ABORT_TIMEOUT,
    CPD_LOGIN_CONN_CLOSED_BY_HOST_DRIVER,
    CPD_LOGIN_DATA_IN_OR_DATA_OUT_PDU_ERROR,
    CPD_LOGIN_CONN_CLOSED_DMA_ERROR,
    CPD_LOGIN_PASSTHROUGH_IOCB_TIMEOUT,
    CPD_LOGIN_ICMP_ERROR,
    CPD_LOGIN_DYNAMIC_IP_LOST,
    CPD_LOGIN_AHS_OVERRUN,
    CPD_LOGIN_OTHER_FIRMMWARE_ERROR,

        // Transport Failures

    CPD_LOGIN_SOCK_ALLOC_ERR_ON_RECONN
               = (CPD_LOGIN_TRANSPORT_ERROR_CLASS + 1),         // 01200501
    CPD_LOGIN_TCP_CONN_ATTEMPT_FAILED,
    CPD_LOGIN_PREV_CONN_CLEANUP_INCOMPLETE,
    CPD_LOGIN_TCP_CONN_NOT_ESTABLISHED,
    CPD_LOGIN_TCP_CONN_ESTABLISH_TIMEOUT,
    CPD_LOGIN_COULD_NOT_CONNECT_TO_TGT,
    CPD_LOGIN_B4_LOGIN_CONN_RESET,
    CPD_LOGIN_B4_LOGIN_CONN_CLOSED,
    CPD_LOGIN_B4_LOGIN_FAILED_TO_ACCEPT_CONN,
    CPD_LOGIN_B4_LOGIN_CXN_LOST,
    CPD_LOGIN_TCP_CONN_ESTABLISH_REFUSED_BY_PEER,
    CPD_LOGIN_TCP_CONN_ESTABLISH_NO_RESPONSE,
    CPD_LOGIN_TCP_CONN_RESET,
    CPD_LOGIN_TCP_XMIT_ERROR,
    CPD_LOGIN_TCP_XMIT_UNDERRUN,
    CPD_LOGIN_TCP_RECV_ERROR,
    CPD_LOGIN_SMP_XFER_ERROR,
    CPD_LOGIN_AFTER_LOGIN_CONN_LOST,

        // SCSI protocol error

        CPD_LOGIN_OVERLAPPED_CMDS_DETECTED
               = (CPD_LOGIN_SCSI_PROTOCOL_ERROR_CLASS + 1),     // 01200601
    CPD_LOGIN_CONN_CLOSED_SCSI_CMD_TIMEOUT,
    CPD_LOGIN_SCSI_RESPONSE_FORMAT_ERROR,
    CPD_LOGIN_SCSI_SENSE_DATA_LEN_TOO_LARGE,
    CPD_LOGIN_SCSI_SENSE_DATA_LEN_INCOMPLETE,
    CPD_LOGIN_SCSI_DATA_UNDERRUN,
    CPD_LOGIN_SCSI_DATA_OVERRUN,


        // Protocol errors - iSCSI

    CPD_LOGIN_ISCSI_VER_NOT_SUPPORTED
               = (CPD_LOGIN_PROTOCOL_ERROR_CLASS + 1),          // 01200701
    CPD_LOGIN_DATA_DISALLOWED_BY_C_BIT,
    CPD_LOGIN_INVALID_TASK_TAG,
    CPD_LOGIN_CURRENT_STAGE_INVALID,
    CPD_LOGIN_NEXT_STAGE_INVALID,
    CPD_LOGIN_ZERO_TSIH,
    CPD_LOGIN_UNEXPECTED_CID,
    CPD_LOGIN_UNEXPECTED_TSIH,
    CPD_LOGIN_UNEXPECTED_ISID,
    CPD_LOGIN_UNEXPECTED_STATSN,
    CPD_LOGIN_UNEXPECTED_CMDSN,
    CPD_LOGIN_FLAGS_ERROR,
    CPD_LOGIN_ILLEGAL_STATE_TRANSITION,
    CPD_LOGIN_PDU_FORMAT_ERROR,
    CPD_LOGIN_UNEXPECTED_OPCODE,

    CPD_LOGIN_TGT_RETURNED_BAD_STATUS
               = (CPD_LOGIN_PROTOCOL_ERROR_CLASS + 0x0010),     // 01200710
    CPD_LOGIN_KEY_REJECTED_DURING_LOGIN,
    CPD_LOGIN_LOGIN_TIMER_EXPIRED,
    CPD_LOGIN_LOGIN_AND_CONN_EST_TIMER_EXPIRED,
    CPD_LOGIN_TOO_MANY_PDU_EXCHANGES,
    CPD_LOGIN_CXN_CLOSED_BY_TIMEOUT,
    CPD_LOGIN_CONN_CLOSED_DURING_LOGOUT_BY_TIMEOUT,
    CPD_LOGIN_REJECT_PDU_RECEIVED,
    CPD_LOGIN_CONN_CLOSED_TMF_TIMEOUT,
    CPD_LOGIN_CONN_CLOSED_KEEP_ALIVE_TIMOUT,
    CPD_LOGIN_CONN_CLOSED_ASYNC_MSG_PDU_RECEIVED,
    CPD_LOGIN_INVALID_KEY_IN_SENDTARGETS_RESP,
    CPD_LOGIN_UNEXPECTED_TEXT_RESP,
    CPD_LOGIN_NOP_TIMEOUT,
    CPD_LOGIN_PAYLOAD_OVERFLOW,
    CPD_LOGIN_CXN_CLOSED_BY_ABORT_TIMEOUT,

    CPD_LOGIN_UNEXPECTED_ITT_IN_TEXT_RESP
               = (CPD_LOGIN_PROTOCOL_ERROR_CLASS + 0x0020),     // 01200720
    CPD_LOGIN_UNEXPECTED_FINAL_FLAG_IN_TEXT_RESP,
    CPD_LOGIN_FINAL_FLAG_TRANSFER_TAG_ERROR,
    CPD_LOGIN_IMPROPER_TEXT_PDU,
    CPD_LOGIN_TEXT_CMD_FAILED_DURING_FF,
    CPD_LOGIN_UNEXPECTED_PDU_RECEIVED,
    CPD_LOGIN_TGT_WENT_TO_CLEANUP_WAIT_STATE,
    CPD_LOGIN_ASYNC_MSG_PDU_SENT,
    CPD_LOGIN_ERROR_SENDING_PDU,
    CPD_LOGIN_INIT_SENT_TMF,
    CPD_LOGIN_ERROR_RECEIVING_PDU,
    CPD_LOGIN_HDR_DIGEST_ERROR,
    CPD_LOGIN_DATA_DIGEST_ERROR,
    CPD_LOGIN_UNABLE_TO_SEND_SNACK,
    CPD_LOGIN_UNEXPECTED_UNSOL_DATA,

    CPD_LOGIN_UNEXPECTED_STATSN_IN_REJECT
               = (CPD_LOGIN_PROTOCOL_ERROR_CLASS + 0x0030),     // 01200730
    CPD_LOGIN_UNEXPECTED_CMDSN_CMDS_MISSING,
    CPD_LOGIN_INB_INVALID_INTERNAL_RP_VALUE,
    CPD_LOGIN_INB_INVALID_DATASN,
    CPD_LOGIN_INB_CTLR_STATUS_ERROR,
    CPD_LOGIN_INB_INVALID_ITT,
    CPD_LOGIN_INB_TOO_MUCH_DATA,
    CPD_LOGIN_INB_UNEXPECTED_DATA,
    CPD_LOGIN_CXN_CLOSED_ERROR_UNSPECIFIED,
    CPD_LOGIN_PDU_DATA_LEN_EXCEEDED,
    CPD_LOGIN_DATA_SEQ_NUM_NOT_SEQUENTIAL,
    CPD_LOGIN_TEXT_PDU_FORMAT_ERROR,
    CPD_LOGIN_TEXT_PDU_NEGOTIATE_ERROR,
    CPD_LOGIN_PDU_PROTOCOL_ERROR_DETECTED,
    CPD_LOGIN_QUEUED_REQUEST_TIMEOUT,
    CPD_LOGIN_STATUS_SEQ_NUM_NOT_SEQUENTIAL,

    CPD_LOGIN_DATA_PDU_OUT_OF_ORDER
                 = (CPD_LOGIN_PROTOCOL_ERROR_CLASS + 0x0040),     // 01200740
    CPD_LOGIN_DATA_PDU_UNEXPECTED_TASK_TAG,
    CPD_LOGIN_ILLEGAL_LUN_FIELD,
    CPD_LOGIN_UNEXPECTED_BUFFER_OFFSET,
    CPD_LOGIN_DISCOVERY_SESSION_TIMEOUT,
    CPD_LOGIN_INCORRECT_LUN_IN_ABORT_TASK,
    CPD_LOGIN_PEER_REQ_TOO_MUCH_DATA,
    CPD_LOGIN_UNEXPECTED_STATSN_OR_EXPSTATSN,

    /* Indicates an SMP invalid/unexpected/inappropriate response */
    CPD_LOGIN_SMP_PROTOCOL_ERROR                                  // 10200750
                 = (CPD_LOGIN_PROTOCOL_ERROR_CLASS + 0x0050),
    CPD_LOGIN_SMP_STUCK_CONFIGURING,


        // Host-initiated actions

        CPD_LOGIN_HOST_LOGOUT
               = (CPD_LOGIN_REMOTE_INITIATED_CLASS + 1),        // 01200801
        CPD_LOGIN_HOST_RELOGIN,
    CPD_LOGIN_CONNECTION_REINSTATEMENT,
    CPD_LOGIN_CONN_LOST_DURING_FULL_FEATURE,
    CPD_LOGIN_CONN_LOST_DURING_LOGOUT,
    CPD_LOGIN_CONN_LOST_AFTER_ESTABLISHMENT,
    CPD_LOGIN_CONN_CLOSED_SENDTARGETS_COMPLETE,
        CPD_LOGIN_SESSION_REINSTATEMENT,
        CPD_LOGIN_SESSION_CLOSE,
    CPD_LOGIN_CONN_LOST_DURING_LOGIN,
    CPD_LOGIN_CONN_CLOSED_BY_PEER,
    CPD_LOGIN_HOST_SCSI_DATA_UNDERRUN,
    CPD_LOGIN_HOST_SCSI_DATA_OVERRUN,

    // Login/Negotiation Failures

    CPD_LOGIN_LOGIN_NEGOTIATION_ERROR
                = (CPD_LOGIN_NEGOTIATION_FAILURE_CLASS + 1),    // 01200901
    CPD_LOGIN_CONN_REFUSED_BY_PEER,
    CPD_LOGIN_KEY_RENEGOTIATE_ATTEMPTED,
    CPD_LOGIN_KEY_INVALID_NAME,
    CPD_LOGIN_KEY_NOT_ALLOWED_IN_CURRENT_STATE,
    CPD_LOGIN_FIRST_BURST_EXCEEDS_MAX_BURST,
    CPD_LOGIN_CTLR_CANT_NEGOTIATE_VALUES,
    CPD_LOGIN_PEER_NEGOTIATED_BAD_BURST,
    CPD_LOGIN_PEER_NEGOTIATED_BAD_VALS_FOR_REC_LEVEL,
    CPD_LOGIN_INCONSISTENT_NEGOTIATION,
    CPD_LOGIN_ISCSI_TARGET_NAME_NOT_SUPPORTED,
    CPD_LOGIN_INVALID_LINK_RATE,

    CPD_LOGIN_ISCSI_DIGESTS_ENABLED
                = (CPD_LOGIN_NEGOTIATION_FAILURE_CLASS + 0x98), // 01200998
    CPD_LOGIN_TIMESTAMPS_NOT_ENABLED
                = (CPD_LOGIN_NEGOTIATION_FAILURE_CLASS + 0x99), // 01200999


    // Initiator (Forward) Authentication Failures

    CPD_LOGIN_NO_PASSWORD_FOUND
        = (CPD_LOGIN_INITIATOR_AUTHENTICATION_FAILURE_CLASS + 1),   // 01200A01
    CPD_LOGIN_PEER_ATTEMPTED_TO_BYPASS_AUTH,
    CPD_LOGIN_CHAP_SECRET_LEN_INVALID,
    CPD_LOGIN_TGT_NOT_SUPPORT_BIDIR_CHAP,
    CPD_LOGIN_CHAP_NAME_NOT_FOUND,
    CPD_LOGIN_CHAP_RESP_VALIDATION_FAIL,
    CPD_LOGIN_IDENTICAL_CHALLANGES_SENT,
    CPD_LOGIN_MISC_AUTH_ERROR,
    CPD_LOGIN_TGT_RETURNED_AUTH_FAILURE_STATUS

    // Target (Reverse) Authentication Failures                 // 01200B01
    //   Use codes for Initiator Authentication Failures plus
    //   CPD_LOGIN_TARGET_AUTHENTICATION_FAILURE_OFFSET


} CPD_LOGIN_REASON;


// Abstract login/logout reasons

#define CPD_LOGIN_REASON_CLASS_MASK             0xFFFFFF00

#define CPD_LOGIN_GET_REASON_CLASS(reason)                          \
            (((reason) >= CPD_LOGIN_CLASS_DEFINITIONS_BEGIN) ?      \
             ((CPD_LOGIN_REASON)((reason) & CPD_LOGIN_REASON_CLASS_MASK)) : (reason))

#define CPD_LOGIN_TARGET_AUTHENTICATION_FAILURE_OFFSET              \
    (CPD_LOGIN_TARGET_AUTHENTICATION_FAILURE_CLASS -                \
     CPD_LOGIN_INITIATOR_AUTHENTICATION_FAILURE_CLASS)


/***********************************************************************
 * CPD_ERROR_INSERTION structure
 ***********************************************************************/

/* Define actions for use with CPD_ERROR_INSERTION */

#define CPD_TEST_CLEAR_STATISTICS               0x00000001
#define CPD_TEST_GET_STATISTICS                 0x00000002
#define CPD_TEST_START_LIP_INSERTION            0x00000004
#define CPD_TEST_STOP_LIP_INSERTION             0x00000008
#define CPD_TEST_START_DROPPING_FRAMES          0x00000010
#define CPD_TEST_STOP_DROPPING_FRAMES           0x00000020
#define CPD_TEST_START_DROPPING_INB_CMDS        0x00000040
#define CPD_TEST_STOP_DROPPING_INB_CMDS         0x00000080
#define CPD_TEST_START_DROPPING_OUTB_CMDS       0x00000100
#define CPD_TEST_STOP_DROPPING_OUTB_CMDS        0x00000200
#define CPD_TEST_START_FAILING_OUTB_CMDS        0x00000400
#define CPD_TEST_STOP_FAILING_OUTB_CMDS         0x00000800

/***********************************************************************
 * CPD_REGISTERED_DEVICE_LIST structure
 ***********************************************************************/

typedef enum CPD_LIST_TYPE_TAG
{
    CPD_LIST_TYPE_DEFAULT                   = 0x9001,

    CPD_LIST_TYPE_FCP                       = 0x9101,

    CPD_LIST_TYPE_SEND_TARGETS              = 0x9201,
    CPD_LIST_TYPE_SLP,
    CPD_LIST_TYPE_ISNS,

    CPD_LIST_TYPE_SMP                       = 0x9301,
    CPD_LIST_TYPE_VIRTUAL,
    CPD_LIST_TYPE_STP,
    CPD_LIST_TYPE_SSP

} CPD_LIST_TYPE;


typedef struct CPD_GET_DEVICE_ADDRESS_TAG
{
    CPD_NAME        name;
    CPD_ADDRESS     address;
    void *          miniport_login_context;
} CPD_GET_DEVICE_ADDRESS, * PCPD_GET_DEVICE_ADDRESS;


/***********************************************************************
 * CPD_ABORT_IO structure
 ***********************************************************************/

typedef enum CPD_ABORT_CODE_TAG
{
    // Define aborts that may be used by CPD_IOCTL_ABORT_IO
    CPD_ABORT_TASK                              = 0x7001,
    CPD_TERMINATE_TASK,
    CPD_ABORT_TASK_SET,
    CPD_CLEAR_TASK_SET,
    CPD_TARGET_WARM_RESET,
    CPD_LOGICAL_UNIT_RESET,
    CPD_CLEAR_ACA,

    // Fibre Channel only aborts
    CPD_TPRLO                                   = 0x7011,
    CPD_GTPRLO,
    CPD_LIP,
    CPD_TARGETED_LIP_RESET,

    // iSCSI only aborts
    CPD_TASK_REASSIGN                           = 0x7021,
    CPD_TARGET_COLD_RESET,
    CPD_TERMINATE_TCP_CONNECTION,
    CPD_ASYNC_MESSAGE,
    CPD_ISCSI_NOP,

    // iSCSI only aborts
    CPD_SAS_LINK_RESET                          = 0x7031,
    CPD_SAS_HARD_RESET,

    // end of list - dummy
    CPD_ABORT_CODE_END_OF_LIST                  = 0x7999
} CPD_ABORT_CODE;

typedef enum CPD_ABORT_STATUS_TAG
{
    CPD_ABORT_STATUS_SUCCESS                    = 0x7051,
    CPD_ABORT_STATUS_REJECTED,
    CPD_ABORT_STATUS_NOT_SUPPORTED,
    CPD_ABORT_STATUS_ABORTED,
    CPD_ABORT_STATUS_FAILED,
    CPD_ABORT_STATUS_TIMEOUT
} CPD_ABORT_STATUS;


/* Async message event codes */
#define CPD_ASYNC_MSG_SCSI_SENSE                0
#define CPD_ASYNC_MSG_REQUEST_LOGOUT            1
#define CPD_ASYNC_MSG_DROP_CONNECTION           2
#define CPD_ASYNC_MSG_DROP_SESSION              3
#define CPD_ASYNC_MSG_REQUEST_RENEGOTIATION     4
#define CPD_ASYNC_MSG_VENDOR_SPECIFIC           255


/***********************************************************************
 * CPD_TARGET_INFO_TYPE structure
 ***********************************************************************/

typedef enum CPD_TARGET_INFO_TYPE_TAG
{
    CPD_TARGET_THIS_TARGET_ONLY             = 0x00000001,
    CPD_TARGET_ALL_TARGETS,
    CPD_TARGET_SPECIFIC_TARGET
} CPD_TARGET_INFO_TYPE;


/***********************************************************************
 * FC XLS_FAILURE_INFO structure
 ***********************************************************************/

typedef struct CPD_XLS_FAILURE_INFO_TAG
{
    UINT_32E            cmd_code;
    UINT_32E            exm_status;
    UINT_32E            device_address_id;
    UINT_32E            device_state;
    UINT_32E            rjt_code;
} CPD_XLS_FAILURE_INFO, * PCPD_XLS_FAILURE_INFO;


/***********************************************************************
 * IS_ADDR_CONFIG structure
 ***********************************************************************/

/* Disable VLAN support - in ISCSI_ADDRESS vlan_id field */
#define CPD_VLAN_DISABLE                        0xFFFF

typedef struct IS_ADDR_CONFIG_TAG
{
    ISCSI_ADDRESS   iscsi_addr;
    CPD_IP_ADDRESS  subnet_mask;
    CPD_IP_ADDRESS  gateway_ip_addr;
    UINT_16E        portal_group_tag;
    void *          ul_portal_context;
    /* Used for get remote MAC address*/
    UINT_32E        interface_index;

    struct
    {
        unsigned int    valid                    : 1;
        unsigned int    ul_portal_context_valid  : 1;
        unsigned int    address_changed          : 1;
        unsigned int    chap                     : 1;
        unsigned int    chap_radius              : 1;
        unsigned int    srp                      : 1;
        unsigned int    spkm1                    : 1;
        unsigned int    spkm2                    : 1;
        unsigned int    kerberos_v5              : 1;

        unsigned int    initiator_authentication : 1;
        unsigned int    target_authentication    : 1;

    } flags;
} IS_ADDR_CONFIG;

/***********************************************************************
 * FC_VPORT_CONFIG structure
 ***********************************************************************/

/* This is the virtual port struct that the config_db has to track various
 * portal specific information for FC.
 */
typedef struct FC_VPORT_CONFIG_TAG
{
    UINT_64     node_name;
    UINT_64     port_name;
    UINT_64     sw_node_name; /*not used in FC*/
    UINT_64     sw_port_name; /*not used in FC*/
    UINT_32E    vlan_id;
    UINT_8E     mac_addr[CPD_MAC_ADDR_LEN];

    /* Class driver portal context is maintained here */
    void        *ul_portal_context;

    struct
    {
        /* This bit is set when the ul_portal_context is set by the osw */
        BITS_32E    ul_portal_context_valid     :1;

        /* This is set after the address (for FCOE, the vlan_id) has been set
         * for this portal
         */
        BITS_32E    valid     :1;

        /* This is set after the SET_NAME ioctl for this portal. */
        BITS_32E    name_valid     :1;

        /* This is set if the upper layer sets the vlan_id to
         * CPD_VLAN_AUTO_CONFIG
         */
        BITS_32E    auto_config_vlan     :1;

        /* Set when we get do a set/delete address, helps the API to know that
         * the address has changed
         */
        BITS_32E    address_changed        :1;

        /* Set when we get do a set/delete name, helps the API to know that
         * the name has changed
         */
        BITS_32E    name_changed        :1;

    } flags;

} FC_VPORT_CONFIG;

/***********************************************************************
 * CPD_HW_PARAM structure
 ***********************************************************************/

typedef enum CPD_HW_IMPEDANCE_TAG
{
    CPD_HW_IMPEDANCE_100    = 0,
    CPD_HW_IMPEDANCE_150    = 1
} CPD_HW_IMPEDANCE;

typedef enum CPD_HW_VOLTAGE_TAG
{
    CPD_HW_VOLTAGE_1100     = 0,
    CPD_HW_VOLTAGE_600      = 1
} CPD_HW_VOLTAGE;

typedef enum CPD_HW_PREEMPHASIS_TAG
{
    CPD_HW_PREEMPHASIS_0    = 0,
    CPD_HW_PREEMPHASIS_6    = 1,
    CPD_HW_PREEMPHASIS_12   = 2,
    CPD_HW_PREEMPHASIS_18   = 3,
    CPD_HW_PREEMPHASIS_25   = 4,
    CPD_HW_PREEMPHASIS_31   = 5,
    CPD_HW_PREEMPHASIS_37   = 6,
    CPD_HW_PREEMPHASIS_43   = 7
} CPD_HW_PREEMPHASIS;

typedef enum CPD_HW_PARAM_TYPE_TAG
{
    CPD_HW_TYPE_IMPED_PREEMP = 0x1
} CPD_HW_PARAM_TYPE;


/***********************************************************************
 * CPD_DUMP_CALLBACK structure
 ***********************************************************************/

// This structure is used to register callbacks that are called
// before a crash dump is taken

typedef struct CPD_DUMP_CALLBACK_TAG
{
    void                                (* function) (void *);
    void *                              argument;
    struct CPD_DUMP_CALLBACK_TAG *      next_callback;
} CPD_DUMP_CALLBACK, * PCPD_DUMP_CALLBACK;

/* These values are GROUP values that are contrived by
 * this code.  That is they are not from a spec.
 * These help with the code to reduce the size of the table.
 */
#define CPD_FRU_ID_HURRICANE_GROUP     0xFFFF0001
#define CPD_FRU_ID_TWISTER_GROUP       0xFFFF0002
#define CPD_FRU_ID_SCORPION_GROUP      0xFFFF0003
#define CPD_FRU_ID_QX4_DEFAULT         0xFFFF0004
#define CPD_FRU_ID_QE4_DEFAULT         0xFFFF0005
#define CPD_FRU_ID_DE4_DEFAULT         0xFFFF0006
#define CPD_FRU_ID_C6_DEFAULT          0xFFFF0007
#define CPD_FRU_ID_QE8_DEFAULT         0xFFFF0008
#define CPD_FRU_ID_DONT_CARE           0xFFFFFFFF


typedef union CPD_IO_HW_ID_TAG
{
    UINT_32E        family_fru;
    struct {
        UINT_16E    family_id;
        UINT_16E    fru_id;
    } fields;
} CPD_IO_HW_ID, * PCPD_IO_HW_ID;

#define CPD_MAC_ADDR_LEN 6


/***********************************************************************
 * CPD_SET_SYSTEM_DEVICES structure
 ***********************************************************************/

typedef struct CPD_SET_SYSTEM_DEVICES_TAG
{
    union
    {
        struct
        {
            UINT_8E loop_id_of_primary;
            UINT_8E loop_id_of_secondary;
        } fc;
        struct
        {
            UINT_8E  phy_of_primary;
            UINT_64E sas_address_of_primary_exp;
            UINT_8E  phy_of_secondary;
            UINT_64E sas_address_of_secondary_exp;
        } sas;
    }u;
} CPD_SET_SYSTEM_DEVICES , *PCPD_SET_SYSTEM_DEVICES;


/***********************************************************************
 * CPD_AFFECTED_INITIATORS structure
 ***********************************************************************/

/* Modified CPD_AFFECTED_INITIATORS structure.
 * CPD_AFFECTED_INITIATORS_MAX_COUNT macro removed.
 * initiators_context array is made of single element. It just acts as
 * a placeholder. Memory for all other affected initiators is assigned
 * after initiator_context[0].
 * Structure definition moved from cpd_interface.h to this file for DVM access.
 * DIMS 123407
 */

typedef struct CPD_AFFECTED_INITIATORS_TAG
{
    void *          login_context;  // "affecting" device
    UINT_32E        max_count;      // max initiators allowed for allocated mem
    UINT_32E        count;          // actual number of initiators in array
    void *          initiator_context[1];
} CPD_AFFECTED_INITIATORS, * PCPD_AFFECTED_INITIATORS;


/***********************************************************************
 * Encryption definitions
 ***********************************************************************/

  /* key handle definitions */
typedef OPAQUE_PTR          CPD_KEY_HANDLE;
  // to be used for key handle when no encryption is desired
#define CPD_KEY_PLAINTEXT   (((OPAQUE_PTR)0) - 1)
  // key handle used internally in invalidate operation to flush all keys
#define CPD_KEY_ALL_KEYS    (((OPAQUE_PTR)0) - 2)


  /* encryption key format field definitions */

#define CPD_ENC_PLAINTEXT           0x10
#define CPD_ENC_WRAPPED             0x11


typedef enum CPD_ENC_MODE_TAG
{
    CPD_ENC_DISABLED = 0x06790001,      // encryption disabled
    CPD_ENC_PLAINTEXT_KEYS,
    CPD_ENC_WRAPPED_DEKS,
    CPD_ENC_WRAPPED_KEKS_DEKS,
    CPD_ENC_PLAINTEXT_KEYS_ECB,         // for testing only
    CPD_ENC_NON_FUNCTIONAL,             // encryption HW broken or bad state

    CPD_ENC_MODE_UNDEFINED = 0
} CPD_ENC_MODE;


#define CPD_ENC_SHA_BASED_HMAC_ALG      0x00010000

typedef enum CPD_ENC_SHA_TYPE_TAG
{
    CPD_ENC_HASH_SHA_1 = 0x78000001,
    CPD_ENC_HASH_SHA_224,
    CPD_ENC_HASH_SHA_256,
    CPD_ENC_HASH_SHA_384,
    CPD_ENC_HASH_SHA_512,
    CPD_ENC_HMAC_SHA_1 =   (CPD_ENC_HASH_SHA_1 | CPD_ENC_SHA_BASED_HMAC_ALG),
    CPD_ENC_HMAC_SHA_224 = (CPD_ENC_HASH_SHA_224 | CPD_ENC_SHA_BASED_HMAC_ALG),
    CPD_ENC_HMAC_SHA_256 = (CPD_ENC_HASH_SHA_256 | CPD_ENC_SHA_BASED_HMAC_ALG),
    CPD_ENC_HMAC_SHA_384 = (CPD_ENC_HASH_SHA_384 | CPD_ENC_SHA_BASED_HMAC_ALG),
    CPD_ENC_HMAC_SHA_512 = (CPD_ENC_HASH_SHA_512 | CPD_ENC_SHA_BASED_HMAC_ALG),

    CPD_ENC_UNKNOWN_SHA_ALG
} CPD_ENC_SHA_TYPE;


/* Multi-processor support, processor bit mask */
typedef BITS_64E        CPD_PROC_MASK;

/***********************************************************************
 * AUTHENTICATION defines
 ***********************************************************************/

        // Support for CHAP
#define CPD_AUTH_CHAP                           0x00000001
        // Support for CHAP with RADIUS password server
#define CPD_AUTH_CHAP_RADIUS                    0x00000002
        // Support for SRP
#define CPD_AUTH_SRP                            0x00000004
        // Support for SPKM_1
#define CPD_AUTH_SPKM_1                         0x00000008
        // Support for SPKM_2
#define CPD_AUTH_SPKM_2                         0x00000010
        // Support for KERBEROS
#define CPD_AUTH_KERBEROS_V5                    0x00000020
        // Support for authentication of the initiator
#define CPD_AUTH_AUTHENTICATE_INITIATORS        0x00000100
        // Support for authentication of the target
#define CPD_AUTH_AUTHENTICATE_TARGETS           0x00000200

        // Support for authentication of the remote node by the local node
#define CPD_AUTH_REMOTE_NODE                    0x10000000
        // Support for authentication of the local node by the remote node
#define CPD_AUTH_LOCAL_NODE                     0x20000000
        // Support for authentication as an initiator
#define CPD_AUTH_INITIATOR                      0x40000000
        // Support for authentication as a target
#define CPD_AUTH_TARGET                         0x80000000

#define CPD_AUTH_USERNAME_MAX_LEN               256

#define CPD_AUTH_NO_CONTEXT                     (ULONG_PTR)0xFFFFFFFFFFFFFFFF

    // special values for remote/local_pw_len
#define CPD_AUTH_NO_PASSWORD_FOUND              0xffff
#define CPD_AUTH_BUFFER_TOO_SMALL               0xfffe


/***********************************************************************
 * CPD_AUTHENTICATE structure
 ***********************************************************************/

/* CPD_AUTHENTICATE auth_mechanism bit definitions */
    /* see ISCSI_CAPABILITIES authentication_mechanisms bit definitions */

typedef enum CPD_AUTH_TYPE_STATUS_TAG
{
    CPD_AUTH_REPLY_PENDING                      = 0x01200001,
    CPD_AUTH_UNABLE_TO_PROCESS,
    CPD_AUTH_MECHANISM_UNSUPPORTED,
    CPD_AUTH_NO_RECORD_FOUND,

    CPD_AUTH_INTEGER_BIG_ENDIAN,
    CPD_AUTH_INTEGER_LITTLE_ENDIAN,
    CPD_AUTH_CHARACTER
} CPD_AUTH_TYPE_STATUS;

#define CPD_AUTH_PASSWORD_MAX_SIZE              100

typedef struct CPD_AUTHENTICATE_TAG
{
    void *                      miniport_context;
    CPD_AUTH_TYPE_STATUS        type_status;
    BITS_32E                    auth_mechanism;
    UINT_16E                    username_len;
    CHAR *                      username;
    UINT_16E                    remote_pw_len;
    UINT_16E                    local_pw_len;
    CHAR                        remote_pw_buffer[CPD_AUTH_PASSWORD_MAX_SIZE];
    CHAR                        local_pw_buffer[CPD_AUTH_PASSWORD_MAX_SIZE];
} CPD_AUTHENTICATE, * PCPD_AUTHENTICATE;


/***********************************************************************
 * SRB Pass-Through Functionality
 ***********************************************************************/

// Miniport recognizes this CDB opcode as a special opcode to expedite
// SRB pass-through functionality.  It treats the CDB in a special way:
//   CDB[0]     this special opcode
//   CDB[1]     portal number (specifies port# for controllers with
//              multiple ports)
//   CDB[2-3]   miniport_login_context (specifies target device)
//   CDB[4-15]  normal CDB[0-11] (note limit of 12-byte CDB)

#define CPD_ENHANCED_CDB                0xFA

#endif  /* CPD_GENERICS_H */
