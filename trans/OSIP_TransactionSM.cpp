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

#include "OSIP_TransactionSM.h"
#include "OSIP_Transaction.h"
#include "OSIP_Core.h"
#include "OSIP_Event.h"

#include "osip_parser.h"

OSIP_IctSM OSIP_IctSM::s_Inst;
OSIP_NictSM OSIP_NictSM::s_Inst;
OSIP_IstSM OSIP_IstSM::s_Inst;
OSIP_NistSM OSIP_NistSM::s_Inst;

//////////////////////////////////// ICT StateMachine ////////////////////////////////////////////////

OSIP_IctSM::OSIP_IctSM(void)
:OPPStateMachine<OSIP_Transaction,OSIP_Event>(OSIP_Transaction::MAX_STATE,OSIP_Event::MAX_EVENT)
{
	//fprintf(stderr,"MAX_STATE=%d Event=%d m_pTable=%p\n",OSIP_Transaction::MAX_STATE,OSIP_Event::MAX_EVENT,m_pTable);
	STATE_MACHINE_IMPL(SendInvite,Ready,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv1xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv1xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv2xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv2xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv3456xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv3456xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv3456xx,Completed,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Timeout,Completed,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Timeout,Trying,OSIP_Transaction,OSIP_Event)
}

OSIP_IctSM::~OSIP_IctSM(void)
{
}

osip_message_t *ict_create_ack(OSIP_Transaction * ict, osip_message_t * response)
{
	int i;
	osip_message_t *ack;

	i = osip_message_init(&ack);
	if (i != 0)
		return NULL;

	/* Section 17.1.1.3: Construction of the ACK request: */
	i = osip_from_clone(response->from, &(ack->from));
	if (i != 0) {
		osip_message_free(ack);
		return NULL;
	}
	i = osip_to_clone(response->to, &(ack->to));	/* include the tag! */
	if (i != 0) {
		osip_message_free(ack);
		return NULL;
	}
	i = osip_call_id_clone(response->call_id, &(ack->call_id));
	if (i != 0) {
		osip_message_free(ack);
		return NULL;
	}
	i = osip_cseq_clone(response->cseq, &(ack->cseq));
	if (i != 0) {
		osip_message_free(ack);
		return NULL;
	}
	osip_free(ack->cseq->method);
	ack->cseq->method = osip_strdup("ACK");
	if (ack->cseq->method == NULL) 
	{
		osip_message_free(ack);
		return NULL;
	}

	ack->sip_method = (char *) osip_malloc(5);
	if (ack->sip_method == NULL) 
	{
		osip_message_free(ack);
		return NULL;
	}
	sprintf(ack->sip_method, "ACK");
	ack->sip_version = osip_strdup(ict->orig_request->sip_version);
	if (ack->sip_version == NULL) 
	{
		osip_message_free(ack);
		return NULL;
	}

	ack->status_code = 0;
	ack->reason_phrase = NULL;

	/* MUST copy REQUEST-URI from Contact header! */
	i = osip_uri_clone(ict->orig_request->req_uri, &(ack->req_uri));
	if (i != 0) {
		osip_message_free(ack);
		return NULL;
	}

	/* ACK MUST contain only the TOP Via field from original request */
	{
		osip_via_t *via;
		osip_via_t *orig_via;

		osip_message_get_via(ict->orig_request, 0, &orig_via);
		if (orig_via == NULL) {
			osip_message_free(ack);
			return NULL;
		}
		i = osip_via_clone(orig_via, &via);
		if (i != 0) {
			osip_message_free(ack);
			return NULL;
		}
		osip_list_add(&ack->vias, via, -1);
	}

	/* ack MUST contains the ROUTE headers field from the original request */
	/* IS IT TRUE??? */
	/* if the answers contains a set of route (or record route), then it */
	/* should be used instead?? ......May be not..... */
	{
		int pos = 0;
		osip_route_t *route;
		osip_route_t *orig_route;

		while (!osip_list_eol(&ict->orig_request->routes, pos)) {
			orig_route =
				(osip_route_t *) osip_list_get(&ict->orig_request->routes, pos);
			i = osip_route_clone(orig_route, &route);
			if (i != 0) {
				osip_message_free(ack);
				return NULL;
			}
			osip_list_add(&ack->routes, route, -1);
			pos++;
		}
	}

	if(ict->orig_request->outbound_ip)
	{
		ack->outbound_ip = osip_strdup(ict->orig_request->outbound_ip);
		ack->outbound_port = ict->orig_request->outbound_port;
	}

	/* may be we could add some other headers: */
	/* For example "Max-Forward" */

	return ack;
}

