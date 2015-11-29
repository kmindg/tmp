#ifndef MODEL_NUMBERS_H
#define MODEL_NUMBERS_H 0x00000001	/* from common dir */

/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2004, 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 * $Id: model_numbers.h,v 1.1.2.1 1999/06/01 18:48:09 fladm Exp $
 ****************************************************************
 *
 *  Description:
 *  This header file contains definitions for the EMC model
 *  numbers of the various controllers we support.
 *
 *  Notes:
 *  Any physical controller board which is unique needs to be
 *  identified by a unique model number.  For example, even
 *  though a Baby Sauna II is *almost* the same as a normal
 *  Sauna II, it requires its own model number because the two
 *  boards are NOT interchangeable in a customer system.
 *
 *  Any new controller which we support needs to have a model
 *  number added here.  In addition, SPID needs to be updated.
 *  
 *
 *  History:
 *  05-25-95: DGM -- extracted from ai_controller.h files.
 *  04-16-97: CNL -- Made K5_MODEL_NUM a real number.
 *
 ****************************************************************/

/*
 * static TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/include/model_numbers.h,v 1.1.2.1 1999/06/01 18:48:09 fladm Exp $";
 */

/***********************************************************************
 * IMPORTANT  !!  IMPORTANT  !!  IMPORTANT  !!  IMPORTANT  !!  IMPORTANT
 *
 * This file contains global data that is useful to other projects
 * outside of Flare.  The values defined here represent an interface
 * to the outside world and are shared by the other groups.  As a result
 * a few rules apply when modifying this file.
 *
 *  1 - Do not delete (ever) a value defined in this file
 *
 *  2 - Only #define's of new values are allowed in this file
 *
 *  3 - Only add values of like data (error code, id value, etc.)
 *      to this file.
 *
 *  4 - Try to understand how values and ranges are being allocated
 *      before adding new entries to ensure they are added in the 
 *      correct area.
 *
 *  5 - If you have any doubts consult project or group leaders
 *      before making a permanent addition.
 *
 ************************************************************************/

// All ASCII Model number strings should no longer than this length.
#define K10_ASCII_MODEL_NUMBER_MAX_SIZE         0x10

/************************/
/* Legacy Model Numbers */
/************************/

/* Alpine
 * Used in:   Hornet's Nest DPE enclosure
 * Processor: PPC7400
 * Info:
 */
#define ALPINE_MODEL_NUM                    4400

/* K1
 * Used in:   Hornet's Nest DAE enclosure
 * Processor: PPC603?
 * Info:      This is an LCC card on steroids SP with a single memory model.
 */
#define K1_MODEL_NUM                        5200

/* K5
 * Used in:   Hornet's Nest DPE enclosure
 * Processor: PPC603? and PPC603?
 * Info:      This is a Power PC based SP with a split memory model
 *            and Fibre front and back ends.
 */
#define K5_MODEL_NUM                        5400

/* K2
 * Used in:   Hornet's Nest DPE enclosure
 * Processor: PPC750 and PPC603?
 * Info:      This is a scaled down K5 SP with a single memory model.
 */
#define K2_MODEL_NUM                        5600

/* K2 -
 * Used in:   Hornet's Nest DPE enclosure
 * Processor: PPC750 and PPC603?
 * Info:      This is a scaled down K2 SP with a 30 drive limit.
 */
#define K2_CAP_30_MODEL_NUM                 5603

/*
 * Crossbow
 * Used in:   Hornet's Nest DPE enclosure
 * Processor: 
 * Info:      This is a Power PC based SP with a ASIC-based DMA hardware XOR
 */
#define CROSSBOW_MODEL_NUM                  5900

/* BABY AMDAS Sauna II
 * Used in:   Bull's AMDAS chassis
 * Processor: 29000
 * Info:      This is a depopulated Sauna II with a different
 *            voltage regulator and no air dam.
 */
#define BABY_AMDAS_MODEL_NUM                7300

/* B.HIVE
 * Used in:   Fat Elmo chassis
 * Processor: 29030/040
 * Info:      This controller was cancelled.
 */
#define HIVE_MODEL_NUM                      7305

/* NUC
 * Used in:   7-UP chassis
 * Processor: 29030
 * Info:      This is sort of a depopulated B.HIVE with a PCMCIA
 *            card slot and a different form factor.
 */
#define NUC_MODEL_NUM                       7310

/* AMDAS Sauna II
 * Used in:   Bull's AMDAS chassis
 * Processor: 29000
 * Info:      This is a Sauna II with a different voltage regulator
 *            and no air dam.
 */
