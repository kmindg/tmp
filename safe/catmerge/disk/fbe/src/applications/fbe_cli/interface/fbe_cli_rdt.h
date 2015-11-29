#ifndef __FBE_CLI_RDT_H__
#define __FBE_CLI_RDT_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_rdt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains LUN/RG command cli definitions.
 *
 * @version
 *  07/01/2010 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"


void fbe_cli_rdt(int argc , char ** argv);

#define RDT_CMD_USAGE "\
 rdt <flags> <operation> <lun decimal> <lba 0x0..capacity> [count 0x0..0x1000]\n\
 Lun number, Lba and count are Hexa-decimal.\n\
Operations:\n\
     r  - Read\n\
     w  - Write (Read is not needed if -sp is used)\n\
     s  - Write Same (Must use -sp) \n\
     c  - Corrupt CRC\n\
     d  - Corrupt Data\n\
     v  - Verify\n\
     f  - Delete Buffer\n\
     z  - Zero\n\
     rv - Read verify \n\
     iv - Incomplete write verify \n\
     sv - System RW verify \n\
     sav - Specific area verify \n\
     wv - Write verify \n\
     ev - Error verify \n\
     av - Abort verify \n\
     vw - verify and then write\n\
     vb - verify with buffer - only for R10\n\
     rb - rebuild - work with pdo/ldo\n\
     wn - write with non cached io\n\
Flags:\n\
    -object_id  [object_id_value] - Use this object Id for block operation. \n\
                                    LUN value is not significant in this case.\n\
    -package_id [sep | physical]  - Use this package Id for block operation. \n\
                                    LUN value is not significant in this case.\n\
                                    above Object Id will be passed with SEP package id if not provided by user\n\
    -d   - Display buffer - Used on reads to display the buffer just read.\n\
    -sp  - Set the pattern - Used on writes to set the rdgen pattern into the buffer being written.\n\
    -cp  - Check pattern - Used on reads to check that the pattern read matches\n\
                           the default rdgen pattern.\n\
    -cz  - Check zeros - Used on reads to check that zeros were read for all blocks.\n\
    -no_crc - Tell raid to not check the crc.  This allows reading and displaying bad blocks.\n\
    -bad_crc - Used on a write type of command to attempt writing a block with a bad crc (not invalidated).\n\
    -pattern [raid | lost | cor_crc | cor_data] - Use this data pattern. \n\
\n\
Examples:  \n\
    Read and display a block from an object:\n\
      rdt -d -object_id 0x13c -package_id sep r 0x15 0 1\n\
    Read and display 2 blocks from lun 4 and tell raid not to check the crc.\n\
      rdt -d -no_crc r 4 0x15 2\n\
    Read and display 2 blocks from lun 4\n\
      rdt -d r 4 0x15 2\n\
    Write 5 blocks to lba 0x100:\n\
      rdt -object_id 0x5 -package_id physical w 0 0x100 0x5\n"


/*!*******************************************************************
 * @def FBE_CLI_RDT_MAX_SECTORS_BLOCK_OP
 *********************************************************************
 * @brief
 *   This is the maximum sectors supported for the block operation.
 *   Note that this limit is not for verify, rebuild and zero operation
 *********************************************************************/
 #define FBE_CLI_RDT_MAX_SECTORS_BLOCK_OP 4096

/*!**************************************************
 * @def FBE_RDT_HEADER_PATTERN
 ***************************************************
 * @brief This pattern appears at the head of all
 * sectors generated.
 ***************************************************/
#define FBE_RDT_HEADER_PATTERN 0x3CC3

/*************************
 * end file fbe_cli_rdt.h
 *************************/

#endif /* end __FBE_CLI_RDT_H__ */
