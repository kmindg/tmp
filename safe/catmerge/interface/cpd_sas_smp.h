/***************************************************************************
 * Copyright (C) EMC Corp 2008 - 2014
 *
 * All rights reserved.
 * 
 * This software contains the intellectual property of EMC Corporation or
 * is licensed to EMC Corporation from third parties. Use of this software
 * and the intellectual property contained therein is expressly limited to
 * the terms and conditions of the License Agreement under which it is
 * provided by or on behalf of EMC
 **************************************************************************/

/*******************************************************************************
 *  sas_smp.h
 *******************************************************************************
 *
 *  DESCRIPTION:
 *      This file contains the defines, enums and struct which describe the SAS-1.1
 *      and SAS-2 SMP commands and responses. This file is only expected to contain
 *      those commands and responses used by the TPM.
 *      Reference the appropriate T10 SAS specs for structure content details.
 *      Style Guide:
 *      SAS_field - fields specific to multiple SMP req's and/or rsp's
 *      SAS_req_field - fields specific to single SMP req's and/or rsp's
 *      The shift and mask may vary for a field depending on the req/rsp definition
 *
 ******************************************************************************/

#ifndef SAS_SMP_H
#define SAS_SMP_H

/* Includes */
#ifndef UEFI
#include "generic_types.h"
#else
#include "cpd_uefi_types.h"
#endif

/* SAS Comliance enum */
typedef enum _SAS_COMPLIANCE
{
    SAS_COMPLIANCE_1_1  = 11,
    SAS_COMPLIANCE_2_0  = 20,
    SAS_COMPLIANCE_UKN  = 99

} SAS_COMPLIANCE;


