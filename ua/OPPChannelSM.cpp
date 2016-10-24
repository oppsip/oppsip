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

#include "OPPChannelSM.h"
#include "osip_parser.h"
#include "osip_message.h"
#include "sdp_message.h"
#include "OPPChannelManager.h"
#include "OSIP_Transaction.h"
#include "OSIP_Dialog.h"
#include "OSIP_Event.h"
#include "OSIP_Core.h"
#include "OPPSipService.h"
#include "OPPTimerMonitor.h"
#include "OPPDebug.h"
#include "OPPSipChannel.h"
#include "OPPSession.h"
#include "stun_parser.h"

OPPChannelSM OPPChannelSM::smInst;

OPPChannelSM::OPPChannelSM()
:OPPStateMachine<OPPSipChannel,OPPSipEvent>(OPPSipChannel::MAX_STATE,OPPSipEvent::MAX_EVENT)
{
	STATE_MACHINE_IMPL(InCall,Ready,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(RingBack,Ring,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Accept,Ring,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Setup,Accept,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(OnHook,Accept,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(OnHook,Setup,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(OnHook,CallOut,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(OnHook,RingBack,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(PeerHangup,Setup,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Timeout,Accept,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(PeerHangup,Ring,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(OutCall,Ready,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(RingBack,CallOut,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(RingBack,RingBack,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Error,CallOut,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Error,RingBack,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Setup,RingBack,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Setup,CallOut,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Recv2xx,Setup,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(ReInvite,Setup,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(Timeout,CallOut,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(InCall,Setup,OPPSipChannel,OPPSipEvent)
	STATE_MACHINE_IMPL(InCall,Accept,OPPSipChannel,OPPSipEvent)

	STATE_MACHINE_IMPL(Dtmf,Setup,OPPSipChannel,OPPSipEvent)
}

OPPChannelSM::~OPPChannelSM()
{
}

OPPChannelSM * OPPChannelSM::sGetInstance()
{
	return &smInst;
}

void OPPChannelSM::OnInCallWhileReady( OPPSipChannel *ch,OPPSipEvent *evt )
{
	char *callee;
	char *caller;
	char *dispname;

	osip_message_t* sip = evt->GetSipMsg();

	osip_body_t *body;

	if( 0 == osip_message_get_body(sip,0,&body) )
	{
		const char *content_type = "application/sdp";
		osip_content_type_t *typet = osip_message_get_content_type(sip);
		if(typet)
		{
			content_type = typet->type;
		}
		if(body)
		{
			ch->SetPeerBody(body->body,content_type);
		}
	}

	callee = sip->req_uri->username;
	if(callee == NULL)
		callee = sip->to->url->username;

	caller = sip->from->url->username;
	
	dispname = sip->from->displayname;
	
	ch->SetState(OPPSipChannel::STATE_Ring);

	OPPChannelManager::sGetInstance()->GetEventCallback()->OnIncall(ch);
}

void OPPChannelSM::OnRingBackWhileRing( OPPSipChannel *ch,OPPSipEvent *evt )
{
	osip_message_t *response;
	osip_message_t *sip = ch->m_IST->orig_request;

	if( 0 != generating_response_default(&response,ch->m_Dialog,180,sip) )
	{
		ch->SetState(OPPSipChannel::STATE_Terminate);
		return;
	}

	complete_answer_that_establish_a_dialog(OPPSipService::sGetInstance()->m_ContactUri,response,sip);

	if(ch->m_Dialog == NULL)
		OSIP_Dialog::sCreateAsUAS(&(ch->m_Dialog),sip,response);

	if( 0 != ch->m_IST->Execute(OSIP_MsgEvent::sCreate(response,ch->m_IST->GetCtxType())) )
		return;
}

void OPPChannelSM::OnAcceptWhileRing( OPPSipChannel *ch,OPPSipEvent *evt )
{
	osip_message_t *response;

	const char *body = evt->GetSdpBody();
	int code = evt->GetResponseCode();
	const char *content_type = evt->GetContentType();

	osip_message_t *sip = ch->m_IST->orig_request;

	if( 0 != generating_response_default(&response,ch->m_Dialog,code,sip) )
	{
		ch->SetState(OPPSipChannel::STATE_Terminate);
		return;
	}

	complete_answer_that_establish_a_dialog(OPPSipService::sGetInstance()->m_ContactUri,response,sip);

	if(ch->m_Dialog == NULL)
		OSIP_Dialog::sCreateAsUAS(&(ch->m_Dialog),sip,response);
	
	if(body && content_type)
	{
		size_t len = strlen(body);
		osip_message_set_body(response,body,len);
		osip_message_set_content_type(response,content_type);
		ch->SetMyBody(body,content_type);
	}

	if( 0 != ch->m_IST->Execute(OSIP_MsgEvent::sCreate(response,ch->m_IST->GetCtxType())) )
	{
		ch->SetState(OPPSipChannel::STATE_Terminate);
		return;
	}

	if( code >= 300 || code < 200)
	{
		ch->SetState(OPPSipChannel::STATE_Terminate);
	}
	else
	{
		osip_body_t *body;
		if( 0 == osip_message_get_body(sip,0,&body) )
		{
			if(body /*&& body->content_type && body->content_type->type && body->content_type->subtype &&
					0 == strncmp(body->content_type->type,"application",11) &&
					0 == strncmp(body->content_type->subtype,"sdp",3)*/ )
			{
				sdp_message_t *sdp;
				sdp_message_init(&sdp);
				sdp_message_parse(sdp,body->body);
				const char *host = sdp_message_c_addr_get(sdp,-1,0);
				const char *str_port = sdp_message_m_port_get(sdp,0);
				ch->SetMediaPara(host,osip_atoi(str_port));
				sdp_message_free(sdp);
			}
		}
		ch->SetFinalResponse(response);

		OPPTimerMonitor::sGetInstance()->AddTimer(500,OPPSipChannel::TIMEOUT_2xxRETRY,static_cast<OPPTimerAware*>(ch),NULL,&ch->m_2xxRetryTimeout);

		ch->SetState(OPPSipChannel::STATE_Accept);
	}
}

void OPPChannelSM::OnSetupWhileAccept( OPPSipChannel *ch,OPPSipEvent *evt )
{
	OPPTimerMonitor::sGetInstance()->CancelTimer(&ch->m_2xxRetryTimeout,OPPSipChannel::TIMEOUT_2xxRETRY,static_cast<OPPTimerAware*>(ch));

	ch->SetState(OPPSipChannel::STATE_Setup);

	if(ch->GetReinviteFlag() == 0)
		OPPChannelManager::sGetInstance()->GetEventCallback()->OnInCallSetup(ch);
}

void OPPChannelSM::OnOnHookWhileAccept( OPPSipChannel *ch,OPPSipEvent *evt )
{
	NetDbg(DBG_INFO,"OnOnHookWhileAccept:%p\n",ch->m_pSipDev);

	OPPTimerMonitor::sGetInstance()->CancelTimer(&ch->m_2xxRetryTimeout,OPPSipChannel::TIMEOUT_2xxRETRY,static_cast<OPPTimerAware*>(ch));

	OnOnHookWhileSetup(ch,evt);
}

void OPPChannelSM::OnTimeoutWhileAccept( OPPSipChannel *ch,OPPSipEvent *evt )
{
	if(evt->GetFlag() == OPPSipChannel::TIMEOUT_2xxRETRY)
	{
		if(ch->m_2xxRetryTimes++ < 7)
		{
			OPPTimerMonitor::sGetInstance()->AddTimer(500*ch->m_2xxRetryTimes,OPPSipChannel::TIMEOUT_2xxRETRY,static_cast<OPPTimerAware*>(ch),NULL,&ch->m_2xxRetryTimeout);
			if(ch->m_FinalResponse)
			{
				char *addr;	int  port;
				osip_response_get_destination(ch->m_FinalResponse,&addr,&port);
				if(addr)
					OSIP_Core::sGetInstance()->SendMsg(ch->m_FinalResponse,addr,port,OSIP_UDP,-1);
			}
		}
		else
		{
			OnOnHookWhileSetup(ch,evt);

			OPPChannelManager::sGetInstance()->GetEventCallback()->OnEndCall(ch);
		}
	}
}

void OPPChannelSM::OnDtmfWhileSetup( OPPSipChannel *ch, OPPSipEvent *evt )
{
	OPPChannelManager::sGetInstance()->GetEventCallback()->OnDTMF(ch,evt->GetDtmf());
}

void OPPChannelSM::OnOnHookWhileSetup( OPPSipChannel *ch,OPPSipEvent *evt )
{
	if(ch->m_Dialog)
	{
		osip_message_t *bye;
		char *local_host = OPPSipService::sGetInstance()->m_LocalHost;
		int local_port = OPPSipService::sGetInstance()->m_ListenPort;
		if( 0 == ch->m_Dialog->generating_request_within_dialog(&bye,"BYE",local_host,local_port,"UDP") )
		{
			OSIP_Transaction *tr = new OSIP_Transaction(NICT);
			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(bye,NICT)) )
			{

			}
		}
	}

	ch->SetState(OPPSipChannel::STATE_Terminate);
}

void OPPChannelSM::OnPeerHangupWhileSetup( OPPSipChannel *ch,OPPSipEvent *evt )
{
	ch->SetState(OPPSipChannel::STATE_Terminate);

	OPPChannelManager::sGetInstance()->GetEventCallback()->OnEndCall(ch);
}

void OPPChannelSM::OnPeerHangupWhileRing( OPPSipChannel *ch,OPPSipEvent *evt )
{
	if(ch->m_IST->orig_request)
	{
		osip_message_t *response;
		if( 0 == generating_response_default(&response,ch->m_Dialog,487,ch->m_IST->orig_request) )
		{
			if( 0 != ch->m_IST->Execute(OSIP_MsgEvent::sCreate(response,IST)) )
			{
				ch->SetState(OPPSipChannel::STATE_Terminate);
				return;
			}
		}
	}
	ch->SetState(OPPSipChannel::STATE_Terminate);

	OPPChannelManager::sGetInstance()->GetEventCallback()->OnEndCall(ch);
}

void OPPChannelSM::OnOutCallWhileReady( OPPSipChannel *ch,OPPSipEvent *evt )
{
	osip_message_t *invite;

	char to[256];
	char from[256];
	sprintf(to,"sip:%s@%s",evt->GetCallee(),ch->m_pSipDev->GetDomain());
	sprintf(from,"sip:%s@%s",evt->GetCaller(),ch->m_pSipDev->GetDomain());
	osip_uri_t *uri = evt->GetDestUri();
	const char *body = evt->GetSdpBody();
	const char *content_type = evt->GetContentType();

	char *local_host = OPPSipService::sGetInstance()->GetLocalHost();
	int   local_port = OPPSipService::sGetInstance()->GetListenPort();
	char *global_contact = OPPSipService::sGetInstance()->GetContact();

	if( 0 == generating_request_out_of_dialog(&invite,"INVITE",uri,from,to,local_host,local_port,global_contact,"UDP",(char*)ch->m_pSipDev->GetOutboundIp(),ch->m_pSipDev->GetOutboundPort()) )
	{	
		if( body && content_type)
		{
			ch->SetMyBody(body,content_type);
			osip_message_set_body(invite,body,strlen(body));
			osip_message_set_content_type(invite,content_type);
		}

		OSIP_Transaction *tr = new OSIP_Transaction(ICT);
		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(invite,ICT)) )
		{
			ch->SetState(OPPSipChannel::STATE_Terminate);
			return;
		}
		else
		{
			ch->SetICT(tr);
			tr->SetUserData(ch);
			OPPTimerMonitor::sGetInstance()->AddTimer(60000,OPPSipChannel::TIMEOUT_CALLOUT,static_cast<OPPTimerAware*>(ch),NULL,&ch->m_tv_call_out);
		}

		ch->SetState(OPPSipChannel::STATE_CallOut);
	}
	else
	{
		ch->SetState(OPPSipChannel::STATE_Terminate);
		return;
	}
	
}

void OPPChannelSM::OnRingBackWhileRingBack(OPPSipChannel *ch,OPPSipEvent *evt)
{
	OnRingBackWhileCallOut(ch,evt);
}

void OPPChannelSM::OnRingBackWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt)
{
	osip_message_t *response = evt->GetSipMsg();

	if(ch->m_Dialog == NULL)
	{
		OSIP_Dialog::sCreateAsUAC(&ch->m_Dialog,ch->m_ICT->orig_request,response);
	}
	else
	{
		ch->m_Dialog->osip_dialog_update_tag_as_uac(response);
		ch->m_Dialog->osip_dialog_update_route_set_as_uac(response);
	}

	OSIP_TRACE(osip_trace
		(__FILE__, __LINE__, TRACE_LEVEL1, NULL,
		"OnRingBackWhileCallOut!\n"));

	osip_body_t *body;

	if( 0 == osip_message_get_body(response,0,&body) )
	{
		if(body /*&& body->content_type && body->content_type->type && body->content_type->subtype &&
				0 == strncmp(body->content_type->type,"application",11) &&
				0 == strncmp(body->content_type->subtype,"sdp",3)*/ )
		{
			sdp_message_t *sdp;
			ch->SetPeerBody(body->body,"application/sdp");
			sdp_message_init(&sdp);
			sdp_message_parse(sdp,body->body);
			const char *host = sdp_message_c_addr_get(sdp,-1,0);
			const char *str_port = sdp_message_m_port_get(sdp,0);
			ch->SetMediaPara(host,osip_atoi(str_port));
			sdp_message_free(sdp);
		}
	}

	ch->SetState(OPPSipChannel::STATE_RingBack);

	OPPChannelManager::sGetInstance()->GetEventCallback()->OnRingBack(ch);
}

void OPPChannelSM::OnOnHookWhileRingBack(OPPSipChannel *ch,OPPSipEvent *evt)
{
	OnOnHookWhileCallOut(ch,evt);
}

void OPPChannelSM::OnOnHookWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt)
{
	osip_message_t *cancel;

	if(ch->m_ICT && ch->m_ICT->orig_request)
	{
		if( 0 != generating_cancel(&cancel,ch->m_ICT->orig_request) )
		{
			ch->SetState(OPPSipChannel::STATE_Terminate);
			return;
		}
		
		OSIP_Transaction *tr = new OSIP_Transaction(NICT);
		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(cancel,NICT)) )
		{
			ch->SetState(OPPSipChannel::STATE_Terminate);
			return;
		}
	}
	
	ch->SetState(OPPSipChannel::STATE_Terminate);
}

void OPPChannelSM::OnSetupWhileRingBack(OPPSipChannel *ch,OPPSipEvent *evt)
{
	OnSetupWhileCallOut(ch,evt);
}

void OPPChannelSM::OnSetupWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt)
{
	osip_message_t *response = evt->GetSipMsg();
	char *local_host = OPPSipService::sGetInstance()->m_LocalHost;
	int local_port = OPPSipService::sGetInstance()->m_ListenPort;

	OPPTimerMonitor::sGetInstance()->CancelTimer(&ch->m_tv_call_out,OPPSipChannel::TIMEOUT_CALLOUT,static_cast<OPPTimerAware*>(ch));

	if(ch->m_Dialog == NULL)
	{
		OSIP_Dialog::sCreateAsUAC(&ch->m_Dialog,ch->m_ICT->orig_request,response);
	}
	else
	{
		ch->m_Dialog->osip_dialog_update_tag_as_uac(response);
		ch->m_Dialog->osip_dialog_update_route_set_as_uac(response);
	}

	if(ch->m_Dialog)
	{
		osip_message_t *ack;
		int      port = 5060;
		osip_route_t *route = NULL;
		char    *host = 0;

		if( 0 == ch->m_Dialog->generating_request_within_dialog(&ack,"ACK",local_host,local_port,"UDP") )
		{
			osip_message_get_route(ack, 0, &route);
			if(route!=NULL)
			{
				if(route->url->port!=NULL)
					port = osip_atoi(route->url->port);
				if(route->url->host != NULL)
					host = route->url->host;
			}
			else
			{
				if(ack->req_uri->port!=NULL)
					port = osip_atoi(ack->req_uri->port);
				host = ack->req_uri->host;
			}

			OSIP_Core::sGetInstance()->SendMsg(ack,host,port,OSIP_UDP,-1);

			ch->SetAck(ack);
		}
		else
		{
			NetDbg(DBG_ERROR,"generating ACK error!\n");
		}
	}

	osip_body_t *body;

	osip_content_type_t *typet = osip_message_get_content_type(response);
	if( 0 == osip_message_get_body(response,0,&body) && typet && typet->type )
	{
		ch->SetPeerBody(body->body,typet->type);
	}

	ch->SetState(OPPSipChannel::STATE_Setup);

	if(ch->GetReinviteFlag() == 0)
		OPPChannelManager::sGetInstance()->GetEventCallback()->OnOutCallSetup(ch);

}

void OPPChannelSM::OnErrorWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt)
{
	osip_message_t *response = evt->GetSipMsg();

	OSIP_TRACE(osip_trace
		(__FILE__, __LINE__, TRACE_LEVEL1, NULL,
		"OnErrorWhileCallOut!\n"));

	if(response->status_code == 401 || response->status_code == 407)
	{
		if(ch->m_pSipDev->GetDevType() == OPPSipDev::DEV_TYPE_REGISTER)
		{
			char result[600];
			osip_authorization_t *auth;

			if(ch->m_ICT && ch->m_ICT->orig_request && 0 > osip_message_get_authorization(ch->m_ICT->orig_request,0,&auth) 
				&& 0 == ((OPPSipRegisterDev*)ch->m_pSipDev)->Calc_Proxy_Auth(ch->m_ICT->orig_request->req_uri,response,result,response->status_code)) 
			{
				osip_message_t *invite;
				osip_via_t *top_via;
				if( 0 == osip_message_clone(ch->m_ICT->orig_request,&invite) )
				{
					if( 0 == osip_message_get_via(invite,0,&top_via) )
					{
						osip_generic_param_t *branch = NULL;
						osip_via_param_get_byname(top_via, "branch", &branch);
						if(branch)
						{
							osip_free(branch->gvalue);
							branch->gvalue = (char*)osip_malloc(128);
							sprintf(branch->gvalue,"z9hG4bK%u",osip_build_random_number());

							osip_message_set_proxy_authorization(invite,result);

							OSIP_Transaction *tr = new OSIP_Transaction(ICT);
							if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(invite,ICT)) )
							{
								ch->SetState(OPPSipChannel::STATE_Terminate);
								return;
							}
							else
							{
								ch->SetICT(tr);
								tr->SetUserData(ch);
								//OPPTimerMonitor::sGetInstance()->AddTimer(60000,OPPSipChannel::TIMEOUT_CALLOUT,static_cast<OPPTimerAware*>(ch),NULL,&ch->m_tv_call_out);
							}

							ch->SetState(OPPSipChannel::STATE_CallOut);
							return;
						}
					}
				}
			}
		}
	}
	
	ch->SetState(OPPSipChannel::STATE_Terminate);

	OPPChannelManager::sGetInstance()->GetEventCallback()->OnError(ch,response->status_code);
	
}

void OPPChannelSM::OnErrorWhileRingBack(OPPSipChannel *ch,OPPSipEvent *evt)
{
	OnErrorWhileCallOut(ch,evt);
}

void OPPChannelSM::OnRecv2xxWhileSetup(OPPSipChannel *ch,OPPSipEvent *evt)
{
	char *host = NULL;
	int   port = 5060;
	osip_route_t *route;

	if(ch->m_Ack == NULL)
	{
		NetDbg(DBG_ERROR,"Error!!! m_Ack if null %s %d\n",__FILE__,__LINE__);
		return;
	}

	osip_message_get_route(ch->m_Ack, 0, &route);
	if(route!=NULL)
	{
		if(route->url->port!=NULL)
			port = osip_atoi(route->url->port);
		if(route->url->host != NULL)
			host = route->url->host;
	}
	else
	{
		if(ch->m_Ack->req_uri->port!=NULL)
			port = osip_atoi(ch->m_Ack->req_uri->port);
		host = ch->m_Ack->req_uri->host;
	}

	if(host)
		OSIP_Core::sGetInstance()->SendMsg(ch->m_Ack,host,port,OSIP_UDP,-1);
	else
		NetDbg(DBG_ERROR,"Error!!! host if null %s %d\n",__FILE__,__LINE__);
}

void OPPChannelSM::OnReInviteWhileSetup(OPPSipChannel *ch,OPPSipEvent *evt)
{
	osip_message_t *request;

	if(ch->m_Dialog && 0 == ch->m_Dialog->generating_request_within_dialog(&request,"INVITE",OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort(),"UDP"))
	{
		const char *sdp_body = evt->GetSdpBody();
		const char *content_type = evt->GetContentType();

		if( sdp_body && content_type)
		{
			osip_message_set_body(request,sdp_body,strlen(sdp_body));
			osip_message_set_content_type(request,content_type);//"application/sdp");
		}

		OSIP_Transaction *tr = new OSIP_Transaction(ICT);

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(request,ICT)) )
		{
			ch->SetState(OPPSipChannel::STATE_Terminate);
			return;
		}
		else
		{
			ch->SetICT(tr);
			tr->SetUserData(ch);
			ch->SetState(OPPSipChannel::STATE_CallOut);
			OPPTimerMonitor::sGetInstance()->AddTimer(8000,OPPSipChannel::TIMEOUT_CALLOUT,static_cast<OPPTimerAware*>(ch),NULL,&ch->m_tv_call_out);
			ch->SetReinviteFlag(1);
		}
	}
}

