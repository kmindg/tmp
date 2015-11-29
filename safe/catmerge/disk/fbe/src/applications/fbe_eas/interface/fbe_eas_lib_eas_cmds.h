#ifndef FBE_CLI_LIB_EAS_CMDS_H
#define FBE_CLI_LIB_EAS_CMDS_H

void fbe_cli_eas(int argc, char** argv);

#define EAS_USAGE         \
"\nEAS <operations> \n"\
"\n"\
"Operations:\n"\
"     -help                                         - Display the usage of the functionality\n"\
"     -start [all | all_ssd | all_hdd | b_e_s]      - Start EAS to PVD\n"\
"     -erase [all | all_ssd | all_hdd | b_e_s]      - Send Erase command to PDO\n"\
"     -state [all | all_ssd | all_hdd | b_e_s]      - Rekey Data Encryption keys\n"\
"     -commit [all | all_ssd | all_hdd | b_e_s]     - Commit to PVD after EAS completed in PDO\n"\
"\n"\
"Example(s):\n"\
"   EAS -start all                      : Start EAS command to all drives\n"\
"   EAS -start 0_1_1                    : Start EAS command to drive 0_1_1\n"\
"   EAS -commit all_ssd                 : Commit EAS to all SSD drives\n"


#endif //FBE_CLI_LIB_EAS_CMDS_H
