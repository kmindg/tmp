#ifndef SEP_CLASS_TABLE_H
#define SEP_CLASS_TABLE_H

extern fbe_class_methods_t fbe_mirror_class_methods;
extern fbe_class_methods_t fbe_striper_class_methods;
extern fbe_class_methods_t fbe_parity_class_methods;
extern fbe_class_methods_t fbe_virtual_drive_class_methods;
extern fbe_class_methods_t fbe_lun_class_methods;
extern fbe_class_methods_t fbe_bvd_interface_class_methods;
extern fbe_class_methods_t fbe_provision_drive_class_methods;
extern fbe_class_methods_t fbe_base_config_class_methods;
extern fbe_class_methods_t fbe_extent_pool_class_methods;
extern fbe_class_methods_t fbe_ext_pool_lun_class_methods;
extern fbe_class_methods_t fbe_ext_pool_metadata_lun_class_methods;

/* Note we will compile different tables for different packages */
static const fbe_class_methods_t * sep_class_table[] = {    &fbe_mirror_class_methods,
                                                            &fbe_striper_class_methods,
                                                            &fbe_parity_class_methods,
                                                            &fbe_virtual_drive_class_methods,
                                                            &fbe_lun_class_methods,
															&fbe_bvd_interface_class_methods,
															&fbe_provision_drive_class_methods,
															&fbe_extent_pool_class_methods,
															&fbe_ext_pool_lun_class_methods,
															&fbe_ext_pool_metadata_lun_class_methods,
															&fbe_base_config_class_methods,
                                                            NULL };



#endif /* SEP_CLASS_TABLE_H */