void OSIP_IctSM::OnSendInviteWhileReady( OSIP_Transaction *tr,OSIP_Event *evt )
{
	int i;

	if( OSIP_SUCCESS != tr->InitRes(evt->GetSipMsg()) )
	{
		tr->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}

	i = OSIP_Core::sGetInstance()->SendMsg(evt->GetSipMsg(), tr->ict_context->destination,
		tr->ict_context->port,OSIP_UDP,tr->out_socket);
	if(i < 0) 
	{
		//ict_handle_transport_error(tr, i);
		tr->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}
#ifndef USE_BLOCKINGSOCKET
	/*
	stop timer E in reliable transport - non blocking socket: 
	the message was just sent
	*/
	if (i == 0) 
	{				/* but message was really sent */
		osip_via_t *via;
		char *proto;

		i = osip_message_get_via(tr->orig_request, 0, &via);	/* get top via */
		if(i < 0) 
		{
			//ict_handle_transport_error(tr, i);
			tr->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}
		proto = via_get_protocol(via);
		if(proto == NULL) 
		{
			//ict_handle_transport_error(tr, i);
			tr->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}
		if(osip_strcasecmp(proto,"TCP") == 0 || osip_strcasecmp(proto,"TLS") == 0 || osip_strcasecmp(proto,"SCTP") == 0) 
		{
			/* reliable protocol is used: */
			tr->ict_context->timer_a_length = -1;	/* A is not ACTIVE */
			tr->ict_context->timer_a_start.tv_sec = -1;
		}
	}
#endif

//	__osip_message_callback(OSIP_ICT_INVITE_SENT, tr, tr->orig_request);
	if(tr->ict_context->timer_a_length >= 0)
		OPPTimerMonitor::sGetInstance()->AddTimer(tr->ict_context->timer_a_length,TIMEOUT_A,static_cast<OPPTimerAware*>(tr),NULL,&(tr->ict_context->timer_a_start));

	if(tr->ict_context->timer_b_length >= 0)
		OPPTimerMonitor::sGetInstance()->AddTimer(tr->ict_context->timer_b_length,TIMEOUT_B,static_cast<OPPTimerAware*>(tr),NULL,&(tr->ict_context->timer_b_start));

	tr->SetState(OSIP_Transaction::STATE_Trying);
}

void OSIP_IctSM::OnTimeoutWhileTrying( OSIP_Transaction *tr,OSIP_Event *evt )
{
	int i;

	if(evt->GetFlag() == TIMEOUT_A)
	{
		/* retransmit REQUEST */
		i = OSIP_Core::sGetInstance()->SendMsg(tr->orig_request, tr->ict_context->destination,
			tr->ict_context->port,OSIP_UDP,tr->out_socket);
		if(i < 0) 
		{
			tr->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}
#ifndef USE_BLOCKINGSOCKET
		/*
		stop timer E in reliable transport - non blocking socket: 
		the message was just sent
		*/
		if (i == 0) 
		{				/* but message was really sent */
			osip_via_t *via;
			char *proto;

			i = osip_message_get_via(tr->orig_request, 0, &via);	/* get top via */
			if (i < 0) 
			{
				tr->SetState(OSIP_Transaction::STATE_Terminated);
				return;
			}
			proto = via_get_protocol(via);
			if (proto == NULL) 
			{
				tr->SetState(OSIP_Transaction::STATE_Terminated);
				return;
			}
			if (osip_strcasecmp(proto, "TCP") != 0
				&& osip_strcasecmp(proto, "TLS") != 0
				&& osip_strcasecmp(proto, "SCTP") != 0) 
			{
			}
			else
			{				/* reliable protocol is used: */
				tr->ict_context->timer_a_length = -1;	/* A is not ACTIVE */
				tr->ict_context->timer_a_start.tv_sec = -1;
			}
		}
#endif
		if(tr->ict_context->timer_a_length > 0)
		{
			tr->ict_context->timer_a_length = tr->ict_context->timer_a_length * 2;

			OPPTimerMonitor::sGetInstance()->AddTimer(tr->ict_context->timer_a_length,TIMEOUT_A,static_cast<OPPTimerAware*>(tr),NULL,&(tr->ict_context->timer_a_start));
		}
	}
	else if(evt->GetFlag() == TIMEOUT_B)
	{
		tr->ict_context->timer_b_length = -1;

		tr->SetState(OSIP_Transaction::STATE_Terminated);

		//__osip_kill_transaction_callback(OSIP_ICT_KILL_TRANSACTION, tr);
	}
	else
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Receive unexpected timer: %d\n",evt->GetFlag()));
	}	
}

void OSIP_IctSM::OnRcv1xxWhileTrying( OSIP_Transaction *tr,OSIP_Event *evt )
{
	if(tr->ict_context->timer_a_start.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&(tr->ict_context->timer_a_start),TIMEOUT_A,static_cast<OPPTimerAware*>(tr));
		tr->ict_context->timer_a_start.tv_sec = -1;
	}
	
	if(tr->last_response != NULL)
		osip_message_free(tr->last_response);

	tr->last_response = evt->GetSipMsg();

	tr->SetState(OSIP_Transaction::STATE_Proceeding);

	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_1XX_RECEIVED, tr, evt->GetSipMsg());
}

void OSIP_IctSM::OnRcv1xxWhileProceeding( OSIP_Transaction *tr,OSIP_Event *evt )
{
	if(tr->last_response != NULL)
		osip_message_free(tr->last_response);

	tr->last_response = evt->GetSipMsg();

	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_1XX_RECEIVED, tr, evt->GetSipMsg());
}

