#ifndef FBE_KMS_API_H
#define FBE_KMS_API_H


/* The hook is used to pause kms at various points.
 */
enum fbe_kms_hook_state_e {
    FBE_KMS_HOOK_STATE_NONE = 0,
    /*! Pause after the load. 
     */
    FBE_KMS_HOOK_STATE_LOAD,
};
enum {
    /* Number of hooks that KMS will have.
     */
    KMS_HOOK_COUNT = 5,
};
typedef fbe_u64_t fbe_kms_hook_state_t;

/* Defines a hook to pause KMS.
 */
typedef struct fbe_kms_hook_s {
    fbe_kms_hook_state_t state;
    fbe_u32_t hit_count;
} fbe_kms_hook_t;


#endif /* FBE_KMS_API_H */
