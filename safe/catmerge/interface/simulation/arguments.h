/*
 * arguments.h
 *
 * Created on: Feb 14, 2012
 * Author: londhn
 * Description: This class is responsible for parsing all the option
 * presented on command line and storing them in the options list.
 * This class exists as a singleton and will be accessed by options class for 
 * processing of the arguments.
 */
#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include "csx_ext.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT 
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT 
#endif

#define ARGUMENTS_ERROR(msg) \
	printf("[ Arguments error, function %s\n", __FUNCTION__);\
	printf msg; \
	printf(" ]\n"); \
	exit(1);

class Arguments{
public:

	SHMEMDLLPUBLIC static void init(int argc, char **argv);

	SHMEMDLLPUBLIC int getArgc();
	SHMEMDLLPUBLIC char** getArgv();
	SHMEMDLLPUBLIC static Arguments *getInstance();

	SHMEMDLLPUBLIC void getCli(const char *cli, char **returnBuffer);

	SHMEMDLLPUBLIC static void resetArgs(int argc, char **argv);
	SHMEMDLLPUBLIC void opt_to_lower(const char* opt_in, char* opt_out);

private:
	static Arguments *mArguments;
	int mArgc;
	char **mArgv;

	Arguments(int argc, char **argv):mArgc(argc),mArgv(argv){};
	~Arguments();
};

#endif /*ARGUMENTS_H*/
