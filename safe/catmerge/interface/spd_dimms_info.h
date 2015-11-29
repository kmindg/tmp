#ifndef  SPD_DIMMS_INFO_H
#define  SPD_DIMMS_INFO_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *  spd_dimms_info.h
 ****************************************************************
 *
 *  Description:
 *  This header file contains structures which define
 *  the format of the DIMMs information.
 *
 *  Documents:
 *  JEDEC:
 *  Serial Presence Detect (SPD), General Standard
 *  Serial Presence Detect (SPD) for DDR2 SDRAM Modules,  Revision 1.3
 *  Serial Presence Detect (SPD) for Fully Buffered DIMM, Revision 1.1
 *  Serial Presence Detect (SPD) for DDR3 SDRAM Modules,  Document Release 6
 *  Serial Presence Detect (SPD) for DDR4 SDRAM Modules,  Document Release 2
 *
 *  EMC:
 *  DIMM Serialization, EMC Serial Number Definition Rev 05 (999-998-716)
 ****************************************************************/

#include "generic_types.h"
#include "csx_ext.h"

#define SPD_DATA_SIZE_1                     256     /* for DDR2 DIMM, FB_DIMM, DDR3 DIMM */
#define SPD_DATA_SIZE_2                     512     /* for DDR4 DIMM */
#define SPD_DEVICE_TYPE_OFFSET              2       /* used to identify the DIMM type */

#define DIMM_CHECKSUM_OFFSET                63      /* for DDR2 DIMM */
#define DIMM_CHECKSUM_COVER_0_TO_62         62
#define DIMM_CHECKSUM_MASK                  0x00FF 
#define DIMM_CRC_OFFSET                     126     /* for FB_DIMM, DDR3 DIMM, DDR4 DIMM */
#define DIMM_CRC_COVER_MASK                 0x80
#define DIMM_CRC_COVER_0_TO_116             116
#define DIMM_CRC_COVER_0_TO_125             125
#define DIMM_CRC_CCITT_PLYNOM               0x1021

#define JEDEC_ID_CODE_SIZE                  2
#define MANUFACTURING_DATE_SIZE             2
#define MODULE_SERIAL_NUMBER_SIZE           4
#define PART_NUMBER_SIZE_1                  18      /* for DDR2 DIMM, FB_DIMM, DDR3 DIMM */
#define PART_NUMBER_SIZE_2                  20      /* for DDR4 DIMM */

/* Common MASK & SHIFT BITS */
#define DIMM_SPD_BYTES_USED_MASK            0x0F    /* Byte 0: Bits 3 - 0 */

/* DDR2 MASK & SHIFT BITS */
#define DDR2_RANK_MASK                      0x07    /* Byte 5: Bits 2 - 0 */

/* FB_DIMM MASK & SHIFT BITS */
#define DDR2_FB_BANK_MASK                   0x03    /* Byte 4: Bits 1 - 0 */
#define DDR2_FB_COL_ADDR_MASK               0x1C    /* Byte 4: Bits 4 - 2 */
#define DDR2_FB_COL_ADDR_SHIFT_BITS         2
#define DDR2_FB_ROW_ADDR_MASK               0xE0    /* Byte 4: Bits 7 - 5 */
#define DDR2_FB_ROW_ADDR_SHIFT_BITS         5
#define DDR2_FB_SDRAM_DEVICE_WIDTH_MASK     0x07    /* Byte 7: Bits 2 - 0 */
#define DDR2_FB_RANK_MASK                   0x38    /* Byte 7: Bits 5 - 3 */
#define DDR2_FB_RANK_SHIFT_BITS             3

