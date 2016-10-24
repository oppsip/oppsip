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

#include "OSIP_Transaction.h"
#include "osip_parser.h"
#include "OSIP_TransactionSM.h"
#include "OSIP_TransManager.h"

#include "OSIP_Event.h"
#include "OSIP_Core.h"

int OSIP_Transaction::ict_context_init(osip_message_t * invite)
{
	osip_route_t *route;
	int i;
	time_t now;

	OSIP_TRACE(osip_trace
		(__FILE__, __LINE__, OSIP_INFO2, NULL, "allocating ICT context\n"));

	ict_context = (osip_ict_t *) osip_malloc(sizeof(osip_ict_t));
	if (ict_context == NULL)
		return OSIP_NOMEM;

	now = time(NULL);
	memset(ict_context, 0, sizeof(osip_ict_t));
	/* for INVITE retransmissions */
	{
		osip_via_t *via;
		char *proto;

		i = osip_message_get_via(invite, 0, &via);	/* get top via */
		if (i < 0)
		{
			osip_free(ict_context);
			ict_context = NULL;
			return i;
		}
		proto = via_get_protocol(via);
		if (proto == NULL) 
		{
			osip_free(ict_context);
			ict_context = NULL;
			return OSIP_SYNTAXERROR;
		}
#ifdef USE_BLOCKINGSOCKET
		if(osip_strcasecmp(proto, "TCP") != 0 && osip_strcasecmp(proto, "TLS") != 0 && osip_strcasecmp(proto, "SCTP") != 0) 
		{	/* for other reliable protocol than TCP, the timer must be desactived by the external application */
			ict_context->timer_a_length = DEFAULT_T1;
			if (64 * DEFAULT_T1 < 32000)
				ict_context->timer_d_length = 32000;
			else
				ict_context->timer_d_length = 64 * DEFAULT_T1;

			ict_context->timer_a_start.tv_sec = -1;
			ict_context->timer_d_start.tv_sec = -1;	/* not started */
		} 
		else
		{				/* reliable protocol is used: */
			ict_context->timer_a_length = -1;	/* A is not ACTIVE */
			ict_context->timer_d_length = 0;	/* MUST do the transition immediatly */
			ict_context->timer_a_start.tv_sec = -1;	/* not started */
			ict_context->timer_d_start.tv_sec = -1;	/* not started */
		}
	}
#else
		if (osip_strcasecmp(proto, "TCP") != 0 && osip_strcasecmp(proto, "TLS") != 0 && osip_strcasecmp(proto, "SCTP") != 0) 
		{	/* for other reliable protocol than TCP, the timer																												must be desactived by the external application */
			ict_context->timer_a_length = DEFAULT_T1;
			if (64 * DEFAULT_T1 < 32000)
				ict_context->timer_d_length = 32000;
			else
				ict_context->timer_d_length = 64 * DEFAULT_T1;

			ict_context->timer_a_start.tv_sec = -1;
			ict_context->timer_d_start.tv_sec = -1;	/* not started */
		} 
		else
		{				/* reliable protocol is used: */
			ict_context->timer_a_length = DEFAULT_T1;
			ict_context->timer_d_length = 0;	/* MUST do the transition immediatly */
			ict_context->timer_a_start.tv_sec = -1;
			ict_context->timer_d_start.tv_sec = -1;	/* not started */
		}
	}
#endif

	/* for PROXY, the destination MUST be set by the application layer,
	this one may not be correct. */
	osip_message_get_route(invite, 0, &route);
	if (route != NULL && route->url != NULL) 
	{
		osip_uri_param_t *lr_param;
		osip_uri_uparam_get_byname(route->url, "lr", &lr_param);
		if (lr_param == NULL) 
		{
			/* using uncompliant proxy: destination is the request-uri */
			route = NULL;
		}
	}

	if (route != NULL) 
	{
		int port = 5060;

		if (route->url->port != NULL)
			port = osip_atoi(route->url->port);

		ict_set_destination(osip_strdup(route->url->host), port);
	}
	else
	{
		int port = 5060;
		/* search for maddr parameter */
		osip_uri_param_t *maddr_param = NULL;

		port = 5060;
		if (invite->req_uri->port != NULL)
			port = osip_atoi(invite->req_uri->port);

		osip_uri_uparam_get_byname(invite->req_uri, "maddr", &maddr_param);
		if (maddr_param != NULL && maddr_param->gvalue != NULL)
			ict_set_destination(osip_strdup(maddr_param->gvalue),port);
		else
			ict_set_destination(osip_strdup(invite->req_uri->host),port);
	}

	ict_context->timer_b_length = 64 * DEFAULT_T1;
	ict_context->timer_b_start.tv_sec = -1;

	return OSIP_SUCCESS;
}

