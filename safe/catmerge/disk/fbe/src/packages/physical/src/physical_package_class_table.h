#ifndef PHYSICAL_PACKAGE_CLASS_TABLE_H
#define PHYSICAL_PACKAGE_CLASS_TABLE_H

extern fbe_class_methods_t fbe_base_board_class_methods;
extern fbe_class_methods_t fbe_base_object_class_methods;
extern fbe_class_methods_t fbe_sas_port_class_methods;
extern fbe_class_methods_t fbe_sas_lcc_class_methods;
extern fbe_class_methods_t fbe_fleet_board_class_methods;
extern fbe_class_methods_t fbe_magnum_board_class_methods;
extern fbe_class_methods_t fbe_armada_board_class_methods;
extern fbe_class_methods_t fbe_sas_bullet_lcc_class_methods;
extern fbe_class_methods_t fbe_sas_enclosure_class_methods;
extern fbe_class_methods_t fbe_eses_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_viper_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_magnum_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_citadel_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_bunker_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_derringer_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_tabasco_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_voyager_icm_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_voyager_ee_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_viking_iosxp_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_viking_drvsxp_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_cayenne_iosxp_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_cayenne_drvsxp_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_naga_iosxp_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_naga_drvsxp_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_fallback_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_boxwood_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_knot_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_ancho_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_pinecone_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_steeljaw_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_ramhorn_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_rhea_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_miranda_enclosure_class_methods;
extern fbe_class_methods_t fbe_sas_calypso_enclosure_class_methods;
extern fbe_class_methods_t fbe_base_port_class_methods;
extern fbe_class_methods_t fbe_sas_pmc_port_class_methods;
extern fbe_class_methods_t fbe_base_physical_drive_class_methods;
extern fbe_class_methods_t fbe_sas_physical_drive_class_methods;
extern fbe_class_methods_t fbe_sas_flash_drive_class_methods;
extern fbe_class_methods_t fbe_sata_paddlecard_drive_class_methods;
extern fbe_class_methods_t fbe_sata_physical_drive_class_methods;
extern fbe_class_methods_t fbe_sata_flash_drive_class_methods;
extern fbe_class_methods_t fbe_sas_pmc_port_class_methods;
extern fbe_class_methods_t fbe_logical_drive_class_methods;
extern fbe_class_methods_t fbe_fc_port_class_methods;
extern fbe_class_methods_t fbe_iscsi_port_class_methods;


/* Note we will compile different tables for different packages */
static const fbe_class_methods_t * physical_package_class_table[] = {&fbe_sas_physical_drive_class_methods,
                                                                     &fbe_sata_physical_drive_class_methods,
                                                                     &fbe_sas_pmc_port_class_methods,
                                                                     &fbe_sas_flash_drive_class_methods,
                                                                     &fbe_sata_paddlecard_drive_class_methods,                            
                                                                     &fbe_sata_flash_drive_class_methods,
                                                                     &fbe_base_board_class_methods,
                                                                     &fbe_fleet_board_class_methods,
                                                                     &fbe_magnum_board_class_methods,
                                                                     &fbe_armada_board_class_methods,
                                                                     &fbe_sas_derringer_enclosure_class_methods,
                                                                     &fbe_base_port_class_methods,
                                                                     &fbe_sas_port_class_methods,
                                                                     &fbe_sas_enclosure_class_methods,
                                                                     &fbe_eses_enclosure_class_methods,
                                                                     &fbe_sas_viper_enclosure_class_methods,
                                                                     &fbe_sas_magnum_enclosure_class_methods,
                                                                     &fbe_sas_citadel_enclosure_class_methods,
                                                                     &fbe_sas_bunker_enclosure_class_methods,
                                                                     &fbe_sas_voyager_icm_enclosure_class_methods,
                                                                     &fbe_sas_voyager_ee_enclosure_class_methods,
                                                                     &fbe_sas_fallback_enclosure_class_methods,
                                                                     &fbe_sas_boxwood_enclosure_class_methods,
                                                                     &fbe_sas_knot_enclosure_class_methods,
                                                                     &fbe_sas_pinecone_enclosure_class_methods,
                                                                     &fbe_sas_steeljaw_enclosure_class_methods,
                                                                     &fbe_sas_ramhorn_enclosure_class_methods,
                                                                     &fbe_sas_ancho_enclosure_class_methods,
                                                                     &fbe_sas_viking_iosxp_enclosure_class_methods,
                                                                     &fbe_sas_viking_drvsxp_enclosure_class_methods,
                                                                     &fbe_sas_cayenne_iosxp_enclosure_class_methods,
                                                                     &fbe_sas_cayenne_drvsxp_enclosure_class_methods,
                                                                     &fbe_sas_naga_iosxp_enclosure_class_methods,
                                                                     &fbe_sas_naga_drvsxp_enclosure_class_methods,
                                                                     &fbe_sas_rhea_enclosure_class_methods,
                                                                     &fbe_sas_miranda_enclosure_class_methods,
                                                                     &fbe_sas_calypso_enclosure_class_methods,
                                                                     &fbe_base_physical_drive_class_methods,
                                                                     &fbe_fc_port_class_methods,
                                                                     &fbe_iscsi_port_class_methods,
                                                                     &fbe_sas_tabasco_enclosure_class_methods,
                                                                     NULL };



#endif /* PHYSICAL_PACKAGE_CLASS_TABLE_H */