/* DDR3 MASK & SHIFT BITS */
#define DDR3_TOTAL_SDRAM_CAPACITY_MASK      0x0F    /* Byte 4: Bits 3 - 0 */
#define DDR3_BANK_MASK                      0x70    /* Byte 4: Bits 6 - 4 */
#define DDR3_BANK_SHIFT_BITS                4
#define DDR3_COL_ADDR_MASK                  0x07    /* Byte 5: Bits 2 - 0 */
#define DDR3_ROW_ADDR_MASK                  0x38    /* Byte 5: Bits 5 - 3 */
#define DDR3_ROW_ADDR_SHIFT_BITS            3
#define DDR3_SDRAM_DEVICE_WIDTH_MASK        0x07    /* Byte 7: Bits 2 - 0 */
#define DDR3_RANK_MASK                      0x38    /* Byte 7: Bits 5 - 3 */
#define DDR3_RANK_SHIFT_BITS                3
#define DDR3_PRIMARY_BUS_WIDTH_MASK         0x07    /* Byte 8: Bits 2 - 0 */

/* DDR4 MASK & SHIFT BITS */
#define DDR4_TOTAL_SDRAM_CAPACITY_MASK      0x0F    /* Byte 4: Bits 3 - 0 */
#define DDR4_BANK_MASK                      0x30    /* Byte 4: Bits 5 - 4 */
#define DDR4_BANK_SHIFT_BITS                4
#define DDR4_BANK_GROUP_MASK                0xC0    /* Byte 4: Bits 7 - 6 */
#define DDR4_BANK_GROUP_SHIFT_BITS          6
#define DDR4_COL_ADDR_MASK                  0x07    /* Byte 5: Bits 2 - 0 */
#define DDR4_ROW_ADDR_MASK                  0x38    /* Byte 5: Bits 5 - 3 */
#define DDR4_ROW_ADDR_SHIFT_BITS            3
#define DDR4_SIGNAL_LOADING_MASK            0x03    /* Byte 6: Bits 1 - 0 */
#define DDR4_DIE_COUNT_MASK                 0x70    /* Byte 6: Bits 6 - 4 */
#define DDR4_DIE_COUNT_SHIFT_BITS           4
#define DDR4_SDRAM_PACKAGE_TYPE_MASK        0x80    /* Byte 6: Bits 7 */
#define DDR4_SDRAM_DEVICE_WIDTH_MASK        0x07    /* Byte 12: Bits 2 - 0 */
#define DDR4_PACKAGE_RANK_MASK              0x38    /* Byte 12: Bits 5 - 3 */
#define DDR4_PACKAGE_RANK_SHIFT_BITS        3
#define DDR4_PRIMARY_BUS_WIDTH_MASK         0x07    /* Byte 13: Bits 2 - 0 */

/* EMC */
#define EMC_DIMM_PART_NUMBER_SIZE           14      /* Size(P/N) */
#define EMC_DIMM_PN_AND_CHECKSUM_GAP_SIZE   50      /* Offset(Checksum) - Offset(P/N) - Size(P/N) */
#define EMC_DIMM_SPECIFIC_DATA_OFFSET_1     176     /* for DDR2 DIMM, FB_DIMM, DDR3 DIMM */
#define EMC_DIMM_SPECIFIC_DATA_OFFSET_2     432     /* for DDR4 DIMM */
#define EMC_DIMM_CHECKSUM_START_OFFSET      63
#define EMC_DIMM_CHECKSUM_END_OFFSET        240     /* 0xF0: checksum */
#define EMC_DIMM_CHECKSUM_START_OFFSET_DDR4 320     /* 0x140, Manufactoring area start */
#define EMC_DIMM_CHECKSUM_END_OFFSET_DDR4   496     /* 0x1F0: checksum */
#define EMC_DIMM_CHECKSUM_MASK              0x00FF 
#define EMC_DIMM_SERIAL_NUMBER_SIZE         9
#define OLD_EMC_DIMM_SERIAL_NUMBER_SIZE     10
#define EMC_DIMM_MODULE_NAME_SIZE           64
#define EMC_DIMM_VENDOR_NAME_SIZE           64

typedef enum _SPD_INFO_FIELD
{
    MODULE_SERIAL_NUMBER,
    MANUFACTURING_LOCATION,
    MODULE_PART_NUMBER,
    MANUFACTURING_DATE,
    EMC_DIMM_SERIAL_NUMBER,
} SPD_INFO_FIELD;

