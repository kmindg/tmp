#include "fbe_private_space_layout.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_emcutil_shell_include.h"

void get_raid_type(fbe_raid_group_type_t raid_type, fbe_char_t *string)
{
    switch (raid_type)
    {
		case FBE_RAID_GROUP_TYPE_RAID0:
			strcpy(string, "RAID 0");
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            strcpy(string, "RAID 10");
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
            strcpy(string, "RAID 1");
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            strcpy(string, "RAID 5");
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            strcpy(string, "RAID 3");
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            strcpy(string, "RAID 6");
            break;
        case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
            strcpy(string, "RAID 1 RAW");
            break;
        default:
            strcpy(string, "UNKNOWN");
            break;
    };
}

void get_region_type(fbe_private_space_layout_region_type_t region_type, fbe_char_t *string)
{
    switch (region_type)
    {
		case FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED:
			strcpy(string, "UNUSED");
            break;
		case FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP:
			strcpy(string, "RAID_GROUP");
            break;
		case FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE:
			strcpy(string, "EXTERNAL_USE");
            break;
        default:
            strcpy(string, "UNKNOWN");
            break;
    };
}

void usage(int argc , char *argv[])
{
    printf("USAGE: %s <layout>\n", argv[0]);
    printf("Supported Layouts:\n");
    printf("    rockies_hw\n");
    printf("    rockies_sim\n");
    printf("    kittyhawk_hw\n");
    printf("    kittyhawk_sim\n");
    printf("    haswell_hw\n");
    printf("    haswell_sim\n");
    printf("    meridian_virt\n");
    printf("    fbe_sim\n");
    exit(1);
}