#define AMDAS_MODEL_NUM                     7340

/* BABY SAUNA II
 * Used in:   Baby Elmo chassis
 * Processor: 29000
 * Info:      This is a depopulated Sauna II with a different
 *            voltage regulator.
 */
#define BABY_SAUNA_MODEL_NUM                7341

/* BABY B.HIVE
 * Used in:   Fat Elmo chassis
 * Processor: 29030/040
 * Info:      This controller was cancelled.
 */
#define BABY_HIVE_MODEL_NUM                 7345

/* SAUNA I
 * Used in:   Fat Elmo chassis
 * Processor: 29000
 * Info:      Original controller board.
 */
#define SAUNA_I_MODEL_NUM                   7427

/* SAUNA II
 * Used in:   Fat Elmo chassis
 * Processor: 29000
 * Info:      Has SIMM memory and Wide SCSI interface.
 */
#define SAUNA_II_MODEL_NUM                  7624

/***************************/
/* FC Series Model Numbers */
/***************************/

/* Longbow
 * Used in:   K10 DAE enclosure
 * Processor: Intel XEON
 * Info:      This is a Wintel 'PC' custom motherboard SP with 
 *            a single memory model. Fiber front and back.
 */
#define K10_MODEL_NUM                       4700
#define K10_MODEL_NUM_ASCII                 "FC4700"

/* Longbow w/2Gb FC
 * Used in:   K10 DAE enclosure
 * Processor: Intel XEON
 * Info:      This is a Wintel 'PC' custom motherboard SP with 
 *            a single memory model. 2Gb fibre front and back end busses.
 */
#define K10_MODEL_NUM_2                     4702
#define K10_MODEL_NUM_2_ASCII               "FC4700-2"

/***************************/
/* CX Series Model Numbers */
/***************************/

/* X1 Lite
 * Used in:   LCC enclosure
 * Processor: Single 800 MHz P4
 * Info:      Third generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre front and back end busses.  512M memory.
 *            Single FE port using HUB.
 */
#define X1_LITE_MODEL_NUM                   200
#define X1_LITE_MODEL_NUM_ASCII             "CX200"

/* X1 Lite Lowcost
 * Used in:   LCC enclosure
 * Processor: Single 800 MHz P4
 * Info:      Third generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre front and back end busses.  512M memory.
 *            Single FE port using HUB.
 */
#define X1_LITE_LOW_COST_MODEL_NUM          200
#define X1_LITE_LOW_COST_MODEL_NUM_ASCII    "CX200LC"

/* Snapper
 * Used in:   LCC enclosure
 * Processor: Single 800 MHz P4
 * Info:      Third generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre front bus and back end bus.  512M memory.
 *            Single FE port using HUB.  Single BE port.
 */
#define SNAPPER_MODEL_NUM                   300
#define SNAPPER_MODEL_NUM_ASCII             "CX300"

/* Freshfish
 * Used in:   LCC enclosure
 * Processor: Single 2 GHz P4
 * Info:      Fourth generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre back end bus, 1Gb ISCSI front end, 1GB memory.
 */
#define FRESHFISH_MODEL_NUM                 300
#define FRESHFISH_MODEL_NUM_ASCII           "CX300i"

/* X1 "SUN" Lite
 * Used in:   LCC enclosure
 * Processor: Single 800 MHz P4
 * Info:      Third generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre front and back end busses. Artificially limited
 *			  X1 configuration.
 */
#define X1_SUN_LITE_MODEL_NUM               305
#define X1_SUN_LITE_MODEL_NUM_ASCII         "CX305"

/* JACKHAMMER (fibre)
 * Used in:   XPE3 enclosure
 * Processor: Single 2.8 GHz P4
 * Info:      2 [4/2/1]Gb FE ports, 1 [4/2]Gb BE ports.
 */
#define JACKHAMMER_MODEL_NUM                3020
#define JACKHAMMER_MODEL_NUM_ASCII          "CX3-20"

/* JACKHAMMER (iSCSI)
 * Used in:   XPE3 enclosure
 * Processor: Single 2.8 GHz P4
 * Info:      4 iSCSI FE ports, 1 [4/2]Gb BE ports.
 */
#define JACKHAMMER_ISCSI_MODEL_NUM          3020
#define JACKHAMMER_ISCSI_MODEL_NUM_ASCII    "CX3-20i"

/* JACKHAMMER (combo)
 * Used in:   XPE3 enclosure
 * Processor: Single 2.8 GHz P4
 * Info:      2 FC FE ports, 4 iSCSI FE ports, 1 FC BE ports.
 */