int OSIP_Transaction::ict_context_free()
{
	if(ict_context == NULL)
		return OSIP_SUCCESS;

	osip_free(ict_context->destination);
	osip_free(ict_context);
	ict_context = NULL;
	return OSIP_SUCCESS;
}

int OSIP_Transaction::ict_set_destination(char *destination, int port)
{
	if (ict_context == NULL)
		return OSIP_BADPARAMETER;
	if (ict_context->destination != NULL)
		osip_free(ict_context->destination);
	ict_context->destination = destination;
	ict_context->port = port;
	return OSIP_SUCCESS;
}

int OSIP_Transaction::ist_context_init(osip_message_t * invite)
{
	int i;

	OSIP_TRACE(osip_trace
		(__FILE__, __LINE__, OSIP_INFO2, NULL, "allocating IST context\n"));

	ist_context = (osip_ist_t *) osip_malloc(sizeof(osip_ist_t));
	if (ist_context == NULL)
		return OSIP_NOMEM;
	memset(ist_context, 0, sizeof(osip_ist_t));
	/* for INVITE retransmissions */
	{
		osip_via_t *via;
		char *proto;

		i = osip_message_get_via(invite, 0, &via);	/* get top via */
		if (i < 0) 
		{
			osip_free(ist_context);
			ist_context = NULL;
			return i;
		}
		proto = via_get_protocol(via);
		if (proto == NULL) 
		{
			osip_free(ist_context);
			ist_context = NULL;
			return OSIP_UNDEFINED_ERROR;
		}

		if(osip_strcasecmp(proto, "TCP") != 0 && osip_strcasecmp(proto, "TLS") != 0 && osip_strcasecmp(proto, "SCTP") != 0) 
		{	/* for other reliable protocol than TCP, the timer must be desactived by the external application */
			ist_context->timer_g_length = DEFAULT_T1;
			ist_context->timer_i_length = DEFAULT_T4;
			ist_context->timer_g_start.tv_sec = -1;	/* not started */
			ist_context->timer_i_start.tv_sec = -1;	/* not started */
		} 
		else
		{	/* reliable protocol is used: */
			ist_context->timer_g_length = -1;	/* A is not ACTIVE */
			ist_context->timer_i_length = 0;	/* MUST do the transition immediatly */
			ist_context->timer_g_start.tv_sec = -1;	/* not started */
			ist_context->timer_i_start.tv_sec = -1;	/* not started */
		}
	}

	ist_context->timer_h_length = 64 * DEFAULT_T1;
	ist_context->timer_h_start.tv_sec = -1;	/* not started */

	return OSIP_SUCCESS;
}

int OSIP_Transaction::ist_context_free()
{
	if (ist_context == NULL)
		return OSIP_SUCCESS;
	
	osip_free(ist_context);
	ist_context = NULL;
	return OSIP_SUCCESS;
}

