#define FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS 25

#define FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS 38

/*! @enum fbe_private_space_layout_object_id_id_t
 *  @brief All of the defined System Object IDs
 */
typedef enum fbe_private_space_layout_object_id_e {
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST = 0,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE = 0,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST = 1,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0 = 1,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_1 = 2,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_2 = 3,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_3 = 4,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST = 4,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST = 5,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0_SPARE = 5,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_1_SPARE = 6,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_2_SPARE = 7,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_3_SPARE = 8,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_LAST = 8,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST = 9,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_0 = 9,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_1 = 10,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_2 = 11,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_3 = 12,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_LAST = 12,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST = 13,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR = 13,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_TRIPLE_MIRROR = 14,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG = 15,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG = 16,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG = 17,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST = 17,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST = 18,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_DDBS = 18,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_BIOS = 19,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_POST = 20,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_GPS = 21,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_BMC = 22,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_BIOS = 23,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_POST = 24,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_GPS = 25,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_BMC = 26,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_BIOS = 27,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_POST = 28,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_GPS = 29,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_BMC = 30,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE = 31,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA = 32,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METASTATISTICS = 33,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_0 = 34,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_1 = 35,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_4 = 36,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM = 37,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MLU_DB = 38,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_L1_CACHE = 39,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE = 40,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_A = 41,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B = 42,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CPL_A = 43,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CPL_B = 44,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_2 = 45,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_3 = 46,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_5 = 47,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_NP = 48,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_SYSTEM_CONFIG = 49,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX = 50,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_UDI_SYSTEM_POOL = 51,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CFDB_LUN = 52,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_AUDIT_LOG = 53,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN = 54,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST = 54,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST = 54
} fbe_private_space_layout_object_id_t;


/*! @enum fbe_private_space_layout_region_id_t
 *  @brief All of the defined private space region IDs
 */
typedef enum fbe_private_space_layout_region_id_e {
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID = 0,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_OLD_PDO_OBJECT_DATA,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_DDMI,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PDO_OBJECT_DATA,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_SIGNATURE,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ALL_FRU_RESERVED,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_KH_TEMPORARY_WORKAROUND,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_TRIPLE_MIRROR_RG,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_CBE_TRIPLE_MIRROR_RG,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_4_DRIVE_R5_RG,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SED_DATABASE,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_CHASSIS_REPLACEMENT_FLAGS,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_DESCRIPTOR,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_HOMEWRECKER_DB,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FUTURE_GROWTH_RESERVED,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_IMAGE_REPOSITORY,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPA,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPB,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PRIMARY_BOOT_VOLUME_SPA,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PRIMARY_BOOT_VOLUME_SPB,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_RESERVED_2,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_VAULT_RG,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_START_OF_USERSPACE_ALIGNMENT,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_LAST
} fbe_private_space_layout_region_id_t;


/*! @enum fbe_private_space_layout_rg_id_t
 *  @brief All of the defined private RG IDs
 */
typedef enum fbe_private_space_layout_rg_id_e {
    FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR = 1000,
    FBE_PRIVATE_SPACE_LAYOUT_RG_ID_CBE_TRIPLE_MIRROR = 1001,
    FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5 = 1002,
    FBE_PRIVATE_SPACE_LAYOUT_RG_ID_RAW_MIRROR = 1003,
    FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT = 1004,
    FBE_PRIVATE_SPACE_LAYOUT_RG_ID_INVALID = 0xFFFFFFFF
} fbe_private_space_layout_rg_id_t;

/*! @enum fbe_private_space_layout_lun_id_t
 *  @brief All of the defined private LUN IDs
 */
typedef enum fbe_private_space_layout_lun_id_e {
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_PSM = 8192,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_EFD_CACHE = 8193,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_L1_CACHE = 8194,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CPL_A = 8195,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CPL_B = 8196,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_A = 8197,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_B = 8198,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_0 = 8199,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_1 = 8200,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_2 = 8201,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_3 = 8202,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_4 = 8203,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_5 = 8204,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_DDBS = 8205,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_BIOS = 8206,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_POST = 8207,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_GPS = 8208,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_BMC = 8209,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_BIOS = 8210,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_POST = 8211,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_GPS = 8212,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_BMC = 8213,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_BIOS = 8214,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_POST = 8215,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_GPS = 8216,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_BMC = 8217,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_DATABASE = 8218,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_RAID_METADATA = 8219,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_RAID_METASTATISTICS = 8220,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MLU_DB = 8221,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP = 8222,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_SYSTEM_CONFIG = 8223,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VAULT = 8224,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CBE_LOCKBOX = 8225,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_UDI_SYSTEM_POOL = 8226,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CFDB = 8227,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CBE_AUDIT_LOG = 8228,
    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID = 0xFFFFFFFF
} fbe_private_space_layout_lun_id_t;

