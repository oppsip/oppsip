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

#include "osip_port.h"
#include "osip_uri.h"
#include "sdp_message.h"
#include "OPPState.h"

#define MIN_USER_EVENT  0
#define MIN_INTERNAL_EVENT 65536

enum dev_type_t
{
	DEV_TYPE_SIP_REGISTER,
	DEV_TYPE_SIP_RELAY,
	DEV_TYPE_SIP_ENDPOINT
};

class OPPSession;

typedef struct input_value_t
{
	char *LocalDomain;
	int nSip_Port;
	char *outbound_ip;
	int   outbound_port;
	OPPSession *pCallBack;
}input_value_t;

typedef struct InitParam_t
{
	input_value_t in;
}InitParam_t;

typedef struct sip_register_dev_t
{
	int  nChannels;
	char UserName[128];
	char Password[128];
	char Domain[128];
	int  nPort;
}sip_register_dev_t;

typedef struct sip_relay_dev_t
{
	int  nChannels;
	char Domain[128];
	char Host[32];
	int  nPort;
}sip_relay_dev_t;

typedef struct sip_endpoint_dev_t
{
	int  nChannels;
	char UserName[128];
	char Password[128];
}sip_endpoint_dev_t;

typedef struct DevPara_t
{
	dev_type_t	nDevType;
	void		*UserData;

	union x_dev_t
	{
		sip_register_dev_t	sip_register_dev;
		sip_relay_dev_t   sip_relay_dev;
		sip_endpoint_dev_t sip_endpoint_dev;
	}x_dev;

}DevPara_t;
/*
typedef void (*RTP_CALLBACK)(long,int,char *,int);

typedef void (*TIMER_CALLBACK_PTR)(int nFlag,void *UserData);

#define MIN_USER_TIMER_FLAG 0x00010000

typedef enum Rtp_Payload_t
{
	RTP_PAYLOAD_G711A = 8,
	RTP_PAYLOAD_G711U = 0,
	RTP_PAYLOAD_G729 = 18,
	RTP_PAYLOAD_G723 = 4
}Rtp_Payload_t;
*/
class OPPDev : public OPPState
{
public:
	OPPDev();
	virtual ~OPPDev();

	virtual void SetUserData(void *Data) = 0;

	virtual void *GetUserData() = 0;
};

class InCallParam_t
{
public:
	InCallParam_t(const char *caller,const char *callee,const char *body,const char *content_type,osip_uri_t *uri)
	{
		this->Caller = osip_strdup(caller);
		this->Callee = osip_strdup(callee);
		this->body = osip_strdup(body);
		this->content_type = osip_strdup(content_type);
		if(uri)
		{	
			osip_uri_clone(uri,&dest_uri);
			if(callee)
			{
				osip_free(dest_uri->username);
				dest_uri->username = osip_strdup(callee);	
			}
		}
		else
		{
			dest_uri = NULL;
		}
	}

	~InCallParam_t()
	{
		if(Caller)
			osip_free(Caller);
		if(Callee)
			osip_free(Callee);
		if(dest_uri)
			osip_uri_free(dest_uri);
		if(body)
			osip_free((void*)body);
		if(content_type)
			osip_free((void*)content_type);
	}

	InCallParam_t *Clone()
	{
		InCallParam_t *p = new InCallParam_t(Caller,Callee,body,content_type,dest_uri);
		return p;
	}
	const char *GetCaller()
	{
		return this->Caller;
	}
	const char *GetCallee()
	{
		return this->Callee;
	}
	const char *GetContentType()
	{
		return this->content_type;
	}
	osip_uri_t *GetDestUri()
	{
		return dest_uri;
	}
	void SetBody(const char *bd)
	{
		if(body)
			osip_free((void*)body);
		
		body = osip_strdup(bd);
	}

	const char *GetBody() const
	{
		return body;
	}
	
	void SetContentType(const char *ct)
	{
		if(content_type)
			osip_free((void*)content_type);

		content_type = osip_strdup(ct);
	}
	const char *GetContentType() const
	{
		return content_type;
	}
private:
	InCallParam_t()
	{	
	};
	char *Caller;
	char *Callee;
	osip_uri_t *dest_uri;
	const char *body;
	const char *content_type;
};

class AcceptParam_t
{
public:
	AcceptParam_t(int code,const char* number,const char *body,const char *content_type)
	{
		m_response_code = code;
		m_number = osip_strdup(number);
		m_body = osip_strdup(body);
		m_content_type = osip_strdup(content_type);
	}

	~AcceptParam_t()
	{
		if(m_number)
			osip_free((void*)m_number);
		if(m_body)
			osip_free((void*)m_body);
		if(m_content_type)
			osip_free((void*)m_content_type);
	}

	AcceptParam_t *Clone()
	{
		return new AcceptParam_t(m_response_code,m_number,m_body,m_content_type);
	}

	void SetBody(const char *body)
	{
		if(m_body)
			osip_free((void*)m_body);
		
		m_body = osip_strdup(body);
	}

	const char *GetBody() const
	{
		return m_body;
	}
	int GetResponseCode() const
	{
		return m_response_code;
	}
	const char *GetContentType() const
	{
		return m_content_type;
	}
	const char *GetNumber() const
	{
		return m_number;
	}
	void SetContentType(const char *content_type)
	{
		if(m_content_type)
			osip_free((void*)m_content_type);

		m_content_type = osip_strdup(content_type);
	}
private:
	int   m_response_code;
	const char *m_number;
	const char *m_body;
	const char *m_content_type;
};

long API_Init(InitParam_t *para);

void API_DebugInit(char *Server,int Port);

void *API_InitDev(DevPara_t *para);

long API_SetUserData(void * DevID,void *UserData); 

void API_DoDispatch();

int	 API_DoSipMsg(struct sockaddr_in *from_addr,char *msg,int msgLen);

int API_SendSipMessage(void * DevSip,char *from_name,char *body,char *content_type);