void OSIP_IctSM::OnRcv2xxWhileTrying( OSIP_Transaction *tr,OSIP_Event *evt )
{
	if(tr->ict_context->timer_a_start.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&(tr->ict_context->timer_a_start),TIMEOUT_A,static_cast<OPPTimerAware*>(tr));
		tr->ict_context->timer_a_start.tv_sec = -1;
	}

	if(tr->last_response != NULL)
		osip_message_free(tr->last_response);
	
	tr->last_response = evt->GetSipMsg();

	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_2XX_RECEIVED, tr, evt->GetSipMsg());

	tr->SetState(OSIP_Transaction::STATE_Terminated);
}

void OSIP_IctSM::OnRcv2xxWhileProceeding( OSIP_Transaction *tr,OSIP_Event *evt )
{
	if(tr->last_response != NULL)
		osip_message_free(tr->last_response);

	tr->last_response = evt->GetSipMsg();

	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_2XX_RECEIVED, tr, evt->GetSipMsg());

	tr->SetState(OSIP_Transaction::STATE_Terminated);
}

void OSIP_IctSM::OnRcv3456xxWhileTrying(OSIP_Transaction *tr,OSIP_Event *evt)
{
	osip_route_t *route;
	int i;

	if(tr->ict_context->timer_a_start.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&(tr->ict_context->timer_a_start),TIMEOUT_A,static_cast<OPPTimerAware*>(tr));
		tr->ict_context->timer_a_start.tv_sec = -1;
	}

	/* leave this answer to the core application */
	if (tr->last_response != NULL)
		osip_message_free(tr->last_response);

	tr->last_response = evt->GetSipMsg();

	{	/* not a retransmission */
		/* automatic handling of ack! */
		osip_message_t *ack = ict_create_ack(tr, evt->GetSipMsg());

		tr->ack = ack;

		if (tr->ack == NULL) 
		{
			tr->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}

		/* reset ict->ict_context->destination only if
		it is not yet set. */
		if (tr->ict_context->destination == NULL) 
		{
			osip_message_get_route(ack, 0, &route);
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

			if (route != NULL && route->url != NULL) 
			{
				int port = 5060;

				if (route->url->port != NULL)
					port = osip_atoi(route->url->port);
				tr->ict_set_destination(osip_strdup(route->url->host), port);
			} 
			else
			{
				int port = 5060;
				/* search for maddr parameter */
				osip_uri_param_t *maddr_param = NULL;

				port = 5060;
				if (ack->req_uri->port != NULL)
					port = osip_atoi(ack->req_uri->port);

				osip_uri_uparam_get_byname(ack->req_uri, "maddr", &maddr_param);
				if (maddr_param != NULL && maddr_param->gvalue != NULL)
					tr->ict_set_destination(osip_strdup(maddr_param->gvalue),port);
				else
					tr->ict_set_destination(osip_strdup(ack->req_uri->host),port);
			}
		}
		i = OSIP_Core::sGetInstance()->SendMsg(ack, tr->ict_context->destination,
			tr->ict_context->port,OSIP_UDP,tr->out_socket);
		if (i < 0) 
		{
			tr->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}
		if (MSG_IS_STATUS_3XX(evt->GetSipMsg()))
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_3XX_RECEIVED, tr, evt->GetSipMsg());
		else if (MSG_IS_STATUS_4XX(evt->GetSipMsg()))
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_4XX_RECEIVED, tr, evt->GetSipMsg());
		else if (MSG_IS_STATUS_5XX(evt->GetSipMsg()))
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_5XX_RECEIVED, tr, evt->GetSipMsg());
		else
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_6XX_RECEIVED, tr, evt->GetSipMsg());

		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_ACK_SENT, tr, evt->GetSipMsg());
	}

	if(tr->ict_context->timer_b_start.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&(tr->ict_context->timer_b_start),TIMEOUT_B,static_cast<OPPTimerAware*>(tr));
		tr->ict_context->timer_b_start.tv_sec = -1;
	}

	/* start timer D (length is set to MAX (64*DEFAULT_T1 or 32000) */
	OPPTimerMonitor::sGetInstance()->AddTimer(tr->ict_context->timer_d_length,TIMEOUT_D,static_cast<OPPTimerAware*>(tr),NULL,&(tr->ict_context->timer_d_start));
	
	tr->SetState(OSIP_Transaction::STATE_Completed);
}

void OSIP_IctSM::OnRcv3456xxWhileProceeding(OSIP_Transaction *tr,OSIP_Event *evt)
{
	OnRcv3456xxWhileTrying(tr,evt);
}

void OSIP_IctSM::OnRcv3456xxWhileCompleted(OSIP_Transaction *tr,OSIP_Event *evt)
{
	int i;

	/* this could be another 3456xx ??? */
	/* we should make a new ACK and send it!!! */
	/* TODO */

//	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN, tr, evt->sip);

	evt->FreeSipMsg();

	i = OSIP_Core::sGetInstance()->SendMsg(tr->ack, tr->ict_context->destination,
		tr->ict_context->port,OSIP_UDP,tr->out_socket);
	if(i == 0) 
	{
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_ACK_SENT_AGAIN, tr, tr->ack);
	} 
	else 
	{
		tr->SetState(OSIP_Transaction::STATE_Terminated);
	}
}

