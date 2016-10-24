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

#include "OSIP_Core.h"
#include "OSIP_Dialog.h"
#include "osip_parser.h"

#include "OSIP_Event.h"
#include "OSIP_TransManager.h"
#include "osip_port.h"
#include "OPPDebug.h"
#include "OSIP_Transport.h"

OSIP_Core OSIP_Core::m_sInst;

OSIP_Core::OSIP_Core(void)
{
}

OSIP_Core::~OSIP_Core(void)
{
}

int OSIP_Core::SetCallback(int type, osip_message_cb_t cb)
{
	if(type >= OSIP_MESSAGE_CALLBACK_COUNT) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"invalid callback type %d\n", type));
		return OSIP_BADPARAMETER;
	}

	msg_callbacks[type] = cb;

	return OSIP_SUCCESS;
}

void OSIP_Core::DoCallback(int type,OSIP_Transaction *tr,osip_message_t *sip)
{
	if(msg_callbacks[type])
		msg_callbacks[type] (tr, sip);
}

int OSIP_Core::Init()
{
	memset(msg_callbacks,0,sizeof(msg_callbacks));

	if( OSIP_SUCCESS != parser_init() )
		return -1;

	return 0;
}

int OSIP_Core::DoTimerCheck()
{
	while(OPPTimerMonitor::sGetInstance()->CheckExpires())
	{
	}
	return 0;
}

int OSIP_Core::DoSipMsg(struct sockaddr_in *from_addr,char *buf,int len)
{
	OSIP_MsgEvent *evt = OSIP_MsgEvent::sCreate(inet_ntoa(from_addr->sin_addr),ntohs(from_addr->sin_port),buf,len);
	if(evt)
	{
		OSIP_TransManager::sGetInstance()->FindTransaction_andExcuteEvent(evt);
	}
	return 0;
}

int OSIP_Core::SendMsg(osip_message_t *sip, char *host, int port, int transport_type, int out_socket)
{
	int		i;
	char	*buf;
	size_t   bufLen;

	if(sip->outbound_ip)
	{
		host = sip->outbound_ip;
		port = sip->outbound_port;
	}

	if(host == NULL || port <= 0)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"host or port invalid, port=%d\n",port));
	}

	i = osip_message_to_str(sip,&buf,&bufLen);
	if(i != OSIP_SUCCESS)
		return -1;

	OSIP_Transport::sGetInstance()->SendMsg((uint8*)buf,bufLen,host,port,transport_type);
	
	osip_free(buf);

	return 0;
}

OSIP_Core *OSIP_Core::sGetInstance()
{
	return &m_sInst;
}

int generating_request_out_of_dialog(osip_message_t **dest, char *method_name, osip_uri_t *req_uri,char *from,char *to,
									 char *local_host,int local_port,char *contact,char *transport,char *outbound_ip,int outbound_port)
{
	/* Section 8.1:
	A valid request contains at a minimum "To, From, Call-iD, Cseq,
	Max-Forwards and Via
	*/
	int i;
	osip_message_t *request;

	i = osip_message_init(&request);
	if(i!=0)
		return -1;

	/* prepare the request-line */
	request->sip_method = osip_strdup(method_name);
	request->sip_version = osip_strdup("SIP/2.0");
	request->status_code = 0;
	request->reason_phrase = NULL;

	i = osip_message_set_to(request, to);
	if(i != 0)
	{
		osip_message_free(request);
		return -1;
	}

	i = osip_message_set_from(request,from);
	if(i != 0)
	{
		osip_message_free(request);
		return -1;
	}

	/* add a tag */
    osip_from_set_tag(request->from,new_random_string());

	if(req_uri == NULL)
	{
		i = osip_uri_clone(request->to->url, &(request->req_uri));
		if(i != 0)
		{
			osip_message_free(request);
			return -1;
		}
	}
	else
	{
		i = osip_uri_clone(req_uri, &(request->req_uri));
		if(i != 0)
		{
			osip_message_free(request);
			return -1;
		}
	}

	osip_call_id_init(&request->call_id);
	osip_call_id_set_host(request->call_id,osip_strdup(local_host));
	osip_call_id_set_number(request->call_id,new_random_string());
	
	osip_cseq_init(&request->cseq);
	osip_cseq_set_number(request->cseq,osip_strdup("20"));
	osip_cseq_set_method(request->cseq,osip_strdup(method_name));

	/* always add the Max-Forward header */
	osip_message_set_max_forwards(request,"70");

	char  via[128];
	sprintf(via, "SIP/2.0/%s %s:%d;branch=z9hG4bK%u;rport",transport,local_host,local_port,osip_build_random_number());
	osip_message_set_via(request,via);

	if(contact)
		osip_message_set_contact(request,contact);

	if(outbound_ip)
	{
		request->outbound_ip = osip_strdup(outbound_ip);
		request->outbound_port = outbound_port;
	}
	
	*dest = request;
	return 0;
}

