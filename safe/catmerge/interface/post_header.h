/***************************************************************
 * Copyright (C) EMC Corporation 2001 - 2006
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************
 *							post_header.h
 ***************************************************************
 * Description: This file contains the definitions for the image
 *				header for POST images.
 * 
 * NOTE: THIS IS PRIMARILY FOR THE USE OF BIGFOOT SYSTEMS FOR
 *		 CREATING EXPANDER BINARIES. IT MUST BE KEPT CURRENT
 *		 WITH THE HEADER.H FILE POST USES.
 * 
 *		 MOST of the #defines are not even used in any of the
 *		 flare or expander code. We really just want the image IDs
 *		 and header structure.
 ***************************************************************/

#ifndef _HEADER_H
#define _HEADER_H

#define MB_1	(1 * 1024 * 1024)
#define MB_2	(2 * 1024 * 1024)
#define KB_512	(512 * 1024)
#define KB_256	(256 * 1024)
#define KB_128	(128 * 1024)
#define KB_64	(64 * 1024)
#define KB_28	(28 * 1024)

// defines for Data Directory Boot Service Image Type
#define NT_DDBS			0x01
#define XP_DDBS			0x02
#define HAMMERS_XP_DDBS 0x03
#define PIRANHA_DDBS 	0x10

#if defined(CHAMELEONII) || defined(X1) || defined(X1LITE)
#define DDBS_IMAGE_TYPE NT_DDBS
#elif defined(BARRACUDA) || defined(TARPON) || defined(SNAPPER) || defined(CX300I)
#define DDBS_IMAGE_TYPE XP_DDBS
#elif defined(PIRANHA) || defined(PIRANHAII) || defined(BIGFOOT) || defined(WILSONPEAK) || defined(MAMBA)
#define DDBS_IMAGE_TYPE PIRANHA_DDBS
#else
#define DDBS_IMAGE_TYPE HAMMERS_XP_DDBS
#endif

#define HEADER_SIZE					sizeof(HEADER)

/*
 * change based on the SP type
 */
#if defined (MAMBA)

#define SP_POST		 MAMBA_POST
#define SP_POST_SIZE MAMBA_POST_SIZE
#define SP_POST_SEED MAMBA_POST_SEED

#define SP_BIOS		 MAMBA_BIOS
#define SP_BIOS_SIZE MAMBA_BIOS_SIZE
#define SP_BIOS_SEED MAMBA_BIOS_SEED

#define SP_DIAG		 MAMBA_DIAG
#define SP_DIAG_SIZE MAMBA_DIAG_SIZE
#define SP_DIAG_SEED MAMBA_DIAG_SEED

#define SP_EAST		 MAMBA_EAST
#define SP_EAST_SIZE MAMBA_EAST_SIZE
#define SP_EAST_SEED EAST_CHECKSUM_SEED

#define SP_NAME	"Mamba"

#endif


#define SP_RESET_PIC_SIZE	KB_64