void OSIP_IctSM::OnTimeoutWhileCompleted(OSIP_Transaction *tr,OSIP_Event *evt)
{
	if(evt->GetFlag() == TIMEOUT_D)
	{
		tr->SetState(OSIP_Transaction::STATE_Terminated);
	}
	else
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Receive unexpected timer: %d\n",evt->GetFlag()));
	}
}

OSIP_IctSM * OSIP_IctSM::sGetInstance()
{
	return &s_Inst;
}

//////////////////////////////////////////// NICT StateMachine ///////////////////////////////////////////////////
OSIP_NictSM::OSIP_NictSM(void)
:OPPStateMachine<OSIP_Transaction,OSIP_Event>(OSIP_Transaction::MAX_STATE,OSIP_Event::MAX_EVENT)
{
	STATE_MACHINE_IMPL(SendRequest,Ready,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv1xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv1xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv2xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv2xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv3456xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Rcv3456xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Timeout,Completed,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Timeout,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Timeout,Proceeding,OSIP_Transaction,OSIP_Event)
}

OSIP_NictSM::~OSIP_NictSM(void)
{
}

void OSIP_NictSM::OnSendRequestWhileReady( OSIP_Transaction *nict,OSIP_Event *evt )
{
	int i;

	if( OSIP_SUCCESS != nict->InitRes(evt->GetSipMsg()) )
	{
		nict->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}

	i = OSIP_Core::sGetInstance()->SendMsg(evt->GetSipMsg(), nict->nict_context->destination,
		nict->nict_context->port,OSIP_UDP,nict->out_socket);
	if(i == 0) 
	{
		if(nict->nict_context->timer_e_length > 0) 
			OPPTimerMonitor::sGetInstance()->AddTimer(nict->nict_context->timer_e_length,TIMEOUT_E,static_cast<OPPTimerAware*>(nict),NULL,&(nict->nict_context->timer_e_start));

		if(nict->nict_context->timer_f_length > 0)
			OPPTimerMonitor::sGetInstance()->AddTimer(nict->nict_context->timer_f_length,TIMEOUT_F,static_cast<OPPTimerAware*>(nict),NULL,&(nict->nict_context->timer_f_start));

		nict->SetState(OSIP_Transaction::STATE_Trying);
	} 
	else
	{
		nict->SetState(OSIP_Transaction::STATE_Terminated);
	}
}

void OSIP_NictSM::OnTimeoutWhileTrying( OSIP_Transaction *nict,OSIP_Event *evt )
{
	int i;

	if(evt->GetFlag() == TIMEOUT_E)
	{
		/* retransmit REQUEST */
		i = OSIP_Core::sGetInstance()->SendMsg(nict->orig_request, nict->nict_context->destination,
			nict->nict_context->port,OSIP_UDP,nict->out_socket);
		if(i < 0) 
		{
			nict->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}
#ifndef USE_BLOCKINGSOCKET
		/*
		stop timer E in reliable transport - non blocking socket: 
		the message was just sent
		*/
		if (i == 0) 
		{				/* but message was really sent */
			osip_via_t *via;
			char *proto;

			i = osip_message_get_via(nict->orig_request, 0, &via);	/* get top via */
			if(i < 0) 
			{
				nict->SetState(OSIP_Transaction::STATE_Terminated);
				return;
			}
			proto = via_get_protocol(via);
			if(proto == NULL) 
			{
				nict->SetState(OSIP_Transaction::STATE_Terminated);
				return;
			}
			if(osip_strcasecmp(proto, "TCP") != 0
				&& osip_strcasecmp(proto, "TLS") != 0
				&& osip_strcasecmp(proto, "SCTP") != 0) 
			{
			}
			else
			{	/* reliable protocol is used: */
				nict->nict_context->timer_e_length = -1;	/* E is not ACTIVE */
				nict->nict_context->timer_e_start.tv_sec = -1;
			}
		}
#endif
		/* reset timer */
		if(nict->nict_context->timer_e_length > 0)
		{
			nict->nict_context->timer_e_length = nict->nict_context->timer_e_length * 2;

			if(nict->nict_context->timer_e_length > 4000)
				nict->nict_context->timer_e_length = 4000;

			OPPTimerMonitor::sGetInstance()->AddTimer(nict->nict_context->timer_e_length,TIMEOUT_E,static_cast<OPPTimerAware*>(nict),NULL,&(nict->nict_context->timer_e_start));
		}

//		if (i == 0)					/* but message was really sent */
//			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NICT_REQUEST_SENT_AGAIN, nict,	nict->orig_request);
	}
	else if(evt->GetFlag() == TIMEOUT_F)
	{
		if(nict->nict_context->timer_e_start.tv_sec > 0)
		{
			OPPTimerMonitor::sGetInstance()->CancelTimer(&(nict->nict_context->timer_e_start),TIMEOUT_E,static_cast<OPPTimerAware*>(nict));
			nict->nict_context->timer_e_start.tv_sec = -1;
		}

		nict->SetState(OSIP_Transaction::STATE_Terminated);
	}
	else
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Receive unexpected timer: %d\n",evt->GetFlag()));
	}
	
}

