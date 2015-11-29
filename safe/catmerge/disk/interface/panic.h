#ifndef PANIC_H
#define PANIC_H 0x00000001      /* from common dir */
#define FLARE__STAR__H

/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/**************************************************************
 *  $Id: panic.h,v 1.15.12.1 2000/10/13 14:23:35 fladm Exp $
 **************************************************************
 *
 *  FLARE Panic base codes.  Subsystems add positive offsets  
 *  to the panic base codes, to create actual panic codes.  All
 *  panic codes are to be 32 bits, and specified in HEX.  
 *  The base value are the 3 most significant digits.  
 *
 *  For example:
 *       BOGUS_PANIC_BASE     0x11100000
 *
 *  This means that each subsystem may have 1,048,575 different
 *  panics.  (This is the 5 least significant digits.)  
 *
 *  History:
 *     30-Jun-89:Sam Pendleton created this file.
 *     08-Jan-93 RER added BEM_PANIC_BASE
 *     17-Jan-94 DGM added HAT_PANIC_BASE
 *     25-Jan-94 DWD added ULOG panic base.
 *     08-Mar-95 JJN added FCF_PANIC_BASE and FCB_PANIC_BASE.
 *     13-Apr-95: Added TRO panic base.  DWD
 *     29-Sep-95 SML add FCLI panic
 *     05-Oct-01 CJH add ADM panic base
 *
 *      $Log: panic.h,v $
 *      Revision 1.15.12.1  2000/10/13 14:23:35  fladm
 *      User: rfoley
 *      Reason: MERGE
 *      Initial update to merge cat1 and the raidNT changes into
 *      the k10prep branch.
 *
 *      Revision 1.15.10.2  2000/10/04 18:32:21  fladm
 *      User: kschleicher
 *      Reason: Development
 *      Added Expansion Support.
 *      Bugs #567, 708, 566, 734
 *
 *      Revision 1.15.10.1  2000/07/21 17:03:59  fladm
 *      User: rfoley
 *      Reason: MERGE
 *      Merge from raidmm -> cat1.
 *
 *      Revision 1.10.4.1  2000/05/18 18:00:47  fladm
 *      User: gordon
 *      Reason: Crossbow_Development
 *      Merged the Crossbow XOR driver implementation into the
 *      RaidMM branch.
 *
 *      Revision 1.2  1999/01/26 21:11:37  fladm
 *      User: jjyoung
 *      Reason: 01.00.50
 *      Roll Hiraid_stage into tree
 *
 *      Revision 1.1  1999/01/05 22:28:33  fladm
 *      User: dibb
 *      Reason: initial
 *      Initial tree population
 *
 *      Revision 1.25  1998/11/23 19:14:37  hopkins
 *       Added FPP_PANIC_BASE  #define
 *
 *      Revision 1.24  1998/10/14 18:38:10  bruce
 *       Initial version for new Hiraid tree;  synch up with k2.2 tree.
 *
 *      Revision 1.23  1998/08/26 15:07:47  rfoley
 *       Added SHD_PANIC_BASE.
 *
 *      Revision 1.22  1998/06/12 19:10:44  bruce
 *       added DD_PANIC_BASE
 *
 *      Revision 1.21  1998/03/05 14:15:30  browlands
 *       added LG_PANIC_BASE
 *
 *      Revision 1.20  1998/02/19 15:09:49  dbeauregard
 *       Added panic bases for Virtual Drivers.
 *
 *      Revision 1.19  1998/01/08 21:22:32  ginga
 *      ADDED to PRODUCT hiraid from PRODUCT k2 on Wed Feb 18 12:57:10 EST 1998
 *       Added panic prototype
 *
 *      Revision 1.18  1997/12/19 19:55:46  kdobkows
 *       Updated to k5ph3_1_30_20. SM_UPLOAD_PANIC_BASE added.
 *
 *      Revision 1.17  1997/10/20 20:12:54  kdobkows
 *       Added EXEC_PANIC_BASE.
 *
 *      Revision 1.16  1997/08/08 21:43:08  bruss
 *       Added IPHASE_PANIC_BASE
 *
 *      Revision 1.15  1997/07/09 13:39:53  kdobkows
 *       added XOR_PANIC_BASE for K2
 *
 *      Revision 1.14  1997/01/21 16:03:35  lathrop
 *      ADDED to PRODUCT k2 from PRODUCT k5 on Tue Jul  8 15:55:34 EDT 1997
 *       add FC_PANIC_BASE
 *
 *      Revision 1.13  1996/10/15 19:00:44  gordon
 *       Added HW_PANIC_BASE
 *
 *      Revision 1.12  1996/05/30 16:21:44  sears
 *       removed ddc
 *
 *      Revision 1.11  1996/05/28 12:46:40  lathrop
 *       make fcli different from g-man panics
 *
 * Revision 1.10  1996/04/23  17:29:01  pilon
 *  added Fast Raid3 stuff
 *
 * Revision 1.8  1996/03/25  14:31:59  alec
 *  Added ATM panic base.
 *
 * Revision 1.7  1996/01/10  19:22:17  alec
 *  Added FEP_PANIC_BASE.
 *
 * Revision 1.6  1995/11/09  19:32:54  chughes
 *  Add base value for DMA Sequencer Panics
 *
 *
 **************************************************************/