// image IDs
typedef enum image_id {	// POST Images
						CROSSFIRE_POST = 0x01,
						CATAPULT_POST = 0x02,
						CHAMELEONII_POST = 0x03,
						X1_POST = 0x04,
						X1LITE_POST = 0x05,
						TARPON_POST = 0x06,
						BARRACUDA_POST = 0x07,
						PIRANHA_POST = 0x08,
						SNAPPER_POST = 0x09,
						HAMMERHEAD_POST = 0x0A,
						CX300I_POST = 0x0B,
						SJHAMMER_POST = 0x0C,	// Sledgehammer and Jackhammer use the same POST
						SLEDGEHAMMER_POST = SJHAMMER_POST,
						PIRANHAII_POST = 0x0D,
						BIGFOOT_POST = 0x0E,
						DREADNOUGHT_POST = 0x0F,
						TRIDENT_POST = 0x81,
						IRONCLAD_POST = TRIDENT_POST,
						WILSONPEAK_POST = 0x82,
						MAMBA_POST = 0x83,

						// BIOS Images
						CROSSFIRE_BIOS = 0x11,
						CATAPULT_BIOS = 0x12,
						CHAMELEONII_BIOS = 0x13,
						X1_BIOS = 0x14,
						X1LITE_BIOS = X1_BIOS,		// X1 and X1 Lite use the same BIOS
						TARPON_BIOS = 0x15,
						CX300I_BIOS = TARPON_BIOS,	// Tarpon and CX300i use the same BIOS
						BARRACUDA_BIOS = 0x16,
						PIRANHA_BIOS = 0x17,
						SNAPPER_BIOS = 0x18,
						HAMMERHEAD_BIOS = 0x1A,
						SJHAMMER_BIOS = 0x1B,	// Sledgehammer and Jackhammer use the same BIOS
						PIRANHAII_BIOS = 0x1C,
						BIGFOOT_BIOS = 0x1D,
						DREADNOUGHT_BIOS = 0x1E,
						TRIDENT_BIOS = 0x1F,
						WILSONPEAK_BIOS = 0xA0,
						MAMBA_BIOS = 0xA1,
					

						// FRUMON Images
						FRUMON_2GB = 0x21,
						KOTO_FRUMON = 0x22,
						STILETTO_2GB = 0x23,
						STILETTO_4GB = 0x24,

						// E3 Diagnostic Images
						CHAMELEONII_DIAG = 0x31,
						X1_DIAG = 0x32,
						X1LITE_DIAG = 0x33,
						TARPON_DIAG = 0x34,
						BARRACUDA_DIAG = 0x35,
						PIRANHA_DIAG = 0x36,
						SNAPPER_DIAG = 0x37,
						HAMMERHEAD_DIAG = 0x38,
						CX300I_DIAG = 0x39,
						JACKHAMMER_DIAG = 0x3A,
						SLEDGEHAMMER_DIAG = 0x3B,
						PIRANHAII_DIAG = 0x3C,
						BIGFOOT_DIAG = 0x3D,
						DREADNOUGHT_DIAG = 0x3E,
						TRIDENT_DIAG = 0x3F,
						IRONCLAD_DIAG = 0x91,
						WILSONPEAK_DIAG = 0x92,
						MAMBA_DIAG = 0x93,


						// Data Directory Boot Service Images
						DD_BOOT_SERVICE = 0x40,

						// EAST
						HAMMERHEAD_EAST = 0x51,
						SJHAMMER_EAST = 0x52,
						JACKHAMMER_EAST = 0x53,		// not currently used
						DREADNOUGHT_EAST = 0x54,    // not used
						TRIDENT_EAST = 0x55,        // not used
						IRONCLAD_EAST = 0x56,       // not used
						WILDCATS_EAST = 0x57,
						MAMBA_EAST = 0x58,


						// firmware images
						QLISP4010_FW = 0x60,		// QLogic iSCSI firmware
						RESET_PIC_FW = 0x61,
						RESET_CONTROLLER_FW = 0x62,

						// Bigfoot programmables
						BIGFOOT_EXPANDER	= 0x70,
                        BIGF_DPE_MC			= 0x71,
                        BIGF_CC				= 0x72,
                        BIGFOOT_MUX			= 0x73,
                        BIGF_CPLD			= 0x74,
                        BIGFOOT_BOOT		= 0x75,
                        BIGFOOT_ISTR		= 0x76,
                        BIGF_LSI			= 0x77,
						BIGF_DAE_MC			= 0x78,

                        MAMBA_PSFW			= 0x79,
						MAMBA_CPLD			= 0x7A,
						MAMBA_LSI			= 0x7B,
						MAMBA_DPE_MC		= 0x7C,
						MAMBA_DAE_MC		= 0x7D,
						MAMBA_PS1 = 0x7E,	// original AC supply
						MAMBA_PS2 = 0x7F,	// 2nd source AC supply
						MAMBA_PS3 = 0xB0,	// DC supply
						MAMBA_DPE_MCFW		= 0xB1,
						MAMBA_DAE_MCFW		= 0xB2,
						MAMBA_DPE_DLMC		= 0xB3,
						MAMBA_DAE_DLMC		= 0xB4,

#ifdef MAMBA
						BIGFOOT_CPLD	= MAMBA_CPLD,
						BIGFOOT_LSI		= MAMBA_LSI,
						BIGFOOT_CC		= MAMBA_PSFW,
						BIGFOOT_DPE_MC	= MAMBA_DPE_MC,
						BIGFOOT_DAE_MC	= MAMBA_DAE_MC,
#else
						BIGFOOT_CPLD	= BIGF_CPLD,
						BIGFOOT_LSI		= BIGF_LSI,
						BIGFOOT_CC		= BIGF_CC,
						BIGFOOT_DPE_MC	= BIGF_DPE_MC,
						BIGFOOT_DAE_MC	= BIGF_DAE_MC
#endif
						} IMAGE_ID;

