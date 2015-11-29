#ifndef FBE_SATA_INTERFACE_H
#define FBE_SATA_INTERFACE_H

//SATA #defines
#define FBE_SATA_Q_DEPTH_MASK       0x0000000F


//SATA Disk Designator Block ("Inscription") #defines 
#define FBE_SATA_DD_BLOCK_REV_NUMBER_OFFSET         8    
#define FBE_SATA_DD_BLOCK_DRIVE_RPM_OFFSET          12
#define FBE_SATA_DD_BLOCK_POWER_MODE_OFFSET         16
#define FBE_SATA_DD_BLOCK_FEATURES_OFFSET           20
#define FBE_SATA_DD_BLOCK_MUX_FW_REV_OFFSET         24
#define FBE_SATA_DD_BLOCK_INQUIRY_DATA_OFFSET       127

#define FBE_SATA_EMC_SIGNATURE   "EMC2SATA"

#define FBE_SATA_DD_BLOCK_SIGNATURE_SIZE           8
#define FBE_SATA_DD_BLOCK_REV_NUMBER_SIZE          4    
#define FBE_SATA_DD_BLOCK_DRIVE_RPM_SIZE           4
#define FBE_SATA_DD_BLOCK_POWER_MODE_SIZE          1 
#define FBE_SATA_DD_BLOCK_FEATURES_SIZE            4
#define FBE_SATA_DD_BLOCK_MUX_FW_REV_SIZE          4
#define FBE_SATA_DD_BLOCK_INQUIRY_DATA_SIZE        129

//SATA ENUMS
typedef enum fbe_sata_fis_type_e{
	FBE_SATA_FIS_TYPE_REGISTER_HOST_TO_DEVICE = 0x27, /*!< Register FIS ?Host to Device */
	FBE_SATA_FIS_TYPE_REGISTER_DEVICE_TO_HOST = 0x34, /*!< Register FIS ?Device to Host */
	FBE_SATA_FIS_DMA_ACTIVATE				  = 0x39, /*!< DMA Activate FIS ?Device to Host */
	FBE_SATA_FIS_DMA_SETUP					  = 0x41, /*!< DMA Setup FIS ?Bi-directional */
	FBE_SATA_FIS_DATA						  = 0x46, /*!< Data FIS ?Bi-directional */
	FBE_SATA_FIS_BIST_ACTIVATE				  = 0x58, /*!< BIST Activate FIS ?Bi-directional */
	FBE_SATA_FIS_PIO_SETUP					  = 0x5F, /*!< PIO Setup FIS ?Device to Host */
	FBE_SATA_FIS_SET_DEVICE_BITS			  = 0xA1, /*!< Set Device Bits FIS ?Device to Host */
}fbe_sata_fis_type_t;

typedef enum fbe_sata_command_e {
	FBE_SATA_COMMAND_FIS_WRITE_UNCORRECTABLE = 0x45,
	FBE_SATA_READ_FPDMA_QUEUED               = 0x60,
	FBE_SATA_WRITE_FPDMA_QUEUED              = 0x61,
}fbe_sata_command_t;