void OSIP_NictSM::OnTimeoutWhileProceeding( OSIP_Transaction *nict,OSIP_Event *evt )
{
	int i;

	if(evt->GetFlag() == TIMEOUT_E)
	{
		/* retransmit REQUEST */
		i = OSIP_Core::sGetInstance()->SendMsg(nict->orig_request, nict->nict_context->destination,
			nict->nict_context->port,OSIP_UDP,nict->out_socket);
		if(i < 0) 
		{
			nict->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}
#ifndef USE_BLOCKINGSOCKET
		/*
		stop timer E in reliable transport - non blocking socket: 
		the message was just sent
		*/
		if (i == 0) 
		{				/* but message was really sent */
			osip_via_t *via;
			char *proto;

			i = osip_message_get_via(nict->orig_request, 0, &via);	/* get top via */
			if(i < 0) 
			{
				nict->SetState(OSIP_Transaction::STATE_Terminated);
				return;
			}
			proto = via_get_protocol(via);
			if(proto == NULL) 
			{
				nict->SetState(OSIP_Transaction::STATE_Terminated);
				return;
			}
			if(osip_strcasecmp(proto, "TCP") != 0
				&& osip_strcasecmp(proto, "TLS") != 0
				&& osip_strcasecmp(proto, "SCTP") != 0) 
			{
			}
			else
			{	/* reliable protocol is used: */
				nict->nict_context->timer_e_length = -1;	/* E is not ACTIVE */
				nict->nict_context->timer_e_start.tv_sec = -1;
			}
		}
#endif
		/* reset timer */
		if(nict->nict_context->timer_e_length > 0)
		{
			nict->nict_context->timer_e_length = 4000;

			OPPTimerMonitor::sGetInstance()->AddTimer(nict->nict_context->timer_e_length,TIMEOUT_E,static_cast<OPPTimerAware*>(nict),NULL,&(nict->nict_context->timer_e_start));
		}

		//		if (i == 0)					/* but message was really sent */
		//			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NICT_REQUEST_SENT_AGAIN, nict,	nict->orig_request);
	}
	else if(evt->GetFlag() == TIMEOUT_F)
	{
		nict->SetState(OSIP_Transaction::STATE_Terminated);
	}
	else
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Receive unexpected timer: %d\n",evt->GetFlag()));
	}

}

void OSIP_NictSM::OnRcv1xxWhileTrying( OSIP_Transaction *nict,OSIP_Event *evt )
{
	/* leave this answer to the core application */

	if(nict->last_response != NULL)
		osip_message_free(nict->last_response);
	
	nict->last_response = evt->GetSipMsg();
//	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NICT_STATUS_1XX_RECEIVED, nict, evt->sip);
	nict->SetState(OSIP_Transaction::STATE_Proceeding);
}

void OSIP_NictSM::OnRcv1xxWhileProceeding( OSIP_Transaction *nict,OSIP_Event *evt )
{
	OnRcv1xxWhileTrying(nict,evt);
}

void OSIP_NictSM::OnRcv2xxWhileTrying( OSIP_Transaction *nict,OSIP_Event *evt )
{
	/* leave this answer to the core application */

	if (nict->last_response != NULL)
		osip_message_free(nict->last_response);
	
	nict->last_response = evt->GetSipMsg();

	if(nict->nict_context->timer_e_start.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&(nict->nict_context->timer_e_start),TIMEOUT_E,static_cast<OPPTimerAware*>(nict));
		nict->nict_context->timer_e_start.tv_sec = -1;
	}

	if(nict->nict_context->timer_f_start.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&(nict->nict_context->timer_f_start),TIMEOUT_F,static_cast<OPPTimerAware*>(nict));
		nict->nict_context->timer_f_start.tv_sec = -1;
	}

	if(nict->nict_context->timer_k_length > 0)
	{	
		OPPTimerMonitor::sGetInstance()->AddTimer(nict->nict_context->timer_k_length,TIMEOUT_K,static_cast<OPPTimerAware*>(nict),NULL,&(nict->nict_context->timer_k_start));
		nict->SetState(OSIP_Transaction::STATE_Completed);
	}
	else if(nict->nict_context->timer_k_length == 0)
	{
		nict->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}
	else
	{
		nict->SetState(OSIP_Transaction::STATE_Completed);
	}

	if(MSG_IS_STATUS_2XX(nict->last_response))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NICT_STATUS_2XX_RECEIVED, nict,nict->last_response);
	else if(MSG_IS_STATUS_3XX(nict->last_response))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NICT_STATUS_3XX_RECEIVED, nict,nict->last_response);
	else if(MSG_IS_STATUS_4XX(nict->last_response))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NICT_STATUS_4XX_RECEIVED, nict,nict->last_response);
	else if(MSG_IS_STATUS_5XX(nict->last_response))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NICT_STATUS_5XX_RECEIVED, nict,nict->last_response);
	else if(MSG_IS_STATUS_6XX(nict->last_response))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NICT_STATUS_6XX_RECEIVED, nict,nict->last_response);
}

void OSIP_NictSM::OnRcv2xxWhileProceeding( OSIP_Transaction *nict,OSIP_Event *evt )
{
	OnRcv2xxWhileTrying(nict,evt);
}