// Image ID Masks
#define POST_IMAGE		0x00
#define BIOS_IMAGE		0x10
#define FRUMON_IMAGE	0x20
#define DIAG_IMAGE		0x30
#define DDBS_IMAGE		0x40
#define EAST_IMAGE		0x50
#define IMAGE_ID_MASK	0xF0
#define POST_IMAGE1		0x80
#define DIAG_IMAGE1		0x90
#define BIOS_IMAGE1		0xA0

// checksum seeds
#define CROSSFIRE_POST_SEED		0x43524F53				// CROS
#define CROSSFIRE_BIOS_SEED		0x63726F73				// cros
#define CATAPULT_POST_SEED		0x43415441				// CATA
#define CATAPULT_BIOS_SEED		0x63617461				// cata
#define CHAMELEONII_POST_SEED	0x4348414D				// CHAM
#define CHAMELEONII_BIOS_SEED	0x6368616D				// cham
#define X1_POST_SEED			0x58315350				// X1SP
#define X1_BIOS_SEED			0x78317370				// x1sp
#define X1LITE_POST_SEED		0x58314C54				// X1LT
#define X1LITE_BIOS_SEED		X1_BIOS_SEED			// X1 and X1 Lite use the same BIOS
#define TARPON_POST_SEED		0x54415250				// TARP
#define TARPON_BIOS_SEED		0x74617270				// tarp
#define BARRACUDA_POST_SEED		0x42415252				// BARR
#define BARRACUDA_BIOS_SEED		0x62617272				// barr
#define SNAPPER_POST_SEED		0x534E4150				// SNAP
#define SNAPPER_BIOS_SEED		0x736E6170				// snap
#define PIRANHA_POST_SEED		0x50495241				// PIRA
#define PIRANHA_BIOS_SEED		0x70697261				// para
#define PIRANHAII_POST_SEED		0x50414949				// PAII
#define PIRANHAII_BIOS_SEED		0x70616969				// paii
#define HAMMERHEAD_POST_SEED	0x48414D4D				// HAMM
#define HAMMERHEAD_BIOS_SEED	0x68616D6D				// hamm
#define CX300I_POST_SEED		TARPON_POST_SEED		// Use the same seed as Tarpon because the BIOS uses the same image
#define CX300I_BIOS_SEED		TARPON_BIOS_SEED		// Tarpon and CX300i use the same BIOS
#define SJHAMMER_POST_SEED		0x4A53484D				// JSHM
#define SLEDGEHAMMER_POST_SEED	SJHAMMER_POST_SEED		// JSHM
#define SJHAMMER_BIOS_SEED		0x6A73686D				// jshm
#define BIGFOOT_POST_SEED		0x42494746				// BIGF
#define BIGFOOT_BIOS_SEED		0x62696766				// bigf
#define WILSONPEAK_POST_SEED	0x57494C53				// WILS
#define WILSONPEAK_BIOS_SEED	0x77696C73				// wils
#define MAMBA_POST_SEED			0x4D414D42				// MAMB
#define MAMBA_BIOS_SEED			0x6d616D62				// mamb
#define DREADNOUGHT_POST_SEED	0x48404D4E				// DRNT
#define DREADNOUGHT_BIOS_SEED	0x68606D6E				// drnt
#define TRIDENT_POST_SEED		0x4E50474D				// TRDT
#define TRIDENT_BIOS_SEED		0x6EA73086				// trdt
#define IRONCLAD_POST_SEED		TRIDENT_POST_SEED		// IRCD
#define FRUMON_SEED				0x4652554D				// FRUM
#define KLONDIKE_SEED			FRUMON_SEED				// Klondike checksum seed is the same as the FRUMON
#define KOTO_FRUMON_SEED		0x4B4F544F				// KOTO
#define STILETTO_2GB_SEED		0x53543247				// ST2G
#define STILETTO_4GB_SEED		0x53543447				// ST4G
#define CHAMELEONII_DIAG_SEED	0x44494147				// DIAG
#define X1_DIAG_SEED			0x58314447				// X1DG
#define X1LITE_DIAG_SEED		0x58314C44				// X1LD
#define TARPON_DIAG_SEED		0x54504447				// TPDG
#define BARRACUDA_DIAG_SEED		0x42414447				// BADG
#define SNAPPER_DIAG_SEED		0x534E4447				// SNDG
#define PIRANHA_DIAG_SEED		0x50494447				// PIDG
#define PIRANHAII_DIAG_SEED		0x50494944				// PIID
#define HAMMERHEAD_DIAG_SEED	0x48484447				// HHDG
#define CX300I_DIAG_SEED		0x43334447				// C3DG
#define JACKHAMMER_DIAG_SEED	0x4A4B4447				// JKDG
#define SLEDGEHAMMER_DIAG_SEED	0x53474447				// SGDG
#define BIGFOOT_DIAG_SEED		0x42494744				// BIGD
#define WILSONPEAK_DIAG_SEED	0x57494C44				// WILD
#define MAMBA_DIAG_SEED			0x4D414D44				// MAMD
#define DREADNOUGHT_DIAG_SEED	0x78707D7E				// DNDG
#define TRIDENT_DIAG_SEED		0x4687337E				// TDDG
#define IRONCLAD_DIAG_SEED		0x75709D70				// ICDG
#define DD_BOOT_SERVICE_SEED	0x44444253				// DDBS
#define EAST_CHECKSUM_SEED		0x45415354				// EAST
#define QLISP4010_FW_SEED		0x514C4953				// QLIS		QLogic iSCSI FW
#define RESET_PIC_SEED			0x52504943				// RPIC
#define BIGFOOT_FW_SEED			0x53604963				// Random
#define RESET_CONT_FW_SEED		0x72706963				// RCFW //rpic





