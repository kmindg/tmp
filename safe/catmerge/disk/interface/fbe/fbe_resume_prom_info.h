#ifndef FBE_RESUME_PROM_INFO_H
#define FBE_RESUME_PROM_INFO_H

#include "fbe/fbe_devices.h"
#include "resume_prom.h"
#include "fbe_sg_element.h"

typedef enum fbe_fru_prom_id_e {
	FBE_FRU_PROM_ID_INVALID					= 0,
	FBE_LCC_A_RESUME_PROM					= 0x01,
	FBE_LCC_B_RESUME_PROM					= 0x02,
	FBE_POWER_SUPPLY_A_RESUME_PROM			= 0x03,
	FBE_POWER_SUPPLY_A_POWER_FAULT_REGISTER	= 0x04,
	FBE_POWER_SUPPLY_B_RESUME_PROM			= 0x05,
	FBE_POWER_SUPPLY_B_POWER_FAULT_REGISTER	= 0x06,
	FBE_MIDPLANE_RESUME_PROM				= 0x07,
	FBE_MIDPLANE_NVRAM						= 0x08,
	FBE_FRU_PROM_ID_LAST
} fbe_fru_prom_id_t;

#define	FBE_RESUME_PROM_READ_SIZE	32

/* WARNING! The #defs and structs below are copied from
 * catmerge/interface/cm_resume_prom_info.h.  This is an
 * ugly expedient made necessary because the fbe code
 * cannot include that file.
 */

#define FBE_EMC_TLA_PART_NUM_SIZE          16
#define FBE_EMC_TLA_ARTWORK_REV_SIZE       3
#define FBE_EMC_TLA_ASSEMBLY_REV_SIZE      3
#define FBE_EMPTY_SPACE_1_SIZE             2

#define FBE_RP_SEMAPHORE_SIZE              1
#define FBE_EMPTY_SPACE_2_SIZE             7

#define FBE_EMC_TLA_SERIAL_NUM_SIZE        16
#define FBE_EMPTY_SPACE_3_SIZE             80

#define FBE_VENDOR_PART_NUM_SIZE           32
#define FBE_VENDOR_ARTWORK_REV_SIZE        3
#define FBE_VENDOR_ASSEMBLY_REV_SIZE       3
#define FBE_EMPTY_SPACE_4_SIZE             10

#define FBE_VENDOR_SERIAL_NUM_SIZE         32 
#define FBE_EMPTY_SPACE_5_SIZE             48

#define FBE_VENDOR_NAME_SIZE               32
#define FBE_LOCATION_MANUFACTURE_SIZE      32
#define FBE_YEAR_MANUFACTURE_SIZE          4
#define FBE_MONTH_MANUFACTURE_SIZE         2
#define FBE_DAY_MANUFACTURE_SIZE           2
#define FBE_EMPTY_SPACE_6_SIZE             8

#define FBE_TLA_ASSEMBLY_NAME_SIZE         32
#define FBE_EMPTY_SPACE_7_SIZE            144

#define FBE_NUM_PROG_SIZE                  1
#define FBE_EMPTY_SPACE_8_SIZE             15

#define FBE_PROG_NAME_SIZE                 8
#define FBE_PROG_REV_SIZE                  4
#define FBE_PROG_CHECKSUM_SIZE             4
#define FBE_EMPTY_SPACE_9_SIZE             12

#define FBE_WWN_SEED_SIZE                  4
#define FBE_EMPTY_SPACE_10_SIZE            16

#define FBE_RESERVED_REGION_1_SIZE         1
#define FBE_EMPTY_SPACE_11_SIZE            4

#define FBE_PCBA_PART_NUM_SIZE             16
#define FBE_PCBA_ASSEMBLY_REV_SIZE         3
#define FBE_PCBA_SERIAL_NUM_SIZE           16
#define FBE_EMPTY_SPACE_12_SIZE            2

#define FBE_BLADE_MGMT_FRU_FAM_TYPE_SIZE   2
#define FBE_EMPTY_SPACE_13_SIZE            2

#define FBE_EMC_ALT_MB_PART_SIZE           16
#define FBE_EMPTY_SPACE_14_SIZE            4