void OSIP_NictSM::OnRcv3456xxWhileTrying( OSIP_Transaction *nict,OSIP_Event *evt )
{
	OnRcv2xxWhileTrying(nict,evt);
}

void OSIP_NictSM::OnRcv3456xxWhileProceeding( OSIP_Transaction *nict,OSIP_Event *evt )
{
	OnRcv2xxWhileTrying(nict,evt);
}

void OSIP_NictSM::OnTimeoutWhileCompleted( OSIP_Transaction *nict,OSIP_Event *evt )
{
	if(evt->GetFlag() == TIMEOUT_K)
	{
		nict->SetState(OSIP_Transaction::STATE_Terminated);
	}
	else
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Receive unexpected timer: %d\n",evt->GetFlag()));
	}
}

OSIP_NictSM * OSIP_NictSM::sGetInstance()
{
	return &s_Inst;
}
//////////////////////////////////// IST StateMachine /////////////////////////////////////////
OSIP_IstSM::OSIP_IstSM(void)
:OPPStateMachine<OSIP_Transaction,OSIP_Event>(OSIP_Transaction::MAX_STATE,OSIP_Event::MAX_EVENT)
{
	STATE_MACHINE_IMPL(RcvInvite,Ready,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(RcvInvite,Completed,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(RcvInvite,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(RcvAck,Completed,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(RcvAck,Confirmed,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send1xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send2xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send3456xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Timeout,Completed,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Timeout,Confirmed,OSIP_Transaction,OSIP_Event)
}

OSIP_IstSM::~OSIP_IstSM(void)
{
}

void OSIP_IstSM::OnRcvInviteWhileReady( OSIP_Transaction *ist,OSIP_Event *evt )
{
	if( OSIP_SUCCESS != ist->InitRes(evt->GetSipMsg()) )
	{
		ist->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}

	ist->SetState(OSIP_Transaction::STATE_Proceeding);

	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_INVITE_RECEIVED, ist, evt->GetSipMsg());
}

void OSIP_IstSM::OnRcvInviteWhileProceeding( OSIP_Transaction *ist,OSIP_Event *evt )
{
	int i;
	/* delete retransmission */
	evt->FreeSipMsg();

	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_INVITE_RECEIVED_AGAIN, ist,ist->orig_request);

	if(ist->last_response != NULL) 
	{	/* retransmit last response */
		i = ist->SendResponse(ist->last_response);
		if (i != 0) 
		{
			ist->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		} 
		else
		{
			if (MSG_IS_STATUS_1XX(ist->last_response))
				OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_1XX_SENT, ist,ist->last_response);
			else if (MSG_IS_STATUS_2XX(ist->last_response))
				OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_2XX_SENT_AGAIN, ist,ist->last_response);
			else
				OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_3456XX_SENT_AGAIN,ist, ist->last_response);
		}
	}
}

void OSIP_IstSM::OnRcvInviteWhileCompleted( OSIP_Transaction *ist,OSIP_Event *evt )
{
	OnRcvInviteWhileProceeding(ist,evt);
}

void OSIP_IstSM::OnTimeoutWhileCompleted( OSIP_Transaction *ist,OSIP_Event *evt )
{
	if(evt->GetFlag() == TIMEOUT_G)
	{
		int i;

		ist->ist_context->timer_g_length = ist->ist_context->timer_g_length * 2;
		if(ist->ist_context->timer_g_length > 4000)
			ist->ist_context->timer_g_length = 4000;
	
		OPPTimerMonitor::sGetInstance()->AddTimer(ist->ist_context->timer_g_length,TIMEOUT_G,static_cast<OPPTimerAware*>(ist),NULL,&(ist->ist_context->timer_g_start));

		i = ist->SendResponse(ist->last_response);
		if (i != 0) 
		{
			//ist_handle_transport_error(ist, i);
			ist->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}
	}
	else if(evt->GetFlag() == TIMEOUT_H)
	{
		ist->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}
//	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_3456XX_SENT_AGAIN, ist,ist->last_response);
}

void OSIP_IstSM::OnSend1xxWhileProceeding( OSIP_Transaction *ist,OSIP_Event *evt )
{
	int i;

	if (ist->last_response != NULL)
		osip_message_free(ist->last_response);
	
	ist->last_response = evt->GetSipMsg();

	i = ist->SendResponse(evt->GetSipMsg());
	if (i != 0) 
	{
		//ist_handle_transport_error(ist, i);
		ist->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}
	else
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_1XX_SENT, ist, ist->last_response);
}

void OSIP_IstSM::OnSend2xxWhileProceeding( OSIP_Transaction *ist,OSIP_Event *evt )
{
	int i;

	if (ist->last_response != NULL)
		osip_message_free(ist->last_response);
	
	ist->last_response = evt->GetSipMsg();

	i = ist->SendResponse(evt->GetSipMsg());
	if(i != 0) 
	{
		//ist_handle_transport_error(ist, i);
		ist->SetState(OSIP_Transaction::STATE_Terminated);
	}
	else 
		ist->SetState(OSIP_Transaction::STATE_Terminated);
}

