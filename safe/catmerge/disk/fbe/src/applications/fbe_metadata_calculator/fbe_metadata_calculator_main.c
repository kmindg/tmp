#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_emcutil_shell_include.h"

static void pvd_get_exported_size(fbe_u64_t drive_capacity, fbe_u64_t * exported_size);
static void vd_get_exported_size(fbe_u64_t imported_size, fbe_u64_t * exported_size);
static void parity_get_exported_size(fbe_u64_t imported_size, fbe_u64_t number_of_data_disks, fbe_u64_t * exported_size);
static void raid10_get_exported_size(fbe_u64_t imported_size, fbe_u64_t number_of_data_disks, fbe_u64_t * exported_size);
static void lun_get_exported_size(fbe_u64_t imported_size, fbe_u64_t number_of_data_disks, fbe_u64_t * exported_size);
static void lun_get_extent_size(fbe_u64_t wanted_size, fbe_u64_t number_of_data_disks, fbe_u64_t * extent_size);

static void psl_test(void);

typedef struct use_case_s{
	fbe_u64_t drive_capacity;
	fbe_u64_t number_of_data_disks;
	fbe_u64_t number_of_luns;
}use_case_t;

/* 6 drive sizes; 2, 6, 10, 14 data disks; 1, 50, 99 luns; = 6 * 4 * 3  */
#define MAX_USE_CASES (6 * 4 * 3)

static use_case_t use_case_table[MAX_USE_CASES];

#define MB (520.0 / (double)(1024.0 * 1024.0))
#define GB (520.0 / (double)(1024.0 * 1024.0 * 1024.0))

enum constants_e{
	CHUNK_SIZE = 2048, /* 2048 520 blocks */
	BLOCK_SIZE = 520, /* SEP block size */
	BLOCK_DATA_SIZE = 512, /* useful data size in one block */
	PVD_BYTES_PER_CHUNK = 2, /* two bytes will describe one chunk of metadata */
	VD_BYTES_PER_CHUNK = 128, /* 128 bytes will describe one chunk of metadata */
	PARITY_BYTES_PER_CHUNK = 128, /* 128 bytes will describe one chunk of metadata */
	LUN_BYTES_PER_CHUNK = 2, /* two bytes will describe one chunk of metadata */
};