#define FBE_CHANNEL_SPEED_SIZE             2
#define FBE_SAN_NAS_SIZE                   2
#define FBE_DAE_ENCL_ID_SIZE               8
#define FBE_DRIVE_SPIN_UP_SELECT_SIZE      2
#define FBE_SP_FAMILY_FRU_ID_SIZE          4
#define FBE_SYSTEM_ORIENTATION_SIZE        2
#define FBE_EMPTY_SPACE_15_SIZE            2

#define FBE_EMC_PART_NUM_SIZE              16
#define FBE_EMC_ARTWORK_REV_SIZE           3
#define FBE_EMC_ASSEMBLY_REV_SIZE          3
#define FBE_EMC_SERIAL_NUM_SIZE            16
#define FBE_EMPTY_SPACE_16_SIZE            14

#define FBE_RESUME_PROM_CHECKSUM_SIZE      4

/* Maximum programmables count as defined in the resume PROM Spec. */
#define	FBE_MAX_PROG_COUNT	84

#define FBE_RESUME_PROM_CHECKSUM_SEED	0x64656573	/* DEES */

#define INVALID_WWN_SEED        0xFFFFFFFF

/* Pack the following structures.
 */
#pragma pack(push, resume_prom_info, 1)

/* This structure is used to keep information about the programmables
 * on the resume PROMs
 */
typedef struct fbe_prog_details_s
{
	fbe_u8_t prog_name[FBE_PROG_NAME_SIZE];
	fbe_u8_t prog_rev[FBE_PROG_REV_SIZE];
	fbe_u32_t prog_checksum;
} fbe_prog_details_t;

typedef enum fbe_resume_prom_status_e {
    /* Resume PROM read operation has not been queued yet in ESP */
    FBE_RESUME_PROM_STATUS_UNINITIATED = 0,
    /* Resume PROM read operation has been queued in ESP */
    FBE_RESUME_PROM_STATUS_QUEUED,
    /* Only one resume PROM operation can be outstanding. The caller 
     * can retry the resume PROM operation after some time 
     */    
    FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS,
    /* The device is not present. Applicable only for 
     * Personality module (PM) Resume Prom if the PM is not inserted. 
     */    
    FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT,  
    /* The device is not valid for this platform */
    FBE_RESUME_PROM_STATUS_DEVICE_NOT_VALID_FOR_PLATFORM,
    /* Resume PROM operation completed successfully */ 
    FBE_RESUME_PROM_STATUS_READ_SUCCESS,
    /*
     * The status below is considered as the actual fault 
     */
    /* The resume PROM device is dead */    
    FBE_RESUME_PROM_STATUS_DEVICE_DEAD,
    /* Checksum error encountered during resume PROM read */    
    FBE_RESUME_PROM_STATUS_CHECKSUM_ERROR,
    /* The device returned an error, the caller can retry the command */
    FBE_RESUME_PROM_STATUS_DEVICE_ERROR,
    /* The buffer passed is too small */ 
    FBE_RESUME_PROM_STATUS_BUFFER_SMALL,
    /* Tried to access a field that either does not exist or 
     * does not have a valid value 
     */
    FBE_RESUME_PROM_STATUS_INVALID_FIELD,
    /* The field cannot be written */    
    FBE_RESUME_PROM_STATUS_FIELD_NOT_WRITABLE,
    /* The resume PROM device ID is not valid */    
    FBE_RESUME_PROM_STATUS_UNKNOWN_DEVICE_ID,
    /* SMB error is returned when there is a problem with the SMBUS. 
     * There is nothing the caller can do about this. The system should be 
     * rebooted to clear this error
     */
    FBE_RESUME_PROM_STATUS_SMBUS_ERROR,
    /* Not enough resource available. */
    FBE_RESUME_PROM_STATUS_INSUFFICIENT_RESOURCES,
    /* The request to the device has been timed out */
    FBE_RESUME_PROM_STATUS_DEVICE_TIMEOUT,
    /* SMBus Arbitration Error prevented read/write operation */
    FBE_RESUME_PROM_STATUS_ARB_ERROR,
    /* The resume PROM device is powered off */    
    FBE_RESUME_PROM_STATUS_DEVICE_POWERED_OFF,
    
    FBE_RESUME_PROM_STATUS_INVALID = 0xFF,
} fbe_resume_prom_status_t;