typedef enum fbe_sata_pio_command_e{
	/* PIO data-in command protocol */
	FBE_SATA_PIO_COMMAND_IDENTIFY_DEVICE = 0xEC,
#if 0
	FBE_SATA_PIO_COMMAND_IDENTIFY_PACKET_DEVICE,
	FBE_SATA_PIO_COMMAND_READ_BUFFER,
	FBE_SATA_PIO_COMMAND_READ_LOG_EXT,
	FBE_SATA_PIO_COMMAND_READ_MULTIPLE,
	FBE_SATA_PIO_COMMAND_READ_MULTIPLE_EXT,
	FBE_SATA_PIO_COMMAND_READ_SECTORS,
	FBE_SATA_PIO_COMMAND_SMART_READ_DATA,
	FBE_SATA_PIO_COMMAND_SMART_READ_LOG_SECTOR,
#endif

    FBE_SATA_PIO_COMMAND_READ_NATIVE_MAX_ADDRESS_EXT    = 0x27,
    FBE_SATA_PIO_COMMAND_FLUSH_CACHE_EXT                = 0xEA,
    FBE_SATA_PIO_COMMAND_SET_FEATURES                   = 0xEF,
    FBE_SATA_PIO_COMMAND_READ_SECTORS_EXT               = 0x24,
    FBE_SATA_PIO_COMMAND_SMART                          = 0xB0,
	FBE_SATA_PIO_COMMAND_READ_LOG_EXT                   = 0x2F,
	FBE_SATA_PIO_COMMAND_EXECUTE_DEVICE_DIAGNOSTIC      = 0x90,
    FBE_SATA_PIO_COMMAND_CHECK_POWER_MODE               = 0xE5,
    FBE_SATA_PIO_COMMAND_DOWNLOAD_MICROCODE             = 0x92,
    FBE_SATA_PIO_COMMAND_STANDBY_IMMEDIATE              = 0xE0

#if 0
	/* PIO data-out command protocol */
	FBE_SATA_PIO_COMMAND_CFA_WRITE_MULTIPLE_WITHOUT_ERASE,
	FBE_SATA_PIO_COMMAND_CFA_WRITE_SECTORS_WITHOUT_ERASE,
	FBE_SATA_PIO_COMMAND_DOWNLOAD_MICROCODE,
	FBE_SATA_PIO_COMMAND_SECURITY_DISABLE_PASSWORD,
	FBE_SATA_PIO_COMMAND_SECURITY_ERASE_UNIT,
	FBE_SATA_PIO_COMMAND_SECURITY_SET_PASSWORD,
	FBE_SATA_PIO_COMMAND_SECURITY_UNLOCK,
	FBE_SATA_PIO_COMMAND_SMART_WRITE_LOG_SECTOR,
	FBE_SATA_PIO_COMMAND_WRITE_BUFFER,
	FBE_SATA_PIO_COMMAND_WRITE_LOG_EXT,
	FBE_SATA_PIO_COMMAND_WRITE_MULTIPLE,
	FBE_SATA_PIO_COMMAND_WRITE_MULTIPLE_EXT,
	FBE_SATA_PIO_COMMAND_WRITE_SECTORS,
	FBE_SATA_PIO_COMMAND_WRITE_SECTORS_EXT
#endif
}fbe_sata_pio_command_t;

enum fbe_sata_data_e {
    FBE_SATA_IDENTIFY_DEVICE_DATA_SIZE = 512,
    FBE_SATA_DISK_DESIGNATOR_BLOCK_SIZE = 512, //Inscription sector size
    FBE_SATA_SMART_ATTRIBUTES_DATA_SIZE = 512,
    FBE_SMART_SCT_DATA_SIZE = 512,
    FBE_SATA_DISK_DESIGNATOR_BLOCK_SECTOR_COUNT = 1, //Inscription sector size
    FBE_SATA_SMART_ATTRIBUTES_SECTOR_COUNT  = 1,
    FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE = 20,
    FBE_SATA_READ_NATIVE_MAX_ADDRESS_EXT_DATA_SIZE = 20,
	FBE_SATA_SERIAL_NUMBER_OFFSET = 20, /* Identify device words 10-19 */
	FBE_SATA_SERIAL_NUMBER_SIZE = 20, 
	FBE_SATA_FIRMWARE_REVISION_OFFSET = 46,	/* Identify device words 23-26 */
	FBE_SATA_FIRMWARE_REVISION_SIZE = 8,  
	FBE_SATA_MODEL_NUMBER_OFFSET = 54,	/* Identify device words 27-46 */
	FBE_SATA_MODEL_NUMBER_SIZE = 40,  
	FBE_SATA_MAX_LBA_OFFSET = 200, /* Identify device words 100-103 */
    FBE_SATA_MAX_LBA_SIZE = 8,
    FBE_SATA_IDENTIFY_MAX_LBA_OFFSET = 100, //Identify device word offset
    FBE_SATA_Q_DEPTH_OFFSET = 150,
    FBE_SATA_SPEED_CAPABILITY = 152,   // Identify device word 76
    FBE_SATA_SPEED_CAPABILITY_1_5_GBPS_BIT_MASK = 0x00000002,
    FBE_SATA_SPEED_CAPABILITY_3_0_GBPS_BIT_MASK = 0x00000004,
    FBE_SATA_PIO_MODE_OFFSET = 128,    /* Identify device word 64*/
    FBE_SATA_PIO_MODE_4_BIT_MASK = 0x00000002,
    FBE_SATA_PIO_MODE_3_BIT_MASK = 0x00000001,
    FBE_SATA_UDMA_MODE_OFFSET = 176,   /* Identify device word 88*/
    FBE_SATA_UDMA_MODE_6_BIT_MASK = 0x00000040,
    FBE_SATA_UDMA_MODE_5_BIT_MASK = 0x00000020,
    FBE_SATA_UDMA_MODE_4_BIT_MASK = 0x00000010,
    FBE_SATA_UDMA_MODE_3_BIT_MASK = 0x00000008,
    FBE_SATA_UDMA_MODE_2_BIT_MASK = 0x00000004,
    FBE_SATA_ENABLED_COMMANDS1 = 170, /* Identify device word 85*/
    FBE_SATA_WCE_BIT_MASK = 0x00000020,
    FBE_SATA_SCT_SUPPORT_OFFSET = 412, /*Identify device word 406*/

};