/*
 * LOCAL TEXT unique_file_id[] = "$Header: /uddsw/fladm/repositories/project/CVS/flare/src/include/panic.h,v 1.2 1999/01/26 21:11:37 fladm Exp $";
 */

/* This mask can be used to determine what the base code is */
#define PANIC_BASE_MASK         0xFFF00000

#define HEMI_PANIC_BASE         0x00000000
#define MD_PANIC_BASE           0x00100000
#define HI_PANIC_BASE           0x00200000
#define TRACE_PANIC_BASE        0x00210000
#define SFE_PANIC_BASE          0x00220000
#define AEP_PANIC_BASE          0x00240000
#define ULOG_PANIC_BASE         0x00250000
#define GM_PANIC_BASE           0x00260000
#define FCLI_PANIC_BASE         0x00270000
#define DCA_PANIC_BASE          0x00280000
#define SJ_PANIC_BASE           0x002A0000
#define SC_PANIC_BASE           0x002B0000
#define BM_PANIC_BASE           0x00300000
#define SCSI_PANIC_BASE         0x00500000
#define MBUS_PANIC_BASE         0x00600000
#define RBLD_PANIC_BASE         0x00700000
#define CM_PANIC_BASE           0x00800000
#define PIO_PANIC_BASE          0x00900000
#define VME_PANIC_BASE          0x00A00000
#define FED_PANIC_BASE          0x00A00000
#define DIAG_PANIC_BASE         0x00B00000
#define VP_PANIC_BASE           0x00C00000
#define ZP_PANIC_BASE           0x00D00000
#define CMI_PANIC_BASE          0x00E00000
#define MHB_PANIC_BASE          0x00F00000
#define CHKPT_VERIFY_PANIC_BASE 0x00F10000
#define NONVOL_PANIC_BASE       0x00F20000
#define SYSX_PANIC_BASE         0x00F30000
#define BM_DEMON_PANIC_BASE     0x00F40000
#define SERIAL_MAN_PANIC_BASE   0x00F50000
#define DB_PANIC_BASE           0x00F60000	/* Database Panic Base */
#define LM_PANIC_BASE           0x00FF0000
#define TOD_PANIC_BASE          0x01000000
#define FCF_PANIC_BASE          0x01100000	/* Fibre Channel
	   * Front End Panic Base
	 */

#define FCB_PANIC_BASE          0x01200000	/* Fibre Channel
	   * Back End Panic Base
	 */

#define DMA_PANIC_BASE          0x01300000	/* DMA Sequencer
	   * Driver Panic Base
	 */