int OSIP_Transaction::nict_context_init(osip_message_t * request)
{
	osip_route_t *route;
	int i;
	time_t now;

	nict_context = (osip_nict_t *) osip_malloc(sizeof(osip_nict_t));
	if (nict_context == NULL)
		return OSIP_NOMEM;
	now = time(NULL);
	memset(nict_context, 0, sizeof(osip_nict_t));
	/* for REQUEST retransmissions */
	{
		osip_via_t *via;
		char *proto;

		i = osip_message_get_via(request, 0, &via);	/* get top via */
		if (i < 0) 
		{
			osip_free(nict_context);
			nict_context = NULL;
			return i;
		}
		proto = via_get_protocol(via);
		if (proto == NULL)
		{
			osip_free(nict_context);
			nict_context = NULL;
			return OSIP_UNDEFINED_ERROR;
		}
#ifdef USE_BLOCKINGSOCKET
		if(osip_strcasecmp(proto, "TCP") != 0 && osip_strcasecmp(proto, "TLS") != 0 && osip_strcasecmp(proto, "SCTP") != 0) 
		{
			nict_context->timer_e_length = DEFAULT_T1;
			nict_context->timer_k_length = DEFAULT_T4;
			nict_context->timer_e_start.tv_sec = -1;
			nict_context->timer_k_start.tv_sec = -1;	/* not started */
		}
		else
		{	/* reliable protocol is used: */
			nict_context->timer_e_length = -1;	/* E is not ACTIVE */
			nict_context->timer_k_length = 0;	/* MUST do the transition immediatly */
			nict_context->timer_e_start.tv_sec = -1;
			nict_context->timer_k_start.tv_sec = -1;	/* not started */
		}
	}
#else
		if(osip_strcasecmp(proto, "TCP") != 0 && osip_strcasecmp(proto, "TLS") != 0	&& osip_strcasecmp(proto, "SCTP") != 0) 
		{
			nict_context->timer_e_length = DEFAULT_T1;
			nict_context->timer_k_length = DEFAULT_T4;
			nict_context->timer_e_start.tv_sec = -1;
			nict_context->timer_k_start.tv_sec = -1;	/* not started */
		}
		else
		{		/* reliable protocol is used: */
			nict_context->timer_e_length = DEFAULT_T1;
			nict_context->timer_k_length = 0;	/* MUST do the transition immediatly */
			nict_context->timer_e_start.tv_sec = -1;
			nict_context->timer_k_start.tv_sec = -1;	/* not started */
		}
	}
#endif
	/* for PROXY, the destination MUST be set by the application layer,
	this one may not be correct. */
	osip_message_get_route(request, 0, &route);
	if(route != NULL && route->url != NULL)
	{
		osip_uri_param_t *lr_param;
		osip_uri_uparam_get_byname(route->url, "lr", &lr_param);
		if (lr_param == NULL) 
		{
			/* using uncompliant proxy: destination is the request-uri */
			route = NULL;
		}
	}

	if (route != NULL) 
	{
		int port = 5060;

		if (route->url->port != NULL)
			port = osip_atoi(route->url->port);
		nict_set_destination(osip_strdup(route->url->host), port);
	} 
	else
	{
		int port = 5060;
		/* search for maddr parameter */
		osip_uri_param_t *maddr_param = NULL;

		port = 5060;
		if (request->req_uri->port != NULL)
			port = osip_atoi(request->req_uri->port);

		osip_uri_uparam_get_byname(request->req_uri, "maddr", &maddr_param);
		if (maddr_param != NULL && maddr_param->gvalue != NULL)
			nict_set_destination(osip_strdup(maddr_param->gvalue), port);
		else
			nict_set_destination(osip_strdup(request->req_uri->host), port);
	}

	nict_context->timer_f_length = 64 * DEFAULT_T1;
	nict_context->timer_f_start.tv_sec = -1;

	return OSIP_SUCCESS;
}

int OSIP_Transaction::nict_context_free()
{
	if (nict_context == NULL)
		return OSIP_SUCCESS;

	osip_free(nict_context->destination);
	osip_free(nict_context);
	nict_context = NULL;
	return OSIP_SUCCESS;
}

int OSIP_Transaction::nict_set_destination(char *destination, int port)
{
	if (nict_context->destination != NULL)
		osip_free(nict_context->destination);
	nict_context->destination = destination;
	nict_context->port = port;
	return OSIP_SUCCESS;
}

