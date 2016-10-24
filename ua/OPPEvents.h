 /**
 * ,--. ,--, ,--, .--. ,-  ,--,
 * |  | |__| |__| |__   |  |__|
 * |  | |    |       |  |  |
 * '--' '    '    `--` --- '
 *
 * Copyright 2016 by Li Yucun <liycliyc@hotmail.com>
 * 
 * This file is part of OPPSIP.
 * 
 *  OPPSIP is free software:  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OPPSIP is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OPPSIP.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "OPPEvent.h"
#include "stdio.h"
#include "stdlib.h"
#include "osip_message.h"
#include "OPPApi.h"
#include "OPPSipService.h"

class OSIP_Transaction;

class OPPSipEvent : public OPPEvent
{
public:
	typedef enum opp_sip_event_t
	{
		EVENT_InCall = 0,
		EVENT_RingBack,
		EVENT_Accept,
		EVENT_Setup,
		EVENT_PeerHangup,
		EVENT_OnHook,
		EVENT_Recv2xx,
		EVENT_Timeout,
		EVENT_OutCall,
		EVENT_Error,
		EVENT_ReInvite,
		EVENT_Refer,
		EVENT_AllocMedia,
		EVENT_AllocResponse,
		EVENT_Dtmf,
		MAX_EVENT
	}opp_sip_event_t;

	explicit OPPSipEvent(opp_sip_event_t type)
		:OPPEvent(type)
	{
	}
	
	virtual ~OPPSipEvent()
	{
	}

public:
	virtual osip_message_t *GetSipMsg(){return NULL;}

	virtual int GetFlag(){return -1;}
	
	virtual osip_uri_t *GetDestUri(){return NULL;}
	virtual char *GetCallee(){return NULL;}
	virtual char *GetCaller(){return NULL;}
	
	virtual int GetResponseCode(){return -1;}
	virtual const char *GetSdpBody(){return NULL;}
	virtual const char *GetContentType(){return NULL;}
	virtual int GetAllocPara(const char **Host,int *Port){return -1;}
	virtual int GetDtmf(){return -1;}
};

class OPPSipOutCallEvent : public OPPSipEvent
{
public:
	explicit OPPSipOutCallEvent(const char *Caller,const char *Callee,const char *body,const char *content_type,osip_uri_t *uri)
		:OPPSipEvent(OPPSipEvent::EVENT_OutCall)
	{
		m_Caller = osip_strdup(Caller);
		m_Callee = osip_strdup(Callee);
		m_body	 = osip_strdup(body);
		m_content_type = osip_strdup(content_type);

		osip_uri_clone(uri,&m_dest_uri);
	}

	virtual ~OPPSipOutCallEvent()
	{
		if(m_Caller)
			osip_free(m_Caller);

		if(m_Callee)
			osip_free(m_Callee);

		if(m_body)
			osip_free(m_body);

		if(m_content_type)
			osip_free(m_content_type);

		if(m_dest_uri)
			osip_uri_free(m_dest_uri);
	}

	virtual char *GetCaller()
	{
		return m_Caller;
	}

	virtual char *GetCallee()
	{
		return m_Callee;
	}

	virtual osip_uri_t *GetDestUri()
	{
		return m_dest_uri;
	}

	virtual const char *GetSdpBody()
	{
		return m_body;
	}

	virtual const char *GetContentType()
	{
		return m_content_type;
	}

private:
	char *m_Caller;
	char *m_Callee;
	char *m_body;
	char *m_content_type;
	osip_uri_t *m_dest_uri;
};

class OPPSipReferEvent : public OPPSipEvent
{
public:
	explicit OPPSipReferEvent(OSIP_Transaction *tr,osip_message_t *sip)
		:OPPSipEvent(OPPSipEvent::EVENT_Refer),m_tr(tr),m_sip(sip)
	{
	}

	~OPPSipReferEvent()
	{
	}

	virtual osip_message_t *GetSipMsg()
	{
		return m_sip;
	}
private:
	OSIP_Transaction *m_tr;
	osip_message_t *m_sip;
};

class OPPSipInCallEvent : public OPPSipEvent
{
public:
	explicit OPPSipInCallEvent(OSIP_Transaction *tr,osip_message_t *sip)
		:OPPSipEvent(OPPSipEvent::EVENT_InCall),m_tr(tr),m_sip(sip)
	{
	}
	~OPPSipInCallEvent()
	{
	}

	virtual osip_message_t *GetSipMsg()
	{
		return m_sip;
	}
private:
	OSIP_Transaction *m_tr;
	osip_message_t *m_sip;
};

class OPPSipRingBackEvent : public OPPSipEvent
{
public:
	explicit OPPSipRingBackEvent(OSIP_Transaction *tr,osip_message_t *sip)
		:OPPSipEvent(OPPSipEvent::EVENT_RingBack),m_tr(tr),m_sip(sip)
	{
	}
	~OPPSipRingBackEvent()
	{
	}

	osip_message_t *GetSipMsg()
	{
		return m_sip;
	}
private:
	OSIP_Transaction *m_tr;
	osip_message_t *m_sip;
};

class OPPSipSetupEvent : public OPPSipEvent
{
public:
	explicit OPPSipSetupEvent(OSIP_Transaction *tr,osip_message_t *sip)
		:OPPSipEvent(OPPSipEvent::EVENT_Setup),m_tr(tr),m_sip(sip)
	{
	}
	~OPPSipSetupEvent()
	{
	}
	osip_message_t *GetSipMsg()
	{
		return m_sip;
	}
private:
	OSIP_Transaction *m_tr;
	osip_message_t *m_sip;
};

class OPPSipErrorEvent : public OPPSipEvent
{
public:
	explicit OPPSipErrorEvent(OSIP_Transaction *tr,osip_message_t *sip)
		:OPPSipEvent(OPPSipEvent::EVENT_Error),m_tr(tr),m_sip(sip)
	{
	}
	~OPPSipErrorEvent()
	{
	}
	osip_message_t *GetSipMsg()
	{
		return m_sip;
	}
private:
	OSIP_Transaction *m_tr;
	osip_message_t *m_sip;
};

class OPPSipEndCallEvent : public OPPSipEvent
{
public:
	explicit OPPSipEndCallEvent(OSIP_Transaction *tr,osip_message_t *sip)
		:OPPSipEvent(OPPSipEvent::EVENT_PeerHangup),m_tr(tr),m_sip(sip)
	{
	}
	~OPPSipEndCallEvent()
	{
	}
	osip_message_t *GetSipMsg()
	{
		return m_sip;
	}
private:
	OSIP_Transaction *m_tr;
	osip_message_t *m_sip;
};

class OPPSipCancelCallEvent : public OPPSipEvent
{
public:
	explicit OPPSipCancelCallEvent(OSIP_Transaction *tr,osip_message_t *sip)
		:OPPSipEvent(OPPSipEvent::EVENT_PeerHangup),m_tr(tr),m_sip(sip)
	{
	}
	~OPPSipCancelCallEvent()
	{
	}
	osip_message_t *GetSipMsg()
	{
		return m_sip;
	}
private:
	OSIP_Transaction *m_tr;
	osip_message_t *m_sip;
};

class OPPSipRetry2xxEvent : public OPPSipEvent
{
public:
	explicit OPPSipRetry2xxEvent(osip_message_t *sip)
		:OPPSipEvent(OPPSipEvent::EVENT_Recv2xx),m_sip(sip)
	{
	}
private:
	osip_message_t *m_sip;
};

class OPPSipAcceptEvent : public OPPSipEvent
{
public:
	explicit OPPSipAcceptEvent(int code,const char *number,const char *body,const char *content_type)
		:OPPSipEvent(OPPSipEvent::EVENT_Accept),m_nCode(code),m_body(NULL)
	{
		if(number)
		{
			if(OPPSipService::sGetInstance()->GetListenPort() == 5060)
				snprintf(m_mynumber,sizeof(m_mynumber),"sip:%s@%s",number,OPPSipService::sGetInstance()->GetLocalHost());
			else
				snprintf(m_mynumber,sizeof(m_mynumber),"sip:%s@%s:%d",number,OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort());

			m_mynumber[sizeof(m_mynumber)-1] = '\0';
		}
		else
			m_mynumber[0] = 0;

		m_body = osip_strdup(body);
		m_content_type = osip_strdup(content_type);
	}

	virtual int GetResponseCode()
	{
		return m_nCode;
	}

	virtual const char *GetSdpBody()
	{
		return m_body;
	}

	virtual const char *GetContentType()
	{
		return m_content_type;
	}

	~OPPSipAcceptEvent()
	{
		osip_free(m_body);
		osip_free(m_content_type);
	}
private:
	char  m_mynumber[128];
	int   m_nCode;
	char *m_body;
	char *m_content_type;
};


class OPPSipOnHookEvent : public OPPSipEvent
{
public:
	explicit OPPSipOnHookEvent()
		:OPPSipEvent(OPPSipEvent::EVENT_OnHook)
	{
	}
};

class OPPSipReInviteEvent : public OPPSipEvent
{
public:
	explicit OPPSipReInviteEvent(const char *body,const char *content_type)
		:OPPSipEvent(OPPSipEvent::EVENT_ReInvite)
	{
		m_body = osip_strdup(body);
		m_content_type = osip_strdup(content_type);
	}

	virtual const char *GetSdpBody()
	{
		return m_body;
	}
	
	virtual const char *GetContentType()
	{
		return m_content_type;
	}

	~OPPSipReInviteEvent()
	{
		if(m_body)
			osip_free(m_body);
		if(m_content_type)
			osip_free(m_content_type);
	}
private:
	char *m_body;
	char *m_content_type;
};

class OPPSipTimeoutEvent : public OPPSipEvent
{
public:
	OPPSipTimeoutEvent(int flag)
		:OPPSipEvent(OPPSipEvent::EVENT_Timeout),m_Flag(flag)
	{
	}

	virtual int GetFlag()
	{
		return m_Flag;
	}

private:
	int m_Flag;
};

class OPPSipDTMFEvent : public OPPSipEvent
{
public:
	OPPSipDTMFEvent(int dtmf)
		:OPPSipEvent(OPPSipEvent::EVENT_Dtmf),m_Dtmf(dtmf)
	{
	}
	virtual int GetDtmf()
	{
		return m_Dtmf;
	}
private:
	int m_Dtmf;
};