// image sizes
#define CATAPULT_POST_SIZE		KB_512
#define CATAPULT_BIOS_SIZE		KB_512
#define CROSSFIRE_POST_SIZE		KB_512
#define CROSSFIRE_BIOS_SIZE		KB_512
#define CHAMELEONII_POST_SIZE	KB_512
#define CHAMELEONII_BIOS_SIZE	KB_512
#define X1_POST_SIZE			KB_512
#define X1_BIOS_SIZE			KB_512
#define X1LITE_POST_SIZE		KB_512
#define X1LITE_BIOS_SIZE		X1_BIOS_SIZE			// X1 and X1 Lite use the same BIOS
#define TARPON_POST_SIZE		(KB_512 + KB_128)
#define TARPON_BIOS_SIZE		(KB_512 - KB_128)
#define BARRACUDA_POST_SIZE		KB_512
#define BARRACUDA_BIOS_SIZE		KB_512
#define SNAPPER_POST_SIZE		KB_512
#define SNAPPER_BIOS_SIZE		KB_512
#define PIRANHA_POST_SIZE		(KB_512 + KB_128)
#define PIRANHA_BIOS_SIZE		(KB_512 - KB_128)
#define PIRANHAII_POST_SIZE		(KB_512 + KB_128)
#define PIRANHAII_BIOS_SIZE		(KB_512 - KB_128)
#define HAMMERHEAD_POST_SIZE	(MB_1 + KB_512)
#define HAMMERHEAD_BIOS_SIZE	KB_512
#define DREADNOUGHT_POST_SIZE	(MB_1 + KB_512)
#define DREADNOUGHT_BIOS_SIZE	KB_512
#define CX300I_POST_SIZE		(KB_512 + KB_128)
#define CX300I_BIOS_SIZE		TARPON_BIOS_SIZE		// Tarpon and CX300i use the same BIOS
#define SJHAMMER_POST_SIZE		(MB_1 + KB_512)			// Jackhammer and Sledgehammer use the same POST
#define SJHAMMER_BIOS_SIZE		KB_512					// Jackhammer and Sledgehammer use the same BIOS
#define TRIDENT_POST_SIZE		(MB_1 + KB_512)			// Jackhammer and Sledgehammer use the same POST
#define TRIDENT_BIOS_SIZE		KB_512					// Jackhammer and Sledgehammer use the same BIOS
#define SLEDGEHAMMER_POST_SIZE	SJHAMMER_POST_SIZE		// Jackhammer and Sledgehammer use the same POST
#define SLEDGEHAMMER_BIOS_SIZE	SJHAMMER_BIOS_SIZE		// Jackhammer and Sledgehammer use the same BIOS
#define IRONCLAD_POST_SIZE		TRIDENT_POST_SIZE		// Jackhammer and Sledgehammer use the same POST
#define IRONCLAD_BIOS_SIZE		TRIDENT_BIOS_SIZE		// Jackhammer and Sledgehammer use the same BIOS
#define BIGFOOT_POST_SIZE		(MB_1 + KB_512)	//previous (KB_512 + KB_128)
#define BIGFOOT_BIOS_SIZE		KB_512			//previous (KB_512 - KB_128)
#define WILSONPEAK_POST_SIZE	(MB_1 + KB_512)
#define WILSONPEAK_BIOS_SIZE	KB_512
#define MAMBA_POST_SIZE			((MB_1 + KB_512) - KB_128)
#define MAMBA_BIOS_SIZE			(KB_512 + KB_128)
#define FRUMON_2GB_SIZE			KB_28
#define KOTO_FRUMON_SIZE		KB_28
#define STILETTO_2GB_SIZE		KB_512
#define STILETTO_4GB_SIZE		KB_512
#define CHAMELEONII_DIAG_SIZE	MB_2
#define X1_DIAG_SIZE			MB_2
#define X1LITE_DIAG_SIZE		MB_2
#define TARPON_DIAG_SIZE		MB_2
#define BARRACUDA_DIAG_SIZE		MB_2
#define SNAPPER_DIAG_SIZE		MB_2
#define PIRANHA_DIAG_SIZE		MB_2
#define PIRANHAII_DIAG_SIZE		MB_2
#define HAMMERHEAD_DIAG_SIZE	MB_2
#define CX300I_DIAG_SIZE		MB_2
#define JACKHAMMER_DIAG_SIZE	MB_2
#define SLEDGEHAMMER_DIAG_SIZE	MB_2
#define DREADNOUGHT_DIAG_SIZE	MB_2
#define IRONCLAD_DIAG_SIZE		MB_2
#define TRIDENT_DIAG_SIZE		MB_2
#define BIGFOOT_DIAG_SIZE		MB_2
#define WILSONPEAK_DIAG_SIZE	MB_2
#define MAMBA_DIAG_SIZE			MB_2
#define DD_BOOT_SERVICE_SIZE	KB_64
#define QLISP4010_FW_SIZE		(KB_512 + HEADER_SIZE)
#define HAMMERHEAD_EAST_SIZE	MB_2
#define SJHAMMER_EAST_SIZE		MB_2
#define DREADNOUGHT_EAST_SIZE	MB_2
#define TRIDENT_EAST_SIZE		MB_16
#define WILDCATS_EAST_SIZE		MB_16
#define MAMBA_EAST_SIZE			MB_16
#define RESET_CONT_FW_SIZE		KB_512

