/*! @var fbe_private_space_layout_rockies_hw
 *  
 *  @brief This structure defines the system's private space layout
 *         in a table of private space regions, and a table of
 *         private LUNs.
 */
fbe_private_space_layout_t fbe_private_space_layout_rockies_hw = 
{
    /* Private Space Regions */
    {
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_OLD_PDO_OBJECT_DATA,
            "OLD_PDO_OBJECT_DATA",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,
            FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS, /* Number of FRUs */
            {
                0, /* FRUs */
            },
            0x00000000, /* Starting LBA */
            0x00001000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_DDMI,
            "DDMI",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS, /* Number of FRUs */
            {
                0, /* FRUs */
            },
            0x00001000, /* Starting LBA */
            0x00001000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PDO_OBJECT_DATA,
            "PDO_OBJECT_DATA",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS, /* Number of FRUs */
            {
                0, /* FRUs */
            },
            0x00002000, /* Starting LBA */
            0x00005000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_SIGNATURE,
            "FRU_SIGNATURE",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS, /* Number of FRUs */
            {
                0, /* FRUs */
            },
            0x00007000, /* Starting LBA */
            0x00000800, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ALL_FRU_RESERVED,
            "All FRU Reserved Space",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,
            FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS, /* Number of FRUs */
            {
                0, /* FRUs */
            },
            0x00007800, /* Starting LBA */
            0x00008800, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_TRIPLE_MIRROR_RG,
            "Triple Mirror RG",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x00010000, /* Starting LBA */
            0x05EBA800, /* Size */
            {
                FBE_RAID_GROUP_TYPE_RAID1,
                FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR,
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                0x05EBA000, /* Exported Capacity */
                FBE_PRIVATE_SPACE_LAYOUT_FLAG_ENCRYPTED_RG,  /* Set Encrypt/Un-Encrypt Flag for future usage */
            }
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_CBE_TRIPLE_MIRROR_RG,
            "CBE Mirror RG",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x05ECA800, /* Starting LBA */
            0x0021C000, /* Size */
            {
                FBE_RAID_GROUP_TYPE_RAID1,
                FBE_PRIVATE_SPACE_LAYOUT_RG_ID_CBE_TRIPLE_MIRROR,
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_TRIPLE_MIRROR,
                0x0021B800, /* Exported Capacity */
                FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE,  /* Set Encrypt/Un-Encrypt Flag for future usage */
            }
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_4_DRIVE_R5_RG,
            "Parity RG",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
            4, /* Number of FRUs */
            {
                0, 1, 2, 3, /* FRUs */
            },
            0x060E6800, /* Starting LBA */
            0x02F40800, /* Size */
            {
                FBE_RAID_GROUP_TYPE_RAID5,
                FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5,
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
                0x08D8F800, /* Exported Capacity */
                FBE_PRIVATE_SPACE_LAYOUT_FLAG_ENCRYPTED_RG,  /* Set Encrypt/Un-Encrypt Flag for future usage */
            }
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS,
            "ICA_FLAGS",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x09027000, /* Starting LBA */
            0x00000800, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SED_DATABASE,
            "SED Database",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x09027800, /* Starting LBA */
            0x00000800, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG,
            "SEP_SYSTEM_DB",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x09028000, /* Starting LBA */
            0x0000A000, /* Size */
            {
                FBE_RAID_GROUP_TYPE_RAW_MIRROR,
                FBE_PRIVATE_SPACE_LAYOUT_RG_ID_RAW_MIRROR,
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG,
                0x00004800, /* Exported Capacity */
                FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE,  /* Set Encrypt/Un-Encrypt Flag for future usage */
            }
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_CHASSIS_REPLACEMENT_FLAGS,
            "CHASSIS_REPLACEMENT_FLAGS",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x09032000, /* Starting LBA */
            0x00000800, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_DESCRIPTOR,
            "FRU_DESCRIPTOR",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x09032800, /* Starting LBA */
            0x00000800, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_HOMEWRECKER_DB,
            "HOMEWRECKER_DB",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x09033000, /* Starting LBA */
            0x00000800, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FUTURE_GROWTH_RESERVED,
            "FUTURE_GROWTH_RESERVED",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,
            4, /* Number of FRUs */
            {
                0, 1, 2, 3, /* FRUs */
            },
            0x09033800, /* Starting LBA */
            0x00064000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_IMAGE_REPOSITORY,
            "DD_MS_ICA_IMAGE_REP_TYPE",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            3, /* Number of FRUs */
            {
                0, 1, 2, /* FRUs */
            },
            0x09097800, /* Starting LBA */
            0x01400000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPA,
            "UTILITY_BOOT_VOLUME_SPA",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            2, /* Number of FRUs */
            {
                1, 3, /* FRUs */
            },
            0x0A497800, /* Starting LBA */
            0x01400000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPB,
            "UTILITY_BOOT_VOLUME_SPB",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            2, /* Number of FRUs */
            {
                0, 2, /* FRUs */
            },
            0x0A497800, /* Starting LBA */
            0x01400000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PRIMARY_BOOT_VOLUME_SPA,
            "PRIMARY_BOOT_VOLUME_SPA",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            2, /* Number of FRUs */
            {
                0, 2, /* FRUs */
            },
            0x0B897800, /* Starting LBA */
            0x10F00000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PRIMARY_BOOT_VOLUME_SPB,
            "PRIMARY_BOOT_VOLUME_SPB",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
            2, /* Number of FRUs */
            {
                1, 3, /* FRUs */
            },
            0x0B897800, /* Starting LBA */
            0x10F00000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_RESERVED_2,
            "RESERVED_2",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,
            4, /* Number of FRUs */
            {
                0, 1, 2, 3, /* FRUs */
            },
            0x1C797800, /* Starting LBA */
            0x00100000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_VAULT_RG,
            "Vault RG",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
            4, /* Number of FRUs */
            {
                0, 1, 2, 3, /* FRUs */
            },
            0x1C897800, /* Starting LBA */
            0x05011800, /* Size */
            {
                FBE_RAID_GROUP_TYPE_RAID3,
                FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT,
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG,
                0x0F003000, /* Exported Capacity */
                FBE_PRIVATE_SPACE_LAYOUT_FLAG_ENCRYPTED_RG,  /* Set Encrypt/Un-Encrypt Flag for future usage */
            }
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_START_OF_USERSPACE_ALIGNMENT,
            "USERSPACE_ALIGN",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,
            4, /* Number of FRUs */
            {
                0, 1, 2, 3, /* FRUs */
            },
            0x218A9000, /* Starting LBA */
            0x00024800, /* Size */
        },
        /* Terminator */
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID,
            "",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,
            0, /* Number of FRUs */
            {
                0, /* FRUs */
            },
            0x0, /* Starting LBA */
            0x0, /* Size in Blocks */
        },
    },

    /* Private Space LUNs */
    {
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_DDBS,
            "DDBS Library",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_DDBS, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_DDBS,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00000000, /* Address Offset within RG */
            0x00002000, /* Internal Capacity */
            0x00001000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_DDBS", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_BIOS,
            "Jetfire BIOS",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_BIOS, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_JETFIRE_BIOS,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00002000, /* Address Offset within RG */
            0x00009000, /* Internal Capacity */
            0x00008000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_JETFIRE_BIOS", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_POST,
            "Jetfire POST",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_POST, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_JETFIRE_POST,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x0000B000, /* Address Offset within RG */
            0x00009000, /* Internal Capacity */
            0x00008000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_JETFIRE_POST", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_GPS,
            "Jetfire GPS",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_GPS, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_JETFIRE_GPS,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00014000, /* Address Offset within RG */
            0x00033000, /* Internal Capacity */
            0x00032000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_JETFIRE_GPS", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_BMC,
            "Jetfire BMC",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_BMC, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_JETFIRE_BMC,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00047000, /* Address Offset within RG */
            0x00011000, /* Internal Capacity */
            0x00010000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_JETFIRE_BMC", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_BIOS,
            "Megatron BIOS",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_BIOS, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_MEGATRON_BIOS,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00058000, /* Address Offset within RG */
            0x00009000, /* Internal Capacity */
            0x00008000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_MEGATRON_BIOS", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_POST,
            "Megatron POST",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_POST, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_MEGATRON_POST,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00061000, /* Address Offset within RG */
            0x00009000, /* Internal Capacity */
            0x00008000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_MEGATRON_POST", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_GPS,
            "Megatron GPS",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_GPS, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_MEGATRON_GPS,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x0006A000, /* Address Offset within RG */
            0x00033000, /* Internal Capacity */
            0x00032000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_MEGATRON_GPS", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_BMC,
            "Megatron BMC",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_BMC, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_MEGATRON_BMC,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x0009D000, /* Address Offset within RG */
            0x00011000, /* Internal Capacity */
            0x00010000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_MEGATRON_BMC", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_BIOS,
            "Futures BIOS",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_BIOS, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_FUTURES_BIOS,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x000AE000, /* Address Offset within RG */
            0x00009000, /* Internal Capacity */
            0x00008000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_FUTURES_BIOS", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_POST,
            "Futures POST",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_POST, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_FUTURES_POST,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x000B7000, /* Address Offset within RG */
            0x00009000, /* Internal Capacity */
            0x00008000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_FUTURES_POST", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_GPS,
            "Futures GPS",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_GPS, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_FUTURES_GPS,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x000C0000, /* Address Offset within RG */
            0x00033000, /* Internal Capacity */
            0x00032000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_FUTURES_GPS", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_BMC,
            "Futures BMC",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_BMC, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_FUTURES_BMC,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x000F3000, /* Address Offset within RG */
            0x00011000, /* Internal Capacity */
            0x00010000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_FUTURES_BMC", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_DATABASE,
            "MCR DATABASE",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00104000, /* Address Offset within RG */
            0x00201000, /* Internal Capacity */
            0x00200000, /* External Capacity */
            FBE_FALSE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_RAID_METADATA,
            "RAID_METADATA",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00305000, /* Address Offset within RG */
            0x0001A000, /* Internal Capacity */
            0x00019000, /* External Capacity */
            FBE_FALSE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_RAID_METASTATISTICS,
            "RAID_METASTATISTICS",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METASTATISTICS, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x0031F000, /* Address Offset within RG */
            0x0001A000, /* Internal Capacity */
            0x00019000, /* External Capacity */
            FBE_FALSE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_0,
            "VCX LUN 0",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_0, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00339000, /* Address Offset within RG */
            0x02C01000, /* Internal Capacity */
            0x02C00000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_1,
            "VCX LUN 1",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_1, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x02F3A000, /* Address Offset within RG */
            0x01601000, /* Internal Capacity */
            0x01600000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_4,
            "VCX LUN 4",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_4, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x0453B000, /* Address Offset within RG */
            0x00801000, /* Internal Capacity */
            0x00800000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_PSM,
            "PSM",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x04D3C000, /* Address Offset within RG */
            0x01001000, /* Internal Capacity */
            0x01000000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "Flare_PSM", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_L1_CACHE,
            "L1_CACHE",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_L1_CACHE, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x05D3D000, /* Address Offset within RG */
            0x00065000, /* Internal Capacity */
            0x00064000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_CDR", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_EFD_CACHE,
            "EFD_CACHE",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x05DA2000, /* Address Offset within RG */
            0x00033000, /* Internal Capacity */
            0x00032000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CBE_LOCKBOX,
            "CBE_LOCKBOX",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_CBE_TRIPLE_MIRROR, /* RG ID */
            0x00000000, /* Address Offset within RG */
            0x0001A000, /* Internal Capacity */
            0x00019000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CBE_LOCKBOX", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES_MR1_CBE, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CBE_AUDIT_LOG,
            "CBE_AUDIT_LOG",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_AUDIT_LOG, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_CBE_TRIPLE_MIRROR, /* RG ID */
            0x0001A000, /* Address Offset within RG */
            0x00201000, /* Internal Capacity */
            0x00200000, /* External Capacity */
            FBE_FALSE, /* Export as Device */
            "CBE_AUDIT_LOG", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_A,
            "WIL A",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_A, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x00000000, /* Address Offset within RG */
            0x00042000, /* Internal Capacity */
            0x00040000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_B,
            "WIL B",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x00042000, /* Address Offset within RG */
            0x00042000, /* Internal Capacity */
            0x00040000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CPL_A,
            "CPL A",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CPL_A, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x00084000, /* Address Offset within RG */
            0x00202800, /* Internal Capacity */
            0x00200000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CPL_B,
            "CPL B",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CPL_B, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x00286800, /* Address Offset within RG */
            0x00202800, /* Internal Capacity */
            0x00200000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_2,
            "VCX LUN 2",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_2, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x00489000, /* Address Offset within RG */
            0x00402000, /* Internal Capacity */
            0x00400000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_3,
            "VCX LUN 3",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_3, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x0088B000, /* Address Offset within RG */
            0x00402000, /* Internal Capacity */
            0x00400000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_5,
            "VCX LUN 5",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_5, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x00C8D000, /* Address Offset within RG */
            0x08002800, /* Internal Capacity */
            0x08000000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP,
            "MCR Raw Mirror NP LUN",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_NP, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_RAW_MIRROR, /* RG ID */
            0x00000000, /* Address Offset within RG */
            0x00001800, /* Internal Capacity */
            0x00000800, /* External Capacity */
            FBE_FALSE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_SYSTEM_CONFIG,
            "MCR Raw Mirror System Config LUN",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_SYSTEM_CONFIG, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_RAW_MIRROR, /* RG ID */
            0x00001800, /* Address Offset within RG */
            0x00003000, /* Internal Capacity */
            0x00002000, /* External Capacity */
            FBE_FALSE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VAULT,
            "Cache Vault",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT, /* RG ID */
            0x00000000, /* Address Offset within RG */
            0x0F003000, /* Internal Capacity */
            0x0F000000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_VAULT", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        /* Terminator*/
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID,
            "",
            0x0,                /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            0x0,                /* RG ID */
            0x0,                /* Address Offset within RG */
            0x0,                /* Internal Capacity */
            0x0,                /* External Capacity */
            FBE_FALSE,          /* Export as Device */
            "",                 /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
       },
    }
};
