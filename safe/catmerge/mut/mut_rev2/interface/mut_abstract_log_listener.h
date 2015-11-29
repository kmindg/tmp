/*
 * mut_abstract_log_listener.h
 *
 *  Created on: Sep 26, 2011
 *      Author: londhn
 */

#ifndef MUT_ABSTRACT_LOG_LISTENER_H_
#define MUT_ABSTRACT_LOG_LISTENER_H_

class Mut_abstract_log_listener {
public:
	virtual void send_text_msg(const char *buffer) = 0;
};

#endif /* MUT_ABSTRACT_LOG_LISTENER_H_ */