int __cdecl main (int argc , char *argv[])
{
	fbe_status_t status;
	fbe_u32_t i;
	fbe_u32_t j;
	fbe_private_space_layout_region_t region;
	fbe_private_space_layout_lun_info_t lun;
    fbe_private_space_layout_extended_lun_info_t lun_ext;
	fbe_lba_t lba;
	fbe_char_t raid_type[16];
	fbe_char_t region_type[16];

#include "fbe/fbe_emcutil_shell_maincode.h"

    if(argc != 2) {
        usage(argc, argv);
    }
    else {
        if(!strcmp(argv[1], "rockies_hw") ) {
            fbe_private_space_layout_initialize_library(SPID_DEFIANT_M1_HW_TYPE);
        }
        else if(!strcmp(argv[1], "rockies_sim") ) {
            fbe_private_space_layout_initialize_library(SPID_DEFIANT_S1_HW_TYPE);
        }
        else if(!strcmp(argv[1], "kittyhawk_hw") ) {
            fbe_private_space_layout_initialize_library(SPID_NOVA1_HW_TYPE);
        }
        else if(!strcmp(argv[1], "kittyhawk_sim") ) {
            fbe_private_space_layout_initialize_library(SPID_NOVA_S1_HW_TYPE);
        }
        else if(!strcmp(argv[1], "haswell_hw") ) {
            fbe_private_space_layout_initialize_library(SPID_HYPERION_1_HW_TYPE);
        }
        else if(!strcmp(argv[1], "haswell_sim") ) {
            fbe_private_space_layout_initialize_library(SPID_OBERON_S1_HW_TYPE);
        }
        else if(!strcmp(argv[1], "meridian_virt") ) {
            fbe_private_space_layout_initialize_library(SPID_MERIDIAN_ESX_HW_TYPE);
        }
        else if(!strcmp(argv[1], "fbe_sim") ) {
            fbe_private_space_layout_initialize_fbe_sim();
        }
        else {
            usage(argc, argv);
        }
    }

	printf("<xml>\n");
	for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
	{
		status = fbe_private_space_layout_get_region_by_index(i, &region);
        if(status != FBE_STATUS_OK) {
            break;
        }
		if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
		{
            break;
        }
        printf("<region>\n");
        printf("    <region_id>%d</region_id>\n", region.region_id);
        printf("    <region_name>%s</region_name>\n", region.region_name);
        get_region_type(region.region_type, region_type);
        printf("    <region_type>%s</region_type>\n", region_type);	
        if(region.number_of_frus == FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS)
        {
            printf("    <number_of_frus>ALL</number_of_frus>\n");
        }
        else
        {
            printf("    <number_of_frus>%d</number_of_frus>\n", region.number_of_frus);
            for(j = 0; j < region.number_of_frus; j++)
            {
                printf("    <fru_id>%d</fru_id>\n", region.fru_id[j]);
            }
        }
        printf("    <starting_block_address>0x%llX</starting_block_address>\n",
	       (unsigned long long)region.starting_block_address);
        printf("    <size_in_blocks>0x%llX</size_in_blocks>\n",
	       (unsigned long long)region.size_in_blocks);
        if(region.region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP)
        {
            get_raid_type(region.raid_info.raid_type, raid_type);
            printf("    <raid_type>%s</raid_type>\n", raid_type);
            printf("    <raid_group_id>%d</raid_group_id>\n", region.raid_info.raid_group_id);
            printf("    <object_id>0x%X</object_id>\n", region.raid_info.object_id);
        }
        printf("</region>\n");
	}
	
	for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; i++ )
	{
		status = fbe_private_space_layout_get_lun_by_index(i, &lun);
        if(status != FBE_STATUS_OK) {
            break;
        }
		if(lun.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
		{
            break;
        }
        printf("<lun>\n");
        printf("    <lun_number>%d</lun_number>\n", lun.lun_number);
        printf("    <lun_name>%s</lun_name>\n", lun.lun_name);
        printf("    <object_id>0x%X</object_id>\n", lun.object_id);
        printf("    <ica_image_type>%d</ica_image_type>\n", lun.ica_image_type);
        printf("    <raid_group_id>%d</raid_group_id>\n", lun.raid_group_id);
        printf("    <raid_group_address_offset>0x%llX</raid_group_address_offset>\n",
	       (unsigned long long)lun.raid_group_address_offset);
        printf("    <internal_capacity>0x%llX</internal_capacity>\n",
	       (unsigned long long)lun.internal_capacity);
        printf("    <external_capacity>0x%llX</external_capacity>\n",
	       (unsigned long long)lun.external_capacity);
        printf("    <export_device_b>%d</export_device_b>\n", lun.export_device_b);
        printf("    <exported_device_name>%s</exported_device_name>\n", lun.exported_device_name);
        printf("    <lun_extended>\n");

        status = fbe_private_space_layout_get_lun_extended_info_by_lun_number(lun.lun_number, &lun_ext);

        if(lun_ext.number_of_frus == FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS)
        {
            printf("        <number_of_frus>ALL</number_of_frus>\n");
        }
        else
        {
            printf("        <number_of_frus>%d</number_of_frus>\n", lun_ext.number_of_frus);
            for(j = 0; j < lun_ext.number_of_frus; j++)
            {
                printf("        <fru_id>%d</fru_id>\n", lun_ext.fru_id[j]);
            }
        }
        printf("        <starting_block_address>0x%llX</starting_block_address>\n",
	       (unsigned long long)lun_ext.starting_block_address);
        printf("        <size_in_blocks>0x%llX</size_in_blocks>\n",
	       (unsigned long long)lun_ext.size_in_blocks);

        printf("    </lun_extended>\n");
        printf("</lun>\n");

    }

    printf("<minimum_system_drive_capacity>0x%llX</minimum_system_drive_capacity>\n", (unsigned long long)TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY);
    printf("<minimum_non_system_drive_capacity>0x%x</minimum_non_system_drive_capacity>\n", TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
    printf("<number_of_system_drives>%d</number_of_system_drives>\n", fbe_private_space_layout_get_number_of_system_drives());

    printf("<start_of_userspace>\n");
    for(i = 0; i < 16; i++ )
    {
        status = fbe_private_space_layout_get_start_of_user_space(i, &lba);
        printf("    <fru id=\"%d\">0x%llX</fru>\n", i, (unsigned long long)lba);
    }
    printf("</start_of_userspace>\n");
    printf("</xml>\n");
}