int OSIP_Transaction::nist_context_init(osip_message_t * invite)
{
	int i;
	nist_context = (osip_nist_t *) osip_malloc(sizeof(osip_nist_t));
	if(nist_context == NULL)
		return OSIP_NOMEM;
	memset(nist_context, 0, sizeof(osip_nist_t));
	/* for INVITE retransmissions */
	{
		osip_via_t *via;
		char *proto;

		i = osip_message_get_via(invite, 0, &via);	/* get top via */
		if(i < 0) 
		{
			osip_free(nist_context);
			nist_context = NULL;
			return i;
		}
		proto = via_get_protocol(via);
		if(proto == NULL)
		{
			osip_free(nist_context);
			nist_context = NULL;
			return OSIP_UNDEFINED_ERROR;
		}

		if(osip_strcasecmp(proto, "TCP") != 0 && osip_strcasecmp(proto, "TLS") != 0 && osip_strcasecmp(proto, "SCTP") != 0) 
		{
			(nist_context)->timer_j_length = 64 * DEFAULT_T1;
			(nist_context)->timer_j_start.tv_sec = -1;	/* not started */
		}
		else
		{	/* reliable protocol is used: */
			(nist_context)->timer_j_length = 0;	/* MUST do the transition immediatly */
			(nist_context)->timer_j_start.tv_sec = -1;	/* not started */
		}
	}

	return OSIP_SUCCESS;
}

int OSIP_Transaction::nist_context_free()
{
	if (nist_context == NULL)
		return OSIP_SUCCESS;

	osip_free(nist_context);
	nist_context = NULL;
	return OSIP_SUCCESS;
}

OSIP_Transaction::OSIP_Transaction(osip_fsm_type_t type)
:OPPState(),user_instance(NULL),config(NULL)
,topvia(NULL),from(NULL),to(NULL),callid(NULL),cseq(NULL),orig_request(NULL),last_response(NULL),ack(NULL)
,ctx_type(type),ict_context(NULL),nict_context(NULL),ist_context(NULL),nist_context(NULL),m_MapKey(NULL),m_refCount(0)
{
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"OSIP_Transaction=%p type=%d\n",this,type));

	switch(type)
	{
	case ICT:
		m_StateMachine = OSIP_IctSM::sGetInstance();
		break;
	case NICT:
		m_StateMachine = OSIP_NictSM::sGetInstance();
		break;
	case IST:
		m_StateMachine = OSIP_IstSM::sGetInstance();
		break;
	case NIST:
		m_StateMachine = OSIP_NistSM::sGetInstance();
		break;
	default:
		m_StateMachine = NULL;
		break;
	}
}

OSIP_Transaction::~OSIP_Transaction(void)
{
	FreeRes();
}

int OSIP_Transaction::InitRes(osip_message_t *request)
{
	int i;
	osip_via_t *via;

	if(request == NULL)
		return OSIP_SYNTAXERROR;

	orig_request = request; //MUST NOT move this line to other places in this function;

	via = (osip_via_t*)osip_list_get(&request->vias, 0);
	if(via == NULL)
		return OSIP_SYNTAXERROR;

	i = osip_via_clone(via, &topvia);
	if(i != 0)
		return OSIP_BADPARAMETER;

	i = osip_from_clone(request->from, &from);
	if (i != 0)
		return OSIP_BADPARAMETER;

	i = osip_to_clone(request->to, &to);
	if(i != 0)
		return OSIP_BADPARAMETER;

	i = osip_call_id_clone(request->call_id, &callid);
	if(i != 0)
		return OSIP_BADPARAMETER;

	i = osip_cseq_clone(request->cseq, &cseq);
	if(i != 0)
		return OSIP_BADPARAMETER;

	switch(ctx_type)
	{
	case ICT:
		i = ict_context_init(request);
		break;
	case IST:
		i = ist_context_init(request);
		break;
	case NICT:
		i = nict_context_init(request);
		break;
	case NIST:
		i = nist_context_init(request);
		break;
	}
	if(i != 0)
		return OSIP_BADPARAMETER;

	osip_generic_param_t *branch;
	osip_via_param_get_byname(topvia, "branch", &branch);
	if(branch == NULL)
		return OSIP_BADPARAMETER;

	m_MapKey = osip_strdup(branch->gvalue);

	OSIP_TransManager::sGetInstance()->AddTrans(m_MapKey,this);

	return OSIP_SUCCESS;
}