#define JACKHAMMER_COMBO_MODEL_NUM          3020
#define JACKHAMMER_COMBO_MODEL_NUM_ASCII    "CX3-20c"

/* JACKHAMMER (headhunter)
 * Used in:   XPE3 enclosure
 * Processor: Single 2.8 GHz P4
 * Info:      2 native and 4 expansion FE ports, 1 FC BE ports.
 */
#define JACKHAMMER_HEADHUNTER_MODEL_NUM         3020
#define JACKHAMMER_HEADHUNTER_MODEL_NUM_ASCII   "CX3-20f"

/* TACKHAMMER (fibre)
 * Used in:   XPE3 enclosure
 * Processor: Single 2.8 GHz P4 (no hyperthreading)
 * Info:      2 [4/2/1]Gb FE ports, 1 [4/2]Gb BE ports.
 */
#define TACKHAMMER_MODEL_NUM                    3010
#define TACKHAMMER_MODEL_NUM_ASCII              "CX3-10"

/* TACKHAMMER (iSCSI)
 * Used in:   XPE3 enclosure
 * Processor: Single 2.8 GHz P4 (no hyperthreading)
 * Info:      2 iSCSI FE ports, 1 [4/2]Gb BE ports.
 */
#define TACKHAMMER_ISCSI_MODEL_NUM              3010
#define TACKHAMMER_ISCSI_MODEL_NUM_ASCII        "CX3-10i"

/* TACKHAMMER (combo)
 * Used in:   XPE3 enclosure
 * Processor: Single 2.8 GHz P4 (no hyperthreading)
 * Info:      2 [4/2/1]Gb FE ports, 2 iSCSI FE ports, 1 [4/2]Gb BE ports.
 */
#define TACKHAMMER_COMBO_MODEL_NUM              3010
#define TACKHAMMER_COMBO_MODEL_NUM_ASCII        "CX3-10c"

/* TACKHAMMER (headhunter)
 * Used in:   XPE3 enclosure
 * Processor: Single 2.8 GHz P4 (no hyperthreading)
 * Info:      2 native and 4 expansion FE ports, 1 [4/2]Gb BE ports.
 */
#define TACKHAMMER_HEADHUNTER_MODEL_NUM         3010
#define TACKHAMMER_HEADHUNTER_MODEL_NUM_ASCII   "CX3-10f"

/* X1
 * Used in:   LCC enclosure
 * Processor: Single 800 MHz P4
 * Info:      Third generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre front and back end busses.
 */
#define X1_MODEL_NUM                            400
#define X1_MODEL_NUM_ASCII                      "CX400"

/* TARPON
 * Used in:   LCC enclosure
 * Processor: Dual 2 GHz P4
 * Info:      Fourth generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre front and back end busses.
 */
#define TARPON_MODEL_NUM                        500
#define TARPON_MODEL_NUM_ASCII                  "CX500"

/* TARPON
 * Used in:   LCC enclosure
 * Processor: Dual 2 GHz P4
 * Info:      Fourth generation Wintel architecture platform.  Single memory
 *            model, 1Gb ISCSI front end, 2Gb fibre back end busses.
 */
#define TARPON_ISCSI_MODEL_NUM                  500
#define TARPON_ISCSI_MODEL_NUM_ASCII            "CX500i"

/* SLEDGEHAMMER (fibre)
 * Used in:   XPE3 enclosure
 * Processor: Dual 2 GHz P4
 * Info:      2 [4/2/1]Gb FE ports, 2 [4/2]Gb BE ports.
 */
#define SLEDGEHAMMER_MODEL_NUM                  3040
#define SLEDGEHAMMER_MODEL_NUM_ASCII            "CX3-40"

/* SLEDGEHAMMER (iSCSI)
 * Used in:   XPE3 enclosure
 * Processor: Dual 2 GHz P4
 * Info:      4 iSCSI FE ports, 2 [4/2]Gb BE ports.
 */
#define SLEDGEHAMMER_ISCSI_MODEL_NUM            3040
#define SLEDGEHAMMER_ISCSI_MODEL_NUM_ASCII      "CX3-40i"

/* SLEDGEHAMMER (combo)
 * Used in:   XPE3 enclosure
 * Processor: Dual 2 GHz P4
 * Info:      4 iSCSI FE ports, 2 FC FE ports, 2 FC BE ports.
 */
