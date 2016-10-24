/*
The oSIP library implements the Session Initiation Protocol (SIP -rfc3261-)
Copyright (C) 2001,2002,2003,2004,2005,2006,2007 Aymeric MOIZARD jack@atosc.org

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#pragma once
#include "OPPStateMachine.h"

class OSIP_Transaction;
class OSIP_Event;
class OSIP_IctSM : public OPPStateMachine<OSIP_Transaction,OSIP_Event>
{
public:
	OSIP_IctSM();
	~OSIP_IctSM();

	static OSIP_IctSM *sGetInstance();

private:
	static void OnSendInviteWhileReady(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnTimeoutWhileTrying(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnRcv1xxWhileTrying(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnRcv1xxWhileProceeding(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnRcv2xxWhileTrying(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnRcv2xxWhileProceeding(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnRcv3456xxWhileTrying(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnRcv3456xxWhileProceeding(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnRcv3456xxWhileCompleted(OSIP_Transaction *tr,OSIP_Event *evt);
	static void OnTimeoutWhileCompleted(OSIP_Transaction *tr,OSIP_Event *evt);

private:
	static OSIP_IctSM s_Inst;
};

class OSIP_NictSM : public OPPStateMachine<OSIP_Transaction,OSIP_Event>
{
public:
	OSIP_NictSM();
	~OSIP_NictSM();

	static OSIP_NictSM *sGetInstance();
private:
	static void OnSendRequestWhileReady(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnTimeoutWhileTrying(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnTimeoutWhileProceeding(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnRcv1xxWhileTrying(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnRcv1xxWhileProceeding(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnRcv2xxWhileTrying(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnRcv2xxWhileProceeding(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnRcv3456xxWhileTrying(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnRcv3456xxWhileProceeding(OSIP_Transaction *nict,OSIP_Event *evt);
	static void OnTimeoutWhileCompleted(OSIP_Transaction *nict,OSIP_Event *evt);

private:
	static OSIP_NictSM s_Inst;
};

class OSIP_IstSM : public OPPStateMachine<OSIP_Transaction,OSIP_Event>
{
public:
	OSIP_IstSM();
	~OSIP_IstSM();

	static OSIP_IstSM *sGetInstance();
private:
	static void OnRcvInviteWhileReady(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnRcvInviteWhileProceeding(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnRcvInviteWhileCompleted(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnTimeoutWhileCompleted(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnSend1xxWhileProceeding(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnSend2xxWhileProceeding(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnSend3456xxWhileProceeding(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnRcvAckWhileCompleted(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnRcvAckWhileConfirmed(OSIP_Transaction *ist,OSIP_Event *evt);
	static void OnTimeoutWhileConfirmed(OSIP_Transaction *ist,OSIP_Event *evt);

private:
	static OSIP_IstSM s_Inst;
};

class OSIP_NistSM : public OPPStateMachine<OSIP_Transaction,OSIP_Event>
{
public:
	OSIP_NistSM();
	~OSIP_NistSM();

	static OSIP_NistSM *sGetInstance();
private:

	static void OnRcvRequestWhileReady(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnRcvRequestWhileTrying(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnRcvRequestWhileCompleted(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnSend1xxWhileTrying(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnSend1xxWhileProceeding(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnSend2xxWhileTrying(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnSend2xxWhileProceeding(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnSend3456xxWhileTrying(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnSend3456xxWhileProceeding(OSIP_Transaction *nist,OSIP_Event *evt);
	static void OnTimeoutWhileCompleted(OSIP_Transaction *nist,OSIP_Event *evt);
private:
	static OSIP_NistSM s_Inst;
};

