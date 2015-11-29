/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *  vendorDeviceID.h
 ****************************************************************
 *
 *  Description:
 *  This header file contains the vendor and device IDs of various
 *  PCI components used throughout the system. 
 *
 *  History:
 *      Jan-07-2008 . Phil Leef - Created
 *
 ****************************************************************/

#ifndef VENDOR_DEVICE_ID_H
#define VENDOR_DEVICE_ID_H

/* Vendor IDs */
#define VEN_ID_EMC_CORPORATION                      0x1120
#define VEN_ID_EMC_CORPORATION_USB                  0x1B7F
#define VEN_ID_TEXASINSTRUMENTS_USB                 0x0451
#define VEN_ID_AGILENT_TECHNOLOGIES                 0x15BC
#define VEN_ID_QLOGIC_CORPORATION                   0x1077
#define VEN_ID_HEWLETT_PACKARD_COMPANY              0x103C
#define VEN_ID_PMC_SIERRA                           0x11F8
#define VEN_ID_BROADCOM_CORPORATION                 0x14E4
#define VEN_ID_LSI_CORPORATION                      0x1000
#define VEN_ID_PLX                                  0x10B5
#define VEN_ID_INTEL                                0x8086
#define VEN_ID_EMULEX                               0x10DF
#define VEN_ID_VMWARE                               0x15AD
#define VEN_ID_RH                                   0x1af4

/* Hewlett Packard Device IDs */
#define DEV_ID_TACHYON_TL                           0x1028
#define DEV_ID_TACHYON_XL2                          0x1029
#define DEV_ID_TACHYON_TS                           0x102A

/* Agilent (formally HP) Device IDs */ 
#define DEV_ID_TACHYON_DX2                          0x0100
#define DEV_ID_TACHYON_QX4                          0x0103
#define DEV_ID_TACHYON_QE4                          0x1200
#define DEV_ID_TACHYON_DE4                          0x1203

/* PMC-Sierra (formlery Agilent) Device IDs */
#define DEV_ID_TACHYON_SPC_PM8001                   0x8001
#define DEV_ID_TACHYON_SPC_PM8008                   0x8008
#define DEV_ID_TACHYON_SPC_PM8009                   0x8009
#define DEV_ID_TACHYON_SPC_PM8018                   0x8018
#define DEV_ID_TACHYON_SPC_PM8019                   0x8019
#define DEV_ID_TACHYON_QE8                          0x8032
#define DEV_ID_TACHYON_C6                           0x8036
#define DEV_ID_SPC_PM8071                           0x8071
#define DEV_ID_SPC_PM8073                           0x8073

/* Qlogic Device IDs */
#define DEV_ID_22                                   0x4022
#define DEV_ID_32_NIC                               0xB032
#define DEV_ID_32                                   0xC032
#define DEV_ID_EP8112_NIC                           0x8800
#define DEV_ID_EP8112_FCOE                          0x8801
#define DEV_ID_8324                                 0x2831
#define DEV_ID_EP8324_NIC                           0x8830
#define DEV_ID_EP8324_FCOE                          0x8831
#define DEV_ID_EP8324_ISCSI                         0x8832

/* Broadcom Device IDs */
#define DEV_ID_NETXTREME_II                         0x164C
#define DEV_ID_NETXTREME_57710                      0x164E
#define DEV_ID_NETXTREME_57711                      0x164F
#define DEV_ID_NETXTREME_57712                      0x1662
#define DEV_ID_NETXTREME_5709                       0x1639
#define DEV_ID_5719                                 0x1657
#define DEV_ID_NETXTREME_57840_4_10                 0x16A1

/* LSI Device IDs */
#define DEV_ID_SAS1068E                             0x0058
#define DEV_ID_LSISAS2008                           0x0072