#define SLEDGEHAMMER_COMBO_MODEL_NUM            3040
#define SLEDGEHAMMER_COMBO_MODEL_NUM_ASCII      "CX3-40c"

/* SLEDGEHAMMER (headhunter)
 * Used in:   XPE3 enclosure
 * Processor: Dual 2 GHz P4
 * Info:      2 native and 2 expansion FC FE ports, 4 FC BE ports.
 */
#define SLEDGEHAMMER_HEADHUNTER_MODEL_NUM       3040
#define SLEDGEHAMMER_HEADHUNTER_MODEL_NUM_ASCII "CX3-40f"

/* Chameleon 2
 * Used in:   XPE enclosure
 * Processor: Dual 2.0 GHz P4 Xeon 400 FSB
 * Info:      Second generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre front and back end busses.
 */
#define CHAM_2_MODEL_NUM                        600
#define CHAM_2_MODEL_NUM_ASCII                  "CX600"

/* Hammerhead
 * Used in:   XPE2 enclosure
 * Processor: Dual 3.06 GHz Xeon 533 FSB
 * Info:      4 [4/2/1]Gb FE ports, 4 [4/2]Gb BE ports.
 */

#define HAMMERHEAD_MODEL_NUM                    3080
#define HAMMERHEAD_MODEL_NUM_ASCII              "CX3-80"

/* Barracuda
 * Used in:   XPE enclosure
 * Processor: Dual 3.06 GHz Xeon 533 FSB
 * Info:      Second generation Wintel architecture platform.  Single memory
 *            model, 2Gb fibre front and back end busses.
 */
#define BARRACUDA_MODEL_NUM                     700
#define BARRACUDA_MODEL_NUM_ASCII               "CX700"

/* THESE ARE JUST HERE TO SATISFY COMPILATION */

/* Dreadnought
 * Used in:   XPE3 enclosure
 * Processor: Dual-core 3.06 GHz
 * Info:      Hammer Kicker Platform - 6 I/O Modules
 */

#define DREADNOUGHT_MODEL_NUM                   4960
#define DREADNOUGHT_MODEL_NUM_ASCII             "CX4-960"

/* Trident
 * Used in:   XPE4 enclosure
 * Processor: Dual-core 2.2 GHz
 * Info:      Hammer Kicker Platform - 5 I/O Modules
 */

#define TRIDENT_MODEL_NUM                       4480
#define TRIDENT_MODEL_NUM_ASCII                 "CX4-480"

/* IronClad
 * Used in:   XPE4 enclosure
 * Processor: Dual-core 2.2 GHz
 * Info:      Hammer Kicker Platform - 5 I/O Modules
 */

#define IRONCLAD_MODEL_NUM                      4240
#define IRONCLAD_MODEL_NUM_ASCII                "CX4-240"

/* Nautilus
 * Used in:   XPE4 enclosure
 * Processor: Dual-core 2.2 GHz
 * Info:      Hammer Kicker Platform - 3 I/O Modules
 */

#define NAUTILUS_MODEL_NUM                      4120
#define NAUTILUS_MODEL_NUM_ASCII                "CX4-120"

/* Zodiac
 * Used in:   XPE4 enclosure
 * Processor: Uni-core 2.4 GHz
 * Info:      CX SAS-based Platform - 2 I/O Modules
 */

#define ZODIAC_MODEL_NUM                        4130
#define ZODIAC_MODEL_NUM_ASCII                  "CX4-130"

/* Corsair
 * Used in:   XPE3 enclosure
 * Processor: Dual-core 3.06 GHz
 * Info:      Armada Platform - 6 I/O Modules
 */
#define CORSAIR_MODEL_NUM				    5960
#define CORSAIR_MODEL_NUM_ASCII			    "CX5-960"

/* Mustang
 * Used in:   XPE4 enclosure
 * Processor: Dual-core 2.2 GHz
 * Info:      Armada Platform - 5 I/O Modules
 */
#define MUSTANG_MODEL_NUM                   7500
#define MUSTANG_MODEL_NUM_ASCII             "VNX7500"
#define MUSTANG_PLATFORM_NAME_ASCII         "Mustang"
#define MUSTANGXM_PLATFORM_NAME_ASCII       "Mustang_XM"

/* SpitFire
 * Used in:   XPE4 enclosure
 * Processor: Dual-core 2.2 GHz
 * Info:      Armada Platform - 5 I/O Modules
 */
#define SPITFIRE_MODEL_NUM                  5700
#define SPITFIRE_MODEL_NUM_ASCII            "VNX5700"
#define SPITFIRE_PLATFORM_NAME_ASCII        "Spitfire"