/* This structure is used to keep information about resume PROMs.
 * It exactly depicts the layout of the resume PROM.
 */
typedef struct fbe_resume_prom_info_s
{
	fbe_u8_t emc_tla_part_num[FBE_EMC_TLA_PART_NUM_SIZE];
	fbe_u8_t emc_tla_artwork_rev[FBE_EMC_TLA_ARTWORK_REV_SIZE];
	fbe_u8_t emc_tla_assembly_rev[FBE_EMC_TLA_ASSEMBLY_REV_SIZE];
	fbe_u8_t empty_space_1[FBE_EMPTY_SPACE_1_SIZE];

	fbe_u8_t rp_semaphore[FBE_RP_SEMAPHORE_SIZE];
	fbe_u8_t empty_space_2[FBE_EMPTY_SPACE_2_SIZE];

	fbe_u8_t emc_tla_serial_num[FBE_EMC_SERIAL_NUM_SIZE];
	fbe_u8_t empty_space_3[FBE_EMPTY_SPACE_3_SIZE];

	fbe_u8_t vendor_part_num[FBE_VENDOR_PART_NUM_SIZE];
	fbe_u8_t vendor_artwork_rev[FBE_VENDOR_ARTWORK_REV_SIZE];
	fbe_u8_t vendor_assembly_rev[FBE_VENDOR_ASSEMBLY_REV_SIZE];
	fbe_u8_t empty_space_4[FBE_EMPTY_SPACE_4_SIZE];

	fbe_u8_t vendor_serial_num[FBE_VENDOR_SERIAL_NUM_SIZE];
	fbe_u8_t empty_space_5[FBE_EMPTY_SPACE_5_SIZE];

	fbe_u8_t vendor_name[FBE_VENDOR_NAME_SIZE];
	fbe_u8_t loc_mft[FBE_LOCATION_MANUFACTURE_SIZE];
	fbe_u8_t year_mft[FBE_YEAR_MANUFACTURE_SIZE];
	fbe_u8_t month_mft[FBE_MONTH_MANUFACTURE_SIZE];
	fbe_u8_t day_mft[FBE_DAY_MANUFACTURE_SIZE];
	fbe_u8_t empty_space_6[FBE_EMPTY_SPACE_6_SIZE];

	fbe_u8_t tla_assembly_name[FBE_TLA_ASSEMBLY_NAME_SIZE];
	fbe_u8_t empty_space_7[FBE_EMPTY_SPACE_7_SIZE];

	fbe_u8_t num_prog[FBE_NUM_PROG_SIZE];
	fbe_u8_t empty_space_8[FBE_EMPTY_SPACE_8_SIZE];

	fbe_prog_details_t prog_details[FBE_MAX_PROG_COUNT];
	fbe_u8_t empty_space_9[FBE_EMPTY_SPACE_9_SIZE];

	fbe_u32_t wwn_seed;
	fbe_u8_t empty_space_10[FBE_EMPTY_SPACE_10_SIZE];

	fbe_u8_t reserved_region_1[FBE_RESERVED_REGION_1_SIZE];
	fbe_u8_t empty_space_11[FBE_EMPTY_SPACE_11_SIZE];

	fbe_u8_t pcba_part_num[FBE_PCBA_PART_NUM_SIZE];
	fbe_u8_t pcba_assembly_rev[FBE_PCBA_ASSEMBLY_REV_SIZE];
	fbe_u8_t pcba_serial_num[FBE_PCBA_SERIAL_NUM_SIZE];
	fbe_u8_t empty_space_12[FBE_EMPTY_SPACE_12_SIZE];

	fbe_u8_t blade_mgmt_fru_fam_type[FBE_BLADE_MGMT_FRU_FAM_TYPE_SIZE];
	fbe_u8_t empty_space_13[FBE_EMPTY_SPACE_13_SIZE];

	fbe_u8_t emc_alt_mb_part_num[FBE_EMC_ALT_MB_PART_SIZE];
	fbe_u8_t empty_space_14[FBE_EMPTY_SPACE_14_SIZE];

	fbe_u8_t channel_speed[FBE_CHANNEL_SPEED_SIZE];
	fbe_u8_t san_nas[FBE_SAN_NAS_SIZE];
	fbe_u8_t dae_encl_id[FBE_DAE_ENCL_ID_SIZE];
	fbe_u8_t drive_spin_up_select[FBE_DRIVE_SPIN_UP_SELECT_SIZE];
	fbe_u8_t sp_family_fru_id[FBE_SP_FAMILY_FRU_ID_SIZE];
	fbe_u8_t system_orientation[FBE_SYSTEM_ORIENTATION_SIZE];
	fbe_u8_t empty_space_15[FBE_EMPTY_SPACE_15_SIZE];
	
	fbe_u8_t emc_part_num[FBE_EMC_PART_NUM_SIZE];
	fbe_u8_t emc_artwork_rev[FBE_EMC_ARTWORK_REV_SIZE];
	fbe_u8_t emc_assembly_rev[FBE_EMC_ASSEMBLY_REV_SIZE];
	fbe_u8_t emc_serial_num[FBE_EMC_SERIAL_NUM_SIZE];
	fbe_u8_t empty_space_16[FBE_EMPTY_SPACE_16_SIZE];

	fbe_u32_t resume_prom_checksum;

} fbe_resume_prom_info_t;