typedef enum _SPD_DEVICE_TYPE
{
    DDR2_SDRAM              = 0x08, /* AKA R_DIMM */
    DDR2_SDRAM_FBDIMM       = 0x09,
    DDR3_SDRAM              = 0x0B,
    DDR4_SDRAM              = 0x0C,
} SPD_DEVICE_TYPE;

typedef enum _SPD_REVISION
{
    REVISION_00 = 0x00,
    REVISION_01 = 0x01,
    REVISION_02 = 0x02,
    REVISION_03 = 0x03,
    REVISION_04 = 0x04,
    REVISION_05 = 0x05,
    REVISION_06 = 0x06,
    REVISION_07 = 0x07,
    REVISION_08 = 0x08,
    REVISION_09 = 0x09,
    REVISION_10 = 0x10,
    REVISION_11 = 0x11,
    REVISION_12 = 0x12,
} SPD_REVISION;

typedef enum _SPD_SIGNAL_LOADING_TYPE
{
    MULTI_LOAD_STACK    = 0x01,
    SINGLE_LOAD_STACK   = 0x02,
} SPD_SIGNAL_LOADING_TYPE;

#pragma pack(push, spd_dimm_info, 1)

typedef struct _SPD_DDR2_SDRAM_INFO
{
    csx_uchar_t        number_of_spd_bytes;                        /* Byte 0 */
    csx_uchar_t        total_number_of_bytes;
    csx_uchar_t        device_type;
    csx_uchar_t        number_row_addresses;
    csx_uchar_t        number_column_addresses;
    csx_uchar_t        ranks_package_height;                       /* Byte 5 */
    csx_uchar_t        module_data_width;
    csx_uchar_t        reserved_byte_7;
    csx_uchar_t        voltage_level;
    csx_uchar_t        cycle_time;
    csx_uchar_t        access_from_clock;                          /* Byte 10 */
    csx_uchar_t        configuration_type;
    csx_uchar_t        refresh_rate_type;
    csx_uchar_t        device_width;
    csx_uchar_t        error_check_data_width;
    csx_uchar_t        reserved_byte_15;                           /* Byte 15 */
    csx_uchar_t        burst_length;
    csx_uchar_t        number_of_banks;
    csx_uchar_t        cas_latencies;
    csx_uchar_t        module_thickness;
    csx_uchar_t        dimm_type;                                  /* Byte 20 */
    csx_uchar_t        module_attributes;
    csx_uchar_t        device_attributes;
    csx_uchar_t        min_clock_cycle_1;
    csx_uchar_t        t_max_ac_1;
    csx_uchar_t        min_clock_cycle_2;                          /* Byte 25 */
    csx_uchar_t        t_max_ac_2;
    csx_uchar_t        t_min_rp;
    csx_uchar_t        t_min_rrd;
    csx_uchar_t        t_min_rcd;
    csx_uchar_t        t_min_ras;                                  /* Byte 30 */
    csx_uchar_t        module_rank_density;
    csx_uchar_t        t_is;
    csx_uchar_t        t_ih;
    csx_uchar_t        t_ds;
    csx_uchar_t        t_dh;                                       /* Byte 35 */
    csx_uchar_t        t_wr;
    csx_uchar_t        t_wtr;
    csx_uchar_t        t_rtp;
    csx_uchar_t        memory_analysis_probe;
    csx_uchar_t        ext_t_min_rc_rfc;                           /* Byte 40 */
    csx_uchar_t        t_min_rc;
    csx_uchar_t        t_min_rfc;
    csx_uchar_t        t_max_ck;
    csx_uchar_t        t_max_dqsq;
    csx_uchar_t        t_max_qhs;                                  /* Byte 45 */
    csx_uchar_t        pll_relock_time;
    csx_uchar_t        optional_features_not_supported[15];        /* Byte 47 - 61 */
    csx_uchar_t        spd_revision;                               /* Byte 62 */
    csx_uchar_t        checksum;                                   /* Byte 63 */
    csx_uchar_t        module_manufacturing_jedec_id_code[2];      /* Byte 64 - 65 */
    csx_uchar_t        module_manufacturing_jedec_id_code_2[6];    /* Byte 65 - 71 */
    csx_uchar_t        module_manufacturing_location;              /* Byte 72 */
    csx_uchar_t        module_part_number[18];                     /* Byte 73 - 90 */
    csx_uchar_t        module_revision_code[2];                    /* Byte 91 - 92 */
    csx_uchar_t        module_manufacturing_date[2];               /* Byte 93 - 94 */
    csx_uchar_t        module_serial_number[4];                    /* Byte 95 - 98 */
    csx_uchar_t        manufacturers_specific_data[29];            /* Byte 99 - 127 */
    csx_uchar_t        reserved_for_user[128];                     /* Byte 128 - 255 */
} SPD_DDR2_SDRAM_INFO, *PSPD_DDR2_SDRAM_INFO;