int __cdecl main (int argc , char *argv[])
{
	fbe_u32_t ds; /* drive size iterator */
	fbe_u32_t dd; /* data disk iterator */
	fbe_u32_t ln; /* number of luns iterator */
	fbe_u32_t ucn; /* use case number */

	fbe_u64_t pvd_exported_size;
	fbe_u64_t vd_exported_size;
	fbe_u64_t parity_exported_size;
	fbe_u64_t lun_exported_size;

#include "fbe/fbe_emcutil_shell_maincode.h"

	printf("Metadata Calculator\n");

	//psl_test();
	//return 0;

	/* Fill use_case_table */
	ucn = 0;
	for(ds = 1; ds < 7; ds++){
		for(dd = 2; dd < 15; dd += 4){
			for(ln = 1; ln < 100; ln += 49){
				use_case_table[ucn].drive_capacity = (fbe_u64_t)0x10B5C730 * (fbe_u64_t)ds;
				use_case_table[ucn].number_of_data_disks = dd;
				use_case_table[ucn].number_of_luns = ln;
				ucn++;
			}
		}

	}

	for(ucn = 0; ucn < MAX_USE_CASES; ucn++){
		fbe_u64_t pvd_metadata;
		fbe_u64_t vd_metadata;
		fbe_u64_t parity_metadata;
		fbe_u64_t lun_wanted_size;
		fbe_u64_t lun_size;
		fbe_u64_t lun_metadata;

		pvd_get_exported_size(use_case_table[ucn].drive_capacity, &pvd_exported_size);

		vd_get_exported_size(pvd_exported_size, &vd_exported_size);

		parity_get_exported_size(vd_exported_size, use_case_table[ucn].number_of_data_disks, &parity_exported_size);

		/* Check that raid size is aligned to raid chunk size */
		if((parity_exported_size * use_case_table[ucn].number_of_data_disks) % CHUNK_SIZE){
			printf("RAID alignment error\n");
			return 0;
		}

		lun_wanted_size = parity_exported_size * use_case_table[ucn].number_of_data_disks / use_case_table[ucn].number_of_luns;

		/* Tell me what you want and I will tell you how much space do you need */
		lun_get_extent_size(lun_wanted_size, use_case_table[ucn].number_of_data_disks, &lun_size);

		/* Check that lun size is aligned to raid chunk size */
		if((lun_size * use_case_table[ucn].number_of_data_disks) % CHUNK_SIZE){
			printf("LUN alignment error\n");
			return 0;
		}

		lun_get_exported_size(parity_exported_size, use_case_table[ucn].number_of_data_disks, &lun_exported_size);
		/* Check that lun size is aligned to raid chunk size */
		if((lun_exported_size * use_case_table[ucn].number_of_data_disks) % CHUNK_SIZE){
			printf("LUN alignment error\n");
			return 0;
		}

		pvd_metadata = use_case_table[ucn].drive_capacity - pvd_exported_size;
		vd_metadata = pvd_exported_size - vd_exported_size;
		parity_metadata = vd_exported_size - parity_exported_size;
		lun_metadata = use_case_table[ucn].number_of_luns * (lun_size  - lun_wanted_size);

		printf("********************************************\n");
		printf("%llu data disks; %llu luns\n",
		       (unsigned long long)use_case_table[ucn].number_of_data_disks,
		       (unsigned long long)use_case_table[ucn].number_of_luns);
		printf("DISK \t\t PVD \t\t VD \t\t PARITY \t LUN \n");

		printf("%llX \t %llX \t %llX \t %llX \t %llX\n",
		       (unsigned long long)use_case_table[ucn].drive_capacity,
		       (unsigned long long)pvd_exported_size,
		       (unsigned long long)vd_exported_size,
		       (unsigned long long)parity_exported_size,
		       (unsigned long long)lun_size);

		printf("%.3f GB \t %.3f MB \t %.3f MB \t %.3f MB  \t %.3f MB \t %.3f \n", 
										use_case_table[ucn].drive_capacity * GB,
									    pvd_metadata * MB,
										vd_metadata * MB,
										parity_metadata * MB,
										lun_metadata * MB,
										(lun_metadata) * 100.0 / use_case_table[ucn].drive_capacity);

		printf("Big LUN capacity %.3f GB; offset %llX \n\n",
		       lun_exported_size * GB,
		       (unsigned long long)lun_exported_size);
	}

	
    return 0;
}

static void 
pvd_get_exported_size(fbe_u64_t drive_capacity, fbe_u64_t * exported_size)
{
	fbe_u64_t  overall_chunks;	
    fbe_u64_t  bitmap_entries_per_block = 0;
    fbe_u64_t  paged_bitmap_blocks = 0;
    fbe_u64_t  paged_bitmap_chunks = 0;

	/* We assume 520 block drives.
		512 block drives should be normalized to 520 by:
		drive_capacity * 512 / 520
	*/

	/* Calculate number of 2048 block chunks (overall_chunks) */
	overall_chunks = drive_capacity / CHUNK_SIZE;

	/* Compute how many chunk entries can be saved in (1) block. (512 bytes)*/
    bitmap_entries_per_block = BLOCK_DATA_SIZE /  PVD_BYTES_PER_CHUNK; /* This is: 512 / 2 = 256 */

	/* Compute how many blocks we need for paged metadata */
    paged_bitmap_blocks = overall_chunks / bitmap_entries_per_block;
	
	/* Take care of alignment */
	if(overall_chunks % bitmap_entries_per_block){
		paged_bitmap_blocks ++;
	}
	
	/* Calculate number of chunks we need for the paged metadata */
	paged_bitmap_chunks = paged_bitmap_blocks / CHUNK_SIZE;
	/* Take care of alignment */
	if(paged_bitmap_blocks % CHUNK_SIZE){
		paged_bitmap_chunks ++;
	}

	/* We may deside to "mirror" the paged metadata */
	paged_bitmap_chunks *= 2;

	/* We need two more chunks for signature */
	*exported_size = (overall_chunks - paged_bitmap_chunks - 2/*signatures*/) * CHUNK_SIZE; /* This is in blocks */
}	

