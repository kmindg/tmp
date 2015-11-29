#ifndef SPEEDS_H
#define SPEEDS_H
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

/*******************************************************************************
 * speeds.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications.
 *
 ******************************************************************************/

// The following are valid values for HOST_PORT_PARAM_FIBRE.RequestedLinkSpeed
// and HOST_PORT_PARAM_ISCSI.RequestedIpSpeed
#define SPEED_HARDWARE_DEFAULT   0x00000000 // binary equivalent is: 0000 0000 0000 0000

/* ********************************************************************************
**
**   *** THESE BIT DEFINITION VALUES MUST NEVER BE CHANGED.  ***
**
**   THE VALUES ARE USE BY NAVI USING AN IDENTICAL SET OF DEFINES IN ANOTHER FILE.
**   CHANGES WOULD RESULT IN INCOMPATABILITY WITH NAVI.
**
** ********************************************************************************/

#define SPEED_HARDWARE_DEFAULT  0x00000000 // binary equivalent is: 0000 0000 0000 0000
#define SPEED_ONE_GIGABIT       0x00000001 // binary equivalent is: 0000 0000 0000 0001
#define SPEED_TWO_GIGABIT       0x00000002 // binary equivalent is: 0000 0000 0000 0010
#define SPEED_FOUR_GIGABIT      0x00000004 // binary equivalent is: 0000 0000 0000 0100
#define SPEED_EIGHT_GIGABIT     0x00000008 // binary equivalent is: 0000 0000 0000 1000
#define SPEED_TEN_GIGABIT       0x00000010 // binary equivalent is: 0000 0000 0001 0000

#define SPEED_ONE_FIVE_GIGABIT  0x00000020 // binary equivalent is: 0000 0000 0010 0000
#define SPEED_THREE_GIGABIT     0x00000040 // binary equivalent is: 0000 0000 0100 0000
#define SPEED_SIX_GIGABIT       0x00000080 // binary equivalent is: 0000 0000 1000 0000
#define SPEED_TWELVE_GIGABIT    0x00000100 // binary equivalent is: 0000 0001 0000 0000

#define SPEED_TEN_MEGABIT       0x00001000 // binary equivalent is: 0001 0000 0000 0000
#define SPEED_100_MEGABIT       0x00002000 // binary equivalent is: 0010 0000 0000 0000

#define SPEED_SIXTEEN_GIGABIT   0x00004000 // binary equivalent is: 0100 0000 0000 0000
#define SPEED_AUTO_SELECT       0x00008000 // binary equivalent is: 1000 0000 0000 0000
#define SPEED_UNCHANGED         0x80000000

#define SPEED_UNKNOWN           SPEED_UNCHANGED


// The following are valid values for the current IP speed and the available IP
// speeds as reported by the AAQ SCSI command when generating the port report.

#define AAQ_ISCSI_HARDWARE_DEFAULT   0x0000 // binary equivalent is: 00000000000000000
#define AAQ_ISCSI_SPEED_10_GBPS      0x0008 // binary equivalent is: 00000000000001000
#define AAQ_ISCSI_SPEED_1_GBPS       0x0010 // binary equivalent is: 00000000000010000
#define AAQ_ISCSI_SPEED_100_MBPS     0x0020 // binary equivalent is: 00000000000100000
#define AAQ_ISCSI_SPEED_10_MBPS      0x0040 // binary equivalent is: 00000000001000000
#define AAQ_ISCSI_SPEED_AUTO_DETECT  0x0080 // binary equivalent is: 00000000010000000
#define AAQ_ISCSI_SPEED_10_100_MBPS  0x0100 // binary equivalent is: 00000000100000000


// The following are valid values for HOST_PORT_PARAM_ISCSI.RequestedFlowControl

#define FLOW_CONTROL_DISABLED       0x0001 // binary equivalent is: 0000 0001
#define FLOW_CONTROL_AUTO           0x0002 // binary equivalent is: 0000 0010
#define FLOW_CONTROL_ENABLED        0x0004 // binary equivalent is: 0000 0100
#define FLOW_CONTROL_RX_ENABLED     0x0008 // binary equivalent is: 0000 1000
#define FLOW_CONTROL_TX_ENABLED     0x0010 // binary equivalent is: 0001 0000


// EDC MODES: The following are valid values for HOST_PORT_PARAM_ISCSI.RequestedEdcMode

#define EDC_MODE_UNKNOWN            0x0000
#define EDC_MODE_LIMITING           0x0001
#define EDC_MODE_LINEAR             0x0002

// The following are used for PHY Mapping and Polarity

#define PHY_POLARITY_MAP_SHIFT 16
#define PHY_MAP_MASK           0xffff

#endif // SPEEDS_H