void OSIP_IstSM::OnSend3456xxWhileProceeding( OSIP_Transaction *ist,OSIP_Event *evt )
{
	int i;

	if(ist->last_response != NULL)
		osip_message_free(ist->last_response);
	
	ist->last_response = evt->GetSipMsg();

	i = ist->SendResponse(evt->GetSipMsg());
	if (i != 0) 
	{
		//ist_handle_transport_error(ist, i);
		ist->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	} 

	if(ist->ist_context->timer_g_length > 0) 
		OPPTimerMonitor::sGetInstance()->AddTimer(ist->ist_context->timer_g_length,TIMEOUT_G,static_cast<OPPTimerAware*>(ist),NULL,&(ist->ist_context->timer_g_start));

	if(ist->ist_context->timer_h_length > 0)
		OPPTimerMonitor::sGetInstance()->AddTimer(ist->ist_context->timer_h_length,TIMEOUT_H,static_cast<OPPTimerAware*>(ist),NULL,&(ist->ist_context->timer_h_start));

	ist->SetState(OSIP_Transaction::STATE_Completed);

	if (MSG_IS_STATUS_3XX(ist->last_response))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_3XX_SENT, ist,ist->last_response);
	else if (MSG_IS_STATUS_4XX(ist->last_response))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_4XX_SENT, ist,ist->last_response);
	else if (MSG_IS_STATUS_5XX(ist->last_response))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_5XX_SENT, ist,ist->last_response);
	else
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_STATUS_6XX_SENT, ist,ist->last_response);
}

void OSIP_IstSM::OnRcvAckWhileCompleted( OSIP_Transaction *ist,OSIP_Event *evt )
{
	if (ist->ack != NULL)
		osip_message_free(ist->ack);

	ist->ack = evt->GetSipMsg();

	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_ACK_RECEIVED, ist, ist->ack);

	if(ist->ist_context->timer_h_start.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&(ist->ist_context->timer_h_start),TIMEOUT_H,static_cast<OPPTimerAware*>(ist));
		ist->ist_context->timer_h_start.tv_sec = -1;
	}

	if(ist->ist_context->timer_g_start.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&(ist->ist_context->timer_g_start),TIMEOUT_G,static_cast<OPPTimerAware*>(ist));
		ist->ist_context->timer_g_start.tv_sec = -1;
	}
	
	/* set the timer to 0 for reliable, and T4 for unreliable (already set) */
	if(ist->ist_context->timer_i_length > 0)
	{
		OPPTimerMonitor::sGetInstance()->AddTimer(ist->ist_context->timer_i_length,TIMEOUT_I,static_cast<OPPTimerAware*>(ist),NULL,&(ist->ist_context->timer_i_start));
		ist->SetState(OSIP_Transaction::STATE_Confirmed);
	}
	else if(ist->ist_context->timer_i_length == 0)
		ist->SetState(OSIP_Transaction::STATE_Terminated);
}

void OSIP_IstSM::OnRcvAckWhileConfirmed( OSIP_Transaction *ist,OSIP_Event *evt )
{
	if (ist->ack != NULL)
		osip_message_free(ist->ack);

	ist->ack = evt->GetSipMsg();

	OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_ACK_RECEIVED_AGAIN, ist, ist->ack);
}

void OSIP_IstSM::OnTimeoutWhileConfirmed( OSIP_Transaction *ist,OSIP_Event *evt )
{
	if(evt->GetFlag() == TIMEOUT_I)
	{
		ist->SetState(OSIP_Transaction::STATE_Terminated);
	}
}

OSIP_IstSM * OSIP_IstSM::sGetInstance()
{
	return &s_Inst;
}
/////////////////////////////// NIST StateMachine ///////////////////////////////////
OSIP_NistSM::OSIP_NistSM(void)
:OPPStateMachine<OSIP_Transaction,OSIP_Event>(OSIP_Transaction::MAX_STATE,OSIP_Event::MAX_EVENT)
{
	STATE_MACHINE_IMPL(RcvRequest,Ready,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(RcvRequest,Completed,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(RcvRequest,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send1xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send1xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send2xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send2xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send3456xx,Trying,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Send3456xx,Proceeding,OSIP_Transaction,OSIP_Event)
	STATE_MACHINE_IMPL(Timeout,Completed,OSIP_Transaction,OSIP_Event)
}

OSIP_NistSM::~OSIP_NistSM(void)
{
}

void OSIP_NistSM::OnRcvRequestWhileReady( OSIP_Transaction *nist,OSIP_Event *evt )
{
	if( OSIP_SUCCESS != nist->InitRes(evt->GetSipMsg()) )
	{
		nist->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}

	nist->SetState(OSIP_Transaction::STATE_Trying);

	if (MSG_IS_REGISTER(evt->GetSipMsg()))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_REGISTER_RECEIVED, nist,nist->orig_request);
	else if (MSG_IS_BYE(evt->GetSipMsg()))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_BYE_RECEIVED, nist,nist->orig_request);
	else if (MSG_IS_OPTIONS(evt->GetSipMsg()))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_OPTIONS_RECEIVED, nist,nist->orig_request);
	else if (MSG_IS_INFO(evt->GetSipMsg()))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_INFO_RECEIVED, nist,nist->orig_request);
	else if (MSG_IS_CANCEL(evt->GetSipMsg()))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_CANCEL_RECEIVED, nist,nist->orig_request);
	else if (MSG_IS_NOTIFY(evt->GetSipMsg()))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_NOTIFY_RECEIVED, nist,nist->orig_request);
	else if (MSG_IS_SUBSCRIBE(evt->GetSipMsg()))
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_SUBSCRIBE_RECEIVED, nist,nist->orig_request);
	else
		OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_UNKNOWN_REQUEST_RECEIVED, nist,nist->orig_request);
}