enum  fbe_sata_response_e {
    FBE_SATA_RESPONSE_FIS_TYPE	= 0,

	FBE_SATA_RESPONSE_STATUS	= 2,
	FBE_SATA_RESPONSE_STATUS_ERR_BIT = 0x01, /* Shall be set if any bit in ERROR field is set to one. */

	FBE_SATA_RESPONSE_ERROR		= 3
};

typedef enum fbe_sata_fis_offsets_e{
        /* Command FIS offsets */
        FBE_SATA_COMMAND_FIS_TYPE_OFFSET                  = 0,
        FBE_SATA_COMMAND_FIS_CCR_OFFESET                  = 1,
        FBE_SATA_COMMAND_FIS_COMMAND_OFFSET               = 2,
        FBE_SATA_COMMAND_FIS_FEATURES_OFFSET              = 3,
        FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET               = 4,
        FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET               = 5,
        FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET              = 6,
        FBE_SATA_COMMAND_FIS_DEVICE_OFFSET                = 7,
        FBE_SATA_COMMAND_FIS_LBA_LOW_EXT_OFFSET           = 8,
        FBE_SATA_COMMAND_FIS_LBA_MID_EXT_OFFSET           = 9,
        FBE_SATA_COMMAND_FIS_LBA_HIGH_EXT_OFFSET          = 10,
        FBE_SATA_COMMAND_FIS_FEATURES_EXT_OFFSET          = 11,
        FBE_SATA_COMMAND_FIS_COUNT_OFFSET                 = 12,
        FBE_SATA_COMMAND_FIS_COUNT_EXT_OFFSET             = 13,
        FBE_SATA_COMMAND_FIS_CONTROL_OFFSET               = 15,
        /* Response FIS offsets */
        FBE_SATA_RESPONSE_FIS_TYPE_OFFSET                  = 0,
        FBE_SATA_RESPONSE_FIS_STATUS_OFFSET                = 2,       
        FBE_SATA_RESPONSE_FIS_ERROR_OFFSET                 = 3,
        FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET               = 4,
        FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET               = 5,
        FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET              = 6,
        FBE_SATA_RESPONSE_FIS_DEVICE_OFFSET                = 7,
        FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET           = 8,
        FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET           = 9,
        FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET          = 10
}fbe_sata_fis_offsets_t;

typedef enum fbe_sata_fis_values_e{
        /* Response FIS values */ 
        FBE_SATA_RESPONSE_FIS_ERROR_UNC                    = 0x40,
        FBE_SATA_RESPONSE_FIS_ERROR_UNC_IDNF_ABRT          = 0x54,
        FBE_SATA_RESPONSE_FIS_ERROR_ICRC                   = 0x80,
        FBE_SATA_RESPONSE_FIS_ERROR_ICRC_ABRT              = 0x84,
        FBE_SATA_RESPONSE_FIS_ERROR_IDNF                   = 0x10,
        FBE_SATA_RESPONSE_FIS_ERROR_ABRT                   = 0x04,
}fbe_sata_fis_values_t;


