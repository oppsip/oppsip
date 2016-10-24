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

#include "OPPApi.h"
#include "OPPChannelManager.h"

#include "OPPDebug.h"
#include "OPPTimerMonitor.h"

#include "OSIP_Core.h"
#include "OPPSipChannel.h"
#include "OSIP_Transport.h"

long API_Init( InitParam_t *para )
{
	return OPPChannelManager::sGetInstance()->Init(para);
}

void API_DebugInit(char *Server,int Port)
{
	NetDebugInit(NULL,555,Server,Port);
}

void *API_InitDev( DevPara_t *para )
{
	return OPPChannelManager::sGetInstance()->InitDev(para);
}

OPPSipDev *API_GetSipDev(const char *ipRelay,const char *Domain,const char *UserName)
{
	return OPPChannelManager::sGetInstance()->GetSipDev(ipRelay,Domain,UserName);
}

void API_DoDispatch()
{
	while(OPPTimerMonitor::sGetInstance()->CheckExpires())
	{
	};

	OSIP_Transport::sGetInstance()->Run();
}

int API_DoSipMsg( struct sockaddr_in *from_addr,char *msg,int msgLen )
{
	return OSIP_Core::sGetInstance()->DoSipMsg(from_addr,msg,msgLen);
}

OPPDev::OPPDev()
{
}

OPPDev::~OPPDev()
{
}