#define FEP_PANIC_BASE          0x01400000
#define ATM_PANIC_BASE          0x01500000
#define NTFE_PANIC_BASE         0x01600000
#define UTIL_PANIC_BASE         0x01700000	/* gen'l utility panic base */
#define HW_PANIC_BASE           0x01800000
#define FC_PANIC_BASE           0x01900000	/* gen'l Fibre panic base */
#define XOR_PANIC_BASE          0x01A00000
#define IPHASE_PANIC_BASE       0x01B00000	/* Interphase FE and BE */
#define EXEC_PANIC_BASE         0x01C00000
#define XD_PANIC_BASE           0x01D00000	/* Expansion Driver */
#define CA_PANIC_BASE           0x01E00000	/* Cache Driver */
#define CLD_PANIC_BASE		0x01F00000	/* Clean / Dirty Driver */
#define RAID_PANIC_BASE		0x02000000	/* RAID driver */
#define LG_PANIC_BASE		0x02100000	/* Log Driver */
#define SHD_PANIC_BASE		0x02200000	/* Shadow Driver */
#define FPP_PANIC_BASE		0x02300000	/* Fibre Channel peer - peer driver  */
#define DM_PANIC_BASE		0x02400000	/* Diskmail Panic Base */
#define NVCA_PANIC_BASE			0x02500000
#define BIND_DRIVER_PANIC_BASE  0x26000000  /* Bind Driver Panic Base */
#define AM_PANIC_BASE           0x02700000	/* Automode - obsolete */
#define EMM_PANIC_BASE          0x02700000	/* Extended Memory Manager */
#define XOR_DRV_PANIC_BASE      0x02800000	/* XOR Virtual Driver */

#define GEO_ABSTRACT_PANIC_BASE 0x03300000
#define PFSM_PANIC_BASE         0x03400000	/* PFSM Virtual Driver */
#define CPP_PANIC_BASE          0x03500000      /* Objectized C support */
#define ODBS_PANIC_BASE         0x03600000      /* ODB Service */
#define FLARE_DRIVER_PANIC_BASE 0x03700000      /* Flare Driver */
#define ADM_PANIC_BASE          0x03800000      /* ADM module */
#define LCC_DUMMY_PANIC_BASE    0x03900000      /* LCC Dummy. Used in simulation */
#define SERIAL_PANIC_BASE       0x03A00000      /* Module that interfaces with the serial ports */
#define SATA_PANIC_BASE         0x03B00000      /* Serial ATA Miniport Driver */
#define CPD_FCOE_API_PANIC_BASE 0x03B00000      /* DMD Miniport FCoE Driver */

#define DD_PANIC_BASE           0x04000000	/* DH Driver */
#define DBA_PANIC_BASE          0x04100000	 /* Database API panic base */
#define NTBE_PANIC_BASE         0x04200000       /* NTBE Driver */
#define TESTPROC_PANIC_BASE     0x04300000       /* Test proc */
#define GENERIC_PANIC_BASE      0x04400000       /* Generic Flare Driver */
#define REMAP_PANIC_BASE        0x04500000       /* Remap Process */
#define RLT_PANIC_BASE          0x04600000       /* Rebuild Log Thread (RLT) */

/*! @def DST_PANIC_BASE 
 *  @brief 
 * This is the base value for panic errors that are defined and used by
 * the Drive Standby Thread.
 */
#define DST_PANIC_BASE          0x04700000       /* Drive Standby Thread (DST) */

/*  The fbe (MCR) panic facility is unique and separate from the Flare/Hemi
 *  facility, and so defines its own panic base in fbe_panic_sp.h
 *  Note that to assist in triage, an attempt was made to have a base that 
 *  was still unique for fbe panic. We define it here as well as feb_panic_sp.h
 *  to assist triage and prevent future panic base collisions.
 */

#define FBE_PANIC_SP_BASE          0x06000000  

/*
**  Note that Mirror driver begins its offsets at 0x05010000. It has its own 
**  panic facility and does not include this panic.h file.
**  See mirror_panic.h
**
**  The FBE logical environment, which is part of the Flare driver as of 2008,
**  is using a base of 0x06000000 for its offsets.
**  Note that FBE uses its own panic facility, yet maintains a unique value for
**  any triage. It follows the Mirror driver.
**  See fbe_panic.h
*/

#define IDM_PANIC_BASE          0x08200000  /* Module that interfaces with IDM drver */
#define FBE_SHIM_UPPER_DH_PANIC_BASE 0x08300000 /* FBE Shim Upper DH PANIC codes (temporary) */
#define AEN_PANIC_BASE          0x08400000
#define EFUP_PANIC_BASE         0x08500000       /* Enclosure Firmware Upgrade (EFUP) */

/*
 * Prototypes
 */

#ifndef K10_ENV
IMPORT void panic(UINT_32 who, UINT_32 why);
#endif

/*
 * End $Id: panic.h,v 1.15.12.1 2000/10/13 14:23:35 fladm Exp $
 */

#endif /* ndef PANIC_H */