void OSIP_Transaction::FreeRes()
{
	OSIP_TRACE(osip_trace
		(__FILE__, __LINE__, OSIP_INFO2, NULL, "OSIP_Transaction::FreeRes() %p type=%d\n",this,ctx_type));

	if(ict_context)
	{
		if(ict_context->timer_a_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(ict_context->timer_a_start),TIMEOUT_A,static_cast<OPPTimerAware*>(this));
		if(ict_context->timer_b_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(ict_context->timer_b_start),TIMEOUT_B,static_cast<OPPTimerAware*>(this));
		if(ict_context->timer_d_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(ict_context->timer_d_start),TIMEOUT_D,static_cast<OPPTimerAware*>(this));

		ict_context_free();
	}
	else if(nict_context)
	{
		if(nict_context->timer_e_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(nict_context->timer_e_start),TIMEOUT_E,static_cast<OPPTimerAware*>(this));
		if(nict_context->timer_f_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(nict_context->timer_f_start),TIMEOUT_F,static_cast<OPPTimerAware*>(this));
		if(nict_context->timer_k_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(nict_context->timer_k_start),TIMEOUT_K,static_cast<OPPTimerAware*>(this));

		nict_context_free();
	}
	else if(ist_context)
	{
		if(ist_context->timer_g_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(ist_context->timer_g_start),TIMEOUT_G,static_cast<OPPTimerAware*>(this));
		if(ist_context->timer_h_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(ist_context->timer_h_start),TIMEOUT_H,static_cast<OPPTimerAware*>(this));
		if(ist_context->timer_i_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(ist_context->timer_i_start),TIMEOUT_I,static_cast<OPPTimerAware*>(this));

		ist_context_free();
	}
	else if(nist_context)
	{
		if(nist_context->timer_j_start.tv_sec > 0)
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(nist_context->timer_j_start),TIMEOUT_J,static_cast<OPPTimerAware*>(this));

		nist_context_free();
	}

	osip_message_free(orig_request);
	osip_message_free(last_response);
	osip_message_free(ack);

	osip_via_free(topvia);
	osip_from_free(from);
	osip_to_free(to);
	osip_call_id_free(callid);
	osip_cseq_free(cseq);
	if(m_MapKey)
		osip_free(m_MapKey);
	m_MapKey = NULL;
}

int OSIP_Transaction::DeleteTrans()
{
	if(GetCtxType() == ICT)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"--Trans:%p ICT:ref=%d\n",this,m_refCount));
	}

	if(atomic_get_value(&m_refCount) == 0)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"--Trans:%p real deleted\n",this));
		
		delete this;
		return 1;
	}
	else
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"--Trans:%p ref=%d\n",this,m_refCount));
		atomic_decrement(&m_refCount);
		return 0;
	}
}

void OSIP_Transaction::OnTimeout(int flag,void *para)
{
	this->Execute(new OSIP_TimerEvent(flag));
}

int OSIP_Transaction::Execute(OSIP_Event *evt)
{
	int bDel = 0;

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"--Trans:%p State:%d Event:%d...\n",this,GetState(),evt->GetType()));

	if(0 != m_StateMachine->ExecuteSM(this,evt))
	{
		evt->FreeSipMsg();
		if(evt->GetType() == OSIP_Event::EVENT_Kill)
		{
			OSIP_TransManager::sGetInstance()->RemoveTrans(this);
			bDel = DeleteTrans();
		}
	}
	else
	{
		if(GetState() == STATE_Terminated)
		{
			SetState(STATE_Ready);
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"RemoveTrans=%p\n",this));
			OSIP_TransManager::sGetInstance()->RemoveTrans(this);
			bDel = DeleteTrans();
		}
	}

	delete evt;

	return bDel;
}

void OSIP_Transaction::SetUserData( void *p )
{
	user_instance =  p;
}

void * OSIP_Transaction::GetUserData() const
{
	return user_instance;
}

void OSIP_Transaction::AddRef()
{
	atomic_increment(&m_refCount);
}

int OSIP_Transaction::SendResponse(osip_message_t * response)
{
	osip_via_t *via;
	char *host;
	int port;
	osip_generic_param_t *maddr;
	osip_generic_param_t *received;
	osip_generic_param_t *rport;

	via = (osip_via_t *) osip_list_get(&response->vias, 0);
	if(!via)
		return OSIP_SYNTAXERROR;

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

	if(rport == NULL || rport->gvalue == NULL) 
	{
		if (via->port != NULL)
			port = osip_atoi(via->port);
		else
			port = 5060;
	} 
	else
		port = osip_atoi(rport->gvalue);

	return OSIP_Core::sGetInstance()->SendMsg(response,host,port,OSIP_UDP,out_socket);
}