typedef struct _SPD_DDR2_SDRAM_FBDIMM_INFO
{
    csx_uchar_t        number_of_spd_bytes;                        /* Byte 0 */
    csx_uchar_t        spd_revision;
    csx_uchar_t        device_type;
    csx_uchar_t        voltage_level;
    csx_uchar_t        sdram_addressing;
    csx_uchar_t        height_thickness;                           /* Byte 5 */
    csx_uchar_t        dimm_type;
    csx_uchar_t        module_organization;
    csx_uchar_t        fine_timebase_dividend_divisor;
    csx_uchar_t        medium_timebase_dividend;
    csx_uchar_t        medium_timebased_divisor;                   /* Byte 10 */
    csx_uchar_t        t_min_ck;
    csx_uchar_t        t_max_ck;
    csx_uchar_t        cas_latencies_supported;
    csx_uchar_t        t_min_cas;
    csx_uchar_t        t_wr_supported;                             /* Byte 15 */
    csx_uchar_t        t_wr;
    csx_uchar_t        write_latencies_supported;
    csx_uchar_t        additive_latencies_supported;
    csx_uchar_t        t_min_rcd;
    csx_uchar_t        t_min_rrd;                                  /* Byte 20 */
    csx_uchar_t        t_min_rp;
    csx_uchar_t        upper_nibbles_for_ras_rc;
    csx_uchar_t        t_min_ras;
    csx_uchar_t        t_min_rc;
    csx_uchar_t        t_min_rfc_lsb;                              /* Byte 25 */
    csx_uchar_t        t_min_rfc_msb;
    csx_uchar_t        t_wtr;
    csx_uchar_t        t_rtp;
    csx_uchar_t        burst_length_supported;
    csx_uchar_t        terminations_supported;                     /* Byte 30 */
    csx_uchar_t        drivers_supported;
    csx_uchar_t        refresh_rate;
    csx_uchar_t        tcasemax;
    csx_uchar_t        thermal_resistance;
    csx_uchar_t        dt0;                                        /* Byte 35 */
    csx_uchar_t        dt2n_dt2q;
    csx_uchar_t        dt2p;
    csx_uchar_t        dt3n;
    csx_uchar_t        dt4r;
    csx_uchar_t        dt5b;                                       /* Byte 40 */
    csx_uchar_t        dt7;
    csx_uchar_t        reserved_bytes_from_42[18];                 /* Byte 42 - 59 */
    csx_uchar_t        reserved_bytes_from_60[19];                 /* Byte 60 - 78 */
    csx_uchar_t        odt_values;
    csx_uchar_t        reserved_byte_80;                           /* Byte 80 */
    csx_uchar_t        channel_protocol_supported_lsb;
    csx_uchar_t        channel_protocol_supported_msb;
    csx_uchar_t        back_to_back_access_turnaround_time;
    csx_uchar_t        amb_read_access_time_800;
    csx_uchar_t        amb_read_access_time_667;                   /* Byte 85 */
    csx_uchar_t        amb_read_access_time_533;
    csx_uchar_t        thermal_resistance_amb;
    csx_uchar_t        amb_dt_idle_0;
    csx_uchar_t        amb_dt_idle_1;
    csx_uchar_t        amb_dt_idle_2;                              /* Byte 90 */
    csx_uchar_t        amb_dt_active_1;
    csx_uchar_t        amb_dt_active_2;
    csx_uchar_t        amb_dt_l0;
    csx_uchar_t        reserved_bytes_from_94[4];                  /* Byte 94 - 97 */
    csx_uchar_t        amb_case_temp_max;
    csx_uchar_t        reserved_bytes_from_99[2];                  /* Byte 99 - 100 */
    csx_uchar_t        amb_personality_pre[6];
    csx_uchar_t        amb_personality_post[8];
    csx_uchar_t        amb_manufacturing_jedec_id_code[2];         /* Byte 115 - 116 */
    csx_uchar_t        module_manufacturing_jedec_id_code[2];      /* Byte 117 - 118 */
    csx_uchar_t        module_manufacturing_location;              /* Byte 119 */
    csx_uchar_t        module_manufacturing_date[2];               /* Byte 120 - 121 */
    csx_uchar_t        module_serial_number[4];                    /* Byte 122 - 125 */
    csx_uchar_t        crc[2];                                     /* Byte 126 - 127 */
    csx_uchar_t        module_part_number[18];                     /* Byte 128 - 145 */
    csx_uchar_t        module_revision_code[2];                    /* Byte 146 - 147 */
    csx_uchar_t        dram_manufacturers_jedec_id_code[2];        /* Byte 148 - 149 */
    csx_uchar_t        manufacturerers_specific_data[26];          /* Byte 150 - 175 */
    csx_uchar_t        reserved_for_user[80];                      /* Byte 176 - 255 */
} SPD_DDR2_SDRAM_FBDIMM_INFO, *PSPD_DDR2_SDRAM_FBDIMM_INFO;