// Bigfoot programmables
#define BIGFOOT_EXPANDER_SIZE	MB_1
#define BIGFOOT_MC_SIZE			KB_512
#define MAMBA_PSFW_SIZE			KB_128
#define BIGFOOT_CC_SIZE			MAMBA_PSFW_SIZE
#define BIGFOOT_MUX_SIZE		KB_128
#define BIGFOOT_CPLD_SIZE		KB_128
#define BIGFOOT_BOOT_SIZE		KB_256
#define BIGFOOT_ISTR_SIZE		KB_128
#define BIGFOOT_LSI_SIZE		KB_256
#define MAMBA_3PS_SIZE			KB_512 // For blob (3 psfw images)
#define MAMBA_MC_SIZE           BIGFOOT_MC_SIZE

// image names
#define BIOS_NAME	"BIOS"
#define POST_NAME	"Extended POST"
#define DIAG_NAME	"Extended DIAG"
#define DDBS_NAME	"DD Boot Service"
#define EAST_NAME	"EAST"
#define PSFW_NAME	"PS FW"

// Bigfoot programmables
#define EXPANDER_NAME       "EXPANDER"
#define EXPANDER_BOOT_NAME  "BOOT"
#define ISTR_NAME           "ISTR"
#define CPLD_NAME           "CPLD"
#define MUX_NAME            "MUX"
#ifdef MAMBA
#define CC_NAME             "PS FW"
#else
#define CC_NAME             "CC"
#endif
#define DPE_MCFW_NAME	    "DPEMCFW"
#define DAE_MCFW_NAME       "DAEMCFW"
#define DPE_MC_NAME	        "DPEMC"
#define DAE_MC_NAME         "DAEMC"
#define DPE_DLMC_NAME       "DPEDLMC"
#define DAE_DLMC_NAME       "DAEDLMC"
#define LSI_NAME            "LSI"

