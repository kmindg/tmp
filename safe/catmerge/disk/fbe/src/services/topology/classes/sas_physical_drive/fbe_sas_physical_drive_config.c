/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sas_physical_drive_config.c
 ***************************************************************************
 *
 * @brief
 *  This file defines the mode sense/select data structures.
 * 
 * HISTORY
 *   12/30/2010:  Created. Wayne Garrett
 *
 ***************************************************************************/

#include "sas_physical_drive_config.h"

#if 0
fbe_vendor_page_t fbe_def_sas_vendor_tbl[] =
{
/*
 *  Default Drive Table
 *
 *  This table is used for default drive settings.  The default drive at this
 * time is the Seagate drive.
 *
 *  mask indicates which bits are okay to clear at destination.  any bit on is
 * cleared at destination.
 *
 *   page            offset                               mask                       value
 */
    {FBE_MS_RECOV_PG01, FBE_PG01_ERR_CNTL_OFFSET, FBE_PG01_ERR_CNTL_MASK,
     FBE_PG01_ERR_CNTL_BYTE},
    {FBE_MS_RECOV_PG01, FBE_PG01_READ_RETRY_OFFSET, 0xFF,
     FBE_PG01_READ_RETRY_COUNT_BYTE},
    {FBE_MS_RECOV_PG01, FBE_PG01_RECOVERY_LIMIT_OFFSET0, 0xFF,
     FBE_PG01_RECOVERY_LIMIT0_BYTE},
    {FBE_MS_RECOV_PG01, FBE_PG01_RECOVERY_LIMIT_OFFSET1, 0xFF,
     FBE_PG01_RECOVERY_LIMIT1_BYTE},
    {FBE_MS_DISCO_PG02, FBE_PG02_BFR_OFFSET, 0xFF, FBE_PG02_DEFAULT_BFR},
    {FBE_MS_DISCO_PG02, FBE_PG02_BER_OFFSET, 0xFF, FBE_PG02_DEFAULT_BER},
    {FBE_MS_VERRECOV_PG07, FBE_PG07_READ_RETRY_OFFSET, 0xFF,
     FBE_PG07_READ_RETRY_COUNT_BYTE},
    {FBE_MS_VERRECOV_PG07, FBE_PG07_RECOVERY_LIMIT_OFFSET0, 0xFF,
     FBE_PG07_RECOVERY_LIMIT0_BYTE},
    {FBE_MS_VERRECOV_PG07, FBE_PG07_RECOVERY_LIMIT_OFFSET1, 0xFF,
     FBE_PG07_RECOVERY_LIMIT1_BYTE},
    {FBE_MS_CACHING_PG08, FBE_PG08_RD_WR_CACHE_OFFSET, FBE_PG08_RD_WR_CACHE_MASK,
     FBE_PG08_RD_CACHE_BYTE},
    {FBE_MS_CACHING_PG08, FBE_PG08_CACHE_MAX_PREFETCH_OFFSET_0, 0xFF,
     FBE_PG08_MAX_PREFETCH_MASK},
    {FBE_MS_CACHING_PG08, FBE_PG08_CACHE_MAX_PREFETCH_OFFSET_1, 0xFF,
     FBE_PG08_MAX_PREFETCH_MASK},
    {FBE_MS_CACHING_PG08, FBE_PG08_CACHE_SEG_COUNT_OFFSET, 0xFF,
     FBE_PG08_CACHE_SEG_COUNT_BYTE},
    {FBE_MS_CONTROL_PG0A, FBE_PG0A_QERR_OFFSET, 0xFF, FBE_PG0A_QAM_BIT_MASK},
    {FBE_MS_POWER_PG1A, FBE_PG1A_IDLE_STANDBY_OFFSET, FBE_PG1A_IDLE_STANDBY_MASK,
     FBE_PG1A_IDLE_STANDBY_BYTE},
    {FBE_MS_REPORT_PG1C, FBE_PG1C_PERF_OFFSET, FBE_PG1C_PERF_MASK, 
     FBE_PG1C_PERF_BYTE},
    {FBE_MS_REPORT_PG1C, FBE_PG1C_MRIE_OFFSET, FBE_PG1C_MRIE_MASK,
     FBE_PG1C_MRIE_BYTE},
};