/*
OPPDevSipSetupEvent::OPPDevSipSetupEvent( void * DevID,int nChNo,const char *body ) 
:OPPDevEvent(DevID,OPPDevEvent::EVENT_SipSetup,nChNo),m_sdp(NULL)
{
	if(body)
	{
		sdp_message_init(&m_sdp);
		if(0 != sdp_message_parse(m_sdp,body))
		{
			sdp_message_free(m_sdp);
			m_sdp = NULL;
		}
	}
}

OPPDevSipSetupEvent::~OPPDevSipSetupEvent()
{
	if(m_sdp)
		sdp_message_free(m_sdp);
}

int OPPDevSipSetupEvent::GetMediaAddrPort( const char *media_type, char **AudioAddr,int *AudioPort )
{
	int pos = 0;
	char *media;

	if(m_sdp == NULL)
		return -1;

	while(media = sdp_message_m_media_get(m_sdp,pos))
	{
		if(strcmp(media,media_type) == 0)
		{
			char *audio_port = sdp_message_m_port_get(m_sdp,pos);
			char *c_addr = sdp_message_c_addr_get(m_sdp,pos,0);
			if(c_addr == NULL)
				c_addr = sdp_message_c_addr_get(m_sdp,-1,0);

			if(c_addr && audio_port)
			{
				*AudioAddr = c_addr;
				*AudioPort = osip_atoi(audio_port);
				return 0;
			}
		}
		pos++;
	}

	return -1;
}

int OPPDevSipSetupEvent::EnumPayloads(const char *media_type,int *Payload,int *iterator )
{
	if(*iterator < 0 || m_sdp == NULL)
		return -1;

	int pos = 0;
	char *media;

	while(media = sdp_message_m_media_get(m_sdp,pos))
	{
		if(strcmp(media,media_type) == 0)
		{
			char *str;

			if(str = sdp_message_m_payload_get(m_sdp,pos,*iterator))
			{
				*Payload = osip_atoi(str);
				return 0;
			}
			else
				return -1;
		}
		pos++;
	}

	return -1;
}

int OPPDevSipSetupEvent::GetAttr(const char *media_type, const char *field,const char *format,char *buf,int len )
{
	if(m_sdp == NULL)
		return -1;

	int pos = 0;
	char *media;

	while(media = sdp_message_m_media_get(m_sdp,pos))
	{
		if(strcmp(media,media_type) == 0)
		{
			char *str;
			int pos_attr = 0;

			while(str = sdp_message_a_att_field_get(m_sdp,pos,pos_attr))
			{
				if(strcmp(str,field) == 0)
				{
					char *val = sdp_message_a_att_value_get(m_sdp,pos,pos_attr);
					if(val == NULL || strlen(val) == 0)
						return 1;

					int  format_len = 0;

					if(format)
						format_len = strlen(format);

					if(format_len > 0)
					{
						if(strncmp(val,format,format_len) == 0)
						{
							while(*(val+format_len) == ' ')
								format_len++;

							strncpy(buf,val+format_len,len);
							buf[len-1] = 0;
							return 0;
						}
					}
					else
					{
						while(*(val+format_len) == ' ')
							format_len++;

						strncpy(buf,val+format_len,len);
						buf[len-1] = 0;
						return 0;
					}
				}

				pos_attr++;
			}
		}
		pos++;
	}

	return -1;
}

const char * OPPDevSipSetupEvent::GetSendRecvAttr(const char *media_type)
{
	char buf[255];
	if(GetAttr(media_type,"sendrecv","",buf,sizeof(buf)) == 1)
		return "sendrecv";
	else if(GetAttr(media_type,"recvonly","",buf,sizeof(buf)) == 1)
		return "recvonly";
	else if(GetAttr(media_type,"sendonly","",buf,sizeof(buf)) == 1)
		return "sendonly";
	else 
		return "sendrecv";
}

int OPPDevSipSetupEvent::GetPTime(const char *media_type)
{
	char buf[64];
	if(GetAttr(media_type,"ptime","",buf,sizeof(buf)) == 0)
	{
		return osip_atoi(buf);
	}
	return -1;
}

const char * OPPDevSipSetupEvent::EnumMediaType( int *iterator )
{
	if(m_sdp == NULL || *iterator < 0)
		return NULL;

	char *mt = sdp_message_m_media_get(m_sdp,*iterator);

	(*iterator)++;

	return mt;
}

OPPDevSipOutCallEvent::OPPDevSipOutCallEvent( void * DevID,int chNo,const char *disp_name,const char *caller,const char *Callee,const char *body ) :OPPDevEvent(DevID,OPPDevEvent::EVENT_SipOutCall,chNo),m_sdp(NULL)
{
	m_DispName = osip_strdup(disp_name);
	m_Caller = osip_strdup(caller);
	m_Callee = osip_strdup(Callee);

	if(body)
	{
		sdp_message_init(&m_sdp);
		if( 0 != sdp_message_parse(m_sdp,body) )
		{
			NetDbg(DBG_ERROR,"sdp_message_parse error!");
			sdp_message_free(m_sdp);
			m_sdp = NULL;
		}
		else
		{
			NetDbg(DBG_INFO,"sdp_message_parse succ!");
		}
	}
}

OPPDevSipOutCallEvent::~OPPDevSipOutCallEvent( void )
{
	if(m_DispName)
		osip_free(m_DispName);
	if(m_Caller)
		osip_free(m_Caller);
	if(m_Callee)
		osip_free(m_Callee);

	if(m_sdp)
		sdp_message_free(m_sdp);
}

int OPPDevSipOutCallEvent::GetMediaAddrPort(const char *media_type, char **AudioAddr,int *AudioPort )
{
	int pos = 0;
	char *media;

	if(m_sdp == NULL)
		return -1;

	while(media = sdp_message_m_media_get(m_sdp,pos))
	{
		if(strcmp(media,media_type) == 0)
		{
			char *audio_port = sdp_message_m_port_get(m_sdp,pos);
			char *c_addr = sdp_message_c_addr_get(m_sdp,pos,0);
			if(c_addr == NULL)
				c_addr = sdp_message_c_addr_get(m_sdp,-1,0);

			if(c_addr && audio_port)
			{
				*AudioAddr = c_addr;
				*AudioPort = osip_atoi(audio_port);
				return 0;
			}
		}
		pos++;
	}

	return -1;
}

int OPPDevSipOutCallEvent::EnumPayloads( const char *media_type,int *Payload,int *iterator )
{
	if(*iterator < 0 || m_sdp == NULL)
		return -1;

	int pos = 0;
	char *media;

	while(media = sdp_message_m_media_get(m_sdp,pos))
	{
		if(strcmp(media,media_type) == 0)
		{
			char *str;

			if(str = sdp_message_m_payload_get(m_sdp,pos,*iterator))
			{
				*Payload = osip_atoi(str);
				return 0;
			}
			else
				return -1;
		}
		pos++;
	}

	return -1;
}

int OPPDevSipOutCallEvent::GetAttr( const char *media_type,const char *field,const char *format,char *buf,int len )
{
	if(m_sdp == NULL)
		return -1;

	int pos = 0;
	char *media;

	while(media = sdp_message_m_media_get(m_sdp,pos))
	{
		if(strcmp(media,media_type) == 0)
		{
			char *str;
			int pos_attr = 0;

			while(str = sdp_message_a_att_field_get(m_sdp,pos,pos_attr))
			{
				if(strcmp(str,field) == 0)
				{
					char *val = sdp_message_a_att_value_get(m_sdp,pos,pos_attr);
					if(val == NULL || strlen(val) == 0)
						return 1;

					int  format_len = 0;

					if(format)
						format_len = strlen(format);

					if(format_len > 0)
					{
						if(strncmp(val,format,format_len) == 0)
						{
							while(*(val+format_len) == ' ')
								format_len++;

							strncpy(buf,val+format_len,len);
							buf[len-1] = 0;
							return 0;
						}
					}
					else
					{
						while(*(val+format_len) == ' ')
							format_len++;

						strncpy(buf,val+format_len,len);
						buf[len-1] = 0;
						return 0;
					}
				}

				pos_attr++;
			}
		}
		pos++;
	}

	return -1;
}

const char * OPPDevSipOutCallEvent::GetSendRecvAttr(const char *media_type)
{
	char buf[255];
	if(GetAttr(media_type,"sendrecv","",buf,sizeof(buf)) == 1)
		return "sendrecv";
	else if(GetAttr(media_type,"recvonly","",buf,sizeof(buf)) == 1)
		return "recvonly";
	else if(GetAttr(media_type,"sendonly","",buf,sizeof(buf)) == 1)
		return "sendonly";
	else 
		return "sendrecv";
}

int OPPDevSipOutCallEvent::GetPTime(const char *media_type)
{
	char buf[64];
	if(GetAttr(media_type,"ptime","",buf,sizeof(buf)) == 0)
	{
		return osip_atoi(buf);
	}
	return -1;
}

const char * OPPDevSipOutCallEvent::EnumMediaType( int *iterator )
{
	if(*iterator < 0 || m_sdp == NULL)
		return NULL;

	char *mt = sdp_message_m_media_get(m_sdp,*iterator);

	(*iterator)++;

	return mt;
}

*/