int generating_response_default(osip_message_t **dest, OSIP_Dialog *dialog,
							int status, osip_message_t *request)
{
	osip_generic_param_t *tag;
	osip_message_t *response;
	int pos;
	int i;

	i = osip_message_init(&response);

	response->sip_version = (char *)osip_malloc(8);
	sprintf(response->sip_version,"SIP/2.0");
	response->status_code = status;
	response->reason_phrase = osip_strdup(osip_message_get_reason(status));

	response->req_uri = NULL;
	response->sip_method = NULL;

	i = osip_to_clone(request->to, &(response->to));
	if (i!=0) 
		goto grd_error_1;

	i = osip_to_get_tag(response->to,&tag);
	if(i!=0)
	{  /* we only add a tag if it does not already contains one! */
		if(dialog!=NULL && dialog->GetLocalTag()!=NULL)
			osip_to_set_tag(response->to, osip_strdup(dialog->GetLocalTag()));
		else if(status!=100)
				osip_to_set_tag(response->to, new_random_string());
	}

	i = osip_from_clone(request->from, &(response->from));
	if(i!=0) 
		goto grd_error_1;

	pos = 0;
	while(!osip_list_eol(&(request->vias),pos))
	{
		osip_via_t *via;
		osip_via_t *via2;
		via = (osip_via_t *)osip_list_get(&request->vias,pos);
		i = osip_via_clone(via, &via2);
		if( i != 0 )
			goto grd_error_1;
		osip_list_add(&response->vias, via2, -1);
		pos++;
	}

	i = osip_call_id_clone(request->call_id, &(response->call_id));
	if(i!=0)
		goto grd_error_1;
	i = osip_cseq_clone(request->cseq, &(response->cseq));
	if (i!=0)
		goto grd_error_1;

	*dest = response;
	return 0;

grd_error_1:
	osip_message_free(response);
	return -1;
}

int complete_answer_that_establish_a_dialog(char *contact,osip_message_t *response, osip_message_t *request)
{
	int i;
	int pos=0;
	/* 12.1.1:
	copy all record-route in response
	add a contact with global scope
	*/
	while(!osip_list_eol(&(request->record_routes), pos))
	{
		osip_record_route_t *rr;
		osip_record_route_t *rr2;
		rr = (osip_record_route_t*)osip_list_get(&request->record_routes, pos);
		i = osip_record_route_clone(rr, &rr2);
		if(i!=0)
			return -1;
		osip_list_add(&response->record_routes, rr2, -1);
		pos++;
	}

	if(contact != NULL)
		osip_message_set_contact(response, contact);

	return 0;
}