typedef enum fbe_sata_set_features_e{
        FBE_SATA_SET_FEATURES_DISABLE_8_LSI_BIT_PIO   = 0x01,
        FBE_SATA_SET_FEATURES_ENABLE_WCACHE           = 0x02,
        FBE_SATA_SET_FEATURES_TRANSFER                = 0x03,
        FBE_SATA_SET_FEATURES_FIRMWARE_DOWNLOAD       = 0x07,
        FBE_SATA_TRANSFER_UDMA_0                      = 0x40,
        FBE_SATA_TRANSFER_UDMA_1                      = 0x41,
        FBE_SATA_TRANSFER_UDMA_2                      = 0x42,
        FBE_SATA_TRANSFER_UDMA_3                      = 0x43,
        FBE_SATA_TRANSFER_UDMA_4                      = 0x44,
        FBE_SATA_TRANSFER_UDMA_5                      = 0x45,
        FBE_SATA_TRANSFER_UDMA_6                      = 0x46,
        FBE_SATA_TRANSFER_UDMA_7                      = 0x47,
        FBE_SATA_TRANSFER_PIO_SLOW                    = 0x00,
        FBE_SATA_TRANSFER_PIO_0                       = 0x08,
        FBE_SATA_TRANSFER_PIO_1                       = 0x09,
        FBE_SATA_TRANSFER_PIO_2                       = 0x0A,
        FBE_SATA_TRANSFER_PIO_3                       = 0x0B,
        FBE_SATA_TRANSFER_PIO_4                       = 0x0C,
        /* Enable advanced power management */
        FBE_SATA_SET_FEATURES_ENABLE_APM              = 0x05,
        /* Disable media status notification*/
        FBE_SATA_SET_FEATURES_DISABLE_MSN             = 0x31,
        /* Disable read look-ahead            */
        FBE_SATA_SET_FEATURES_DISABLE_RLA             = 0x55,
        /* Enable release interrupt            */
        FBE_SATA_SET_FEATURES_ENABLE_RI               = 0x5D,
        /* Enable SERVICE interrupt            */
        FBE_SATA_SET_FEATURES_ENABLE_SI               = 0x5E,
        /* Disable revert power-on defaults */
        FBE_SATA_SET_FEATURES_DISABLE_RPOD            = 0x66,
        /* Disable write cache                */
        FBE_SATA_SET_FEATURES_DISABLE_WCACHE          = 0x82,
        /* Disable advanced power management*/      
        FBE_SATA_SET_FEATURES_DISABLE_APM             = 0x85,
        /* Enable media status notification */
        FBE_SATA_SET_FEATURES_ENABLE_MSN              = 0x95,
        /* Enable read look-ahead            */
        FBE_SATA_SET_FEATURES_ENABLE_RLA              = 0xAA,
        /* Enable revert power-on defaults  */
        FBE_SATA_SET_FEATURES_ENABLE_RPOD             = 0xCC,
        /* Disable release interrupt        */
        FBE_SATA_SET_FEATURES_DISABLE_RI              = 0xDD,
        /* Disable SERVICE interrupt        */
        FBE_SATA_SET_FEATURES_DISABLE_SI              = 0xDE
}fbe_sata_set_features_t;

typedef enum fbe_sata_smart_e{
        /* Defines for ATA SMART command execution */
        FBE_SATA_SMART_LBA_LOW                        = 0xE0,
        FBE_SATA_SMART_LBA_MID                        = 0x4F,
        FBE_SATA_SMART_LBA_HI                         = 0xC2,
        FBE_SATA_SMART_ENABLE_OPERATIONS              = 0xD8,
        FBE_SATA_SMART_RETURN_STATUS                  = 0xDA,
        FBE_SATA_SMART_ATTRIB_AUTOSAVE                = 0xD2,
        FBE_SATA_SMART_ATTRIB_AUTOSAVE_DISABLE        = 0x00,
        FBE_SATA_SMART_ATTRIB_AUTOSAVE_ENABLE         = 0xF1,
        FBE_SATA_SMART_READ_DATA_SUBCMD               = 0xD0,
        FBE_SATA_SMART_READ_LOG_SUBCMD                = 0xD5,
        FBE_SATA_SMART_WRITE_LOG_SUBCMD               = 0xD6,
        FBE_SATA_SMART_EXEC_OFFLINE_IMMED_SUBCMD      = 0xD4,
        FBE_SATA_SMART_SHORT_ST_CAPTIVE               = 0x81,
        FBE_SATA_SMART_LONG_ST_CAPTIVE                = 0x82,
        FBE_SATA_SMART_SECTOR_COUNT                   = 0x01,
        FBE_SATA_SMART_SEND_DATA_LBA_LOW              = 0xE1
}fbe_sata_smart_t;



