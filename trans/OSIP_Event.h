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
#include "OPPEvent.h"
#include "osip_message.h"
#include "OSIP_Transaction.h"

class OSIP_Event :	public OPPEvent
{
public:
	OSIP_Event(int type);
	virtual ~OSIP_Event();

	typedef enum type_t {
		/* TIMEOUT EVENTS for ICT */
		EVENT_Timeout = 0,
		/* FOR INCOMING MESSAGE */
		EVENT_RcvInvite,/**< Event is an incoming INVITE request */
		EVENT_RcvAck,	  /**< Event is an incoming ACK request */
		EVENT_RcvRequest,  /**< Event is an incoming NON-INVITE and NON-ACK request */
		EVENT_Rcv1xx,	/**< Event is an incoming informational response */
		EVENT_Rcv2xx,	/**< Event is an incoming 2XX response */
		EVENT_Rcv3456xx,	/**< Event is an incoming final response (not 2XX) */

		/* FOR OUTGOING MESSAGE */
		EVENT_SendInvite,/**< Event is an outgoing INVITE request */
		EVENT_SendAck,	  /**< Event is an outgoing ACK request */
		EVENT_SendRequest,  /**< Event is an outgoing NON-INVITE and NON-ACK request */
		EVENT_Send1xx,	/**< Event is an outgoing informational response */
		EVENT_Send2xx,	/**< Event is an outgoing 2XX response */
		EVENT_Send3456xx, /**< Event is an outgoing final response (not 2XX) */

		EVENT_Kill, /**< Event to 'kill' the transaction before termination */
		//EVENT_Unknown,
		MAX_EVENT
	} type_t;

	virtual int GetFlag()
	{
		return TIMEOUT_UNKNOWN;
	}

	virtual osip_message_t *GetSipMsg()
	{
		return NULL;
	}

	virtual void FreeSipMsg()
	{
		return;
	}

private:
	int transactionid;	 /**< identifier of the related osip transaction */
};

class OSIP_TimerEvent : public OSIP_Event
{
public:
	explicit OSIP_TimerEvent(int flag)
		:OSIP_Event(EVENT_Timeout),m_Flag(flag)
	{
	}

	~OSIP_TimerEvent()
	{
	}
	
	virtual int GetFlag()
	{
		return m_Flag;
	}
private:
	int m_Flag;
};

class OSIP_MsgEvent : public OSIP_Event
{
public:
	virtual ~OSIP_MsgEvent();

	static OSIP_MsgEvent *sCreate(const char *from_ip,int from_port,const char *buf,int len);
	static OSIP_MsgEvent *sCreate(osip_message_t *sip,enum osip_fsm_type_t ctx_type);

	virtual void FreeSipMsg();

	virtual osip_message_t *GetSipMsg()
	{
		return m_sip;
	}

	osip_fsm_type_t GetCtxType()
	{
		return m_ctx_type;
	}
private:
	OSIP_MsgEvent(osip_message_t *sip,type_t type,osip_fsm_type_t ctx_type);
	osip_message_t *m_sip; 
	osip_fsm_type_t m_ctx_type;
};