// relocation records
#define MAX_RELOCATE_RECORD	111
#define END_RECORD_ADDRESS	0xFFFFFFF0		// end marker

typedef struct relocate_record
{
	unsigned long address;	// RAM address
	unsigned long size;		// in bytes
} RELOCATE_RECORD;

// image header
#define SP_NAME_SIZE			32
#define IMAGE_NAME_SIZE			16
#define REV_STRING_SIZE			12
#define DATE_TIME_STAMP_SIZE	24
#define SPARE_SIZE				 8

typedef struct header
{
	unsigned long checksum;
	unsigned char sp_name[SP_NAME_SIZE];
	unsigned char image_name[IMAGE_NAME_SIZE];
	unsigned char rev_string[REV_STRING_SIZE];
	unsigned char date_time_stamp[DATE_TIME_STAMP_SIZE];
	unsigned char image_id;
	unsigned char : 8;					// unused
	unsigned char major_rev;
	unsigned char minor_rev;
	unsigned long image_size;			// in bytes
	unsigned long spare[SPARE_SIZE - 2];	// spares for the future (-2 is Bigfoot specific) 
	unsigned long compress;		// Bigfoot/Mamba specific
	unsigned long pmc_checksum; // Bigfoot specific
	unsigned long header_size;			// also offset of where image begins
	RELOCATE_RECORD records[MAX_RELOCATE_RECORD];
	unsigned long : 32;					// unused
} HEADER;

#endif
