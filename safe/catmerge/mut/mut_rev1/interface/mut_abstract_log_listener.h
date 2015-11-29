/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_abstract_log_listener.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of interface Mut_abstract_log_listener
 *
 * Classes:
 * 
 * NOTES:
 *
 * HISTORY:
 *    05/2011 MBuckley  created
 **************************************************************************/

#ifndef __MUTABSTRACTLOGLISTENER__
#define __MUTABSTRACTLOGLISTENER__

class Mut_abstract_log_listener {
public:
    virtual void send_text_msg(const char *buffer) = 0;
    virtual void send_xml_msg(const char *buffer) = 0;
};

#endif