void osip_response_get_destination(osip_message_t * response, char **address,int *portnum)
{
	osip_via_t *via;
	char *host = NULL;
	int port = 0;

	via = (osip_via_t *) osip_list_get(&response->vias, 0);
	if (via) {
		osip_generic_param_t *maddr;
		osip_generic_param_t *received;
		osip_generic_param_t *rport;

		osip_via_param_get_byname(via, "maddr", &maddr);
		osip_via_param_get_byname(via, "received", &received);
		osip_via_param_get_byname(via, "rport", &rport);
		/* 1: user should not use the provided information
		(host and port) if they are using a reliable
		transport. Instead, they should use the already
		open socket attached to this transaction. */
		/* 2: check maddr and multicast usage */
		if (maddr != NULL)
			host = maddr->gvalue;
		/* we should check if this is a multicast address and use
		set the "ttl" in this case. (this must be done in the
		UDP message (not at the SIP layer) */
		else if (received != NULL)
			host = received->gvalue;
		else
			host = via->host;

		if (rport == NULL || rport->gvalue == NULL) {
			if (via->port != NULL)
				port = osip_atoi(via->port);
			else
				port = 5060;
		} else
			port = osip_atoi(rport->gvalue);
	}
	*portnum = port;
	if (host != NULL)
		*address = host;
	else
		*address = NULL;
}

int generating_cancel(osip_message_t **dest, osip_message_t *request_cancelled)
{
	int i;
	osip_message_t *request;

	i = osip_message_init(&request);
	if(i!=0) return -1;

	/* prepare the request-line */
	request->sip_method = osip_strdup("CANCEL");
	request->sip_version = osip_strdup("SIP/2.0");
	request->status_code = 0;
	request->reason_phrase = NULL;

	i = osip_uri_clone(request_cancelled->req_uri, &(request->req_uri));
	if (i!=0) goto gc_error_1;

	i = osip_to_clone(request_cancelled->to, &(request->to));
	if (i!=0) goto gc_error_1;
	i = osip_from_clone(request_cancelled->from, &(request->from));
	if (i!=0) goto gc_error_1;

	/* set the cseq and call_id header */
	i = osip_call_id_clone(request_cancelled->call_id, &(request->call_id));
	if (i!=0) goto gc_error_1;
	i = osip_cseq_clone(request_cancelled->cseq, &(request->cseq));
	if (i!=0) goto gc_error_1;
	osip_free(request->cseq->method);
	request->cseq->method = osip_strdup("CANCEL");

	/* copy ONLY the top most Via Field (this method is also used by proxy) */
	{
		osip_via_t *via;
		osip_via_t *via2;
		i = osip_message_get_via(request_cancelled, 0, &via);
		if(i!=0) goto gc_error_1;
		i = osip_via_clone(via, &via2);
		if(i!=0) goto gc_error_1;
		osip_list_add(&request->vias, via2, -1);
	}

	/* add the same route-set than in the previous request */
	{
		int pos=0;
		osip_route_t *route;
		osip_route_t *route2;
		while(!osip_list_eol(&request_cancelled->routes, pos))
		{
			route = (osip_route_t*)osip_list_get(&request_cancelled->routes, pos);
			i = osip_route_clone(route, &route2);
			if(i!=0) goto gc_error_1;
			osip_list_add(&request->routes, route2, -1);
			pos++;
		}
	}

	if(request_cancelled->outbound_ip)
	{
		request->outbound_ip = osip_strdup(request_cancelled->outbound_ip);
		request->outbound_port = request_cancelled->outbound_port;
	}
	
	/*  {
	proxy_authorization_t *proxy_authorization;
	proxy_authorization_t *proxy_authorization2;

	int pos = 0;
	int i;
	while (!list_eol (request_cancelled->proxy_authorizations, pos))
	{
	proxy_authorization =
	(proxy_authorization_t *) list_get (request_cancelled->proxy_authorizations, pos);
	i = proxy_authorization_clone (proxy_authorization,&proxy_authorization2);
	if (i != 0) goto gc_error_1;
	list_add (request->proxy_authorizations, proxy_authorization2, -1);
	pos++;
	}
	}
	*/
	osip_message_set_content_length(request, "0");
	osip_message_set_max_forwards(request,"70");

	*dest = request;
	return 0;

gc_error_1:
	osip_message_free(request);
	*dest = NULL;
	return -1;
}