/* Lightning
 * Used in:   XPE4 enclosure
 * Processor: Dual-core 2.2 GHz
 * Info:      Armada Platform - 2 I/O Modules
 */
#define LIGHTNING_MODEL_NUM                 5500
#define LIGHTNING_MODEL_NUM_ASCII           "VNX5500"
#define LIGHTNING_PLATFORM_NAME_ASCII       "Lightning"

/* Hellcat
 * Used in:   XPE4 enclosure
 * Processor: Dual-core 2.2 GHz
 * Info:      Armada Platform - 2 I/O Modules
 */
#define HELLCAT_MODEL_NUM                   5300
#define HELLCAT_MODEL_NUM_ASCII             "VNX5300"
#define HELLCAT_PLATFORM_NAME_ASCII         "Hellcat"
#define HELLCATXM_PLATFORM_NAME_ASCII       "Hellcat_XM"

/* Hellcat Lite
 * Used in:   XPE4 enclosure
 * Processor: Dual-core 1.6 GHz
 * Info:      Armada Platform - 2 I/O Modules
 */
#define HELLCAT_LITE_MODEL_NUM              5100
#define HELLCAT_LITE_MODEL_NUM_ASCII        "VNX5100"
#define HELLCAT_LITE_PLATFORM_NAME_ASCII    "Hellcat_Lite"


/* Prometheus M1
 * Info:      Transformers Platform - 11 I/O Modules
 */
#define PROMETHEUS_M1_MODEL_NUM            8000
#ifdef C4_INTEGRATED // Prometheus
#define PROMETHEUS_M1_MODEL_NUM_ASCII      "VNXe8000"
#else // C4_INTEGRATED Prometheus
#define PROMETHEUS_M1_MODEL_NUM_ASCII      "VNX8000"
#endif // C4_INTEGRATED Prometheus
#define PROMETHEUS_M1_PLATFORM_NAME_ASCII  "Prometheus_M1"

/* Prometheus S1
 * Info:      Transformers Platform - 11 I/O Modules    Simulator
 */
#define PROMETHEUS_S1_MODEL_NUM            8000
#define PROMETHEUS_S1_MODEL_NUM_ASCII      "VNX8000"
#define PROMETHEUS_S1_PLATFORM_NAME_ASCII  "Prometheus_S1"

//KHP_TODO AR564350  Revisit model numbers when final configs are available

/* Defiant M1
 * Info:      Transformers Platform - 5 I/O Modules
 */
#define DEFIANT_M1_MODEL_NUM               7600
#ifdef C4_INTEGRATED // Defiant
#define DEFIANT_M1_MODEL_NUM_ASCII         "VNXe7600"
#else // C4_INTEGRATED Defiant
#define DEFIANT_M1_MODEL_NUM_ASCII         "VNX7600"
#endif // C4_INTEGRATED Defiant
#define DEFIANT_M1_PLATFORM_NAME_ASCII     "Defiant_M1"

/* Defiant M2
 * Info:      Transformers Platform - 5 I/O Modules
 */
#define DEFIANT_M2_MODEL_NUM               5800
#ifdef C4_INTEGRATED // Defiant
#define DEFIANT_M2_MODEL_NUM_ASCII         "VNXe5800"
#else // C4_INTEGRATED Defiant
#define DEFIANT_M2_MODEL_NUM_ASCII         "VNX5800"
#endif // C4_INTEGRATED Defiant
#define DEFIANT_M2_PLATFORM_NAME_ASCII     "Defiant_M2"

/* Defiant M3
 * Info:      Transformers Platform - 5 I/O Modules
 */
#define DEFIANT_M3_MODEL_NUM               5600
#ifdef C4_INTEGRATED // Defiant
#define DEFIANT_M3_MODEL_NUM_ASCII         "VNXe5600"
#else // C4_INTEGRATED Defiant
#define DEFIANT_M3_MODEL_NUM_ASCII         "VNX5600"
#endif // C4_INTEGRATED Defiant
#define DEFIANT_M3_PLATFORM_NAME_ASCII     "Defiant_M3"

/* Defiant M4
 * Info:      Transformers Platform - 5 I/O Modules
 */
#define DEFIANT_M4_MODEL_NUM               5400
#ifdef C4_INTEGRATED // Defiant
#define DEFIANT_M4_MODEL_NUM_ASCII         "VNXe5400"
#else // C4_INTEGRATED Defiant
#define DEFIANT_M4_MODEL_NUM_ASCII         "VNX5400"
#endif // C4_INTEGRATED Defiant
#define DEFIANT_M4_PLATFORM_NAME_ASCII     "Defiant_M4"

