#ifndef SC_TAGS_H
#define    SC_TAGS_H

/***************************************************************************
 * Copyright (C) Data General Corporation 1989-1999
 * All rights reserved.
 * Licensed material -- property of Data General Corporation
 ***************************************************************************/

/***************************************************************************
 * $Id: sc_tags.h,v 1.5 1999/04/30 20:16:10 fladm Exp $
 ***************************************************************************
 *
 * DESCRIPTION:
 *    This header file contains the two byte control-tag values for the
 *    various tags used by the storage centric setup interface as well as
 *    numerous other general constants useful in processing the CTLD data
 *    streams.
 *
 *
 *        THE FOLLOWING WAS EXCERPTED FROM THE STORAGE CENTRIC SETUP
 *        INTERFACE SPEC AND DESCRIBES THE CTLD FORMAT USED BY THE
 *        INTERFACE
 *
 *  Appendix A:  TLD (Tag Length Data) Protocol
 *               Nov 4, 1998
 *               rev 0.3
 *
 *  - Summary:
 *      TLD is a  simple,well defined,flexible protocol for data exchange
 *      between FLARE and the outside world. Currently this outside world
 *      is Navisphere and the protocol is currently intended for use with
 *      SCSI Advanced Array Setup/Query
 *  
 *  - Definitions:
 *  
 *      TLD: (Tag-Length-Data). This is the protocol name
 *      CTLD: (Control-Tag-Length-Data). This is the component sequence
 *      BTLD: (Binary-Tag-Length-Data). This is the default mode of TLD.
 *          The tag and length are interpreted as binary
 *      ATLD: (Ascii-Tag-Length-Data). This is the alternate mode of TLD.
 *          The tag and length are ascii based and each terminated by a 
 *          binary 0
 *      TLD stream: The stream of TLD structures that are parsed from
 *          a source. This implies 1 or more TLD structures.
 *          (For our purposes, this is the data after the CDB)
 *      mode: The TLD can be in binary or ascii mode. See below
 *
 *  - The CTLD (or default) structure looks like:
 *  
 *      component    bytes (V=Variable)
 *      ----------------------------------
 *      control        1
 *      tag            V    (max 4 bytes in binary mode)
 *      length        V    (max 8 bytes in binary mode)
 *      data        V
 *  
 *  - The Expanded control byte looks like: (bits 0-7, right to left)
 *  
 *    Binary mode
 *    -----------
 *  
 *      bits        meaning            values
 *      --------------------------------------------------------
 *      0        mode                    0
 *      1        application                reserved
 *      2-4        length bytes
 *      5        sub-tags included        0 = no; 1 = yes
 *      6-7        tag bytes
 *  
 *  
 *    Ascii mode
 *    -----------
 *  
 *      bits        meaning            values
 *      --------------------------------------------------------
 *      0        mode            1
 *      1-7        application        reserved
 *  
 *  
 *  
 *  - Control notes:
 *  
 *    - When in binary mode:
 *      - the mode bit is off
 *      - both the tag and length are encoded as big endian 
 *        (i.e right to left or LSB,right- MSB lsft)
 *        regardless of the source encoding it.
 *      - maximum tag length is 4 bytes
 *      - the binary value extracted for the tag means "the number of bytes
 *        to read the tag + 1"
 *      - maximum data length is 8 bytes
 *      - the binary value extracted for the length means "the number of bytes
 *        to read the data + 1"
 *      - the endian bit determines the binary value of the tag and length
 *
 *    - When in Ascii mode:
 *      - The mode bit is on
 *      - This mode is typically used to:
 *          1. transmit tags that are id strings
 *          2. no endian-ness issues
 *          3. transmit tag, lengths that are bigger than the binary max
 *      - tag is a string terminated by binary zero. There is no limit on 
 *        length. (Although the application may want to impose one)
 *      - The length is an ascii digit sequence terminated by binary zero.
 *        Only the ascii characters [0-9] are allowed. There is no limit on 
 *        length. (Although the application may want to impose one).
 *        A length of zero may be conveyed either by:
 *          1. no ascii digits (i.e a binary zero)
 *          2. a seqence of 1 or more ascii zeros (i.e "0.*")
 *      - The data may have binary zeros, but is dictated by the length.
 *      - The rest of the control byte is application reserved.
 *      - endian bit is strictly for the applications use and applies only to
 *        the data. It must be encoded    and decoded by the application.
 *  
 *  
 *  
 *  - Other notes:
 *    - if length is zero, then there is no data component to follow.
 *    - parsing stops immediately on any error.
 *  
 *  - TLD Error conditions:
 *    - byte stream ends before the designated length is parsed.
 *    - non ascii digits for length when in ascii mode.
 *  
 *  
 *  - Application Error conditions:
 *    - TLD stream (parsed TLD objects) not equal to total length of CDB header
 *    - One TLD is greater than total length of CDB header.
 *    - Any application error when parsing a TLD.
 *      (i.e unknown tag, duplicate tag, unexpected tag, unexpected length, 
 *       bad data, etc ...)
 *
 *
 *
 * NOTES:
 *
 * HISTORY:
 *            30-Dec-98  CHH   Created
 *
 * $Log: sc_tags.h,v $
 * Revision 1.5  1999/04/30 20:16:10  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Interface and VLUT modifications to support new
 * heterogeneous host initiator type and options setup
 *
 * Revision 1.4  1999/04/02 23:00:02  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Improvements to parsing of input CTLD data and handling of
 * unknown tags.
 *
 * Revision 1.3  1999/03/30 14:23:14  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Changes to support peer to peer communications on SC "Get"
 * operations
 *
 * Revision 1.2  1999/03/22 19:51:36  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Added SC_TAG_VA_PARAMETER
 *
 * Revision 1.1  1999/03/10 13:56:59  fladm
 * rename:old file removed
 *
 * Revision 1.1  1999/02/22 22:17:31  fladm
 * User: kdobkows
 * Reason: Storage_Centric
 * Memory VLUT support added.
 *
 ***************************************************************************/

/*
 * LOCAL TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/sc/sc_tags.h,v 1.5 1999/04/30 20:16:10 fladm Exp $"
 */

/*************************
 * INCLUDE FILES
 *************************/

#include "global_ctl_tags.h"

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/*  These codes are used in the CDB's operation qualifier field  */
#define SC_GENERAL_ALL            0

#define    SC_INIT_ALL            0
#define SC_INIT_REQUESTOR         1

#define SC_PHYS_ALL               0
#define SC_PHYS_FLUS              1
#define SC_PHYS_ARRAY_INFO        2
#define SC_PHYS_PORTS             3


/*  A bit to mask the "sub tags included" bit in the CTLD control byte  */
#define SC_SUB_TAGS_INCL          0x20


/*  A bit to mask the reserved bit in a CTLD control byte  */
#define SC_CNTL_RES_BIT           0x02


/* A bit to mask the control byte's mode bit  */
#define SC_CNTL_MODE_BIT          0x01


/*  Mask to strip the length field's length out of a CTL control byte  */
#define SC_CONTROL_LEN_MASK       0x1C


/* Mask to strip the tag field's length out of a CTL control byte  */
#define SC_CONTROL_TAG_MASK       0xC0



#define FLU_NUM_MAX_LEN           2
#define VLU_NUM_MAX_LEN           2
#define PORT_NUM_MAX_LEN          2




/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/************************
 * PROTOTYPE DEFINITIONS
 ************************/

/*
 * End $Id: sc_tags.h,v 1.5 1999/04/30 20:16:10 fladm Exp $
 */

#endif
