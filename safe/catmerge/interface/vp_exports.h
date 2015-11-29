#ifndef VP_EXPORTS_H
#define VP_EXPORTS_H 0x00000001         /* from common dir */
#define FLARE__STAR__H


/***************************************************************************
 * Copyright (C) Data General Corporation 1989-1998
 * All rights reserved.
 * Licensed material -- property of Data General Corporation
 ***************************************************************************/

/********************************************************************
 * $Id: vp_exports.h,v 1.1.8.1 2000/10/13 14:26:23 fladm Exp $
 ********************************************************************
 *
 *      Description:    Global Header file for the verify process.
 *                      Contains declarations used outside Flare.
 *
 * History:
 *      06/21/01 CJH    Created
 *********************************************************************/

/*
 * LOCAL TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/vp/Attic/vp_exports.h,v 1.1.8.1 2000/10/13 14:26:23 fladm Exp $";
 */

/***** INCLUDE FILES *****/

/***** CONSTANTS *****/

/*
 * No-change values for use in IOCTLs
 */
#define VP_SNIFF_TIME_NC      (UNSIGNED_MINUS_1)    /* No change */
#define VP_VERIFY_TIME_NC     (UNSIGNED_MINUS_1)    /* No change */


/***** STRUCTURES *****/

/* The Verify Results Report data structure is used
 * to build the reports required by mode page 0x23.
 */
typedef struct verify_results_report
{
    UINT_8E ReportType;
    UINT_8E VerifyState;
    UINT_8E PercentComplete;
    UINT_8E Passes_MSB;
    UINT_8E Passes_LSB;
    UINT_8E Reserved5;
    UINT_8E Reserved6;
    UINT_8E Reserved7;
    UINT_8E C_CRC_Error_MSB;    /* Correctable Checksum Errors */
    UINT_8E C_CRC_Error_LSB;
    UINT_8E U_CRC_Error_MSB;    /* Uncorrectable Checksum Errors */
    UINT_8E U_CRC_Error_LSB;
    UINT_8E C_WS_Error_MSB;     /* Correctable Write Stamp Errors */
    UINT_8E C_WS_Error_LSB;
    UINT_8E U_WS_Error_MSB;     /* Uncorrectable Write Stamp Errors */
    UINT_8E U_WS_Error_LSB;
    UINT_8E C_TS_Error_MSB;     /* Correctable Time Stamp Errors */
    UINT_8E C_TS_Error_LSB;
    UINT_8E U_TS_Error_MSB;     /* Uncorrectable Time Stamp Errors */
    UINT_8E U_TS_Error_LSB;
    UINT_8E C_SS_Error_MSB;     /* Correctable Shed Stamp Errors */
    UINT_8E C_SS_Error_LSB;
    UINT_8E U_SS_Error_MSB;     /* Uncorrectable Shed Stamp Errors */
    UINT_8E U_SS_Error_LSB;

    UINT_8E C_Coherency_Error_MSB;	/* Correctable Coherency Errors */
    UINT_8E C_Coherency_Error_LSB;
    UINT_8E U_Coherency_Error_MSB;	/* Uncorrectable Coherency Errors */
    UINT_8E U_Coherency_Error_LSB;
}
VERIFY_RESULTS_REPORT;

/* The min and max sniff rates in units of 100 milliseconds 
 * (0 and 255 are valid but result in no change)
 */
#define VP_MIN_SNIFF_TIME       1
#define VP_MAX_SNIFF_TIME       254

/* If the VP_DEFAULT_SNIFF_TIME value changes, please also update following 2 files:
 *  
 * 1. catmerge\disk\flare_driver\flaredrv_reg.c 
 * 2. catmerge\disk\flare\fcli\fcli_other_cmds.c 
 *  
 * Search VP_DEFAULT_SNIFF_TIME in the comments will lead to the place. 
 * The reason we did not include this header file when updating the 2 above file is, 
 * there are some header conflicts, and in order to reduce risk, we did not want to 
 * change header file layout as part of the patch work. 
 */
#define VP_DEFAULT_SNIFF_TIME   10	/* 1 Sniff IO every 1 Second for other platforms */

#define VP_INTERNAL_LUN_DEFAULT_SNIFF_TIME   6	/* 1 Sniff IO every 0.6 Second */

/* VP_HOT_SPARE_SNIFF_RATE is the rate set for idle hot spare sniffing.
 * 
 * Combined with a verify size of 11 MB, this will result
 * in a sniff rate of ~1.5 GB per hour.
 */
#define VP_HOT_SPARE_SNIFF_RATE VP_MAX_SNIFF_TIME

/* 
 * VP_INTERNAL_LUN_SNIFF_DAYS is the number of days it should
 * take to verify internal LUNs.
 * With the Vault LUN size changed to 100 GB and PSM LUN size
 * changed to 40GB, total days to complete sniff verify has 
 * been extended from 4 to 7.
 */
#define VP_INTERNAL_LUN_SNIFF_DAYS  7

/* VP_INTERNAL_LUN_SNIFF_HOURS is the total hours it should take
 * to verify internal LUNs.  This is just VP_INTERNAL_LUN_SNIFF_DAYS
 * multiplied by the number of hours in a day.
 */
#define VP_INTERNAL_LUN_SNIFF_HOURS VP_INTERNAL_LUN_SNIFF_DAYS * 24

/*
 * End $Id: vp_exports.h,v 1.1.8.1 2000/10/13 14:26:23 fladm Exp $
 */

#endif
