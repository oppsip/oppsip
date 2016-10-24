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

#include "OSIP_Event.h"
#include "osip_via.h"
#include "osip_message.h"
#include "osip_parser.h"

OSIP_Event::type_t evt_set_type_incoming_sipmessage(osip_message_t * sip,osip_fsm_type_t *ctx_type)
{
	if (MSG_IS_REQUEST(sip)) 
	{
		if(MSG_IS_INVITE(sip))
		{
			*ctx_type = IST;
			return OSIP_Event::EVENT_RcvInvite;
		}
		else if(MSG_IS_ACK(sip))
		{
			*ctx_type = IST;
			return OSIP_Event::EVENT_RcvAck;
		}
		else
		{
			*ctx_type = NIST;
			return OSIP_Event::EVENT_RcvRequest;
		}
	}
	else
	{
		if(MSG_IS_RESPONSE_FOR(sip,"INVITE"))
			*ctx_type = ICT;
		else
			*ctx_type = NICT;

		if(MSG_IS_STATUS_1XX(sip))
			return OSIP_Event::EVENT_Rcv1xx;
		else if(MSG_IS_STATUS_2XX(sip))
			return OSIP_Event::EVENT_Rcv2xx;
		else
			return OSIP_Event::EVENT_Rcv3456xx;
	}
}

OSIP_Event::type_t evt_set_type_outgoing_sipmessage(osip_message_t * sip)
{
	if(MSG_IS_REQUEST(sip)) 
	{
		if(MSG_IS_INVITE(sip))
			return OSIP_Event::EVENT_SendInvite;
		else if(MSG_IS_ACK(sip))
			return OSIP_Event::EVENT_SendAck;
		else
			return OSIP_Event::EVENT_SendRequest;
	} 
	else
	{
		if(MSG_IS_STATUS_1XX(sip))
			return OSIP_Event::EVENT_Send1xx;
		else if(MSG_IS_STATUS_2XX(sip))
			return OSIP_Event::EVENT_Send2xx;
		else
			return OSIP_Event::EVENT_Send3456xx;
	}
}

OSIP_Event::OSIP_Event(int type)
:OPPEvent(type)
{
}

OSIP_Event::~OSIP_Event()
{
}

OSIP_MsgEvent * OSIP_MsgEvent::sCreate(const char *from_ip,int from_port, const char *buf,int len )
{
	int i;
	osip_message_t *sip;
	i = osip_message_init(&sip);
	if(i != 0)
		return NULL;

	sip->from_ip = osip_strdup(from_ip);
	sip->from_port = from_port;

	if(osip_message_parse(sip, buf, len) != 0) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"could not parse message\n"));
		osip_message_free(sip);
		return NULL;
	} 
	else
	{
		if(sip->call_id != NULL && sip->call_id->number != NULL) 
		{
			OSIP_TRACE(osip_trace
				(__FILE__, __LINE__, OSIP_INFO3, NULL,
				"MESSAGE REC. CALLID:%s\n", sip->call_id->number));
		}

		if(MSG_IS_REQUEST(sip)) 
		{
			if (sip->sip_method == NULL || sip->req_uri == NULL) 
			{
				osip_message_free(sip);
				return NULL;
			}
			osip_via_t *via;
			if( 0 == osip_message_get_via(sip,0,&via) )
			{
				if(via->host && 0 != strcmp(via->host,sip->from_ip))
				{
					osip_via_set_received(via,osip_strdup(sip->from_ip));
				}
				int via_port = 5060;
				if(via->port)
					via_port = osip_atoi(via->port);
				if(via_port != sip->from_port)
				{
					osip_generic_param_t *rp;
					char *rport = (char*)osip_malloc(20);
					sprintf(rport,"%d",sip->from_port);

					osip_via_param_get_byname(via,"rport",&rp);
					if(rp == NULL)
						osip_via_param_add(via,osip_strdup("rport"),rport);
					else
					{
						osip_free(rp->gvalue);
						rp->gvalue = rport;
					}
				}
			}
		}

		osip_fsm_type_t ctx_type;

		type_t evt_type = evt_set_type_incoming_sipmessage(sip,&ctx_type);

		OSIP_MsgEvent *evt = new OSIP_MsgEvent(sip,evt_type,ctx_type);

		return evt;
	}
}

OSIP_MsgEvent * OSIP_MsgEvent::sCreate( osip_message_t *sip,osip_fsm_type_t ctx_type )
{
	if (sip == NULL)
		return NULL;

	if (MSG_IS_REQUEST(sip)) 
	{
		if (sip->sip_method == NULL)
			return NULL;
		if (sip->req_uri == NULL)
			return NULL;
	}

	type_t evt_type = evt_set_type_outgoing_sipmessage(sip);

	return new OSIP_MsgEvent(sip,evt_type,ctx_type);
}

OSIP_MsgEvent::~OSIP_MsgEvent()
{
}

OSIP_MsgEvent::OSIP_MsgEvent( osip_message_t *sip,type_t evt_type,osip_fsm_type_t ctx_type )
:OSIP_Event(evt_type),m_sip(sip),m_ctx_type(ctx_type)
{
}

void OSIP_MsgEvent::FreeSipMsg()
{
	if(m_sip)
		osip_message_free(m_sip);
}


