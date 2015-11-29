/*
 * mut_log.h
 *
 *  Created on: Sep 26, 2011
 *      Author: londhn
 */

//For checkin two we will be using baisc logging proxy.
#ifndef MUT_LOG_H_
#define MUT_LOG_H_

#include "mut_abstract_log_listener.h"
#include "mut_private.h"
#include <fstream>
#include <iostream>
#include <time.h>
#include <string.h>

static ofstream logger;
class Mut_log{
public:
	Mut_log();

	void log_message(const char *message);
	void close_log();
	char *getTime();

private:
    Mut_abstract_log_listener *mListener;


public:
	void mut_log_assert(const char* message);
};

#endif /* MUT_LOG_H_ */