/* Defiant M5
 * Info:      Transformers Platform - 5 I/O Modules
 */
#define DEFIANT_M5_MODEL_NUM               5200
#ifdef C4_INTEGRATED // Defiant
#define DEFIANT_M5_MODEL_NUM_ASCII         "VNX5200"
#else // C4_INTEGRATED Defiant
#define DEFIANT_M5_MODEL_NUM_ASCII         "VNXe5200"
#endif // C4_INTEGRATED Defiant
#define DEFIANT_M5_PLATFORM_NAME_ASCII     "Defiant_M5"

/* Defiant K2
 * Info:      Transformers Platform - 5 I/O Modules
 */
#define DEFIANT_K2_MODEL_NUM               5850
#define DEFIANT_K2_MODEL_NUM_ASCII         "VNXe5850"
#define DEFIANT_K2_PLATFORM_NAME_ASCII     "Defiant_K2"

/* Defiant K3
 * Info:      Transformers Platform - 5 I/O Modules
 */
#define DEFIANT_K3_MODEL_NUM               5500
#define DEFIANT_K3_MODEL_NUM_ASCII         "VNXe5500"
#define DEFIANT_K3_PLATFORM_NAME_ASCII     "Defiant_K3"

/* Defiant S1
 * Info:      Transformers Platform - 5 I/O Modules    Simulator
 */
#define DEFIANT_S1_MODEL_NUM               7600
#define DEFIANT_S1_MODEL_NUM_ASCII         "VNX7600"
#define DEFIANT_S1_PLATFORM_NAME_ASCII     "Defiant_S1"

/* Defiant S4
 * Info:      Transformers Platform - 5 I/O Modules    Simulator
 */
#define DEFIANT_S4_MODEL_NUM               5400
#define DEFIANT_S4_MODEL_NUM_ASCII         "VNX5400"
#define DEFIANT_S4_PLATFORM_NAME_ASCII     "Defiant_S4"

/* Nova 12 GB
 * Info:      Transformers Platform - 1 eSLIC
 */
#define NOVA1_MODEL_NUM                     3200
#define NOVA1_MODEL_NUM_ASCII               "VNXe3200"
#define NOVA1_PLATFORM_NAME_ASCII           "Beachcomber_M1"

/* Bearcat 8 GB
 * Info:      Transformers Platform - 1 eSLIC
 */
#define BEARCAT_MODEL_NUM                   1600
#define BEARCAT_MODEL_NUM_ASCII             "VNXe1600" 
#define BEARCAT_PLATFORM_NAME_ASCII         "Bearcat"

/* Nova 24/48 GB
 * Info:      Transformers Platform - 1 eSLIC
 */
#define NOVA3_MODEL_NUM                     3400
#define NOVA3_MODEL_NUM_ASCII               "VNXe3400"
#define NOVA3_PLATFORM_NAME_ASCII           "Nova3"
#define NOVA3_XM_PLATFORM_NAME_ASCII        "Nova3_XM"

/* Nova 12/24 GB Simulator
 * Info:      Transformers Platform - 1 eSLIC - Simulator
 */
#define NOVA_SIM_MODEL_NUM                  3200
#define NOVA_SIM_MODEL_NUM_ASCII            "VNXe3200"
#define NOVA_SIM_PLATFORM_NAME_ASCII        "Beachcomber_S1"

/* Sentry 12
 * Info:      Sentry Platform for Linux - 2 I/O Modules
 */
#define SENTRY_MODEL_NUM                   3300
#define SENTRY_MODEL_NUM_ASCII             "VNXe3300"
#define SENTRY_PLATFORM_NAME_ASCII         "Sentry"

/* vVNX 12 GB Virtual appliance 
 * Info:      VMware 12 GB ESX based deployment; 
 */
#define MERIDIAN_MODEL_NUM                 100
#define MERIDIAN_MODEL_NUM_ASCII           "vVNX"
#define MERIDIAN_PLATFORM_NAME_ASCII       "Meridian"

/* Virtual platform 
 * Info:     Second generation single/dual, different hypervisors 
 */
#define TUNGSTEN_MODEL_NUM                 200
#define TUNGSTEN_MODEL_NUM_ASCII           "vVNX"
#define TUNGSTEN_PLATFORM_NAME_ASCII       "Tungsten"

/* Enterprise
 * Info:      Moons Platform - 9 I/O Modules
 */
