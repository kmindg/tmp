#!/usr/bin/perl

use strict;
use Data::Dumper;
use Storable qw(dclone);

use constant MEGABYTES  => 0x800;
use constant GIGABYTES  => 0x200000;

use constant CHUNK_SIZE => 1 * MEGABYTES;

use constant BIOS_SIZE  => {
   rockies_hw     => 16 * MEGABYTES,
   rockies_sim    => 16 * MEGABYTES,
   kittyhawk_hw   => 0 * MEGABYTES,
   kittyhawk_sim  => 0 * MEGABYTES,
   haswell_hw     => 0 * MEGABYTES,
   haswell_sim    => 0 * MEGABYTES,
   meridian_virt  => 0 * MEGABYTES,
   fbe_sim        => 0 * MEGABYTES,
};

use constant POST_SIZE  => {
   rockies_hw     => 16 * MEGABYTES,
   rockies_sim    => 16 * MEGABYTES,
   kittyhawk_hw   => 0 * MEGABYTES,
   kittyhawk_sim  => 0 * MEGABYTES,
   haswell_hw     => 0 * MEGABYTES,
   haswell_sim    => 0 * MEGABYTES,
   meridian_virt  => 0 * MEGABYTES,
   fbe_sim        => 0 * MEGABYTES,
};

use constant GPS_SIZE   => {
   rockies_hw     => 100 * MEGABYTES,
   rockies_sim    => 100 * MEGABYTES,
   kittyhawk_hw   => 0 * MEGABYTES,
   kittyhawk_sim  => 0 * MEGABYTES,
   haswell_hw     => 0 * MEGABYTES,
   haswell_sim    => 0 * MEGABYTES,
   meridian_virt  => 0 * MEGABYTES,
   fbe_sim        => 0 * MEGABYTES,
};

use constant BMC_SIZE   => {
   rockies_hw     => 32 * MEGABYTES,
   rockies_sim    => 32 * MEGABYTES,
   kittyhawk_hw   => 0 * MEGABYTES,
   kittyhawk_sim  => 0 * MEGABYTES,
   haswell_hw     => 0 * MEGABYTES,
   haswell_sim    => 0 * MEGABYTES,
   meridian_virt  => 0 * MEGABYTES,
   fbe_sim        => 0 * MEGABYTES,
};

my $system_disk_count = 4;

