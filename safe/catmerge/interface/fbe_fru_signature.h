#ifndef FBE_FRU_SIGNATURE_H
#define FBE_FRU_SIGNATURE_H

#define FBE_FRU_SIGNATURE_MAGIC_STRING      "$FBE_FRU_SIGNATURE$"
#define FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE 20

#define FBE_FRU_SIGNATURE_VERSION           1

#define FBE_FRU_SIGNATURE_SIZE_BLOCKS       1

#pragma pack(1)
typedef struct fbe_fru_signature_s
{
    fbe_char_t  magic_string[FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE];
    fbe_u32_t   version;
    fbe_u64_t   system_wwn_seed;
    fbe_u32_t   bus;
    fbe_u32_t   enclosure;
    fbe_u32_t   slot;
   
} fbe_fru_signature_t;
#pragma pack()

#endif /* FBE_FRU_SIGNATURE_H */