typedef struct _SPD_DDR3_SDRAM_INFO
{
    csx_uchar_t        number_of_spd_bytes;                        /* Byte 0 */
    csx_uchar_t        spd_revision;
    csx_uchar_t        device_type;
    csx_uchar_t        dimm_type;
    csx_uchar_t        density_banks;
    csx_uchar_t        sdram_addressing;                           /* Byte 5 */
    csx_uchar_t        voltage_level;
    csx_uchar_t        module_organization;
    csx_uchar_t        module_memory_bus_width;
    csx_uchar_t        fine_timebase_dividend_divisor;
    csx_uchar_t        medium_timebase_dividend;                   /* Byte 10 */
    csx_uchar_t        medium_timebased_divisor;
    csx_uchar_t        t_min_ck;
    csx_uchar_t        reserved_byte_13;
    csx_uchar_t        cas_latencies_supported_lsb;
    csx_uchar_t        cas_latencies_supported_msb;                /* Byte 15 */
    csx_uchar_t        t_min_aa;
    csx_uchar_t        t_min_wr;
    csx_uchar_t        t_min_rcd;
    csx_uchar_t        t_min_rrd;
    csx_uchar_t        t_min_rp;                                   /* Byte 20 */
    csx_uchar_t        upper_nibbles_for_ras_rc;
    csx_uchar_t        t_min_ras_lsb;
    csx_uchar_t        t_min_rc_lsb;
    csx_uchar_t        t_min_rfc_lsb;
    csx_uchar_t        t_min_rfc_msb;                              /* Byte 25 */
    csx_uchar_t        t_min_wtr;
    csx_uchar_t        t_min_rtp;
    csx_uchar_t        upper_nibbles_for_faw;
    csx_uchar_t        t_min_faw;
    csx_uchar_t        sdram_optional_features;                    /* Byte 30 */
    csx_uchar_t        sdram_thermal_refresh_options;
    csx_uchar_t        thermal_sensor;
    csx_uchar_t        sdram_package_type;
    csx_uchar_t        fine_offset_t_min_ck;
    csx_uchar_t        fine_offset_t_min_aa;                       /* Byte 35 */
    csx_uchar_t        fine_offset_t_min_rcd;
    csx_uchar_t        fine_offset_t_min_rp;
    csx_uchar_t        fine_offset_t_min_rc;
    csx_uchar_t        reserved_bytes_from_39[2];                  /* Byte 39 - 40 */
    csx_uchar_t        sdram_mac_value;
    csx_uchar_t        reserved_bytes_from_42[18];                 /* Byte 42 - 59 */
    csx_uchar_t        module_type_specific_section[57];           /* Byte 60 - 116 */
    csx_uchar_t        module_manufacturing_jedec_id_code[2];      /* Byte 117 - 118 */
    csx_uchar_t        module_manufacturing_location;              /* Byte 119 */
    csx_uchar_t        module_manufacturing_date[2];               /* Byte 120 - 121 */
    csx_uchar_t        module_serial_number[4];                    /* Byte 122 - 125 */
    csx_uchar_t        crc[2];                                     /* Byte 126 - 127 */
    csx_uchar_t        module_part_number[18];                     /* Byte 128 - 145 */
    csx_uchar_t        module_revision_code[2];                    /* Byte 146 - 147 */
    csx_uchar_t        dram_manufacturers_jedec_id_code[2];        /* Byte 148 - 149 */
    csx_uchar_t        manufacturers_specific_data[26];            /* Byte 150 - 175 */
    csx_uchar_t        reserved_for_user[80];                      /* Byte 176 - 255 */
} SPD_DDR3_SDRAM_INFO, *PSPD_DDR3_SDRAM_INFO;