fbe_vendor_page_t fbe_seg_sas_vendor_tbl[] =
{
/*
 *  Seagate Drive Table
 *
 *  This table is used for Seagate drive settings.
 *
 *  mask indicates which bits are okay to clear at destination.  any bit on is
 * cleared at destination.
 *
 *   page            offset                               mask                       value
 */
    {FBE_MS_RECOV_PG01, FBE_PG01_ERR_CNTL_OFFSET, FBE_PG01_ERR_CNTL_MASK,
     FBE_PG01_ERR_CNTL_BYTE},
    {FBE_MS_RECOV_PG01, FBE_PG01_READ_RETRY_OFFSET, 0xFF,
     FBE_PG01_READ_RETRY_COUNT_BYTE},
    {FBE_MS_RECOV_PG01, FBE_PG01_RECOVERY_LIMIT_OFFSET0, 0xFF,
     FBE_PG01_RECOVERY_LIMIT0_BYTE},
    {FBE_MS_RECOV_PG01, FBE_PG01_RECOVERY_LIMIT_OFFSET1, 0xFF,
     FBE_PG01_RECOVERY_LIMIT1_BYTE},
    {FBE_MS_VERRECOV_PG07, FBE_PG07_READ_RETRY_OFFSET, 0xFF,
     FBE_PG07_READ_RETRY_COUNT_BYTE},
    {FBE_MS_VERRECOV_PG07, FBE_PG07_RECOVERY_LIMIT_OFFSET0, 0xFF,
     FBE_PG07_RECOVERY_LIMIT0_BYTE},
    {FBE_MS_VERRECOV_PG07, FBE_PG07_RECOVERY_LIMIT_OFFSET1, 0xFF,
     FBE_PG07_RECOVERY_LIMIT1_BYTE},
    {FBE_MS_CACHING_PG08, FBE_PG08_RD_WR_CACHE_OFFSET, FBE_PG08_RD_WR_CACHE_MASK,
     FBE_PG08_RD_CACHE_BYTE},
    {FBE_MS_CACHING_PG08, FBE_PG08_CACHE_MAX_PREFETCH_OFFSET_0, 0xFF,
     FBE_PG08_MAX_PREFETCH_MASK},
    {FBE_MS_CACHING_PG08, FBE_PG08_CACHE_MAX_PREFETCH_OFFSET_1, 0xFF,
     FBE_PG08_MAX_PREFETCH_MASK},
    {FBE_MS_CACHING_PG08, FBE_PG08_CACHE_SEG_COUNT_OFFSET, 0xFF,
     FBE_PG08_CACHE_SEG_COUNT_BYTE},
    {FBE_MS_CONTROL_PG0A, FBE_PG0A_QERR_OFFSET, 0xFF, FBE_PG0A_QAM_BIT_MASK},
    {FBE_MS_POWER_PG1A, FBE_PG1A_IDLE_STANDBY_OFFSET, FBE_PG1A_IDLE_STANDBY_MASK,
     FBE_PG1A_IDLE_STANDBY_BYTE},
    {FBE_MS_REPORT_PG1C, FBE_PG1C_PERF_OFFSET, FBE_PG1C_PERF_MASK, 
     FBE_PG1C_PERF_BYTE},
    {FBE_MS_REPORT_PG1C, FBE_PG1C_MRIE_OFFSET, FBE_PG1C_MRIE_MASK,
     FBE_PG1C_MRIE_BYTE},
};

fbe_vendor_page_t fbe_ibm_sas_vendor_tbl[] =
{
/*
 *  IBM Drive Table
 *
 *  This table contains drive settings for all IBM drives.
 *
 *  mask indicates which bits are okay to clear at destination.  any bit on is
 * cleared at destination.
 *
 *   page              offset                             mask                        value
 */
    {FBE_MS_RECOV_PG01, FBE_PG01_ERR_CNTL_OFFSET, FBE_PG01_ERR_CNTL_MASK,
     FBE_PG01_ERR_CNTL_BYTE},
    {FBE_MS_RECOV_PG01, FBE_PG01_RECOVERY_LIMIT_OFFSET0, 0xFF,
     FBE_PG01_IBM_RECOVERY_LIMIT0_BYTE},
    {FBE_MS_RECOV_PG01, FBE_PG01_RECOVERY_LIMIT_OFFSET1, 0xFF,
     FBE_PG01_IBM_RECOVERY_LIMIT1_BYTE},
    {FBE_MS_DISCO_PG02, FBE_PG02_BFR_OFFSET, 0xFF, FBE_PG02_IBM_DEFAULT_BFR},
    {FBE_MS_DISCO_PG02, FBE_PG02_BER_OFFSET, 0xFF, FBE_PG02_IBM_DEFAULT_BER},
    {FBE_MS_VERRECOV_PG07, FBE_PG07_RECOVERY_LIMIT_OFFSET0, 0xFF,
     FBE_PG07_IBM_RECOVERY_LIMIT0_BYTE},
    {FBE_MS_VERRECOV_PG07, FBE_PG07_RECOVERY_LIMIT_OFFSET1, 0xFF,
     FBE_PG07_IBM_RECOVERY_LIMIT1_BYTE},
    {FBE_MS_CACHING_PG08, FBE_PG08_RD_WR_CACHE_OFFSET, FBE_PG08_RD_WR_CACHE_MASK,
     FBE_PG08_RD_CACHE_BYTE},
    {FBE_MS_CONTROL_PG0A, FBE_PG0A_QERR_OFFSET, 0xFF, FBE_PG0A_QAM_BIT_MASK},
    {FBE_MS_FIBRE_PG19, FBE_PG19_FC_OFFSET, FBE_PG19_DEFAULT_FC_MASK,
     FBE_PG19_DEFAULT_FC_VALUE},
    {FBE_MS_REPORT_PG1C, FBE_PG1C_MRIE_OFFSET, FBE_PG1C_MRIE_MASK,
     FBE_PG1C_MRIE_BYTE},
    {FBE_MS_VENDOR_PG00, FBE_PG00_QPE_OFFSET, FBE_PG00_QPE_MASK, FBE_PG00_QPE_MASK},
    {FBE_MS_VENDOR_PG00, FBE_PG00_EICM_OFFSET, FBE_PG00_HDUP_EICM_MASK,
     FBE_PG00_EICM_MASK},
};
#endif