void OSIP_NistSM::OnRcvRequestWhileTrying( OSIP_Transaction *nist,OSIP_Event *evt )
{
	int i;

	evt->FreeSipMsg();

	if(nist->last_response != NULL)
	{
		i = nist->SendResponse(nist->last_response);
		if (i != 0)
		{
			//nist_handle_transport_error(nist, i);
			nist->SetState(OSIP_Transaction::STATE_Terminated);
			return;
		}
		else
		{
			if(MSG_IS_STATUS_1XX(nist->last_response))
				OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_STATUS_1XX_SENT, nist,nist->last_response);
			else if(MSG_IS_STATUS_2XX(nist->last_response))
				OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_STATUS_2XX_SENT_AGAIN,nist, nist->last_response);
			else
				OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_STATUS_3456XX_SENT_AGAIN,nist, nist->last_response);
		}
	}
}

void OSIP_NistSM::OnSend1xxWhileTrying( OSIP_Transaction *nist,OSIP_Event *evt )
{
	int i;

	if (nist->last_response != NULL)
		osip_message_free(nist->last_response);

	nist->last_response = evt->GetSipMsg();

	i = nist->SendResponse(nist->last_response);
	if(i != 0) 
	{
		//nist_handle_transport_error(nist, i);
		nist->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}

	nist->SetState(OSIP_Transaction::STATE_Proceeding);
}

void OSIP_NistSM::OnRcvRequestWhileCompleted( OSIP_Transaction *nist,OSIP_Event *evt )
{
	OnRcvRequestWhileTrying(nist,evt);
}

void OSIP_NistSM::OnSend1xxWhileProceeding( OSIP_Transaction *nist,OSIP_Event *evt )
{
	OnSend1xxWhileTrying(nist,evt);
}

void OSIP_NistSM::OnSend2xxWhileTrying( OSIP_Transaction *nist,OSIP_Event *evt )
{
	int i;

	if (nist->last_response != NULL)
		osip_message_free(nist->last_response);

	nist->last_response = evt->GetSipMsg();

	i = nist->SendResponse(nist->last_response);
	if (i != 0) 
	{
		//nist_handle_transport_error(nist, i);
		nist->SetState(OSIP_Transaction::STATE_Terminated);
		return;
	}
	else 
	{
		/*if (EVT_IS_SND_STATUS_2XX(evt))
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_STATUS_2XX_SENT, nist,nist->last_response);
		else if (MSG_IS_STATUS_3XX(nist->last_response))
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_STATUS_3XX_SENT, nist,nist->last_response);
		else if (MSG_IS_STATUS_4XX(nist->last_response))
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_STATUS_4XX_SENT, nist,nist->last_response);
		else if (MSG_IS_STATUS_5XX(nist->last_response))
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_STATUS_5XX_SENT, nist,nist->last_response);
		else
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_NIST_STATUS_6XX_SENT, nist,nist->last_response);*/
	}

	if(nist->nist_context->timer_j_length > 0)
	{
		OPPTimerMonitor::sGetInstance()->AddTimer(nist->nist_context->timer_j_length,TIMEOUT_J,static_cast<OPPTimerAware*>(nist),NULL,&(nist->nist_context->timer_j_start));
		nist->SetState(OSIP_Transaction::STATE_Completed);
	}
	else if(nist->nist_context->timer_j_length == 0)
	{
		nist->SetState(OSIP_Transaction::STATE_Terminated);
	}
	else
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"nist timer j length < 0 !!\n"));
	}
}

void OSIP_NistSM::OnSend2xxWhileProceeding( OSIP_Transaction *nist,OSIP_Event *evt )
{
	OnSend2xxWhileTrying(nist,evt);
}

void OSIP_NistSM::OnSend3456xxWhileTrying( OSIP_Transaction *nist,OSIP_Event *evt )
{
	OnSend2xxWhileTrying(nist,evt);
}

void OSIP_NistSM::OnSend3456xxWhileProceeding( OSIP_Transaction *nist,OSIP_Event *evt )
{
	OnSend2xxWhileTrying(nist,evt);
}

void OSIP_NistSM::OnTimeoutWhileCompleted( OSIP_Transaction *nist,OSIP_Event *evt )
{
	if(evt->GetFlag() == TIMEOUT_J)
	{
		if(nist->nist_context)
			nist->nist_context->timer_j_start.tv_sec = -1;
		nist->SetState(OSIP_Transaction::STATE_Terminated);
	}
	else
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Receive unexpected timer: %d\n",evt->GetFlag()));
	}
}

OSIP_NistSM * OSIP_NistSM::sGetInstance()
{
	return &s_Inst;
}