typedef struct _SPD_DDR4_SDRAM_INFO
{
    csx_uchar_t        number_of_spd_bytes;                        /* Byte 0 */
    csx_uchar_t        spd_revision;
    csx_uchar_t        device_type;
    csx_uchar_t        dimm_type;
    csx_uchar_t        density_banks;
    csx_uchar_t        sdram_addressing;                           /* Byte 5 */
    csx_uchar_t        sdram_package_type;
    csx_uchar_t        sdram_optional_features;
    csx_uchar_t        sdram_thermal_refresh_options;
    csx_uchar_t        sdram_other_optional_features;
    csx_uchar_t        reserved_byte_10;                           /* Byte 10 */
    csx_uchar_t        voltage_level;
    csx_uchar_t        module_organization;
    csx_uchar_t        module_memory_bus_width;
    csx_uchar_t        thermal_sensor;
    csx_uchar_t        extended_module_type;                       /* Byte 15 */
    csx_uchar_t        reserved_byte_16;
    csx_uchar_t        timebases;
    csx_uchar_t        t_min_ckavg;
    csx_uchar_t        t_max_ckavg;
    csx_uchar_t        cas_latencies_supported_first_byte;         /* Byte 20 */
    csx_uchar_t        cas_latencies_supported_second_byte;
    csx_uchar_t        cas_latencies_supported_third_byte;
    csx_uchar_t        cas_latencies_supported_fourth_byte;
    csx_uchar_t        t_min_aa;
    csx_uchar_t        t_min_rcd;                                  /* Byte 25 */
    csx_uchar_t        t_min_rp;
    csx_uchar_t        upper_nibbles_for_ras_rc;
    csx_uchar_t        t_min_ras_lsb;
    csx_uchar_t        t_min_rc_lsb;
    csx_uchar_t        t_min_rfc1_lsb;                             /* Byte 30 */
    csx_uchar_t        t_min_rfc1_msb;
    csx_uchar_t        t_min_rfc2_lsb;
    csx_uchar_t        t_min_rfc2_msb;
    csx_uchar_t        t_min_rfc4_lsb;
    csx_uchar_t        t_min_rfc4_msb;                             /* Byte 35 */
    csx_uchar_t        t_min_faw_msb;
    csx_uchar_t        t_min_faw_lsb;
    csx_uchar_t        t_min_rrd_s;
    csx_uchar_t        t_min_rrd_l;
    csx_uchar_t        t_min_ccd_l;                                /* Byte 40 */
    csx_uchar_t        reserved_bytes_from_41[19];                 /* Byte 41 - 59 */
    csx_uchar_t        connector_bit_mapping[18];                  /* Byte 60 - 77 */
    csx_uchar_t        reserved_bytes_from_78[39];                 /* Byte 78 - 116 */
    csx_uchar_t        fine_offset_t_min_ccd_l;
    csx_uchar_t        fine_offset_t_min_rrd_l;
    csx_uchar_t        fine_offset_t_min_rrd_s;
    csx_uchar_t        fine_offset_t_min_rc;                       /* Byte 120 */
    csx_uchar_t        fine_offset_t_min_rp;
    csx_uchar_t        fine_offset_t_min_rcd;
    csx_uchar_t        fine_offset_t_min_aa;
    csx_uchar_t        fine_offset_t_max_ckavg;
    csx_uchar_t        fine_offset_t_min_ckavg;                    /* Byte 125 */
    csx_uchar_t        crc_base_lsb;                               /* Byte 126 */
    csx_uchar_t        crc_base_msb;                               /* Byte 127 */
    csx_uchar_t        module_specific_parameters[128];            /* Byte 128 - 255 */
    csx_uchar_t        reserved_bytes_from_256[64];                /* Byte 256 - 319 */
    csx_uchar_t        module_manufacturers_id_code_lsb;           /* Byte 320 */
    csx_uchar_t        module_manufacturers_id_code_msb;           /* Byte 321 */
    csx_uchar_t        module_manufacturing_location;              /* Byte 322 */
    csx_uchar_t        module_manufacturing_date[2];               /* Byte 323 - 324 */
    csx_uchar_t        module_serial_number[4];                    /* Byte 325 - 328 */
    csx_uchar_t        module_part_number[20];                     /* Byte 329 - 348 */
    csx_uchar_t        module_revision_code;                       /* Byte 349 */
    csx_uchar_t        dram_manufacturers_id_code_lsb;             /* Byte 350 */
    csx_uchar_t        dram_manufacturers_id_code_msb;             /* Byte 351 */
    csx_uchar_t        dram_stepping;                              /* Byte 352 */
    csx_uchar_t        manufacturers_specific_data[29];            /* Byte 353 - 381 */
    csx_uchar_t        reserved_bytes_from_382[2];                 /* Byte 382 - 383 */
    csx_uchar_t        reserved_for_user[128];                     /* Byte 384 - 511 */
} SPD_DDR4_SDRAM_INFO, *PSPD_DDR4_SDRAM_INFO;

typedef struct _SPD_DIMM_EMC_SPECIFIC
{
    UINT_8E               emc_part_number[EMC_DIMM_PART_NUMBER_SIZE];
    UINT_8E               emc_unused[EMC_DIMM_PN_AND_CHECKSUM_GAP_SIZE];
    UINT_8E               emc_dimm_checksum;
} SPD_DIMM_EMC_SPECIFIC, *PSPD_DIMM_EMC_SPECIFIC;

typedef union _SPD_DIMM_INFO
{
    SPD_DDR2_SDRAM_INFO         ddr2_sdram_info;
    SPD_DDR2_SDRAM_FBDIMM_INFO  ddr2_sdram_fbdimm_info;
    SPD_DDR3_SDRAM_INFO         ddr3_sdram_info;
    SPD_DDR4_SDRAM_INFO         ddr4_sdram_info;
} SPD_DIMM_INFO, *PSPD_DIMM_INFO;

#pragma pack(pop, spd_dimm_info)

#endif //SPD_DIMMS_INFO_H
