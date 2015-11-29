#ifndef FBE_CLI_LIB_ENCRYPTION_CMDS_H
#define FBE_CLI_LIB_ENCRYPTION_CMDS_H

void fbe_cli_encryption(int argc, char** argv);

#define ENCRYPTION_USAGE         \
"\nencrypt <operations> \n"\
"\n"\
"Operations:\n"\
"     -help                                 (display the usage of the green functionality.)\n"\
"     -mode                                 (enable/disable/clear/show the mode of the system encryption functionality.)\n"\
"     -push_keys                            Push Data Encryption keys\n"\
"     -rekey                                Rekey Data Encryption keys\n"\
"     -push_kek                             Push Kek Encryption keys\n"\
"     -unregister_kek                       Unregister the KEKs from the system\n"\
"     -port <-set_mode <#> | -info> [-b #]  Sets the mode or gets info of port (All ports if b not specified)\n"\
"     -check_blocks -num_blocks <num> -lba <lba> -b <id> -e <id> -s <id>: Checks if the blocks are encrypted or not on a drive \n"\
"     -force_state -object_id <id> -state <state> : ENGINEERING USE ONLY. Set the encryption state for that object ON A SINGLE SP \n"\
"     -force_mode -object_id <id> -mode <mode> : ENGINEERING USE ONLY. Set the encryption mode for an RG object \n"\
"\n"\
"Example(s):\n"\
"   encrypt -mode enable                    : Enable the system encryption state\n"\
"   encrypt -mode disable                   : Disable the system encryption state\n"\
"   encrypt -mode clear                     : Reset the system encryption state to NONE\n"\
"   encrypt -mode                           : Display the system encryption state\n\n"\
"   encrypt -push_keys                      : Generate and push keys to RG's\n\n"\
"   encrypt -rekey                          : Generate and push rekeys to RG's\n\n"\
"   encrypt -push_kek [keys]                : Push the KEKs provided or generate one if not\n"\
"   encrypt -unregister_kek                 : Unregister the KEKs from the system\n"\
"   encrypt -push_kek_kek [keys]            : Push the KEK of KEKs \n"\
"   encrypt -reestablish [key_handle]       : Reestablish the KEK handle \n"\
"   encrypt -unregister_kek_kek [key_handle]: Unregister KEK of KEKs \n"\
"   encrypt -status                         : Displays the system encryption status\n"\
"   encrypt -backup_start                   : Send backup start command to KMS \n"\
"   encrypt -backup_complete                : Send backup comlete command to KMS \n"\
"   encrypt -backup_info                    : Displays the backup_info\n"\
"   encrypt -backup_restore                 : Restore CST from provided backup file\n"\
"   encrypt -log_start                      : Send log start command to KMS\n"\
"   encrypt -log_complete                   : Send log complete command to KMS\n"\
"   encrypt -log_hash_start                 : Send log hash start command to KMS\n"\
"   encrypt -log_hash_complete              : Send log hash complete command to KMS\n"\
"   encrypt -check_blocks -num_blocks 10 -lba 0 -b 0 -e 0 -s 0\n"\
"   encrypt -port -set_mode 3               :3-Wrapped DEKs.4-Wrapped DEKs and KEKs\n"

#endif //FBE_CLI_LIB_ENCRYPTION_CMDS_H