static void 
vd_get_exported_size(fbe_u64_t imported_size, fbe_u64_t * exported_size)
{
	fbe_u64_t  overall_chunks;	
    fbe_u64_t  bitmap_entries_per_block = 0;
    fbe_u64_t  paged_bitmap_blocks = 0;
    fbe_u64_t  paged_bitmap_chunks = 0;

	/* Calculate number of 2048 block chunks (overall_chunks) */
	overall_chunks = imported_size / CHUNK_SIZE;

	/* Compute how many chunk entries can be saved in (1) block. (512 bytes)*/
    bitmap_entries_per_block = BLOCK_DATA_SIZE /  VD_BYTES_PER_CHUNK; /* This is: 512 / 128 = 4 */

	/* Compute how many blocks we need for paged metadata */
    paged_bitmap_blocks = overall_chunks / bitmap_entries_per_block;
	
	/* Take care of alignment */
	if(overall_chunks % bitmap_entries_per_block){
		paged_bitmap_blocks ++;
	}
	
	/* Calculate number of chunks we need for the paged metadata */
	paged_bitmap_chunks = paged_bitmap_blocks / CHUNK_SIZE;
	/* Take care of alignment */
	if(paged_bitmap_blocks % CHUNK_SIZE){
		paged_bitmap_chunks ++;
	}

	/* We may deside to "mirror" the paged metadata */
	paged_bitmap_chunks *= 2;

	/* We need two more chunks for signatures */
	*exported_size = (overall_chunks - paged_bitmap_chunks - 2/*signatures*/) * CHUNK_SIZE; /* This is in blocks */
}	

static void 
parity_get_exported_size(fbe_u64_t imported_size, fbe_u64_t number_of_data_disks, fbe_u64_t * exported_size)
{
	fbe_u64_t  overall_chunks;	
    fbe_u64_t  bitmap_entries_per_block = 0;
    fbe_u64_t  paged_bitmap_blocks = 0;
    fbe_u64_t  paged_bitmap_chunks = 0;
	fbe_u64_t  metadata_chunks = 0;
	fbe_u64_t parity_chunk_size = 0;

	/* Parity chunk consist of one 2048 chunk per each data drive */
	parity_chunk_size = CHUNK_SIZE * number_of_data_disks; /* in blocks */

	/* Calculate number of parity_chunk_size chunks (overall_chunks) */
	overall_chunks = (imported_size * number_of_data_disks ) / parity_chunk_size;

	/* Compute how many chunk entries can be saved in (1) block. (512 bytes)*/
    bitmap_entries_per_block = BLOCK_DATA_SIZE /  PARITY_BYTES_PER_CHUNK; /* This is: 512 / 128 = 4 */

	/* Compute how many blocks we need for paged metadata */
    paged_bitmap_blocks = overall_chunks / bitmap_entries_per_block;
	
	/* Take care of alignment */
	if(overall_chunks % bitmap_entries_per_block){
		paged_bitmap_blocks ++;
	}
	
	/* Calculate number of chunks we need for the paged metadata */
	paged_bitmap_chunks = paged_bitmap_blocks / parity_chunk_size;
	/* Take care of alignment */
	if(paged_bitmap_blocks % parity_chunk_size){
		paged_bitmap_chunks ++;
	}

	metadata_chunks = paged_bitmap_chunks;

	/* We will put signatue on each drive */
	metadata_chunks += 1;

	/* We will also allocate write log area on each drive 32 for 2MB operation plus one */
	metadata_chunks += 33;

	*exported_size = (overall_chunks - metadata_chunks);

	*exported_size = *exported_size * parity_chunk_size / number_of_data_disks; /* This is in blocks */
}	