/* EMC Device IDs */
#define DEV_ID_SUBSYSTEM_POSEIDON_OP                0x4F70
#define DEV_ID_SUBSYSTEM_POSEIDON_OP_REPLAY_FIX     0x4375

    /* These match the FFID of the SLIC */
#define DEV_ID_SUBSYSTEM_SUPERCELL                  0x0520
#define DEV_ID_SUBSYSTEM_POSEIDON_II                0x0528
#define DEV_ID_SUBSYSTEM_ELNINO                     0x052E
#define DEV_ID_SUBSYSTEM_ERUPTION                   0x0530
#define DEV_ID_SUBSYSTEM_ERUPTION_84833             0x0531
#define DEV_ID_SUBSYSTEM_VORTEX16                   0x0535
#define DEV_ID_SUBSYSTEM_ERUPTION_REV_D             0x0543
#define DEV_ID_SUBSYSTEM_ELNINO_REV_B               0x0544
#define DEV_ID_SUBSYSTEM_VORTEX16Q                  0x0546
#define DEV_ID_SUBSYSTEM_MAELSTROM                  0x0565
#define DEV_ID_SUBSYSTEM_eERUPTION                  0x1410
#define DEV_ID_SUBSYSTEM_eLANDSLIDE                 0x1414
#define DEV_ID_SUBSYSTEM_10G_ON_BEACHCOMBER         0x1904
#define DEV_ID_SUBSYSTEM_HILDA_BEARCAT              0x190A
#define DEV_ID_SUBSYSTEM_ROCKSLIDE_X                0x2305
#define DEV_ID_SUBSYSTEM_VORTEXQ_X                  0x2306
#define DEV_ID_SUBSYSTEM_THUNDERHEAD_X              0x2307
#define DEV_ID_SUBSYSTEM_DOWNBURST_X                0x2309
#define DEV_ID_SUBSYSTEM_MAELSTROM_X                0x230A
#define DEV_ID_SUBSYSTEM_DOWNBURST_X_RDMA           0x230C
#define DEV_ID_SUBSYSTEM_TWINVILLE_OBERON           0x2501
#define DEV_ID_SUBSYSTEM_HILDA_OBERON_FC            0x2505
#define DEV_ID_SUBSYSTEM_HILDA_OBERON_NIC           0x2501

/* EMC USB PIDs */
#define DEV_ID_TOMAHAWK_USB_PID                     0x0001
#define DEV_ID_COROMANDEL_USB_PID                   0x0002
#define DEV_ID_SPARTAN_USB_PID                      0x0003
#define DEV_ID_GLACIER_USB_PID                      0x0004
#define DEV_ID_MUZZLE_USB_PID                       0x0005
#define DEV_ID_PEACEMAKER_USB_PID                   0x0006

/* TI Device IDs */
#define DEV_ID_TI3410                               0x3410

/* Intel iSCSI Device IDs */
#define DEV_ID_INTEL_E1000                          0x100F
#define DEV_ID_INTEL_E1000_100E                     0x100E
#define DEV_ID_INTEL_X540                           0x1528


/* Intel MB Device IDs */
#define DEV_ID_LPC_PCH                              0x1D41
#define DEV_ID_LPC_WELLSBURG_PCH                    0x8D44

#define DEV_ID_CPU_MMCFG_REG_SB                     0x3C28
#define DEV_ID_CPU_MMCFG_REG_IVB                    0x0E28
#define DEV_ID_CPU_MMCFG_REG_HW                     0x2F28

/* Emulex Device IDs */
#define DEV_ID_SKYHAWK_NIC                          0x0720
#define DEV_ID_SKYHAWK_ISCSI                        0x0723
#define DEV_ID_SKYHAWK_FCOE                         0x0724

/* VMware Device IDs */
#define DEV_ID_VMWARE_VMXNET3                       0x07B0

/* RED HAT Device IDs */
#define DEV_ID_RH_VIRTIO                            0x1000

#endif //VENDOR_DEVICE_ID_H