void OPPChannelSM::OnTimeoutWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt)
{
	if(evt->GetFlag() == OPPSipChannel::TIMEOUT_CALLOUT)
	{
		ch->SetState(OPPSipChannel::STATE_Terminate);

		OPPChannelManager::sGetInstance()->GetEventCallback()->OnTimeout(ch,evt->GetFlag());
	}
}

void OPPChannelSM::OnInCallWhileAccept( OPPSipChannel *ch,OPPSipEvent *evt )
{
	OPPTimerMonitor::sGetInstance()->CancelTimer(&ch->m_2xxRetryTimeout,OPPSipChannel::TIMEOUT_2xxRETRY,static_cast<OPPTimerAware*>(ch));

	OPPChannelManager::sGetInstance()->GetEventCallback()->OnInCallSetup(ch);

	OnInCallWhileSetup(ch,evt);
}

void OPPChannelSM::OnInCallWhileSetup( OPPSipChannel *ch,OPPSipEvent *evt )
{
	char *callee;

	osip_message_t* sip = evt->GetSipMsg();

	osip_body_t *body;

	if( 0 == osip_message_get_body(sip,0,&body) )
	{
		ch->SetReinviteFlag(1);
		
		if(body /*&& body->content_type && body->content_type->type && body->content_type->subtype &&
				0 == strncmp(body->content_type->type,"application",11) &&
				0 == strncmp(body->content_type->subtype,"sdp",3)*/ )
		{
			const char *prev_body = NULL;
			const char *content_type = NULL;
			ch->GetPeerBody(&prev_body,&content_type);
			if(prev_body && 0 != strcmp(prev_body,body->body))
			{
				ch->SetPeerBody(body->body,"application/sdp");
				
				ch->SetState(OPPSipChannel::STATE_Ring);

				OPPChannelManager::sGetInstance()->GetEventCallback()->OnReIncall(ch);
			}
			else
			{
				callee = sip->req_uri->username;
				if(callee == NULL)
					callee = sip->to->url->username;
				if(callee == NULL)
					callee = "NA";
				const char *my_body = NULL;
				const char *my_content_type = NULL;
				ch->GetMyBody(&my_body,&my_content_type);
				if(my_body && my_content_type)
				{
					OPPSipAcceptEvent acc_evt(200,callee,my_body,my_content_type);	
					OnAcceptWhileRing(ch,&acc_evt);
				}
				else
				{
					OPPSipAcceptEvent acc_evt(500,callee,NULL,NULL);	
					OnAcceptWhileRing(ch,&acc_evt);
				}
			}
		}
	}
}