/* We have number of data disks as a half of overall disks in raid 10,
	but our metadata is for the mirrors only.
*/
static void 
raid10_get_exported_size(fbe_u64_t imported_size, fbe_u64_t number_of_data_disks, fbe_u64_t * exported_size)
{
	parity_get_exported_size(imported_size, 1, exported_size);
}	

static void 
lun_get_exported_size(fbe_u64_t imported_size, fbe_u64_t number_of_data_disks, fbe_u64_t * exported_size)
{
	fbe_u64_t  overall_chunks;	
    fbe_u64_t  bitmap_entries_per_block = 0;
    fbe_u64_t  paged_bitmap_blocks = 0;
    fbe_u64_t  paged_bitmap_chunks = 0;
	fbe_u64_t  metadata_chunks = 0;

	/* Calculate number of 2048 block chunks (overall_chunks) */
	overall_chunks = (imported_size * number_of_data_disks ) / CHUNK_SIZE;

	/* Compute how many chunk entries can be saved in (1) block. (512 bytes)*/
    bitmap_entries_per_block = BLOCK_DATA_SIZE /  LUN_BYTES_PER_CHUNK; /* This is: 512 / 128 = 4 */

	/* Compute how many blocks we need for paged metadata */
    paged_bitmap_blocks = overall_chunks / bitmap_entries_per_block;
	
	/* Take care of alignment */
	if(overall_chunks % bitmap_entries_per_block){
		paged_bitmap_blocks ++;
	}
	
	/* Calculate number of chunks we need for the paged metadata */
	paged_bitmap_chunks = paged_bitmap_blocks / CHUNK_SIZE;
	/* Take care of alignment */
	if(paged_bitmap_blocks % CHUNK_SIZE){
		paged_bitmap_chunks ++;
	}

	metadata_chunks = paged_bitmap_chunks;

	/* Add signatue */
	metadata_chunks += 1;
	
	*exported_size = (overall_chunks - metadata_chunks);
	/* Align to number of data disks */
	*exported_size -= (*exported_size % number_of_data_disks);

	*exported_size = *exported_size * CHUNK_SIZE / number_of_data_disks; /* This is in blocks */
}

static void 
lun_get_extent_size(fbe_u64_t wanted_size, fbe_u64_t number_of_data_disks, fbe_u64_t * extent_size)
{
	fbe_u64_t  overall_chunks;	
    fbe_u64_t  bitmap_entries_per_block = 0;
    fbe_u64_t  paged_bitmap_blocks = 0;
    fbe_u64_t  paged_bitmap_chunks = 0;
	fbe_u64_t  metadata_chunks = 0;

	/* Calculate number of 2048 block chunks (overall_chunks) */
	overall_chunks = (wanted_size * number_of_data_disks ) / CHUNK_SIZE;
	if((wanted_size * number_of_data_disks ) % CHUNK_SIZE){
		overall_chunks++;
	}

	/* Compute how many chunk entries can be saved in (1) block. (512 bytes)*/
    bitmap_entries_per_block = BLOCK_DATA_SIZE /  LUN_BYTES_PER_CHUNK; /* This is: 512 / 128 = 4 */

	/* Compute how many blocks we need for paged metadata */
    paged_bitmap_blocks = overall_chunks / bitmap_entries_per_block;
	
	/* Take care of alignment */
	if(overall_chunks % bitmap_entries_per_block){
		paged_bitmap_blocks ++;
	}
	
	/* Calculate number of chunks we need for the paged metadata */
	paged_bitmap_chunks = paged_bitmap_blocks / CHUNK_SIZE;
	/* Take care of alignment */
	if(paged_bitmap_blocks % CHUNK_SIZE){
		paged_bitmap_chunks ++;
	}

	metadata_chunks = paged_bitmap_chunks;

	/* Add signatue */
	metadata_chunks += 1;
	
	*extent_size = (overall_chunks + metadata_chunks);
	/* Align to number of data disks */
	*extent_size += (number_of_data_disks - (*extent_size % number_of_data_disks));

	*extent_size = *extent_size * CHUNK_SIZE / number_of_data_disks; /* This is in blocks */
}	

