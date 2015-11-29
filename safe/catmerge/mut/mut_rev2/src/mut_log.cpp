/*
 * mut_logg.cpp
 *
 *  Created on: Sep 26, 2011
 *      Author: londhn
 */

#include "mut_log.h"
#include "mut_test_control.h"
#include <stdio.h>
#include <stdarg.h>

using namespace std;

static bool _mut_logger_called = false;

extern Mut_test_control *mut_control;

Mut_log::Mut_log(){
	if(_mut_logger_called){
		log_message("Mut main logger already exists, use same instant or ingerit the root logger");
	}

	_mut_logger_called = true;

	logger.open("mut_log.log");
}

void Mut_log::close_log()
{
	log_message("Closing the log\n");
	logger.flush();
	logger.close();
}

void Mut_log::log_message(const char *message)
{
	char *time = getTime();

	cout <<time<<" "<<message<<endl;
	logger <<time<<" "<<message<<endl;
}

void Mut_log::mut_log_assert(const char *message)
{
	char *time = getTime();

	cout <<time<<" "<<message<<endl;
	logger <<time<<" "<<message<<endl;

	RaiseException(0,0,0,NULL);
	
	mut_process_test_failure();
}

char *Mut_log::getTime(){
	time_t tempLocalTime;
	struct tm *timeInfo;

	time(&tempLocalTime);

	timeInfo = localtime(&tempLocalTime);

	char *dateTime = asctime(timeInfo);

	dateTime[strlen(dateTime)-1]='\0';

	return dateTime;
}


void mut_printf(const char *format, ...)
{
	char printfBuffer[MUT_LOG_MAX_SIZE];
	char mutPrintfBufferHeader[MUT_LOG_MAX_SIZE];

	va_list argumentList;

	va_start(argumentList, format);
	vsprintf(printfBuffer, format, argumentList);
	va_end(argumentList);
	
	sprintf(mutPrintfBufferHeader,"MutPrintf %s",printfBuffer);
	
	if(mut_control)
	{
		mut_control->mut_log->log_message(mutPrintfBufferHeader);
	}

}


