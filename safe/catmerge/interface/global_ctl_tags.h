#ifndef GLOBAL_TAGS_H
#define    GLOBAL_TAGS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2006
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/***************************************************************************
 * $Id: global_ctl_tags.h,v 1.1 2000/02/09 18:05:29 fladm Exp $
 ***************************************************************************
 *
 * DESCRIPTION:
 *    This header file contains the tag values and database IDs
 *    used by the Advanced Array Setup/Query commands.
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
 *
 * $Log: global_ctl_tags.h,v $
 * Revision 1.1  2000/02/09 18:05:29  fladm
 * User: hopkins
 * Reason: Trunk_EPR_Fixes
 * EPR 3632 - New file containing CTLD tag values from various
 * sources.
 *
 *  04/06/05 P. Caldwell    Added new Sniffer TAG (TAG_USER_SNIFF_CONTROL_ENABLE)
 *  07/12/05 R. Hicks       DIMS 126137: Added backdoor disk power commands 
 *                          (TAG_DH_DOWN & TAG_DH_UP)
 *  08/10/05 E. Carter      Added MTU tags.
 *	10/26/05 R. Hicks		DIMS 126137: Added additional backdoor disk power
							commands (TAG_SOFT_BYPASS & TAG_SOFT_UNBYPASS)
 *	11/02/05 R. Hicks		Added TAG_ELEMENTS_PER_PARITY_STRIPE
 *	11/09/05 R. Hicks		DIMS 110808: Added TAG_QUEUE_FULL_BUSY_COUNTER
 *	12/07/05 R. Hicks		Added TAG_SFP_STATE
 *	12/20/05 K. Boland		Added TAG_K10_PORT_SPEED_REQUESTED
 *	01/17/05 E. Carter		Added tags for Bigfoot
 *	01/17/06 P. Caldwell		Added FastBind TAGS (TAG_PERCENT_ZEROED and TAG_ZERO_THROTTLE)
 *	01/17/06 P. Caldwell		Added TAG_DISK_PROACTIVE_COPY
 *	02/01/06 K. Boland  		Added TAG_FIX_LUN
 *	02/15/06 P. Caldwell		Added TAGs for LCC Stats (TAG_LCC_STATS)
 *	02/27/06 E. Carter		Added additional tags for Bigfoot
 *	07/07/06 G. Peterson		Added SC_TAG_INITIATOR_UID
 *	07/18/06 P. Caldwell		Added TAG_LIP_COUNT for LCC Stats
 *	07/26/06 P. Caldwell		Added TAG_PROACTIVE_SPARE for RAID6 support
 *	08/24/06 R. Hicks		Added additional tags for ALUA
*	10/16/06 P. Caldwell		Added TAG_SUBFRU_STATE
 *	10/26/06 P. Caldwell		Added VBus Statistics
 *          12/18/06 B. Goudreau	            Added TAG_POLL_* set.
  *         12/22/06 B. Goudreau	Moved tag definitions out to new file _Tags.h.
 *  02/19/08 D. Harvey      Added a couple of defintions for AAS/AAQ for PPME support.
***************************************************************************/

/*
 * LOCAL TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/include/global_ctl_tags.h,v 1.1 2000/02/09 18:05:29 fladm Exp $"
 */



//
//	First define some helper macros:
//

//
//	TAGVdefine is used for tags whose value is explicitly specified.
//
#undef TAGVdefine
#define TAGVdefine(TagName, TagValue) TagName = TagValue,

//
//	TAGdefine is used for tags whose value is implicitly determined by
//	their position within the "enum { ... }" statement.
//
#undef TAGdefine
#define TAGdefine(TagName) TagName,

//
//  Now get the files with all the tag names and values.  We have to split
//  the tags into two separate enums to avoid hitting a compiler limit, as
//  discussed in _Tags.h
//

enum K10_TAGS1 
{
#define _TAGS_PART1 1
#include "_Tags.h"
#undef _TAGS_PART1
};


enum K10_TAGS2 
{
#define _TAGS_PART2 1
#include "_Tags.h"
#undef _TAGS_PART2
};


#define MAX_TLD_NUMBER_SIZE         sizeof(long)

////////////////////////////////////////////////////////////////////
//
// Some non-TAG definitions that are also used by Navisphere
//
////////////////////////////////////////////////////////////////////


//
// These codes are used in an Advanced Array Setup/Query CDB's
// database ID field and are used by both the AEP and SC components
//
#define AEP_ADV_DB_ID_PHYSICAL       0
#define AEP_ADV_DB_ID_INITIATOR      1
#define AEP_ADV_DB_ID_VA             2

// Note: 3 is unused by flare although Navisphere uses it */

#define AEP_ADV_DB_ID_FEATURE        4  
#define AEP_ADV_DB_ID_LAYERED        5

// codes 6 - 7 are Storage-Centric unused

// codes 8 - 9 are used for PPME (Power Path Migration) [Invista only at present]
// Imported device from which the Virtual Volume is constructed
#define AEP_ADV_DB_ID_BACKEND        8

// Transition the PPME state of the LUN and the imported device
#define AEP_ADV_DB_ID_LUN_TRANSITION 9

// codes 10 - 15 are reserved by K10 

#define AEP_ADV_DB_ID_MEGAPOLL      16
#define AEP_ADV_DB_ID_FLNET         17
#define AEP_ADV_DB_ID_DISK_OBITUARY 18


/*
 * End $Id: global_ctl_tags.h,v 1.1 2000/02/09 18:05:29 fladm Exp $
 */

#endif
