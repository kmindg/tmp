/*! @var fbe_private_space_layout_haswell_sim
 *  
 *  @brief This structure defines the system's private space layout
 *         in a table of private space regions, and a table of
 *         private LUNs.
 */
fbe_private_space_layout_t fbe_private_space_layout_haswell_sim = 
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
            0x016D0800, /* Size */
            {
                FBE_RAID_GROUP_TYPE_RAID1,
                FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR,
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                0x016D0000, /* Exported Capacity */
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
            0x016E0800, /* Starting LBA */
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
            0x018FC800, /* Starting LBA */
            0x01479000, /* Size */
            {
                FBE_RAID_GROUP_TYPE_RAID5,
                FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5,
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG,
                0x03D39800, /* Exported Capacity */
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
            0x02D75800, /* Starting LBA */
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
            0x02D76000, /* Starting LBA */
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
            0x02D76800, /* Starting LBA */
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
            0x02D80800, /* Starting LBA */
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
            0x02D81000, /* Starting LBA */
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
            0x02D81800, /* Starting LBA */
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
            0x02D82000, /* Starting LBA */
            0x00064000, /* Size */
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_VAULT_RG,
            "Vault RG",
            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
            4, /* Number of FRUs */
            {
                0, 1, 2, 3, /* FRUs */
            },
            0x02DE6000, /* Starting LBA */
            0x000BC000, /* Size */
            {
                FBE_RAID_GROUP_TYPE_RAID3,
                FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT,
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG,
                0x00202800, /* Exported Capacity */
                FBE_PRIVATE_SPACE_LAYOUT_FLAG_ENCRYPTED_RG,  /* Set Encrypt/Un-Encrypt Flag for future usage */
            }
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
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_DATABASE,
            "MCR DATABASE",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00000000, /* Address Offset within RG */
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
            0x00201000, /* Address Offset within RG */
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
            0x0021B000, /* Address Offset within RG */
            0x0001A000, /* Internal Capacity */
            0x00019000, /* External Capacity */
            FBE_FALSE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_4,
            "VCX LUN 4",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_4, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x00235000, /* Address Offset within RG */
            0x00201000, /* Internal Capacity */
            0x00200000, /* External Capacity */
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
            0x00436000, /* Address Offset within RG */
            0x01001000, /* Internal Capacity */
            0x01000000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "Flare_PSM", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MLU_DB,
            "MLU_DB",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MLU_DB, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x01437000, /* Address Offset within RG */
            0x00201000, /* Internal Capacity */
            0x00200000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CLARiiON_MLU_DB", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_L1_CACHE,
            "L1_CACHE",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_L1_CACHE, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR, /* RG ID */
            0x01638000, /* Address Offset within RG */
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
            0x0169D000, /* Address Offset within RG */
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
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_5,
            "VCX LUN 5",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_5, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x00000000, /* Address Offset within RG */
            0x00102000, /* Internal Capacity */
            0x00100000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_UDI_SYSTEM_POOL,
            "UDI SYSTEM POOL",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_UDI_SYSTEM_POOL, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x00102000, /* Address Offset within RG */
            0x03C03000, /* Internal Capacity */
            0x03C00000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "", /* Device Name */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION, /* Flags */
            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
        },
        {
            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CFDB,
            "CFDB",
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CFDB_LUN, /* Object ID */
            FBE_IMAGING_IMAGE_TYPE_INVALID,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5, /* RG ID */
            0x03D05000, /* Address Offset within RG */
            0x00034800, /* Internal Capacity */
            0x00032000, /* External Capacity */
            FBE_TRUE, /* Export as Device */
            "CFDB", /* Device Name */
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
            0x00202800, /* Internal Capacity */
            0x00200000, /* External Capacity */
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