typedef enum fbe_sata_sct_e{
        /* Standard SCT Command Action Codes and Function Codes */
        FBE_SATA_SCT_LONG_SECTOR_ACCESS_ACT_CODE     = 0x01, /* Action code for Long Sector access, used in Read/Write Long */
        FBE_SATA_SCT_LONG_SECTOR_ACCESS_FN_CODE      = 0x02, /* Function code for SCT Write Long */
        FBE_SATA_SCT_ERC_ACT_CODE                    = 0x03, /* Action code for Error Recovery Control (ERC)*/
        FBE_SATA_SCT_ERC_SET_NEW_VAL_FN_CODE         = 0x01,
        FBE_SATA_SCT_ERC_RETURN_CURRENT_VAL_FN_CODE  = 0x02,
        FBE_SATA_SCT_ERC_READ_TIMER_SELCTN_CODE      = 0x01,
        FBE_SATA_SCT_ERC_WRITE_TIMER_SELCTN_CODE     = 0x02,
        FBE_SATA_SCT_ERC_TIME_LIMIT                  = 0x64, /* 10s;  0x01 = 100ms */

        /* For SMART command action codes  */
        FBE_SATA_SCT_BIT0                            = 0x01, /* SCT Feature Set support bit */
        FBE_SATA_SCT_BIT1                            = 0x02, /* Long Sector Access support bit */
        FBE_SATA_SCT_BIT2                            = 0x04, /* LBA Segment Access support bit */
        FBE_SATA_SCT_BIT3                            = 0x08, /* Error Recovery Control support bit */
        FBE_SATA_SCT_BIT4                            = 0x10, /* Feature Control support bit */		
        FBE_SATA_SCT_BIT5                            = 0x20, /* SCT Data Tables support bit */	

        FBE_SATA_SCT_KEY_SECTOR_ACTION_CODE_OFFSET   = 0x0,
        FBE_SATA_SCT_KEY_SECTOR_FUNCTION_CODE_OFFSET = 0x2,
        FBE_SATA_SCT_KEY_SECTOR_LBA_LOW_OFFSET       = 0x4,
        FBE_SATA_SCT_KEY_SECTOR_SELECTION_CODE_OFFSET = 0x4,	 
        FBE_SATA_SCT_KEY_SECTOR_LBA_MID_OFFSET       = 0x5,
        FBE_SATA_SCT_KEY_SECTOR_LBA_HIGH_OFFSET      = 0x6,
        FBE_SATA_SCT_KEY_SECTOR_VALUE_OFFSET         = 0x6,
        FBE_SATA_SCT_LONG_SECTOR_ACCESS_DATA_SEC_COUNT =0x2,

        FBE_SATA_SCT_COMMAND_NO_ERROR                = 0x00,
        FBE_SATA_SCT_COMMAND_ERROR                   = 0x04,
        FBE_SATA_SCT_COMMAND_STATUS_SUCCESS          = 0x50,
        FBE_SATA_SCT_COMMAND_STATUS_ABORT            = 0x51
}fbe_sata_sct_t;

typedef enum fbe_sata_write_uncorrectable_e{
        /* Defines for ATA WRITE CORRECTABLE EXT feature Set */
        FBE_SATA_PSEUDO_UNC_ERROR_W_LOGGING          = 0x55,
        FBE_SATA_FPSEUDO_UNC_ERROR_WO_LOGGING        = 0x5A,
        FBE_SATA_FLAGGED_ERROR_W_LOGGING             = 0xA5,
        FBE_SATA_FLAGGED_ERROR_WO_LOGGING            = 0xAA       
}fbe_sata_write_uncorrectable_t;

#endif /* FBE_SATA_INTERFACE_H */