enum psl_constants_e{
	Triple_Mirror_Raid_Offset	= 0x00010000,

	DDBS_Library				= 0x00010000,
	DDBS_Library_MD				= 0x00011000,

	BIOS_Firmware				= 0x00012A00,
	BIOS_Firmware_MD			= 0x0001AA00,

	POST_Firmware				= 0x0001C400,
	POST_Firmware_MD			= 0x0002C400,

	GPS_Firmware_Region			= 0x0002DE00,
	GPS_Firmware_Region_MD		= 0x00045E00,

	VCX_LUN_0					= 0x0027CC00,
	VCX_LUN_0_MD				= 0x0187CC00,

	VCX_LUN_1					= 0x0187E600,
	VCX_LUN_1_MD				= 0x02E7E600,

	VCX_LUN_4					= 0x02E80000,
	VCX_LUN_4_MD				= 0x03280000,

	Centerra_LUN				= 0x03281A00,
	Centerra_LUN_MD				= 0x04681A00,

	PSM							= 0x04683400,
	PSM_MD						= 0x05683400,

	L1_Cache					= 0x05684E00,
	L1_Cache_MD 				= 0x056B6E00,

	EFD_Cache					= 0x056B8800,
	EFD_Cache_MD				= 0x056EA800,
						
	Triple_Mirror_Raid_MD		= 0x057EC000,
	Triple_Mirror_Raid_Size		= 0x0583F400 - Triple_Mirror_Raid_Offset,

	Vault_Raid3_Offset			= 0x0583F600,

	WIL_A						= 0x0583F600,	
	WIL_A_MD					= 0x05854C00,

	WIL_B						= 0x05855600,
	WIL_B_MD					= 0x0586AC00,	

	CPL_A						= 0x0586B600,								
	CPL_A_MD					= 0x05916200,

	CPL_B						= 0x05916C00,								
	CPL_B_MD					= 0x059C1800,

	VCX_LUN_2					= 0x059C2200,																		
	VCX_LUN_2_MD				= 0x05B17800,

	VCX_LUN_3					= 0x05B18200,								
	VCX_LUN_3_MD				= 0x05C6D800,

	VCX_LUN_5					= 0x05C6E200,																								
	VCX_LUN_5_MD				= 0x08718E00,

	Metadata_LUN				= 0x08719800,
	Metadata_LUN_MD				= 0x08721E00,

	Statistics_LUN				= 0x08722800,
	Statistics_LUN_MD			= 0x0872AE00,

	Vault						= 0x0872B800,							
	Vault_MD					= 0x09B2B800,

	Remaining_Free_Space		= 0x09B2C200,
									
	Vault_Raid3_MD				= 0x09D21800,
	Vault_Raid3_Size			= 0x09D3D200  - Vault_Raid3_Offset,


	PSL_vd_metadata				= 0x16E94800,
	
	PSL_drive_capacity			= 0x16EE38CD, /* We assume that we have 200GB disk */

};