#define ENTERPRISE_MODEL_NUM                9001
#define ENTERPRISE_MODEL_NUM_ASCII          "VNX9001"
#define ENTERPRISE_PLATFORM_NAME_ASCII      "Enterprise"

//TODO - Model Number and Names need to verified for these platforms
//       Values used here are just place holders for testing
/* TRITON_1
 * Info:      Moons Platform - 12 I/O Modules
 */
#define TRITON_1_MODEL_NUM                 9901
#define TRITON_1_MODEL_NUM_ASCII           "VNXe9901"
#define TRITON_1_PLATFORM_NAME_ASCII       "Triton_1"

//TODO - Model Number and Names need to verified for these platforms
//       Values used here are just place holders for testing
/* Hyperion
 * Info:      Moons Platform - 5 I/O Modules
 */
#define HYPERION_1_MODEL_NUM             7701
#define HYPERION_1_MODEL_NUM_ASCII       "VNXe7701"
#define HYPERION_1_PLATFORM_NAME_ASCII   "Hyperion_1"

#define HYPERION_2_MODEL_NUM             7702
#define HYPERION_2_MODEL_NUM_ASCII       "VNXe7702"
#define HYPERION_2_PLATFORM_NAME_ASCII   "Hyperion_2"

#define HYPERION_3_MODEL_NUM             7703
#define HYPERION_3_MODEL_NUM_ASCII       "VNXe7703"
#define HYPERION_3_PLATFORM_NAME_ASCII   "Hyperion_3"

//TODO - Model Number and Names need to verified for these platforms
//       Values used here are just place holders for testing
/* Oberon
 * Info:      Moons Platform - 2 I/O Modules
 */
#define OBERON_1_MODEL_NUM                 3501
#define OBERON_1_MODEL_NUM_ASCII           "VNXe3501"
#define OBERON_1_PLATFORM_NAME_ASCII       "Oberon_1"

#define OBERON_2_MODEL_NUM                 3502
#define OBERON_2_MODEL_NUM_ASCII           "VNXe3502"
#define OBERON_2_PLATFORM_NAME_ASCII       "Oberon_2"

#define OBERON_3_MODEL_NUM                 3503
#define OBERON_3_MODEL_NUM_ASCII           "VNXe3503"
#define OBERON_3_PLATFORM_NAME_ASCII       "Oberon_3"

#define OBERON_4_MODEL_NUM                 3504
#define OBERON_4_MODEL_NUM_ASCII           "VNXe3504"
#define OBERON_4_PLATFORM_NAME_ASCII       "Oberon_4"

/* Oberon S1 simulator
 * Info:      Moons Platform - 2 I/O Modules
 */
#define OBERON_S1_MODEL_NUM                 3501
#define OBERON_S1_MODEL_NUM_ASCII           "VNXe3501"
#define OBERON_S1_PLATFORM_NAME_ASCII       "Oberon_S1"


/***************************/
/* AX Series Model Numbers */
/***************************/

/* Acadia Low Cost 
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 2.6 GHz
 * Info:      SEi = Single Controller (single sp), Entry level, iSCSI.
 *            Third generation Wintel architecture platform.  Removable
 *            battery backed up cache card (ACADIA), 1Gb ISCSI front end, 1.5Gb fibre back end busses. 
 */
#define ACADIA_LC_ISCSI_MODEL_NUM               100
#define ACADIA_LC_ISCSI_MODEL_NUM_ASCII         "AX100SEi"

/* Acadia
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 2.6 GHz
 * Info:      SC = Single Controller (single sp), fibre channel.
 *            Third generation Wintel architecture platform.  Removable
 *            battery backed up cache card (ACADIA), 1.5Gb fibre front and back end busses. 
 */
#define ACADIA_MODEL_NUM                        100
#define ACADIA_MODEL_NUM_ASCII                  "AX100SC"

/* Acadia
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 2.6 GHz
 * Info:      SCi = Single Controller (single sp), iSCSI.
 *            Third generation Wintel architecture platform.  Removable
 *            battery backed up cache card (ACADIA), 1Gb ISCSI front end, 1.5Gb fibre back end busses. 
 */
#define ACADIA_ISCSI_MODEL_NUM                  100
#define ACADIA_ISCSI_MODEL_NUM_ASCII            "AX100SCi"

/* Grand Teton
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 2.6 GHz
 * Info:      Dual controller (dual sp), fibre channel.
 *            Third generation Wintel architecture platform.  Removable
 *            battery backed up cache card (ACADIA), 1.5Gb fibre front and back end busses. 
 */
