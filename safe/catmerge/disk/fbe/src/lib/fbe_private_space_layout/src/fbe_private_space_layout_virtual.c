/*! @var fbe_private_space_layout_virttual
 *  
 *  @brief This structure defines the system's private space layout
 *         in a table of private space regions, and a table of
 *         private LUNs. The structure is empty for virtual platform
 *         because private space is not used.
 */
fbe_private_space_layout_t fbe_private_space_layout_virtual = 
{
    /* Private Space Regions */
    {
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