#pragma pack(pop, resume_prom_info)

typedef void * fbe_resume_completion_context_t;
typedef fbe_status_t (* fbe_resume_completion_function_t) (void *getResumeAsync, fbe_resume_completion_context_t context);

/*!********************************************************************* 
 * @struct fbe_resume_prom_cmd_t 
 *  
 * @brief 
 * This struct is used for Resume prom write for the specified device. 
 * deviceType - Input 
 * location - Input 
 * resumePromField - Input 
 * offset - required when resumePromField is invalid(0). 
 * pBuffer - Input 
 * bufferSize - Input 
 * readOpStatus - Output 
 **********************************************************************/
typedef struct fbe_resume_prom_cmd_s
{
    fbe_u64_t               deviceType;      // Input
    fbe_device_physical_location_t  deviceLocation;  // Input
    RESUME_PROM_FIELD               resumePromField; // Input
    fbe_u32_t                       offset;  // Input
    fbe_u8_t                      * pBuffer;  // Input
    fbe_u32_t                       bufferSize;  // Input
    fbe_resume_prom_status_t        readOpStatus; // Output
    fbe_bool_t                      readResumeSize;// Input
} fbe_resume_prom_cmd_t;

typedef struct fbe_resume_prom_get_resume_async_s
{
    fbe_resume_prom_cmd_t            resumeReadCmd;
    fbe_resume_completion_function_t completion_function;         // fbe_base_env_resume_prom_completion_function
    fbe_resume_completion_context_t  completion_context;          // pBaseEnv
} fbe_resume_prom_get_resume_async_t;

typedef struct fbe_resume_prom_write_asynch_cmd_s
{
  fbe_u64_t                  deviceType;      // Input
  fbe_device_physical_location_t     deviceLocation;  // Input
  void                               *pBuffer;        //Input
  fbe_u32_t                          bufferSize;      //Input  
  fbe_u32_t                          offset;           //Input
  fbe_resume_prom_status_t           writeOpStatus;   //Output
}fbe_resume_prom_write_asynch_cmd_t;

typedef struct fbe_resume_prom_system_id_info_s
{
  fbe_u8_t                        rpPartNum[RESUME_PROM_PRODUCT_PN_SIZE];
  fbe_u8_t                        rpSerialNum[RESUME_PROM_PRODUCT_SN_SIZE];
  fbe_u8_t                        rpRev[RESUME_PROM_PRODUCT_REV_SIZE];
}fbe_resume_prom_system_id_info_t;

typedef struct fbe_resume_prom_write_resume_async_op_s
{
    fbe_sg_element_t sg_element[2];
    fbe_status_t status;
    fbe_status_t (* completion_function)(void *context, void * rpWriteAsynchOp);
    void *encl_mgmt;
    fbe_resume_prom_write_asynch_cmd_t   rpWriteAsynchCmd;
} fbe_resume_prom_write_resume_async_op_t;
#endif /* FBE_RESUME_PROM_INFO_H */