/* Access macros */
#define SAS_SMP_READ_FIELD(field_byte_ptr, field) \
    ( ((*field_byte_ptr) & field##_MASK) >> field##_SHIFT )

#define SAS_SMP_MODIFY_FIELD(field_byte_ptr, field, value) \
    do { \
        *field_byte_ptr &= ~field##_MASK; \
        *field_byte_ptr |= ((value & field##_SIZE) << field##_SHIFT ); \
    } while (0);


/* First, some general field value defines and access masks */

/* SAS Device Type Values */
#define SAS_DEV_TYPE_SIZE                   (0x07)
#define SAS_DEV_TYPE_NONE                   (0x00)
#define SAS_DEV_TYPE_END                    (0x01)
#define SAS_DEV_TYPE_EXP                    (0x02)
#define SAS_DEV_TYPE_EXP_FANOUT             (0x03)
#define SAS_DEV_TYPE_RESERVED               (0x04)

/* Device Type access masks */
#define SAS_DEV_TYPE_SHIFT                  (4)
#define SAS_DEV_TYPE_MASK                   (SAS_DEV_TYPE_SIZE << SAS_DEV_TYPE_SHIFT)

/* SAS Link Up/Reset Reasons */
#define SAS_REASON_SIZE                     (0x0f)
#define SAS_REASON_UNKNOWN                  (0x00)
#define SAS_REASON_POWER_ON                 (0x01)
#define SAS_REASON_HARD_RESET               (0x02)
#define SAS_REASON_LINK_RESET               (0x03)
#define SAS_REASON_SYNC_LOSS                (0x04)
#define SAS_REASON_MUX                      (0x05)
#define SAS_REASON_IT_NEXUS_LOSS            (0x06)
#define SAS_REASON_BREAK_TIMEOUT            (0x07)
#define SAS_PHY_TEST_FUNC_STOPPED           (0x08)
#define SAS_REASON_EXP_REDUCED_FUNC         (0x09)
#define SAS_REASON_RESERVED                 (0x0A)

/* Reason access masks for Identify Address Frame */
#define SAS_IAF_REASON_SHIFT                (0)
#define SAS_IAF_REASON_MASK                 (SAS_REASON_SIZE << SAS_IAF_REASON_SHIFT)

/* SAS Inititor/Target mode bits (used for Identify Address Frame and Discover response */
#define SAS_INIT_TARG_BITS_SIZE             (0x0f)
#define SAS_SMP_INITIATOR                   (0x02)
#define SAS_STP_INITIATOR                   (0x04)
#define SAS_SSP_INITIATOR                   (0x08)
#define SAS_SMP_TARGET                      (0x02)
#define SAS_STP_TARGET                      (0x04)
#define SAS_SSP_TARGET                      (0x08)

/* SAS Routing Attribute bits, use for Discover and Discover List (short) */
#define SAS_ROUTING_ATTR_SIZE               (0x0f)
#define SAS_ROUTING_ATTR_DIRECT             (0x00)
#define SAS_ROUTING_ATTR_SUBTRACTIVE        (0x01)
#define SAS_ROUTING_ATTR_TABLE              (0x02)

#define SAS_ROUTING_ATTR_SHIFT              (0)
#define SAS_ROUTING_ATTR_MASK \
    (SAS_ROUTING_ATTR_SIZE << SAS_ROUTING_ATTR_SHIFT)


/* SAS Additional bits ONLY for Discover response */
#define SAS_ATT_SATA_HOST                  (0x01)
#define SAS_ATT_SATA_DEVICE                (0x01)
#define SAS_ATT_SATA_PORT_SELECTOR         (0x80)

/* SAS Inititor/Target mode bit masks */
#define SAS_INIT_TARG_BITS_SHIFT            (0)
#define SAS_INIT_TARG_BITS_MASK             (SAS_INIT_TARG_BITS_SIZE << SAS_INIT_TARG_BITS_SHIFT)

/* SAS Identify Address Frame zoning related bits */
#define SAS_IAF_BREAK_REPLY_CAPABLE         (0x01)
#define SAS_IAF_REQ_INSIDE_ZPSDS            (0x02)
#define SAS_IAF_REQ_INSIDE_ZPSDS_PER        (0x04)


/* Data Structures */
/* All must be packed */
#pragma pack(1)


/*******************************************************************************
 *  Identify Address Frame
 */

typedef struct SAS11_ID_ADDR_FRAME_TAG      
{
    UINT_8E     dev_addrframe_types;
    UINT_8E     res1;
    UINT_8E     init_type_bits;
    UINT_8E     targ_type_bits;
    UINT_8E     res4[8];
    UINT_64E    sas_address;
    UINT_8E     phy_id;
    UINT_8E     res21[7];
    UINT_32E    crc;

} SAS11_ID_ADDR_FRAME, *PSAS11_ID_ADDR_FRAME;
CPD_COMPILE_TIME_ASSERT(1, sizeof(SAS11_ID_ADDR_FRAME) == 32);

typedef struct SAS20_ID_ADDR_FRAME_TAG
{
    UINT_8E     dev_addrframe_types;
    UINT_8E     reason;
    UINT_8E     init_type_bits;
    UINT_8E     targ_type_bits;
    UINT_64E    device_name;
    UINT_64E    sas_address;
    UINT_8E     phy_id;
    UINT_8E     zoning_bits;
    UINT_8E     res22[6];
    UINT_32E    crc;

} SAS20_ID_ADDR_FRAME, *PSAS20_ID_ADDR_FRAME;
CPD_COMPILE_TIME_ASSERT(2, sizeof(SAS20_ID_ADDR_FRAME) == 32);

typedef union SAS_IDENTIFY_FRAME_TAG
{
    SAS11_ID_ADDR_FRAME     sas11_id_addr_frame; 
    SAS20_ID_ADDR_FRAME     sas20_id_addr_frame; 

} SAS_IDENTIFY_FRAME, *PSAS_IDENTIFY_FRAME;
#define SAS_IDENTIFY_FRAME_INFO_SIZE    (sizeof(SAS_IDENTIFY_FRAME) - sizeof(UINT_32E))


/*******************************************************************************
 *  SMP Requests / Responses
 */

#define SAS_SMP_HDR_SIZE        (4)
#define SAS_SMP_MAX_REQ_SIZE    (1024)
#define SAS_CRC_SIZE            (sizeof(UINT_32E))
#define SAS_SMP_MAX_RSP_SIZE    (SAS_SMP_MAX_REQ_SIZE)
#define SAS_SMP_MAX_FRAME_SIZE  (SAS_SMP_HDR_SIZE + SAS_SMP_MAX_REQ_SIZE + SAS_CRC_SIZE)

/* SMP Frame Types */
#define SAS_FRAME_TYPE_SMP_REQ                  (0x40)
#define SAS_FRAME_TYPE_SMP_RSP                  (0x41)

/* SMP Request Function Types */
/* SMP Input Functions */
#define SAS_SMP_FUNC_REPORT_GENERAL             (0x00)
#define SAS_SMP_FUNC_REPORT_MANF_INFO           (0x01)
#define SAS_SMP_FUNC_READ_GPIO_REG              (0x02)
#define SAS_SMP_FUNC_REPORT_SELF_CONF_STATUS    (0x03)
#define SAS_SMP_FUNC_REPORT_ZONE_PERM_TABLE     (0x04)
#define SAS_SMP_FUNC_REPORT_ZONE_MNGR_PASSWORD  (0x05)
#define SAS_SMP_FUNC_REPORT_BROADCAST           (0x06)
#define SAS_SMP_FUNC_DISCOVER                   (0x10)
#define SAS_SMP_FUNC_REPORT_PHY_ERROR_LOG       (0x11)
#define SAS_SMP_FUNC_REPORT_SATA_PHY            (0x12)
#define SAS_SMP_FUNC_REPORT_ROUTE_INFO          (0x13)
#define SAS_SMP_FUNC_REPORT_PHY_EVENT           (0x14)
#define SAS_SMP_FUNC_DISCOVER_LIST              (0x20)
#define SAS_SMP_FUNC_REPORT_PHY_EVENT_LIST      (0x21)
#define SAS_SMP_FUNC_REPORT_EXP_ROUTE_TABLE_LIST (0x22)
#define SAS_SMP_VU_FUNC_REPORT_DEVICE_PATH      (0x40)  /* Vendor Unique */
/* SMP Output Functions */
#define SAS_SMP_FUNC_CONFIGURE_GENERAL          (0x80)
#define SAS_SMP_FUNC_ENABLE_DISABLE_ZONING      (0x81)
#define SAS_SMP_FUNC_WRITE_GPIO_REG             (0x82)
#define SAS_SMP_FUNC_ZONED_BROADCAST            (0x85)
#define SAS_SMP_FUNC_ZONE_LOCK                  (0x86)
#define SAS_SMP_FUNC_ZONE_ACTIVATE              (0x87)
#define SAS_SMP_FUNC_ZONE_UNLOCK                (0x88)
#define SAS_SMP_FUNC_CONF_ZONE_MNGR_PASSWORD    (0x89)
#define SAS_SMP_FUNC_CONF_ZONE_PHY_INFO         (0x8A)
#define SAS_SMP_FUNC_CONF_ZONE_PERM_TABLE       (0x8B)
#define SAS_SMP_FUNC_CONF_ROUTE_INFO            (0x90)
#define SAS_SMP_FUNC_PHY_CONTROL                (0x91)
#define SAS_SMP_FUNC_PHY_TEST_FUNCTION          (0x92)
#define SAS_SMP_FUNC_CONF_PHY_EVENT             (0x93)


/* SMP Request Function Results */
#define SAS_SMP_RESULT_ACCEPTED                 (0x00)
#define SAS_SMP_RESULT_UNKNOWN_FUNC             (0x01)
#define SAS_SMP_RESULT_FUNC_FAILED              (0x02)
#define SAS_SMP_RESULT_INV_REQ_FRAME_LEN        (0x03)
#define SAS_SMP_RESULT_INV_EXP_CHNG_CNT         (0x04)
#define SAS_SMP_RESULT_BUSY                     (0x05)
#define SAS_SMP_RESULT_INCOMPLETE_DESC_LIST     (0x06)
#define SAS_SMP_RESULT_PHY_DOES_NOT_EXIST       (0x10)
#define SAS_SMP_RESULT_INDEX_DOES_NOT_EXIST     (0x11)
#define SAS_SMP_RESULT_PHY_DOES_NOT_SUPP_SATA   (0x12)
#define SAS_SMP_RESULT_UNKNOWN_PHY_OP           (0x13)
#define SAS_SMP_RESULT_UNKNOWN_PHY_TEST_FUNC    (0x14)
#define SAS_SMP_RESULT_PHY_TEST_IN_PROG         (0x15)
#define SAS_SMP_RESULT_PHY_VACANT               (0x16)
#define SAS_SMP_RESULT_UKNOWN_PHY_EVENT_SOURCE  (0x17)
#define SAS_SMP_RESULT_UNKNOWN_DESC_TYPE        (0x18)
#define SAS_SMP_RESULT_UNKNOWN_PHY_FILTER       (0x19)
#define SAS_SMP_RESULT_AFFILIATION_VIOLATION    (0x1A)
#define SAS_SMP_RESULT_SMP_ZONE_VIOLATION       (0x20)
#define SAS_SMP_RESULT_NO_MGMT_ACCESS_RIGHTS    (0x21)
#define SAS_SMP_RESULT_UNKNOWN_ZONING_EN_DIS_VAL (0x22)
#define SAS_SMP_RESULT_ZONE_LOCK_VIOLATION      (0x23)
#define SAS_SMP_RESULT_NOT_ACTIVATED            (0x24)
#define SAS_SMP_RESULT_ZONE_GRP_OUT_OF_RANGE    (0x25)
#define SAS_SMP_RESULT_NO_PHYSICAL_PRESENCE     (0x26)
#define SAS_SMP_RESULT_SAVING_NOT_SUPPROTED     (0x27)
#define SAS_SMP_RESULT_ZONE_GRP_DOES_NOT_EXIST  (0x28)

/* SMP Phy Control Operations */
#define SAS_SMP_PHY_CTRL_NOP                    (0x00)
#define SAS_SMP_PHY_CTRL_LINK_RESET             (0x01)
#define SAS_SMP_PHY_CTRL_HARD_RESET             (0x02)
#define SAS_SMP_PHY_CTRL_DISABLE                (0x03)
#define SAS_SMP_PHY_CTRL_CLEAR_ERROR_LOG        (0x05)
#define SAS_SMP_PHY_CTRL_CLEAR_AFFILIATION      (0x06)
#define SAS_SMP_PHY_CTRL_XMIT_SATA_PORT_SLCTN_SIGNL (0x07)
#define SAS_SMP_PHY_CTRL_CLEAR_STP_IT_NEX_LOSS  (0x08)
#define SAS_SMP_PHY_CTRL_SET_ATTACHED_DEV_NAME  (0x09)


/* SAS-1.1 Request / Response header formats */
typedef struct SAS11_SMP_REQ_HDR_TAG
{
    UINT_8E     frame_type;
    UINT_8E     function;

} SAS11_SMP_REQ_HDR, *PSAS11_SMP_REQ_HDR;

typedef struct SAS11_SMP_RSP_HDR_TAG
{
    UINT_8E     frame_type;
    UINT_8E     function;
    UINT_8E     result;

} SAS11_SMP_RSP_HDR, *PSAS11_SMP_RSP_HDR;

/* SAS-2 Request / Response header formats */
typedef struct SAS20_SMP_REQ_HDR_TAG
{
    UINT_8E     frame_type;
    UINT_8E     function;
    UINT_8E     alloc_resp_len;
    UINT_8E     request_len;

} SAS20_SMP_REQ_HDR, *PSAS20_SMP_REQ_HDR;

typedef struct SAS20_SMP_RSP_HDR_TAG
{
    UINT_8E     frame_type;
    UINT_8E     function;
    UINT_8E     result;
    UINT_8E     response_len;

} SAS20_SMP_RSP_HDR, *PSAS20_SMP_RSP_HDR;


/*******************************************************************************
 *  SMP Report General
 */

/* The following are bits in the route_table_bits field */
#define SAS_REP_GEN_CONFIGURABLE_ROUTE_TABLE    (0x01)
#define SAS_REP_GEN_CONFIGURING                 (0x02)
#define SAS_REP_GEN_CONFIGURES_OTHERS           (0x04)
#define SAS_REP_GEN_OPEN_REJ_RETRY_SUPP         (0x08)
#define SAS_REP_GEN_STP_CONTINUE_AWT            (0x10)
#define SAS_REP_GEN_SELF_CONFIGURING            (0x20)
#define SAS_REP_GEN_ZONE_CONFIGURING            (0x40)
#define SAS_REP_GEN_TABLE_TO_TABLE_SUPP         (0x80)

#define SAS_REP_GEN_ZONING_ENABLED              (0x01)
#define SAS_REP_GEN_ZONING_SUPP                 (0x02)
#define SAS_REP_GEN_PHYS_PRES_ASSERTED          (0x04)
#define SAS_REP_GEN_PHYS_PRES_SUPP              (0x08)
#define SAS_REP_GEN_ZONE_LOCKED                 (0x10)
#define SAS_REP_GEN_NR_ZONE_GRPS_SIZE           (0x03)
#define SAS_REP_GEN_NR_ZONE_GRPS_SHIFT          (6)
#define SAS_REP_GEN_NR_ZONE_GRPS_MASK           \
    (SAS_REP_GEN_NR_ZONE_GRPS_SIZE << SAS_REP_GEN_NR_ZONE_GRPS_SHIFT)

#define SAS_REP_GEN_SV_ZONING_EN_SUPP           (0x01)
#define SAS_REP_GEN_SV_ZONING_PERM_SUPP         (0x02)
#define SAS_REP_GEN_SV_ZONING_PHY_INFO_SUPP     (0x04)
#define SAS_REP_GEN_SV_ZONING_PASSWD_SUPP       (0x08)
#define SAS_REP_GEN_SAVING                      (0x10)

#define SAS_REP_GEN_REDUCED_FUNC                (0x80)

#define SAS_REP_GEN_LONG_RESP                   (0x80)

typedef struct SAS11_SMP_REPORT_GENERAL_REQ_TAG
{
    SAS11_SMP_REQ_HDR   hdr;

    UINT_8E             res2[2];

    UINT_32E            crc;

} SAS11_SMP_REPORT_GENERAL_REQ, *PSAS11_SMP_REPORT_GENERAL_REQ;
CPD_COMPILE_TIME_ASSERT(3, sizeof(SAS11_SMP_REPORT_GENERAL_REQ) == 8);

typedef struct SAS20_SMP_REPORT_GENERAL_REQ_TAG
{
    SAS20_SMP_REQ_HDR   hdr;

    UINT_32E            crc;

} SAS20_SMP_REPORT_GENERAL_REQ, *PSAS20_SMP_REPORT_GENERAL_REQ;
CPD_COMPILE_TIME_ASSERT(4, sizeof(SAS20_SMP_REPORT_GENERAL_REQ) == 8);

typedef struct SAS11_SMP_REPORT_GENERAL_RSP_TAG
{
    SAS11_SMP_RSP_HDR   hdr;

    UINT_8E             res3;
    UINT_16E            exp_change_cnt;
    UINT_16E            exp_route_indexes;
    UINT_8E             res8;
    UINT_8E             nr_phys;
    UINT_8E             route_table_bits;
    UINT_8E             res11;
    UINT_64E            enclosure_id;
    UINT_8E             res20[8];
    
    UINT_32E            crc;

} SAS11_SMP_REPORT_GENERAL_RSP, *PSAS11_SMP_REPORT_GENERAL_RSP;
CPD_COMPILE_TIME_ASSERT(5, sizeof(SAS11_SMP_REPORT_GENERAL_RSP) == 32);

typedef struct SAS20_SMP_REPORT_GENERAL_RSP_TAG
{
    SAS20_SMP_RSP_HDR   hdr;

    UINT_16E            exp_change_cnt;
    UINT_16E            exp_route_indexes;
    UINT_8E             long_resp;
    UINT_8E             nr_phys;
    UINT_8E             route_table_bits;
    UINT_8E             res11;
    UINT_64E            enclosure_id;
    UINT_8E             res20[10];
    UINT_16E            stp_bus_inact_time_limit;
    UINT_16E            stp_max_conn_time_limit;
    UINT_16E            stp_smp_nexus_loss_time_limit;
    UINT_8E             zoning_bits_1;
    UINT_8E             zoning_bits_2;
    UINT_16E            max_nr_routed_sas_addrs;
    UINT_64E            act_zone_mgr_sas_addr;
    UINT_16E            zone_lock_inact_time_limit;
    UINT_8E             res50[3];
    UINT_8E             first_enc_conn_el_index;
    UINT_8E             nr_of_enc_conn_el_indexes;
    UINT_8E             res55;
    UINT_8E             reduced_func;
    UINT_8E             time_2_reduced_func;
    UINT_8E             initial_time_2_reduced_func;
    UINT_8E             max_reducsed_func_time;
    UINT_16E            last_self_config_list_desc_index;
    UINT_16E            max_self_config_list_desc;
    UINT_16E            last_phy_event_list_desc_index;
    UINT_16E            max_nr_phy_event_list_desc;
    UINT_16E            stp_rej_to_open_time_limit;
    UINT_8E             res70[2];

    UINT_32E            crc;

} SAS20_SMP_REPORT_GENERAL_RSP, *PSAS20_SMP_REPORT_GENERAL_RSP;
CPD_COMPILE_TIME_ASSERT(6, sizeof(SAS20_SMP_REPORT_GENERAL_RSP) == 76);


/*******************************************************************************
 *  SMP Report Manufacturer Information
 */


#define SAS_REP_MANF_INFO_SAS_1_1_FORMAT        (0x01)
#define SAS_REP_MANF_INFO_VEND_ID_SIZE          (8)
#define SAS_REP_MANF_INFO_PROD_ID_SIZE          (16)
#define SAS_REP_MANF_INFO_PROD_REV_SIZE         (4)
#define SAS_REP_MANF_INFO_COMP_VEND_ID_SIZE     (8)
#define SAS_REP_MANF_INFO_COMP_ID_SIZE          (2)


typedef struct SAS20_SMP_REPORT_MANF_INFO_REQ_TAG
{
    SAS20_SMP_REQ_HDR   hdr;

    UINT_32E            crc;
    
} SAS20_SMP_REPORT_MANF_INFO_REQ, *PSAS20_SMP_REPORT_MANF_INFO_REQ;
CPD_COMPILE_TIME_ASSERT(7, sizeof(SAS20_SMP_REPORT_MANF_INFO_REQ) == 8);


typedef struct SAS20_SMP_REPORT_MANF_INFO_RSP_TAG
{
    SAS20_SMP_RSP_HDR   hdr;

    UINT_16E            exp_change_cnt;
    UINT_8E             res6[2];
    UINT_8E             sas1_1;
    UINT_8E             res9[3];
    UINT_8E             vendor_id       [SAS_REP_MANF_INFO_VEND_ID_SIZE];
    UINT_8E             prod_id         [SAS_REP_MANF_INFO_PROD_ID_SIZE];
    UINT_8E             prod_rev        [SAS_REP_MANF_INFO_PROD_REV_SIZE];
    UINT_8E             comp_vendor_id  [SAS_REP_MANF_INFO_COMP_VEND_ID_SIZE];
    UINT_8E             comp_id         [SAS_REP_MANF_INFO_COMP_ID_SIZE];
    UINT_8E             comp_rev;
    UINT_8E             res51;
    UINT_8E             vu_emc_slots;
    UINT_8E             vu_unused[7];

    UINT_32E            crc;
    
} SAS20_SMP_REPORT_MANF_INFO_RSP, *PSAS20_SMP_REPORT_MANF_INFO_RSP;
CPD_COMPILE_TIME_ASSERT(8, sizeof(SAS20_SMP_REPORT_MANF_INFO_RSP) == 64);


/*******************************************************************************
 *  SMP Discover
 */

#define SAS_SMP_DISC_IGNORE_ZONE_GRP        (0x01)

/* Negotiated Physical/Logical Link Rates */
#define SAS_NEGO_LINK_RATE_SIZE                 (0x0f)
#define SAS_NEGO_LINK_RATE_UNKNOWN              (0x00)
#define SAS_NEGO_LINK_RATE_DISABLED             (0x01)
#define SAS_NEGO_LINK_RATE_PHY_RESET_PROBLEM    (0x02)
#define SAS_NEGO_LINK_RATE_SPINUP_HOLD          (0x03)
#define SAS_NEGO_LINK_RATE_PORT_SELECTOR        (0x04)
#define SAS_NEGO_LINK_RATE_RESET_IN_PROGRESS    (0x05)
#define SAS_NEGO_LINK_RATE_UNSUPP_PHY_ATTACHED  (0x06)
#define SAS_NEGO_LINK_RATE_RESERVED             (0x07)
#define SAS_NEGO_LINK_RATE_G1                   (0x08)
#define SAS_NEGO_LINK_RATE_G2                   (0x09)
#define SAS_NEGO_LINK_RATE_G3                   (0x0A)
#define SAS_NEGO_LINK_RATE_G4                   (0x0B)
/* The SAS-2 spec resrves 0x0C to 0x0F for future link rates */
#define SAS_NEGO_LINK_RATE_FUTURE1              (0x0C)
#define SAS_NEGO_LINK_RATE_FUTURE2              (0x0D)
#define SAS_NEGO_LINK_RATE_FUTURE3              (0x0E)
#define SAS_NEGO_LINK_RATE_FUTURE4              (0x0F)
#define SAS_NEGO_LINK_RATE_FUTURE5              (0x10)

/* Negotiated Link Rate access masks */
#define SAS_NEGO_LINK_RATE_SHIFT                (0)
#define SAS_NEGO_LINK_RATE_MASK                 (SAS_NEGO_LINK_RATE_SIZE << SAS_NEGO_LINK_RATE_SHIFT)

#define SAS_SMP_DISC_VIRT_PHY                   (0x80)

typedef struct SAS11_SMP_DISCOVER_REQ_TAG
{
    SAS11_SMP_REQ_HDR   hdr;

    UINT_8E             res2[7];
    UINT_8E             phy_id;
    UINT_8E             res10[2];

    UINT_32E            crc;

} SAS11_SMP_DISCOVER_REQ, *PSAS11_SMP_DISCOVER_REQ;
CPD_COMPILE_TIME_ASSERT(9, sizeof(SAS11_SMP_DISCOVER_REQ) == 16);

typedef struct SAS20_SMP_DISCOVER_REQ_TAG
{
    SAS20_SMP_REQ_HDR   hdr;

    UINT_8E             res4[4];
    UINT_8E             ignore_zone_grp;
    UINT_8E             phy_id;
    UINT_8E             res10[2];

    UINT_32E            crc;

} SAS20_SMP_DISCOVER_REQ, *PSAS20_SMP_DISCOVER_REQ;
CPD_COMPILE_TIME_ASSERT(10, sizeof(SAS20_SMP_DISCOVER_REQ) == 16);


typedef struct SAS11_SMP_DISCOVER_RSP_TAG
{
    SAS11_SMP_RSP_HDR   hdr;

    UINT_8E             res3[6];
    UINT_8E             phy_id;
    UINT_8E             res10[2];
    UINT_8E             att_dev_type;
    UINT_8E             nego_phy_link_rate;
    UINT_8E             att_init_bits;
    UINT_8E             att_targ_bits;
    UINT_64E            sas_addr;
    UINT_64E            att_sas_addr;
    UINT_8E             att_phy_id;
    UINT_8E             res33[7];
    UINT_8E             min_link_rates;
    UINT_8E             max_link_rates;
    UINT_8E             phy_chng_cnt;
    UINT_8E             virt_phy_part_path;
    UINT_8E             routing_attr;
    UINT_8E             conn_type;
    UINT_8E             conn_el_index;
    UINT_8E             conn_phys_link;
    UINT_8E             res48[2];
    UINT_8E             vendor_specific[2];

    UINT_32E            crc;

} SAS11_SMP_DISCOVER_RSP, *PSAS11_SMP_DISCOVER_RSP;
CPD_COMPILE_TIME_ASSERT(11, sizeof(SAS11_SMP_DISCOVER_RSP) == 56);

typedef struct SAS20_SMP_DISCOVER_RSP_TAG
{
    SAS20_SMP_RSP_HDR   hdr;

    UINT_16E            exp_chng_cnt;
    UINT_8E             res6[3];
    UINT_8E             phy_id;
    UINT_8E             res10[2];
    UINT_8E             att_dev_type_reason;
    UINT_8E             nego_log_link_rate;
    UINT_8E             att_init_bits;
    UINT_8E             att_targ_bits;
    UINT_64E            sas_addr;
    UINT_64E            att_sas_addr;
    UINT_8E             att_phy_id;
    UINT_8E             att_zone_break_bits;
    UINT_8E             res34[6];
    UINT_8E             min_link_rates;
    UINT_8E             max_link_rates;
    UINT_8E             phy_chng_cnt;
    UINT_8E             virt_phy_part_path;
    UINT_8E             routing_attr;
    UINT_8E             conn_type;
    UINT_8E             conn_el_index;
    UINT_8E             conn_phys_link;
    UINT_8E             res48[2];
    UINT_8E             vendor_specific[2];
    UINT_64E            att_dev_name;
    UINT_8E             zone_bits;
    UINT_8E             res61[2];
    UINT_8E             zone_grp;
    UINT_8E             self_conf_status;
    UINT_8E             self_conf_level_com;
    UINT_8E             res66[2];
    UINT_64E            self_conf_sas_addr;
    UINT_32E            prog_phy_cap;
    UINT_32E            curr_phy_cap;
    UINT_32E            att_phy_cap;
    UINT_8E             res88[6];
    UINT_8E             reason_nego_phy_link_rate;
    UINT_8E             nego_ssc_hw_mux_supp;
    UINT_8E             def_zone_bits;
    UINT_8E             res97[2];
    UINT_8E             def_zone_grp;
    UINT_8E             saved_zone_bits;
    UINT_8E             res101[2];
    UINT_8E             saved_zone_group;
    UINT_8E             shadow_zone_bits;
    UINT_8E             res105[2];
    UINT_8E             shadow_zone_grp;
    UINT_8E             device_slot_nr;
    UINT_8E             group_nr;
    UINT_8E             path_to_enclosure[2];

    UINT_32E            crc;

} SAS20_SMP_DISCOVER_RSP, *PSAS20_SMP_DISCOVER_RSP;
CPD_COMPILE_TIME_ASSERT(12, sizeof(SAS20_SMP_DISCOVER_RSP) == 116);

/*******************************************************************************
 *  SMP Discover List (SAS-2 ONLY)
 */

#define SAS_DISC_LIST_REQUEST_LEN           (0x06)
#define SAS_DISC_LIST_DESC_ARRAY_SIZE       (980)
#define SAS_SMP_DISC_LIST_IGNORE_ZONE_GRP   (0x80)

#define SAS_DISC_LIST_FILTER_SIZE           (0x0F)
#define SAS_DISC_LIST_FILTER_ALL_PHYS       (0x00)
#define SAS_DISC_LIST_FILTER_DEV_EXP        (0x01)
#define SAS_DISC_LIST_FILTER_DEV_END_EXP    (0x02)

/* Phy Filter access masks */
#define SAS_DISC_LIST_FILTER_SHIFT          (0)
#define SAS_DISC_LIST_FILTER_MASK \
    (SAS_DISC_LIST_FILTER_SIZE << SAS_DISC_LIST_FILTER_SHIFT)

#define SAS_DISC_LIST_DESC_TYPE_SIZE        (0x0F)
#define SAS_DISC_LIST_DESC_TYPE_FULL        (0x00)
#define SAS_DISC_LIST_DESC_TYPE_SHORT       (0x01)

/* Phy Filter access masks */
#define SAS_DISC_LIST_DESC_TYPE_SHIFT       (0)
#define SAS_DISC_LIST_DESC_TYPE_MASK \
    (SAS_DISC_LIST_DESC_TYPE_SIZE << SAS_DISC_LIST_DESC_TYPE_SHIFT)

/* The following are bits in the route_table_bits field */
#define SAS_DISC_LIST_CONFIGURABLE_ROUTE_TABLE    (0x01)
#define SAS_DISC_LIST_CONFIGURING                 (0x02)
#define SAS_DISC_LIST_ZONE_CONFIGURING            (0x04)
#define SAS_DISC_LIST_SELF_CONFIGURING            (0x08)
#define SAS_DISC_LIST_ZONING_ENABLED              (0x40)
#define SAS_DISC_LIST_ZONING_SUPP                 (0x80)


typedef struct SAS20_SMP_DISCOVER_LIST_REQ_TAG
{
    SAS20_SMP_REQ_HDR   hdr;

    UINT_8E             res4[4];
    UINT_8E             starting_phy_id;
    UINT_8E             max_nr_descriptors;
    UINT_8E             ignore_zg_phy_filter;
    UINT_8E             descriptor_type;
    UINT_8E             res12[4];
    UINT_8E             vendor_specific[12];

    UINT_32E            crc;

} SAS20_SMP_DISCOVER_LIST_REQ, *PSAS20_SMP_DISCOVER_LIST_REQ;
CPD_COMPILE_TIME_ASSERT(13, sizeof(SAS20_SMP_DISCOVER_LIST_REQ) == 32);

typedef struct SAS20_SMP_DISC_LIST_SHORT_DESC_TAG
{
    UINT_8E             phy_id;
    UINT_8E             result;
    UINT_8E             att_dev_type_reason;
    UINT_8E             nego_log_link_rate;
    UINT_8E             att_init_bits;
    UINT_8E             att_targ_bits;
    UINT_8E             virt_phy_routing_attr;
    UINT_8E             reason;
    UINT_8E             zone_grp;
    UINT_8E             zone_bits;
    UINT_8E             att_phy_id;
    UINT_8E             phy_chng_cnt;
    UINT_64E            att_sas_addr;
    UINT_8E             res20[4];

} SAS20_SMP_DISC_LIST_SHORT_DESC, *PSAS20_SMP_DISC_LIST_SHORT_DESC;
CPD_COMPILE_TIME_ASSERT(14, sizeof(SAS20_SMP_DISC_LIST_SHORT_DESC) == 24);

typedef struct SAS20_SMP_DISCOVER_LIST_RSP_TAG
{
    SAS20_SMP_RSP_HDR   hdr;

    UINT_16E            exp_change_cnt;
    UINT_8E             res6[2];
    UINT_8E             starting_phy_id;
    UINT_8E             nr_descriptors;
    UINT_8E             phy_filter;
    UINT_8E             descriptor_type;
    UINT_8E             descriptor_len;
    UINT_8E             res13[3];
    UINT_8E             route_table_bits;
    UINT_8E             res17;
    UINT_16E            last_self_conf_desc_idx;
    UINT_16E            last_phy_event_desc_idx;
    UINT_8E             res22[10];
    UINT_8E             vendor_specific[16];
    UINT_8E             desc_list[SAS_DISC_LIST_DESC_ARRAY_SIZE];

    UINT_32E            crc;

} SAS20_SMP_DISC_LIST_RSP, *PSAS20_SMP_DISC_LIST_RSP;
CPD_COMPILE_TIME_ASSERT(15, sizeof(SAS20_SMP_DISC_LIST_RSP) == 1032);

#define SAS20_SMP_DISC_LIST_MAX_SHORT_DESCS \
    (SAS_DISC_LIST_DESC_ARRAY_SIZE/sizeof(SAS20_SMP_DISC_LIST_SHORT_DESC))


/****************************************************************
* SMP PHY CONTROL
*********************/
typedef struct SAS11_PHY_CONTROL_REQ_TAG
{
    SAS11_SMP_REQ_HDR   hdr;

    UINT_8E             reserved_2[7];
    UINT_8E             phy_id;
    UINT_8E             phy_operation;
    UINT_8E             update_pway_to_val;     //***Use only bit 0. The rest are reserved.
    UINT_8E             reserved_12[20];
    UINT_8E             min_phys_link_rate;     //***Use only bits 4-7. The rest are reserved.
    UINT_8E             max_phys_link_rate;     //***Use only bits 4-7. The rest are reserved.
    UINT_8E             reserved_34[2];
    UINT_8E             pway_to_val;            //***Use only bits 0-3. The rest are reserved.
    UINT_8E             reserved_37[3];

    UINT_32E            crc;

} SAS11_PHY_CONTROL_REQ, *PSAS11_PHY_CONTROL_REQ;
CPD_COMPILE_TIME_ASSERT(16, sizeof(SAS11_PHY_CONTROL_REQ) == 44);

typedef struct SAS11_PHY_CONTROL_RSP_TAG
{
    SAS11_SMP_RSP_HDR   hdr;

    UINT_8E             reserved_3;

    UINT_32E            crc;

} SAS11_PHY_CONTROL_RSP, *PSAS11_PHY_CONTROL_RSP;
CPD_COMPILE_TIME_ASSERT(17, sizeof(SAS11_PHY_CONTROL_RSP) == 8);

typedef struct SAS20_PHY_CONTROL_REQ_TAG
{
    SAS20_SMP_REQ_HDR   hdr;
    
    UINT_16E            exp_change_count;
    UINT_8E             reserved_6[3];
    UINT_8E             phy_id;
    UINT_8E             phy_operation;
    UINT_8E             update_pway_to_val;     //***Use only bit 0. The rest are reserved.
    UINT_8E             reserved_12[12];
    UINT_64E            attached_dev_name;
    UINT_8E             min_phys_link_rate;     //***Use only bits 4-7. The rest are reserved.
    UINT_8E             max_phys_link_rate;     //***Use only bits 4-7. The rest are reserved.
    UINT_8E             reserved_34[2];
    UINT_8E             pway_to_val;            //***Use only bits 0-3. The rest are reserved.
    UINT_8E             reserved_37[3];

    UINT_32E            crc;

} SAS20_PHY_CONTROL_REQ, *PSAS20_PHY_CONTROL_REQ;
CPD_COMPILE_TIME_ASSERT(18, sizeof(SAS20_PHY_CONTROL_REQ) == 44);

typedef struct SAS20_PHY_CONTROL_RSP_TAG
{
    SAS20_SMP_RSP_HDR   hdr;

    UINT_32E            crc;

} SAS20_PHY_CONTROL_RSP, *PSAS20_PHY_CONTROL_RSP;
CPD_COMPILE_TIME_ASSERT(19, sizeof(SAS20_PHY_CONTROL_RSP) == 8);


/*******************************************************************************
 *  SMP Report Device Path (Vendor Unique, SAS-2 ONLY)
 */

#define SAS_VU_REP_DEVICE_PATH_REQ_LEN          (2)     /* In dwords */
#define SAS_VU_REP_DEVICE_PATH_RSP_LEN_NO_BITMAP (1)    /* In dwords */
#define SAS_VU_REP_DEVICE_PATH_RSP_LEN_BITMAP   (11)    /* In dwords */
#define SAS_VU_REP_DEVICE_PATH_DIRECT_ATT_BIT   (0x01)
#define SAS_VU_REP_DEVICE_PATH_BITMAP_SIZE      (32)    /* In bytes */


typedef struct SAS20_SMP_VU_REPORT_DEVICE_PATH_REQ_TAG
{
    SAS20_SMP_REQ_HDR   hdr;

    UINT_64E            sas_address;

    UINT_32E            crc;

} SAS20_SMP_VU_REPORT_DEVICE_PATH_REQ, *PSAS20_SMP_VU_REPORT_DEVICE_PATH_REQ;
CPD_COMPILE_TIME_ASSERT(20 ,sizeof(SAS20_SMP_VU_REPORT_DEVICE_PATH_REQ) == 16);

typedef struct SAS20_SMP_VU_REPORT_DEVICE_PATH_RSP_TAG
{
    SAS20_SMP_RSP_HDR   hdr;

    UINT_8E             direct_bit;
    UINT_8E             res5;
    UINT_16E            exp_chng_cnt;
    UINT_64E            sas_address;
    UINT_8E             bitmap[SAS_VU_REP_DEVICE_PATH_BITMAP_SIZE];

    UINT_32E            crc;

} SAS20_SMP_VU_REPORT_DEVICE_PATH_RSP, *PSAS20_SMP_VU_REPORT_DEVICE_PATH_RSP;
CPD_COMPILE_TIME_ASSERT(21, sizeof(SAS20_SMP_VU_REPORT_DEVICE_PATH_RSP) == 52);


//-------------------------------------------

/* Only needs to be as big as biggest command */
typedef union SAS_SMP_REQ_TAG
{
    SAS11_SMP_REPORT_GENERAL_REQ    sas11_report_general_req;
    SAS20_SMP_REPORT_GENERAL_REQ    sas20_report_general_req;
    SAS20_SMP_REPORT_MANF_INFO_REQ  sas20_report_manf_info_req;
    SAS11_SMP_DISCOVER_REQ          sas11_discover_req;
    SAS20_SMP_DISCOVER_REQ          sas20_discover_req;
    SAS20_SMP_DISCOVER_LIST_REQ     sas20_discover_list_req;
    SAS11_PHY_CONTROL_REQ           sas11_phy_ctrl_req;
    SAS20_PHY_CONTROL_REQ           sas20_phy_ctrl_req;
    SAS20_SMP_VU_REPORT_DEVICE_PATH_REQ sas20_vu_rep_dev_path_req;

} SAS_SMP_REQ, *PSAS_SMP_REQ;

/* Allow for the largest legal response, a full SMP response frame */
typedef union SAS_SMP_RSP_TAG
{
    SAS11_SMP_REPORT_GENERAL_RSP    sas11_report_general_rsp;
    SAS20_SMP_REPORT_GENERAL_RSP    sas20_report_general_rsp;
    SAS20_SMP_REPORT_MANF_INFO_RSP  sas20_report_manf_info_rsp;
    SAS11_SMP_DISCOVER_RSP          sas11_discover_rsp;
    SAS20_SMP_DISCOVER_RSP          sas20_discover_rsp;
    SAS20_SMP_DISC_LIST_RSP         sas20_dis_list_rsp;
    SAS11_PHY_CONTROL_RSP           sas11_phy_ctrl_rsp;
    SAS20_PHY_CONTROL_RSP           sas20_phy_ctrl_rsp;
    SAS20_SMP_VU_REPORT_DEVICE_PATH_RSP sas20_vu_rep_dev_path_rsp;

    UINT_8E                         raw_rsp[SAS_SMP_MAX_FRAME_SIZE];

} SAS_SMP_RSP, *PSAS_SMP_RSP;


#pragma pack()


#endif  /* SAS_SMP_H */
