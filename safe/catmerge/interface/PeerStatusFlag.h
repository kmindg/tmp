// PeerStatusFlag.h
//
// Copyright (C) 2001	EMC
//
// This code provides methods to set and access the state (alive or dead)
// of the Peer SP.  Based on the SimpleReg class.
//
//	Revision History
//	----------------
//	23 Feb 01	H. Weiner	Initial version.
//	30 Sep 01	B. Yang		Changed OpenGlobal() to public method.
//-------------------------------------------------------------------Includes
//
#ifndef K10REG_UTIL_IMPL_H
#define K10REG_UTIL_IMPL_H

#define PEER_DEAD_KEY		"PeerDead"
#define PEERSTATUSFLAG_NAME	"PeerStatusFlag"

#include "K10TraceMsg.h"

class PeerStatusFlag:public SimpleReg
{
public:

	PeerStatusFlag() { INIT_K10TRACE( mnpTrace, mpWho, PEERSTATUSFLAG_NAME )};

	~PeerStatusFlag() {};

	long SetDead();

	long SetAlive();

	long GetStatus( bool &bIsDead );

	long OpenGlobal();

protected:

	NPtr	<K10TraceMsg> mnpTrace;
	char*	mpWho;

};

#endif
