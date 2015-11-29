#ifndef FBE_CLI_PVD_OBJECT_H
#define FBE_CLI_PVD_OBJECT_H

void fbe_cli_pvd_force_create(int argc , char ** argv);
void fbe_cli_auto_hot_spare(int argc , char ** argv);

#define AUTO_HOT_SPARE_USAGE  "\
auto_hot_spare / ahs - change the hot spare auto swap on or off\n\
usage:  ahs -help                   - For this help information.\n\
        ahs -on : System will swap in any unused drive even if they are no configured as hot spared\n\
        ahs -off: System will not swap hot spares unless they are configured as one.\n\
        ahs -state: Shows what state the setting is right now  \n\
" 

#endif /*FBE_CLI_PVD_OBJECT_H*/