static void 
lun_md_check(const char * lun_name, 
			 fbe_u64_t number_of_data_disks, 
			 fbe_u64_t current_lun_start, 
			 fbe_u64_t current_lun_md, 
			 fbe_u64_t next_lun_start)
{
	fbe_u64_t current_wanted_size = current_lun_md - current_lun_start;
	fbe_u64_t lun_size;
	fbe_u64_t lun_metadata;

	lun_get_extent_size(current_wanted_size, number_of_data_disks, &lun_size);
	lun_metadata = lun_size - current_wanted_size;
	printf("%s \t Start %08llX End %08llX size %.3f MB metadata size %.3f MB  \n", 
	       lun_name,
	       (unsigned long long)current_lun_start,
	       (unsigned long long)(current_lun_start + lun_size),
	       current_wanted_size * MB, lun_metadata * MB);

	if(current_lun_start + lun_size > next_lun_start){
		printf("%s metadata ERROR: %llX  > %llX \n", lun_name,
		       (unsigned long long)(current_lun_start + lun_size),
		       (unsigned long long)next_lun_start);
		return;
	}
}

static void psl_test(void)
{						
	fbe_u64_t pvd_exported_size = 0;
	fbe_u64_t vd_exported_size = 0;
	fbe_u64_t parity_exported_size = 0;

	fbe_u64_t pvd_metadata = 0;
	fbe_u64_t vd_metadata = 0;
	fbe_u64_t parity_metadata = 0;

	pvd_get_exported_size(PSL_drive_capacity, &pvd_exported_size);
	pvd_metadata = PSL_drive_capacity - pvd_exported_size;
	printf("PVD metadata offset %llX; size %.3f MB\n",
	       (unsigned long long)pvd_exported_size, pvd_metadata * MB);
	
	vd_get_exported_size(pvd_exported_size, &vd_exported_size);
	vd_metadata = pvd_exported_size - vd_exported_size;
	printf("VD  metadata offset %llX; size %.3f MB\n",
	       (unsigned long long)vd_exported_size, vd_metadata * MB);

	/* Check if we have enough space for VD metadata */
	if(vd_exported_size < PSL_vd_metadata){
		printf("VD metadata offset ERROR: %llX  < %llX \n",
			(unsigned long long)vd_exported_size,
			(unsigned long long)PSL_vd_metadata);
		return ;
	}

	/* Check if we have enough space for triple mirror metadata */
	parity_get_exported_size(Triple_Mirror_Raid_Size, 1, &parity_exported_size);
	parity_metadata = Triple_Mirror_Raid_Size - parity_exported_size;
	printf("PSM size %.3f GB metadata offset %llX; size %.3f MB\n",
	       Triple_Mirror_Raid_Size * GB, 
	       (unsigned long long)(parity_exported_size + Triple_Mirror_Raid_Offset),
	       parity_metadata * MB);

	/* Check if Triple mirror exported size is big enough */
	if(parity_exported_size < Triple_Mirror_Raid_MD - Triple_Mirror_Raid_Offset){
		printf("Triple Mirror exported size ERROR: %llX  < %llX \n",
		       (unsigned long long)parity_exported_size,
		       (unsigned long long)(Triple_Mirror_Raid_MD - Triple_Mirror_Raid_Offset));
		return ;
	}

	if(parity_exported_size + Triple_Mirror_Raid_Offset < Triple_Mirror_Raid_MD){
		printf("Triple Mirror metadata offset ERROR: %llX  < %llX \n",
		       (unsigned long long)(parity_exported_size + Triple_Mirror_Raid_Offset), (unsigned long long)Triple_Mirror_Raid_MD);
		return ;
	}

	/* Start LUN calculations */
	/* DDBS_Library */
	lun_md_check("DDBS_Library\t", 1, DDBS_Library, DDBS_Library_MD, BIOS_Firmware);

	/* BIOS_Firmware */
	lun_md_check("BIOS_Firmware\t", 1, BIOS_Firmware, BIOS_Firmware_MD, POST_Firmware);

	/* POST_Firmware */
	lun_md_check("POST_Firmware\t", 1, POST_Firmware, POST_Firmware_MD, GPS_Firmware_Region);

	/* GPS_Firmware_Region */
	lun_md_check("GPS_Firmware_Region", 1, GPS_Firmware_Region, GPS_Firmware_Region_MD, VCX_LUN_0);

	/* VCX_LUN_0 */
	lun_md_check("VCX_LUN_0\t", 1, VCX_LUN_0, VCX_LUN_0_MD, VCX_LUN_1);

	/* VCX_LUN_1 */
	lun_md_check("VCX_LUN_1\t", 1, VCX_LUN_1, VCX_LUN_1_MD, VCX_LUN_4);

	/* VCX_LUN_4 */
	lun_md_check("VCX_LUN_4\t", 1, VCX_LUN_4, VCX_LUN_4_MD, Centerra_LUN);

	/* Centerra_LUN */
	lun_md_check("Centerra_LUN\t", 1, Centerra_LUN, Centerra_LUN_MD, PSM);

	/* PSM */
	lun_md_check("PSM\t\t", PSM, 1, PSM_MD, L1_Cache);

	/* L1_Cache */
	lun_md_check("L1_Cache\t", 1, L1_Cache, L1_Cache_MD, EFD_Cache);

	/* EFD_Cache */
	lun_md_check("EFD_Cache\t", 1, EFD_Cache, EFD_Cache_MD, 0x056EC200 /* Future growth region */);



	/* Check if we have enough space for Vault Raid3  metadata */
	parity_get_exported_size(Vault_Raid3_Size, 3, &parity_exported_size);
	parity_metadata = Vault_Raid3_Size - parity_exported_size;
	printf("Vault Raid3 size %.3f GB metadata offset %llX; size %.3f MB\n",
	       Vault_Raid3_Size * GB,
	       (unsigned long long)parity_exported_size + Vault_Raid3_Offset,
	       parity_metadata * MB);

	/* Check if Vault Raid3 exported size is big enough */
	if(parity_exported_size < Vault_Raid3_MD - Vault_Raid3_Offset){
		printf("Vault Raid3 exported size ERROR: %llX  < %llX \n",
		       (unsigned long long)parity_exported_size,
		       (unsigned long long)(Vault_Raid3_MD - Vault_Raid3_Offset));
		return ;
	}

	if(parity_exported_size + Vault_Raid3_Offset < Vault_Raid3_MD){
		printf("Vault Raid3 metadata offset ERROR: %llX  < %llX \n",
		       (unsigned long long)(parity_exported_size + Vault_Raid3_Offset),
		       (unsigned long long)Vault_Raid3_MD);
		return ;
	}

	/* Start LUN calculations */
	/* WIL_A */
	lun_md_check("WIL_A\t\t", 3, WIL_A, WIL_A_MD, WIL_B);

	/* WIL_B */
	lun_md_check("WIL_B\t\t", 3, WIL_B, WIL_B_MD, CPL_A);

	/* CPL_A */
	lun_md_check("CPL_A\t\t", 3, CPL_A, CPL_A_MD, CPL_B);

	/* CPL_B */
	lun_md_check("CPL_B\t\t", 3, CPL_B, CPL_B_MD, VCX_LUN_2);

	/* VCX_LUN_2 */
	lun_md_check("VCX_LUN_2\t", 3, VCX_LUN_2, VCX_LUN_2_MD, VCX_LUN_3);

	/* VCX_LUN_3 */
	lun_md_check("VCX_LUN_3\t", 3, VCX_LUN_3, VCX_LUN_3_MD, VCX_LUN_5);

	/* VCX_LUN_5 */
	lun_md_check("VCX_LUN_5\t", 3, VCX_LUN_5, VCX_LUN_5_MD, Metadata_LUN);

	/* Metadata_LUN */
	lun_md_check("Metadata_LUN\t", 3, Metadata_LUN, Metadata_LUN_MD, Statistics_LUN);

	/* Statistics_LUN */
	lun_md_check("Statistics_LUN\t", 3, Statistics_LUN, Statistics_LUN_MD, Vault);

	/* Vault */
	lun_md_check("Vault\t\t", 3, Vault, Vault_MD, Remaining_Free_Space);

}