my $master_psl = [
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_OLD_PDO_OBJECT_DATA",
        Name => "OLD_PDO_OBJECT_DATA",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED",
        Size => 2 * MEGABYTES,
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_DDMI",
        Name => "DDMI",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => 2 * MEGABYTES,
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PDO_OBJECT_DATA",
        Name => "PDO_OBJECT_DATA",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => 10 * MEGABYTES,
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_SIGNATURE",
        Name => "FRU_SIGNATURE",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => 1 * MEGABYTES,
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ALL_FRU_RESERVED",
        Name => "All FRU Reserved Space",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED",
        Size => 17 * MEGABYTES,
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_KH_TEMPORARY_WORKAROUND",
        Name => "KH_TEMPORARY_WORKAROUND",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED",
        Size => {
                  rockies_hw     => 0,
                  rockies_sim    => 0,
                  kittyhawk_hw   => 0,
                  kittyhawk_sim  => 0,
                  meridian_virt  => 0,
               },
        Frus => [0,1,2,3],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_TRIPLE_MIRROR_RG",
        Name => "Triple Mirror RG",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP",
        Frus => [0,1,2],
        RAIDType => "FBE_RAID_GROUP_TYPE_RAID1",
        RGID => "FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR",
        ObjectID => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR",
        Flags => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_ENCRYPTED_RG", # Set Encrytion Flag for future usage
        ReservedSize => {
                  rockies_hw     => 458 * MEGABYTES,
                  rockies_sim    => 0 * MEGABYTES,
                  kittyhawk_hw   => 458 * MEGABYTES,
                  kittyhawk_sim  => 0 * MEGABYTES,
                  haswell_hw     => 458 * MEGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 8 * MEGABYTES,
               },
        LUNs => [
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_DDBS",
                Number          => 8205,
                Name            => "DDBS Library",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_DDBS",
                ObjectID_Number => 18,
                DeviceName      => "CLARiiON_DDBS",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_DDBS",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => {
                  rockies_hw     => 2 * MEGABYTES,
                  rockies_sim    => 0 * MEGABYTES,
                  kittyhawk_hw   => 2 * MEGABYTES,
                  kittyhawk_sim  => 0 * MEGABYTES,
                  haswell_hw     => 2 * MEGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
            ####
            #### JETFIRE FIRMWARE
            ####
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_BIOS",
                Number          => 8206,
                Name            => "Jetfire BIOS",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_BIOS",
                ObjectID_Number => 19,
                DeviceName      => "CLARiiON_JETFIRE_BIOS",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_JETFIRE_BIOS",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => BIOS_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_POST",
                Number          => 8207,
                Name            => "Jetfire POST",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_POST",
                ObjectID_Number => 20,
                DeviceName      => "CLARiiON_JETFIRE_POST",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_JETFIRE_POST",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => POST_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_GPS",
                Number          => 8208,
                Name            => "Jetfire GPS",
                ObjectID_Number => 21,
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_GPS",
                DeviceName      => "CLARiiON_JETFIRE_GPS",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_JETFIRE_GPS",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => GPS_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_JETFIRE_BMC",
                Number          => 8209,
                Name            => "Jetfire BMC",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_JETFIRE_BMC",
                ObjectID_Number => 22,
                DeviceName      => "CLARiiON_JETFIRE_BMC",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_JETFIRE_BMC",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => BMC_SIZE,
            },
            ####
            #### MEGATRON FIRMWARE
            ####
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_BIOS",
                Number          => 8210,
                Name            => "Megatron BIOS",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_BIOS",
                ObjectID_Number => 23,
                DeviceName      => "CLARiiON_MEGATRON_BIOS",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_MEGATRON_BIOS",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => BIOS_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_POST",
                Number          => 8211,
                Name            => "Megatron POST",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_POST",
                ObjectID_Number => 24,
                DeviceName      => "CLARiiON_MEGATRON_POST",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_MEGATRON_POST",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => POST_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_GPS",
                Number          => 8212,
                Name            => "Megatron GPS",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_GPS",
                ObjectID_Number => 25,
                DeviceName      => "CLARiiON_MEGATRON_GPS",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_MEGATRON_GPS",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => GPS_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MEGATRON_BMC",
                Number          => 8213,
                Name            => "Megatron BMC",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MEGATRON_BMC",
                ObjectID_Number => 26,
                DeviceName      => "CLARiiON_MEGATRON_BMC",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_MEGATRON_BMC",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => BMC_SIZE,
            },
            ####
            #### FUTURES FIRMWARE
            ####
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_BIOS",
                Number          => 8214,
                Name            => "Futures BIOS",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_BIOS",
                ObjectID_Number => 27,
                DeviceName      => "CLARiiON_FUTURES_BIOS",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_FUTURES_BIOS",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => BIOS_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_POST",
                Number          => 8215,
                Name            => "Futures POST",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_POST",
                ObjectID_Number => 28,
                DeviceName      => "CLARiiON_FUTURES_POST",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_FUTURES_POST",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => POST_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_GPS",
                Number          => 8216,
                Name            => "Futures GPS",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_GPS",
                ObjectID_Number => 29,
                DeviceName      => "CLARiiON_FUTURES_GPS",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_FUTURES_GPS",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => GPS_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_FUTURES_BMC",
                Number          => 8217,
                Name            => "Futures BMC",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FUTURES_BMC",
                ObjectID_Number => 30,
                DeviceName      => "CLARiiON_FUTURES_BMC",
                ImageType       => "FBE_IMAGING_IMAGE_TYPE_FUTURES_BMC",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => BMC_SIZE,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_DATABASE",
                Number          => 8218,
                Name            => "MCR DATABASE",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE",
                ObjectID_Number => 31,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL",
                Export          => 0,
                Size            => {
                  rockies_hw     => 1 * GIGABYTES,
                  rockies_sim    => 1 * GIGABYTES,
                  kittyhawk_hw   => 1 * GIGABYTES,
                  kittyhawk_sim  => 1 * GIGABYTES,
                  haswell_hw     => 1 * GIGABYTES,
                  haswell_sim    => 1 * GIGABYTES,
                  meridian_virt  => 1 * GIGABYTES,
                  fbe_sim        => 20 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_RAID_METADATA",
                Number          => 8219,
                Name            => "RAID_METADATA",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA",
                ObjectID_Number => 32,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL",
                Export          => 0,
                Size            => 50 * MEGABYTES,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_RAID_METASTATISTICS",
                Number          => 8220,
                Name            => "RAID_METASTATISTICS",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METASTATISTICS",
                ObjectID_Number => 33,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL",
                Export          => 0,
                Size            => 50 * MEGABYTES,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_0",
                Number          => 8199,
                Name            => "VCX LUN 0",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_0",
                ObjectID_Number => 34,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION",
                Export          => 1,
                Size            => {
                  rockies_hw     => 22 * GIGABYTES,
                  rockies_sim    => 22 * GIGABYTES,
                  kittyhawk_hw   => 0 * MEGABYTES,
                  kittyhawk_sim  => 0 * MEGABYTES,
                  haswell_hw     => 0 * MEGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_1",
                Number          => 8200,
                Name            => "VCX LUN 1",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_1",
                ObjectID_Number => 35,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION",
                Export          => 1,
                Size            => {
                  rockies_hw     => 11 * GIGABYTES,
                  rockies_sim    => 11 * GIGABYTES,
                  kittyhawk_hw   => 0 * MEGABYTES,
                  kittyhawk_sim  => 0 * MEGABYTES,
                  haswell_hw     => 0 * MEGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_4",
                Number          => 8203,
                Name            => "VCX LUN 4",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_4",
                ObjectID_Number => 36,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION",
                Export          => 1,
                Size            => {
                  rockies_hw     => 4 * GIGABYTES,
                  rockies_sim    => 4 * GIGABYTES,
                  kittyhawk_hw   => 6 * GIGABYTES,
                  kittyhawk_sim  => 1 * GIGABYTES,
                  haswell_hw     => 6 * GIGABYTES,
                  haswell_sim    => 1 * GIGABYTES,
                  meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_PSM",
                Number          => 8192,
                Name            => "PSM",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM",
                ObjectID_Number => 37,
                DeviceName      => "Flare_PSM",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL",
                Export          => 1,
                Size            => {
                  rockies_hw     => 8 * GIGABYTES,
                  rockies_sim    => 8 * GIGABYTES,
                  kittyhawk_hw   => 24 * GIGABYTES,
                  kittyhawk_sim  => 8 * GIGABYTES,
                  haswell_hw     => 24 * GIGABYTES,
                  haswell_sim    => 8 * GIGABYTES,
                  meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MLU_DB",
                Number          => 8221,
                Name            => "MLU_DB",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MLU_DB",
                ObjectID_Number => 38,
                DeviceName      => "CLARiiON_MLU_DB",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL",
                Export          => 1,
                Size            => {
                  rockies_hw     => 0 * GIGABYTES,
                  rockies_sim    => 0 * GIGABYTES,
                  kittyhawk_hw   => 1 * GIGABYTES,
                  kittyhawk_sim  => 1 * GIGABYTES,
                  haswell_hw     => 1 * GIGABYTES,
                  haswell_sim    => 1 * GIGABYTES,
	          meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_L1_CACHE",
                Number          => 8194,
                Name            => "L1_CACHE",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_L1_CACHE",
                ObjectID_Number => 39,
                DeviceName      => "CLARiiON_CDR",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL",
                Export          => 1,
                Size            => {
                  rockies_hw     => 200 * MEGABYTES,
                  rockies_sim    => 200 * MEGABYTES,
                  kittyhawk_hw   => 200 * MEGABYTES,
                  kittyhawk_sim  => 200 * MEGABYTES,
                  haswell_hw     => 200 * MEGABYTES,
                  haswell_sim    => 200 * MEGABYTES,
 		  meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_EFD_CACHE",
                Number          => 8193,
                Name            => "EFD_CACHE",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_EFD_CACHE",
                ObjectID_Number => 40,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL",
                Export          => 1,
                Size            => {
                  rockies_hw     => 100 * MEGABYTES,
                  rockies_sim    => 100 * MEGABYTES,
                  kittyhawk_hw   => 100 * MEGABYTES,
                  kittyhawk_sim  => 100 * MEGABYTES,
                  haswell_hw     => 100 * MEGABYTES,
                  haswell_sim    => 100 * MEGABYTES,
                  meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
        ],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_CBE_TRIPLE_MIRROR_RG",
        Name => "CBE Mirror RG",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP",
        Frus => [0,1,2],
        RAIDType => "FBE_RAID_GROUP_TYPE_RAID1",
        RGID => "FBE_PRIVATE_SPACE_LAYOUT_RG_ID_CBE_TRIPLE_MIRROR",
        ObjectID => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_TRIPLE_MIRROR",
        Flags => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE", # Set Un-Encrytion Flag for future usage
        ReservedSize => {
                  rockies_hw     => 1 * MEGABYTES,
                  rockies_sim    => 1 * MEGABYTES,
                  kittyhawk_hw   => 1 * MEGABYTES,
                  kittyhawk_sim  => 1 * MEGABYTES,
                  haswell_hw     => 1 * MEGABYTES,
                  haswell_sim    => 1 * MEGABYTES,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 1 * MEGABYTES,
               },
        LUNs => [
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CBE_LOCKBOX",
                Number          => 8225,
                Name            => "CBE_LOCKBOX",
                DeviceName      => "CBE_LOCKBOX",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_LOCKBOX",
                ObjectID_Number => 50,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                CommitLevel     => "FLARE_COMMIT_LEVEL_ROCKIES_MR1_CBE",
                Export          => 1,
                Size            => {
                  rockies_hw     => 50 * MEGABYTES,
                  rockies_sim    => 50 * MEGABYTES,
                  kittyhawk_hw   => 50 * MEGABYTES,
                  kittyhawk_sim  => 50 * MEGABYTES,
                  haswell_hw     => 50 * MEGABYTES,
                  haswell_sim    => 50 * MEGABYTES,
                  meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CBE_AUDIT_LOG",
                Number          => 8228,
                Name            => "CBE_AUDIT_LOG",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_AUDIT_LOG",
                ObjectID_Number => 53,
                DeviceName      => "CBE_AUDIT_LOG",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL",
                Export          => 0,
                Size            => {
                  rockies_hw     => 1 * GIGABYTES,
                  rockies_sim    => 1 * GIGABYTES,
                  kittyhawk_hw   => 0 * GIGABYTES,
                  kittyhawk_sim  => 0 * GIGABYTES,
                  haswell_hw     => 1 * GIGABYTES,
                  haswell_sim    => 1 * GIGABYTES,
                  meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 2 * MEGABYTES,
               },
            },
        ],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_4_DRIVE_R5_RG",
        Name => "Parity RG",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP",
        Frus => {
            rockies_hw     => [0,1,2,3],
            rockies_sim    => [0,1,2,3],
            kittyhawk_hw   => [0,1,2,3],
            kittyhawk_sim  => [0,1,2,3],
            haswell_hw     => [0,1,2,3],
            haswell_sim    => [0,1,2,3],
            meridian_virt  => [0,1,2,3],
            fbe_sim        => [0,1,2,3],
        },
        RAIDType => {
            rockies_hw     => "FBE_RAID_GROUP_TYPE_RAID5",
            rockies_sim    => "FBE_RAID_GROUP_TYPE_RAID5",
            kittyhawk_hw   => "FBE_RAID_GROUP_TYPE_RAID5",
            kittyhawk_sim  => "FBE_RAID_GROUP_TYPE_RAID5",
            haswell_hw     => "FBE_RAID_GROUP_TYPE_RAID5",
            haswell_sim    => "FBE_RAID_GROUP_TYPE_RAID5",
            meridian_virt  => "FBE_RAID_GROUP_TYPE_UNUSED",
            fbe_sim        => "FBE_RAID_GROUP_TYPE_RAID5"
        },
        RGID => "FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5",
        ObjectID => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_4_DRIVE_R5_RG",
        Flags => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_ENCRYPTED_RG", # Set Encrytion Flag for future usage
        ReservedSize => {
                  rockies_hw     => 0.5 * GIGABYTES,
                  rockies_sim    => 0 * MEGABYTES,
                  kittyhawk_hw   => 0.5 * GIGABYTES,
                  kittyhawk_sim  => 0 * MEGABYTES,
                  haswell_hw     => 0.5 * MEGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 1 * MEGABYTES,
               },
        LUNs => [
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_A",
                Number          => 8197,
                Name            => "WIL A",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_A",
                ObjectID_Number => 41,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE",
                Export          => 1,
                Size => {
                    rockies_hw     => 128 * MEGABYTES,
                    rockies_sim    => 128 * MEGABYTES,
                    kittyhawk_hw   => 0,
                    kittyhawk_sim  => 0,
                    haswell_hw     => 0,
                    haswell_sim    => 0,
                    meridian_virt  => 0,
                    fbe_sim        => 1 * MEGABYTES,
                 },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_B",
                Number          => 8198,
                Name            => "WIL B",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_WIL_B",
                ObjectID_Number => 42,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE",
                Export          => 1,
                Size => {
                    rockies_hw     => 128 * MEGABYTES,
                    rockies_sim    => 128 * MEGABYTES,
                    kittyhawk_hw   => 0,
                    kittyhawk_sim  => 0,
                    haswell_hw     => 0,
                    haswell_sim    => 0,
                    meridian_virt  => 0,
                    fbe_sim        => 1 * MEGABYTES,
                 },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CPL_A",
                Number          => 8195,
                Name            => "CPL A",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CPL_A",
                ObjectID_Number => 43,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE",
                Export          => 1,
                Size => {
                    rockies_hw     => 1 * GIGABYTES,
                    rockies_sim    => 1 * GIGABYTES,
                    kittyhawk_hw   => 0,
                    kittyhawk_sim  => 0,
                    haswell_hw     => 0,
                    haswell_sim    => 0,
                    meridian_virt  => 0,
                    fbe_sim        => 1 * MEGABYTES,
                 },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CPL_B",
                Number          => 8196,
                Name            => "CPL B",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CPL_B",
                ObjectID_Number => 44,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE",
                Export          => 1,
                Size => {
                    rockies_hw     => 1 * GIGABYTES,
                    rockies_sim    => 1 * GIGABYTES,
                    kittyhawk_hw   => 0,
                    kittyhawk_sim  => 0,
                    haswell_hw     => 0,
                    haswell_sim    => 0,
                    meridian_virt  => 0,
                    fbe_sim        => 1 * MEGABYTES,
                 },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_2",
                Number          => 8201,
                Name            => "VCX LUN 2",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_2",
                ObjectID_Number => 45,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION",
                Export          => 1,
                Size            => {
                  rockies_hw     => 2 * GIGABYTES,
                  rockies_sim    => 2 * GIGABYTES,
                  kittyhawk_hw   => 0 * MEGABYTES,
                  kittyhawk_sim  => 0 * MEGABYTES,
                  haswell_hw     => 0,
                  haswell_sim    => 0,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 1 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_3",
                Number          => 8202,
                Name            => "VCX LUN 3",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_3",
                ObjectID_Number => 46,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION",
                Export          => 1,
                Size            => {
                  rockies_hw     => 2 * GIGABYTES,
                  rockies_sim    => 2 * GIGABYTES,
                  kittyhawk_hw   => 0 * MEGABYTES,
                  kittyhawk_sim  => 0 * MEGABYTES,
                  haswell_hw     => 0,
                  haswell_sim    => 0,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 1 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_5",
                Number          => 8204,
                Name            => "VCX LUN 5",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_5",
                ObjectID_Number => 47,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION",
                Export          => 1,
                Size            => {
                  rockies_hw     => 64 * GIGABYTES,
                  rockies_sim    => 64 * GIGABYTES,
                  kittyhawk_hw   => 10 * GIGABYTES,
                  kittyhawk_sim  => 0.5 * GIGABYTES,
                  haswell_hw     => 10 * GIGABYTES,
                  haswell_sim    => 0.5 * GIGABYTES,
                  meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 1 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_UDI_SYSTEM_POOL",
                Number          => 8226,
                Name            => "UDI SYSTEM POOL",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_UDI_SYSTEM_POOL",
                ObjectID_Number => 51,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION",
                Export          => 1,
                Size            => {
                  rockies_hw     => 0 * MEGABYTES,
                  rockies_sim    => 0 * MEGABYTES,
                  kittyhawk_hw   => 38 * GIGABYTES,
                  kittyhawk_sim  => 30 * GIGABYTES,
                  haswell_hw     => 38 * GIGABYTES,
                  haswell_sim    => 30 * GIGABYTES,
                  meridian_virt  => 0 * GIGABYTES,
                  fbe_sim        => 0 * MEGABYTES,
               },
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_CFDB",
                Number          => 8227,
                Name            => "CFDB",
                DeviceName      => "CFDB",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CFDB_LUN",
                ObjectID_Number => 52,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ | FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE | FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION",
                Export          => 1,
                Size            => {
                  rockies_hw     => 0 * MEGABYTES,
                  rockies_sim    => 0 * MEGABYTES,
                  kittyhawk_hw   => 100 * MEGABYTES,
                  kittyhawk_sim  => 100 * MEGABYTES,
                  haswell_hw     => 100 * MEGABYTES,
                  haswell_sim    => 100 * MEGABYTES,
                  meridian_virt  => 0 * MEGABYTES,
                  fbe_sim        => 0 * MEGABYTES,
               },
            },
        ],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS",
        Name => "ICA_FLAGS",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => 1 * MEGABYTES,
        Frus => [0,1,2],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SED_DATABASE",
        Name => "SED Database",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => 1 * MEGABYTES,
        Frus => [0,1,2],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG",
        Name => "SEP_SYSTEM_DB",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP",
        Frus => [0,1,2],
        RAIDType => "FBE_RAID_GROUP_TYPE_RAW_MIRROR",
        RGID => "FBE_PRIVATE_SPACE_LAYOUT_RG_ID_RAW_MIRROR",
        ObjectID => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG",
        Flags => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE", # Set Un-Encrytion Flag for future usage
        ReservedSize => {
                  rockies_hw     => 10 * MEGABYTES,
                  rockies_sim    => 10 * MEGABYTES,
                  kittyhawk_hw   => 10 * MEGABYTES,
                  kittyhawk_sim  => 10 * MEGABYTES,
                  haswell_hw     => 10 * MEGABYTES,
                  haswell_sim    => 10 * MEGABYTES,
                  meridian_virt  => 10 * MEGABYTES,
                  fbe_sim        => 1 * MEGABYTES,
               },
        LUNs => [
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP",
                Number          => 8222,
                Name            => "MCR Raw Mirror NP LUN",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_NP",
                ObjectID_Number => 48,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED",
                Export          => 0,
                Size            => 1 * MEGABYTES,
            },
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_SYSTEM_CONFIG",
                Number          => 8223,
                Name            => "MCR Raw Mirror System Config LUN",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_RAW_SYSTEM_CONFIG",
                ObjectID_Number => 49,
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL | FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED",
                Export          => 0,
                Size            => 4 * MEGABYTES,
            },
        ],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_CHASSIS_REPLACEMENT_FLAGS",
        Name => "CHASSIS_REPLACEMENT_FLAGS",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => 1 * MEGABYTES,
        Frus => [0,1,2],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_DESCRIPTOR",
        Name => "FRU_DESCRIPTOR",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => 1 * MEGABYTES,
        Frus => [0,1,2],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_HOMEWRECKER_DB",
        Name => "HOMEWRECKER_DB",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => 1 * MEGABYTES,
        Frus => [0,1,2],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FUTURE_GROWTH_RESERVED",
        Name => "FUTURE_GROWTH_RESERVED",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED",
        Size => {
                  rockies_hw     => 200 * MEGABYTES,
                  rockies_sim    => 200 * MEGABYTES,
                  kittyhawk_hw   => 200 * MEGABYTES,
                  kittyhawk_sim  => 200 * MEGABYTES,
                  haswell_hw     => 200 * MEGABYTES,
                  haswell_sim    => 200 * MEGABYTES,
                  meridian_virt  => 1 * MEGABYTES,
               },
        Frus => [0,1,2,3],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_IMAGE_REPOSITORY",
        Name => "DD_MS_ICA_IMAGE_REP_TYPE",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => {
                  rockies_hw     => 10 * GIGABYTES,
                  rockies_sim    => 0 * MEGABYTES,
                  kittyhawk_hw   => 0 * MEGABYTES,
                  kittyhawk_sim  => 0 * MEGABYTES,
                  meridian_virt  => 0 * MEGABYTES,
                  haswell_hw     => 0 * MEGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  fbe_sim        => 1 * MEGABYTES,
               },
        Frus => [0,1,2],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPA",
        Name => "UTILITY_BOOT_VOLUME_SPA",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => {
                  rockies_hw     => 10 * GIGABYTES,
                  rockies_sim    => 0,
                  kittyhawk_hw   => 1 * GIGABYTES,
                  kittyhawk_sim  => 0,
                  haswell_hw     => 1 * GIGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0,
                  fbe_sim        => 1 * MEGABYTES,
               },
        Frus => [1,3],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPB",
        Name => "UTILITY_BOOT_VOLUME_SPB",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => {
                  rockies_hw     => 10 * GIGABYTES,
                  rockies_sim    => 0,
                  kittyhawk_hw   => 1 * GIGABYTES,
                  kittyhawk_sim  => 0,
                  haswell_hw     => 1 * GIGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0,
                  fbe_sim        => 0 * MEGABYTES,
               },
        Frus => [0,2],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PRIMARY_BOOT_VOLUME_SPA",
        Name => "PRIMARY_BOOT_VOLUME_SPA",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => {
                  rockies_hw     => 135.5 * GIGABYTES,
                  rockies_sim    => 0,
                  kittyhawk_hw   => 44.5 * GIGABYTES,
                  kittyhawk_sim  => 0,
                  haswell_hw     => 44.5 * GIGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0,
                  fbe_sim        => 0,
               },
        Frus => [0,2],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PRIMARY_BOOT_VOLUME_SPB",
        Name => "PRIMARY_BOOT_VOLUME_SPB",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE",
        Size => {
                  rockies_hw     => 135.5 * GIGABYTES,
                  rockies_sim    => 0,
                  kittyhawk_hw   => 44.5 * GIGABYTES,
                  kittyhawk_sim  => 0,
                  haswell_hw     => 44.5 * GIGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0,
               },
        Frus => [1,3],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_RESERVED_2",
        Name => "RESERVED_2",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED",
        Size => {
                  rockies_hw     => 0.5 * GIGABYTES,
                  rockies_sim    => 0,
                  kittyhawk_hw   => 0.5 * GIGABYTES,
                  kittyhawk_sim  => 0,
                  haswell_hw     => 0.5 * GIGABYTES,
                  haswell_sim    => 0 * MEGABYTES,
                  meridian_virt  => 0,
                  fbe_sim        => 0,
               },
        Frus => [0,1,2,3],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_VAULT_RG",
        Name => "Vault RG",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP",
        Frus => {
            rockies_hw     => [0,1,2,3],
            rockies_sim    => [0,1,2,3],
            kittyhawk_hw   => [0,1,2,3],
            kittyhawk_sim  => [0,1,2,3],
            haswell_hw     => [0,1,2,3],
            haswell_sim    => [0,1,2,3],
            meridian_virt  => [0,1,2,3],
            fbe_sim        => [0,1,2,3],
        },
        RAIDType => {
            rockies_hw     => "FBE_RAID_GROUP_TYPE_RAID3",
            rockies_sim    => "FBE_RAID_GROUP_TYPE_RAID3",
            kittyhawk_hw   => "FBE_RAID_GROUP_TYPE_RAID3",
            kittyhawk_sim  => "FBE_RAID_GROUP_TYPE_RAID3",
            haswell_hw     => "FBE_RAID_GROUP_TYPE_RAID3",
            haswell_sim    =>"FBE_RAID_GROUP_TYPE_RAID3",
            meridian_virt  => "FBE_RAID_GROUP_TYPE_UNUSED",
            fbe_sim        => "FBE_RAID_GROUP_TYPE_RAID3"
        },
        RGID => "FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT",
        ObjectID => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG",
        Flags => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_ENCRYPTED_RG", # Set Encrytion Flag for future usage
        LUNs => [
            {
                ID              => "FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VAULT",
                Number          => 8224,
                Name            => "Cache Vault",
                ObjectID        => "FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN",
                ObjectID_Number => 54,
                DeviceName      => "CLARiiON_VAULT",
                Flags           => "FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE",
                Export          => 1,
                Size            => {
                    rockies_hw     => 120 * GIGABYTES,
                    rockies_sim    => 8 * GIGABYTES,
                    kittyhawk_hw   => 8 * GIGABYTES,
                    kittyhawk_sim  => 1 * GIGABYTES,
                    haswell_hw     => 32 * GIGABYTES,
                    haswell_sim    => 1 * GIGABYTES,
                    meridian_virt  => 0 * GIGABYTES,
                    fbe_sim        => 10 * MEGABYTES,
                },
            },
        ],
    },
    {
        ID => "FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_START_OF_USERSPACE_ALIGNMENT",
        Name => "USERSPACE_ALIGN",
        Type => "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED",
        Size => {
                    rockies_hw     => 0x00024800,
                    rockies_sim    => 0,
                    kittyhawk_hw   => 0,
                    kittyhawk_sim  => 0,
                    haswell_hw     => 0,
                    haswell_sim    => 0,
                    meridian_virt  => 0,
                    fbe_sim        => 0,
                },
        Frus => [0,1,2,3],
    },
];

#################################################################################################
#
# Start execution here
#
#################################################################################################

my $PLATFORM;
my @region_ids;
my @rg_object_ids;
my @lun_object_ids;
my @rgs;
my @luns;
my @image_types;
my $region_table;
my $lun_table;
my @start_lba;

foreach my $platform (qw(rockies_hw rockies_sim kittyhawk_hw kittyhawk_sim haswell_hw haswell_sim meridian_virt fbe_sim)) {
   my $psl = dclone($master_psl);

   $PLATFORM = $platform;
   @region_ids = ();
   @rg_object_ids = ();
   @lun_object_ids = ();
   @rgs = ();
   @luns = ();
   @image_types = ();
   $region_table = "";
   $lun_table = "";
   @start_lba = ();

   REGION:
   foreach my $region (@{$psl}) {
       push(@region_ids, $region->{ID});

       my $rg_info;
       
       if($region->{Type} eq "FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP") {
           if(GetFamilySpecificValue($region, 'RAIDType') eq 'FBE_RAID_GROUP_TYPE_UNUSED') {
               next REGION;
           }
           $rg_info = handle_raid_group($region);
       }

       if(GetFamilySpecificValue($region, 'Size') == 0) {
          next REGION;
       }
       
       my $start_lba;

       my $frus_ref     = GetFamilySpecificValue($region, 'Frus');
       
       if(! ref($frus_ref)) {
           for(my $i = 0; $i < $system_disk_count; $i++) {
               if($start_lba[$i] > $start_lba) {
                   $start_lba = $start_lba[$i];
               }
           }
       }
       else {
           foreach my $i (@{$frus_ref}) {
               if($i >= $system_disk_count) {
                   die;
               }
               if($start_lba[$i] > $start_lba) {
                   $start_lba = $start_lba[$i];
               }
           }
       }
       
       
       $region_table .= "        {\n";
       $region_table .= "            $region->{ID},\n";
       $region_table .= "            \"$region->{Name}\",\n";
       $region_table .= "            $region->{Type},\n";
       if(ref $frus_ref) {
           my @frus = @{$frus_ref};
           my $frus = scalar @frus;
           $region_table .= "            $frus, /* Number of FRUs */\n";
           $region_table .= "            {\n";
           $region_table .= "                " . join(", ", @frus) . ", /* FRUs */\n";
           $region_table .= "            },\n";
       }
       else {
           $region_table .= "            FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS, /* Number of FRUs */\n";
           $region_table .= "            {\n";
           $region_table .= "                0, /* FRUs */\n";
           $region_table .= "            },\n";
       }
       $region_table .= sprintf("            0x%08X, /* Starting LBA */\n", $start_lba);
       $region_table .= sprintf("            0x%08X, /* Size */\n", GetFamilySpecificValue($region, 'Size'));
       $region_table .= $rg_info;
       $region_table .= "        },\n";
   
       $start_lba += GetFamilySpecificValue($region, 'Size');
       
       if(! ref($frus_ref)) {
           for(my $i = 0; $i < $system_disk_count; $i++) {
               $start_lba[$i] = $start_lba;
           }
       }
       else {
           foreach my $i (@{$frus_ref}) {
               $start_lba[$i] = $start_lba;
           }
       }
   }

   open(FILE, "> fbe_private_space_layout_$platform.c");
   
   print FILE "/*! \@var fbe_private_space_layout_$platform\n";
   print FILE " *  \n";
   print FILE " *  \@brief This structure defines the system's private space layout\n";
   print FILE " *         in a table of private space regions, and a table of\n";
   print FILE " *         private LUNs.\n";
   print FILE " */\n";
   print FILE "fbe_private_space_layout_t fbe_private_space_layout_$platform = \n";
   print FILE "{\n";
   
   print FILE "    /* Private Space Regions */\n";
   print FILE "    {\n";
   print FILE $region_table;
   print FILE "        /* Terminator */\n";
   print FILE "        {\n";
   print FILE "            FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID,\n";
   print FILE "            \"\",\n";
   print FILE "            FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,\n";
   print FILE "            0, /* Number of FRUs */\n";
   print FILE "            {\n";
   print FILE "                0, /* FRUs */\n";
   print FILE "            },\n";
   print FILE "            0x0, /* Starting LBA */\n";
   print FILE "            0x0, /* Size in Blocks */\n";
   print FILE "        },\n";
   print FILE "    },\n\n";
   
   print FILE "    /* Private Space LUNs */\n";
   print FILE "    {\n";
   print FILE $lun_table;
   print FILE "        /* Terminator*/\n";
   print FILE "        {\n";
   print FILE "            FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID,\n";
   print FILE "            \"\",\n";
   print FILE "            0x0,                /* Object ID */\n";
   print FILE "            FBE_IMAGING_IMAGE_TYPE_INVALID,\n";
   print FILE "            0x0,                /* RG ID */\n";
   print FILE "            0x0,                /* Address Offset within RG */\n";
   print FILE "            0x0,                /* Internal Capacity */\n";
   print FILE "            0x0,                /* External Capacity */\n";
   print FILE "            FBE_FALSE,          /* Export as Device */\n";
   print FILE "            \"\",                 /* Device Name */\n";
   print FILE "            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */\n";
   print FILE "       },\n";
   print FILE "    }\n";
   print FILE "};\n";
   close(FILE);

   open(FILE, "> ../../../../../../interface/fbe_private_space_layout_generated.h");

   my $num_regions = scalar(@region_ids) + 1;
   print FILE "#define FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS $num_regions\n";
   print FILE "\n";
   my $num_luns = scalar(@lun_object_ids) + 1;
   print FILE "#define FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS $num_luns\n";
   print FILE "\n";

   print FILE "/*! \@enum fbe_private_space_layout_object_id_id_t\n";
   print FILE " *  \@brief All of the defined System Object IDs\n";
   print FILE " */\n";
   
   my $object_id = 0;
   print FILE "typedef enum fbe_private_space_layout_object_id_e {\n";
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST = $object_id,\n";
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE = $object_id,\n";
   
   $object_id++;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST = $object_id,\n";
   for(my $i = 0; $i < $system_disk_count; $i++) {
       printf(FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_%d = %d,\n", $i, $object_id);
       $object_id++;
   }
   $object_id--;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST = $object_id,\n";
   
   $object_id++;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST = $object_id,\n";
   for(my $i = 0; $i < $system_disk_count; $i++) {
       printf(FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_%d_SPARE = %d,\n", $i, $object_id);
       $object_id++;
   }
   $object_id--;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_LAST = $object_id,\n";
   
   $object_id++;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST = $object_id,\n";
   for(my $i = 0; $i < $system_disk_count; $i++) {
       printf(FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_%d = %d,\n", $i, $object_id);
       $object_id++;
   }
   $object_id--;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_LAST = $object_id,\n";
   
   $object_id++;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST = $object_id,\n";
   foreach my $oid (@rg_object_ids) {
       print FILE "    $oid = $object_id,\n";
       $object_id++;
   }
   $object_id--;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST = $object_id,\n";
   
   $object_id++;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST = $object_id,\n";
   my %static_numbers;
   my @assign_numbers;
   foreach my $lun (@luns) {
       if(exists($lun->{ObjectID_Number})) {
           if($static_numbers{$lun->{ObjectID_Number}}) {
               die("Duplicate LUN Object ID number: $lun->{ObjectID_Number}\n");
           }
           $static_numbers{$lun->{ObjectID_Number}} = $lun;
       }else {
           die("LUN needs Object ID number: $object_id\n");
       }
   }
   foreach my $lun_id (sort {$a <=> $b} keys(%static_numbers)) {
       my $lun = $static_numbers{$lun_id};
       print FILE "    $lun->{ObjectID} = $lun->{ObjectID_Number},\n";
       $object_id++;
   }

   $object_id--;
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST = $object_id,\n";
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST = $object_id\n";
   print FILE "} fbe_private_space_layout_object_id_t;\n";
   print FILE "\n\n";
   
   print FILE "/*! \@enum fbe_private_space_layout_region_id_t\n";
   print FILE " *  \@brief All of the defined private space region IDs\n";
   print FILE " */\n";
   print FILE "typedef enum fbe_private_space_layout_region_id_e {\n";
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID = 0,\n";
   foreach my $rid (@region_ids) {
       print FILE "    $rid,\n";
   }
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_LAST\n";
   print FILE "} fbe_private_space_layout_region_id_t;\n";
   print FILE "\n\n";
   
   print FILE "/*! \@enum fbe_private_space_layout_rg_id_t\n";
   print FILE " *  \@brief All of the defined private RG IDs\n";
   print FILE " */\n";
   print FILE "typedef enum fbe_private_space_layout_rg_id_e {\n";
   my $rg_id = 1000;
   foreach my $rg (@rgs) {
       print FILE "    $rg->{RGID} = $rg_id,\n";
       $rg_id++;
   }
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_RG_ID_INVALID = 0xFFFFFFFF\n";
   print FILE "} fbe_private_space_layout_rg_id_t;\n\n";
   
   my $lun_id = 8192;
   my %static_numbers;
   my @assign_numbers;
   foreach my $lun (@luns) {
       if(exists($lun->{Number})) {
           if($static_numbers{$lun->{Number}}) {
               die("Duplicate LUN number: $lun->{Number}\n");
           }
           $static_numbers{$lun->{Number}} = $lun;
       }
       else {
           push(@assign_numbers, $lun);
       }
   }
   
   print FILE "/*! \@enum fbe_private_space_layout_lun_id_t\n";
   print FILE " *  \@brief All of the defined private LUN IDs\n";
   print FILE " */\n";
   print FILE "typedef enum fbe_private_space_layout_lun_id_e {\n";
   while(@assign_numbers) {
       my $lun;
       if(exists($static_numbers{$lun_id})) {
           $lun = $static_numbers{$lun_id};
           delete($static_numbers{$lun_id});
       }
       else {
           $lun = shift(@assign_numbers);
       }
       print FILE "    $lun->{ID} = $lun_id,\n";
       $lun_id++;
   }
   foreach my $lun_id (sort {$a <=> $b} keys(%static_numbers)) {
       my $lun = $static_numbers{$lun_id};
       print FILE "    $lun->{ID} = $lun_id,\n";
   }
   print FILE "    FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID = 0xFFFFFFFF\n";
   print FILE "} fbe_private_space_layout_lun_id_t;\n\n";

   close(FILE);
   
   my $start_lba;
   for(my $i = 0; $i < $system_disk_count; $i++) {
       if($start_lba[$i] > $start_lba) {
           $start_lba = $start_lba[$i];
       }
   }

   #if($start_lba % CHUNK_SIZE) {
   #   $start_lba += (CHUNK_SIZE - ($start_lba % CHUNK_SIZE));
   #}

   my $pvd_chunks = int(($start_lba + CHUNK_SIZE - 1) / CHUNK_SIZE);
   my $pvd_mdata_blocks = $pvd_chunks / 256;
   my $mdata_chunks = int(($pvd_mdata_blocks + CHUNK_SIZE - 1) / CHUNK_SIZE);

   my $psl_size   = ($start_lba + ($mdata_chunks * 2 * CHUNK_SIZE)) / (CHUNK_SIZE);

   printf "The PSL for %s requires %d Chunks (%.2f GB, 0x%X Blocks) per system disk\n", $platform, $psl_size,
   $psl_size / 1024, $psl_size * CHUNK_SIZE;

   my $diff_blocks = ($psl_size * CHUNK_SIZE) - 562884302;

   my $diff = (($psl_size * CHUNK_SIZE) - 562884302) / CHUNK_SIZE;
   if($diff_blocks > 0) {
      print  "    WARNING: The PSL is $diff MB too large for a 300GB drive.\n";
      printf "    WARNING: The PSL is 0x%08X blocks too large for a 300GB drive.\n", $diff_blocks;
   }
   elsif($diff_blocks < 0) {
      $diff = abs($diff);
      $diff_blocks = abs($diff_blocks);
      print  "    INFO: The PSL leaves $diff MB free on a 300GB drive.\n";
      printf "    INFO: The PSL leaves 0x%08X blocks free on a 300GB drive.\n", $diff_blocks;
   }
}

sub handle_raid_group {
    my($region) = @_;

    my $raid_type = GetFamilySpecificValue($region, 'RAIDType');
    
    my $rg_info;
    $rg_info .= "            {\n";
    $rg_info .= "                $raid_type,\n";
    $rg_info .= "                $region->{RGID},\n";
    $rg_info .= "                $region->{ObjectID},\n";
        
    my $rg_size;
    LUN:
    foreach my $lun (@{$region->{LUNs}}) {
        handle_lun($lun, $region);
        if(GetFamilySpecificValue($lun, 'Size') == 0) {
           next LUN;
        }
        $region->{Size} = GetFamilySpecificValue($region, 'Size') + (GetFamilySpecificValue($lun, 'Size') / number_of_data_disks($region));
        $rg_size += GetFamilySpecificValue($lun, 'Size');
    }

    if($raid_type ne 'FBE_RAID_GROUP_TYPE_RAW_MIRROR') {
       $rg_size += GetFamilySpecificValue($region, 'ReservedSize');
    }

    
    $rg_info .= sprintf("                0x%08X, /* Exported Capacity */\n", $rg_size);
    $rg_info .= "                $region->{Flags},  /* Set Encrypt/Un-Encrypt Flag for future usage */\n";
    $rg_info .= "            }\n";
    
    $region->{Size} = $rg_size;

    if($raid_type eq 'FBE_RAID_GROUP_TYPE_RAW_MIRROR') {
       $region->{Size} += GetFamilySpecificValue($region, 'ReservedSize');
    }

    $region->{Size} += metadata_size_for_rg($region);
    
    $region->{Size} = $region->{Size} / number_of_data_disks($region);
    if(($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID3')
       || ($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID5')
       || ($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID6'))
    {
        $region->{Size} += (32 * MEGABYTES); # Journal area
    }
    
    push(@rg_object_ids, $region->{ObjectID});
    push(@rgs, $region);
    
    return $rg_info;
}

sub handle_lun {
    my($lun, $region) = @_;
    
    if((! exists $lun->{Number}) && (! length($lun->{DeviceName})) && ($lun->{Export})) {
        die("$lun->{Name} needs a number or device name!");
    }

    push(@lun_object_ids, $lun->{ObjectID});
    push(@luns, $lun);
    
    if(GetFamilySpecificValue($lun, 'Size') == 0) {
       return;
    }

    my $md_size = metadata_size_for_lun($lun, $region);
    
    $lun_table .= "        {\n";
    $lun_table .= "            $lun->{ID},\n";
    $lun_table .= "            \"$lun->{Name}\",\n";
    $lun_table .= "            $lun->{ObjectID}, /* Object ID */\n";
    if(exists $lun->{ImageType}) {
        $lun_table .= "            $lun->{ImageType},\n";
        push(@image_types, $lun->{ImageType});
    }
    else {
        $lun_table .= "            FBE_IMAGING_IMAGE_TYPE_INVALID,\n";
    }
    $lun_table .= "            $region->{RGID}, /* RG ID */\n";
    
    $lun_table .= sprintf("            0x%08X, /* Address Offset within RG */\n", GetFamilySpecificValue($region, 'Size') * number_of_data_disks($region));
    $lun_table .= sprintf("            0x%08X, /* Internal Capacity */\n", GetFamilySpecificValue($lun, 'Size') + $md_size);
    $lun_table .= sprintf("            0x%08X, /* External Capacity */\n", GetFamilySpecificValue($lun, 'Size'));
    if($lun->{Export}) {
        $lun_table .= "            FBE_TRUE, /* Export as Device */\n";
    }
    else {
        $lun_table .= "            FBE_FALSE, /* Export as Device */\n";
    }
    $lun_table .= "            \"$lun->{DeviceName}\", /* Device Name */\n";
    $lun_table .= "            $lun->{Flags}, /* Flags */\n";
    if($lun->{CommitLevel}) {
       $lun_table .= "            $lun->{CommitLevel}, /* Commit Level*/\n";
    }
    else {
       $lun_table .= "            FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/\n";
    }
    $lun_table .= "        },\n";
    
    $lun->{Size} = GetFamilySpecificValue($lun, 'Size') + $md_size;
}


sub metadata_size_for_lun {
    my($lun, $region) = @_;
    
    my $size = GetFamilySpecificValue($lun, 'Size');
    
    $size += 256; # For cache bitmap header + bitmap
    
    my $round_to = CHUNK_SIZE * number_of_data_disks($region);
    
    if($size % $round_to) {
        #print FILE "Rounding Size of LUN\n";
        $size += ($round_to - ($size % $round_to)); # Round up to a chunk for alignment
    }
    
    my $md_size_bytes = ($size / CHUNK_SIZE) * 2;
    my $md_size_blocks = int(($md_size_bytes + 511) / 512);
    $md_size_blocks += 256; # For clean/dirty flags
    
    if($md_size_blocks % CHUNK_SIZE) {
        $md_size_blocks += (CHUNK_SIZE - ($md_size_blocks % CHUNK_SIZE)); # Round up to a chunk for alignment
    }
    
    #print FILE "LU \$md_size_blocks = $md_size_blocks\n";
    
    $size += $md_size_blocks;
    
    $size = round_to_chunk_per_data_disk($region, $size);
    
    return($size - GetFamilySpecificValue($lun, 'Size'));
}

sub metadata_size_for_rg {
    my($region) = @_;
    
    my $size = GetFamilySpecificValue($region, 'Size');
    
    my $round_to = CHUNK_SIZE * number_of_data_disks($region);
    
    if($size % $round_to) {
        #print FILE "Rounding Size of LUN\n";
        $size += ($round_to - ($size % $round_to)); # Round up to a chunk for alignment
    }
    
    my $md_size_bytes = ($size / CHUNK_SIZE) * 4;
    my $md_size_blocks = int(($md_size_bytes + 511) / 512);
    if($md_size_blocks % CHUNK_SIZE) {
        $md_size_blocks += (CHUNK_SIZE - ($md_size_blocks % CHUNK_SIZE)); # Round up to a chunk for alignment
    }
    
    #print FILE "RG \$md_size_blocks = $md_size_blocks\n";
    $size += $md_size_blocks;
    
    $size = round_to_chunk_per_data_disk($region, $size);
    
    return($size - GetFamilySpecificValue($region, 'Size'));
}

sub number_of_data_disks {
    my($region) = @_;
    
    my $frus_ref     = GetFamilySpecificValue($region, 'Frus');
    my $num_frus     = scalar(@{$frus_ref});
    my $data_disks   = 0;
    my $raid_type    = GetFamilySpecificValue($region, 'RAIDType');
    
    if($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID0') {
        $data_disks = $num_frus;
    }
    elsif($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID1') {
        $data_disks = 1;
    }
    elsif($raid_type eq 'FBE_RAID_GROUP_TYPE_RAW_MIRROR') {
        $data_disks = 1;
    }
    elsif($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID10') {
        $data_disks = $num_frus / 2;
    }
    elsif($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID3') {
        $data_disks = $num_frus - 1;
    }
    elsif($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID5') {
        $data_disks = $num_frus - 1;
    }
    elsif($raid_type eq 'FBE_RAID_GROUP_TYPE_RAID6') {
        $data_disks = $num_frus - 2;
    }
    else {
        print FILE Dumper($region);
        die;
    }
    
    return($data_disks);
}

sub round_to_chunk_per_data_disk {
    my($region, $value) = @_;
    
    my $data_disks = number_of_data_disks($region);
    
    
    my $round_to = CHUNK_SIZE * $data_disks;
    if($value % $round_to) {
        #print FILE "Rounding $value up to $data_disks data disks\n";
        $value += ($round_to - ($value % $round_to)); # Round up to a chunk for alignment
    }
    
    return($value);
}

sub GetFamilySpecificValue {
   my($entity, $key) = @_;

   if(ref($entity->{$key}) =~ m/^HASH/) {
      return($entity->{$key}->{$PLATFORM});
   }
   else {
      $entity->{$key};
   }
}
