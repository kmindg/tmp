#ifndef ESP_CLASS_TABLE_H
#define ESP_CLASS_TABLE_H

extern fbe_class_methods_t fbe_encl_mgmt_class_methods;
extern fbe_class_methods_t fbe_sps_mgmt_class_methods;
extern fbe_class_methods_t fbe_drive_mgmt_class_methods;
extern fbe_class_methods_t fbe_module_mgmt_class_methods;
extern fbe_class_methods_t fbe_ps_mgmt_class_methods;
extern fbe_class_methods_t fbe_board_mgmt_class_methods;
extern fbe_class_methods_t fbe_cooling_mgmt_class_methods;

/* Note we will compile different tables for different packages */
static const fbe_class_methods_t * esp_class_table[] CSX_MAYBE_UNUSED = {&fbe_encl_mgmt_class_methods,
                                                        &fbe_sps_mgmt_class_methods,
                                                        &fbe_drive_mgmt_class_methods,
                                                        &fbe_module_mgmt_class_methods,
                                                        &fbe_ps_mgmt_class_methods,   
                                                        &fbe_board_mgmt_class_methods,
                                                        &fbe_cooling_mgmt_class_methods,
                                                        NULL };

#endif /* ESP_CLASS_TABLE_H */