#define GRAND_TETON_MODEL_NUM                   100
#define GRAND_TETON_MODEL_NUM_ASCII             "AX100"

/* Grand Teton
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 2.6 GHz
 * Info:      i = Dual controller (dual sp), iSCSI.
 *            Third generation Wintel architecture platform.  Removable
 *            battery backed up cache card (ACADIA), 1Gb ISCSI front end, 1.5Gb fibre back end busses. 
 */
#define GRAND_TETON_ISCSI_MODEL_NUM             100
#define GRAND_TETON_ISCSI_MODEL_NUM_ASCII       "AX100i"

/* Yosemite
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 1.2 GHz
 * Info:      SEi = Single controller (single sp), iSCSI.
 *            Third generation Wintel architecture platform.  Removable
 *            battery backed up cache card, 1Gb ISCSI front-end,
 *            3 Gb SATA back-end bus.  
 */
#define YOSEMITE_ISCSI_MODEL_NUM            150
#define YOSEMITE_ISCSI_MODEL_NUM_ASCII      "AX150SCi"

/* Yosemite
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 1.2 GHz
 * Info:      SC = Single Controller (single sp), fibre channel.
 *            Third generation Wintel architecture platform. Removable
 *            battery backed up cache card, two 2Gb fibre front-end ports 
 *            3 Gb SATA back-end bus.  
 */
#define YOSEMITE_MODEL_NUM                  150
#define YOSEMITE_MODEL_NUM_ASCII            "AX150SC"

/* Big Sur
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 1.2 GHz
 * Info:      Dual controller (dual sp), fibre channel.
 *            Third generation Wintel architecture platform. 
 *            Two 2Gb fibre front-end ports, 3Gb SATA back end bus. 
 */
#define BIG_SUR_MODEL_NUM                   150
#define BIG_SUR_MODEL_NUM_ASCII             "AX150"

/* Big Sur
 * Used in:   LCC Emulated enclosure
 * Processor: Single Celeron 1.2 GHz
 * Info:      i = Dual controller (dual sp), iSCSI.
 *            Third generation Wintel architecture platform. 
 *            Two 1Gb ISCSI ports, 3 Gb SATA back end bus. 
 */
#define BIG_SUR_ISCSI_MODEL_NUM             150
#define BIG_SUR_ISCSI_MODEL_NUM_ASCII       "AX150i"


/* Yeti
 * Used in:   LCC Emulated enclosure
 * Processor: Single AMD 1.8 GHz
 * Info:      Dual controller (dual sp), fibre channel.
 *            Third generation Wintel architecture platform. 
 *            Two 4Gb fibre front end ports, SAS/SATA-2 back end bus. 
 */
#define YETI_MODEL_NUM                      350
#define YETI_MODEL_NUM_ASCII                "AX350"

/* Yeti
 * Used in:   LCC Emulated enclosure
 * Processor: Single AMD 1.8 GHz
 * Info:      Dual controller (dual sp), iSCSI.
 *            Third generation Wintel architecture platform. 
 *            Two 1Gb iSCSI front end ports, SAS/SATA-2 back end bus. 
 */
#define YETI_ISCSI_MODEL_NUM                350
#define YETI_ISCSI_MODEL_NUM_ASCII          "AX350i"

/* Boomslang
 * Used in:   LCC Emulated enclosure
 * Processor: Single Intel 1.66 GHz Sossaman 
 * Info:      Single or dual controller, SBB form factor, fibre channel.
 *            Third generation Wintel architecture platform. 
 *            Two 4Gb fibre front end ports, SAS/SATA-2 back end bus. 
 */
#define BOOMSLANG_MODEL_NUM                   4050
#define BOOMSLANG_MODEL_NUM_ASCII             "AX4-5"
#define BOOMSLANG_SC_MODEL_NUM_ASCII          "AX4-5SC"

/* Boomslang
 * Used in:   LCC Emulated enclosure
 * Processor: Single Intel 1.66 GHz Sossaman
 * Info:      Single or dual controller, SBB form factor, iSCSI.
 *            Third generation Wintel architecture platform. 
 *            Two 1Gb iSCSI front end ports, SAS/SATA-2 back end bus. 
 */
#define BOOMSLANG_ISCSI_MODEL_NUM                   4050
#define BOOMSLANG_ISCSI_MODEL_NUM_ASCII             "AX4-5i"
#define BOOMSLANG_SC_ISCSI_MODEL_NUM_ASCII          "AX4-5SCi"

#endif	/* ndef MODEL_NUMBERS_H */ /* end file model_numbers.h */
