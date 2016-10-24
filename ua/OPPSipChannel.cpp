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

#include "OPPSipChannel.h"
#include "OSIP_Transaction.h"
#include "OSIP_Dialog.h"
#include "OPPChannelSM.h"
#include "OPPEvents.h"
#include "OPPSipService.h"
#include "OSIP_Core.h"
#include "OSIP_Event.h"

#include "sdp_message.h"
#include "osip_parser.h"
#include "OPPChannelManager.h"
#include "osip_md5.h"
#include "OPPDebug.h"
#include "stun_parser.h"

OPPSipChannel::OPPSipChannel(OPPSipDev *pDev,int nChNo)
:m_ICT(NULL),m_IST(NULL),m_NICT(NULL),m_NIST(NULL),m_FromUri(NULL),m_FinalResponse(NULL),m_2xxRetryTimes(0),m_bFree(1)
,m_pSipDev(pDev),m_nChNo(nChNo),m_Dialog(NULL),m_Ack(NULL),m_InCallParam(NULL)
,m_AcceptParam(NULL),m_MediaHost(NULL),m_AllocHost(NULL),m_PeerBody(NULL),m_PeerContentType(NULL),m_MyBody(NULL),m_MyContentType(NULL)
,m_reInviteFlag(0)
{
	m_2xxRetryTimeout.tv_sec = -1;
	m_tv_call_out.tv_sec = -1;
	
	m_SM = OPPChannelSM::sGetInstance();
}

OPPSipChannel::~OPPSipChannel()
{
}

void OPPSipChannel::SetICT( OSIP_Transaction *tr )
{
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"%p SetICT=%p\n",this,tr));

	if(m_ICT)
		m_ICT->DeleteTrans();

	if(tr)
		tr->AddRef();

	m_ICT = tr;
}

void OPPSipChannel::SetIST( OSIP_Transaction *tr )
{
	if(m_IST)
		m_IST->DeleteTrans();

	if(tr)
		tr->AddRef();

	m_IST = tr;
}

void OPPSipChannel::SetNICT( OSIP_Transaction *tr )
{
	if(m_NICT)
		m_NICT->DeleteTrans();

	if(tr)
		tr->AddRef();

	m_NICT = tr;
}

void OPPSipChannel::SetNIST( OSIP_Transaction *tr )
{
	if(m_NIST)
		m_NIST->DeleteTrans();

	if(tr)
		tr->AddRef();

	m_NIST = tr;
}

void OPPSipChannel::SetFromUri( osip_from_t *From )
{
	if(m_FromUri)
		osip_free(m_FromUri);

	osip_from_to_str(From,&m_FromUri);
}

void OPPSipChannel::SetFinalResponse( osip_message_t *response )
{
	if(m_FinalResponse)
		osip_message_free(m_FinalResponse);

	if(response)
		m_FinalResponse = osip_message_addref(response);
	else
		m_FinalResponse = NULL;
}

void OPPSipChannel::SetAck( osip_message_t *ack )
{
	if(m_Ack)
		osip_message_free(m_Ack);

	m_Ack = ack;
}

void OPPSipChannel::SetMediaPara(const char *Host,int port)
{
	if(m_MediaHost)
		osip_free((void*)m_MediaHost);

	m_MediaHost = osip_strdup(Host);

	m_MediaPort = port;
}

int OPPSipChannel::GetMediaPara(const char **Host,uint16 *port)
{
	if(m_MediaHost)
	{
		*Host = m_MediaHost;
		*port = m_MediaPort;
		return 0;
	}
	else
		return -1;
}

void OPPSipChannel::SetPeerBody(const char *body,const char *content_type)
{
	if(m_PeerBody)
		osip_free(m_PeerBody);

	if(m_PeerContentType)
		osip_free(m_PeerContentType);

	m_PeerBody = osip_strdup(body);
	m_PeerContentType = osip_strdup(content_type);
}

int	OPPSipChannel::GetPeerBody(const char **body,const char **content_type)
{
	if(m_PeerBody && m_PeerContentType)
	{
		*body = m_PeerBody;
		*content_type = m_PeerContentType;
		return 0;
	}
	else
		return -1;
}

void OPPSipChannel::Execute( OPPSipEvent *evt )
{
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"$$$$ SipChannel:%p state=%d evt=%d\n",this,GetState(),evt->GetType()));
	
	m_SM->ExecuteSM(this,evt);
	if(GetState() == STATE_Terminate)
	{
		printf("FreeRes:%p\n",this);
		FreeRes();
	}
}

void OPPSipChannel::FreeRes()
{
	m_bFree = 1;

	if(m_FromUri)
		osip_free(m_FromUri);
	m_FromUri = NULL;

	if(m_FinalResponse)
		osip_message_free(m_FinalResponse);
	m_FinalResponse = NULL;

	if(m_Dialog)
		delete m_Dialog;
	m_Dialog = NULL;

	SetICT(NULL);
	SetNICT(NULL);
	SetIST(NULL);
	SetNIST(NULL);
	SetAck(NULL);

	SetInCallParam(NULL);
	SetAcceptParam(NULL);
	SetMediaPara(NULL,0);
	SetAllocHostPort(NULL,0);
	SetPeerBody(NULL,NULL);
	SetMyBody(NULL,NULL);

	m_2xxRetryTimes = 0;

	if(m_tv_call_out.tv_sec >= 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&m_tv_call_out,OPPSipChannel::TIMEOUT_CALLOUT,static_cast<OPPTimerAware*>(this));
	}

	SetReinviteFlag(0);
	
	SetState(STATE_Ready);

	//Other resource need being set to the initial state?
}

void OPPSipChannel::DoInfo( int dtmf )
{
	if(GetState() == OPPSipChannel::STATE_Setup)
	{
		osip_message_t *request;
		if(m_Dialog && 0 == m_Dialog->generating_request_within_dialog(&request,"INFO",OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort(),"UDP"))
		{
			char *sdp_body = (char*)osip_malloc(100);
			if(dtmf == 10)
				strcpy(sdp_body,"Signal=*\r\nDuration=160\r\n");
			else if(dtmf == 11)
				strcpy(sdp_body,"Signal=#\r\nDuration=160\r\n");
			else
				sprintf(sdp_body,"Signal=%d\r\nDuration=160\r\n",dtmf);
			
			const char *content_type = "application/dtmf";
			if( sdp_body && content_type )
			{
				osip_message_set_body(request,sdp_body,strlen(sdp_body));
				osip_free(sdp_body);
				osip_message_set_content_type(request,content_type);
			}

			OSIP_Transaction *tr = new OSIP_Transaction(NICT);
			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(request,NICT)) )
				return;
		}
	}
}

void OPPSipChannel::DoInCall( InCallParam_t *para )
{
	if(GetState() == OPPSipChannel::STATE_Ready)
	{
		m_bFree = 0;
		
		OPPSipOutCallEvent evt(para->GetCaller(),para->GetCallee(),para->GetBody(),para->GetContentType(),para->GetDestUri());

		Execute(&evt);		
	}
	else if(GetState() == OPPSipChannel::STATE_Setup)
	{
		OPPSipReInviteEvent evt(para->GetBody(),para->GetContentType());

		Execute(&evt);	
	}
	else
	{
		NetDbg(DBG_ERROR,"DoInCall:state=%d, dev=%p\n",GetState(),m_pSipDev);
	}
}

void OPPSipChannel::DoAcceptCall(AcceptParam_t *para)
{
	OPPSipAcceptEvent evt(para->GetResponseCode(),para->GetNumber(),para->GetBody(),para->GetContentType());

	Execute(&evt);
}

void OPPSipChannel::DoEndCall()
{
	OPPSipOnHookEvent evt;

	Execute(&evt);
}

void OPPSipChannel::DoRingBack()
{
	OPPSipRingBackEvent evt(NULL,NULL);

	Execute(&evt);
}

void OPPSipChannel::OnTimeout( int flag ,void *para)
{
	OPPSipTimeoutEvent evt(flag);

	Execute(&evt);
}

void OPPSipChannel::OnDTMF(int dtmf)
{
	OPPSipDTMFEvent evt(dtmf);
	Execute(&evt);
}

void OPPSipChannel::DoNotify( const char *msg )
{
	if(m_Dialog)
	{
		osip_message_t *notify;

		if( 0 == m_Dialog->generating_request_within_dialog(&notify,"NOTIFY",OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort(),"UDP") )
		{
			osip_message_set_content_type(notify,"message/sipfrag");
			osip_message_set_body(notify,msg,strlen(msg));

			OSIP_Transaction *tr = new OSIP_Transaction(NICT);

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(notify,tr->GetCtxType())) )
				return;
		}
	}
}

void OPPSipChannel::DoBlindTransfer( const char *TransTo )
{
	if(GetState() == STATE_Setup && m_Dialog)
	{
		osip_message_t *sip;
		char *host = OPPSipService::sGetInstance()->GetLocalHost();
		int port = OPPSipService::sGetInstance()->GetListenPort();
		if( 0 == m_Dialog->generating_request_within_dialog(&sip,"REFER",host,port,"UDP") )
		{
			char buf[256];
			sprintf(buf,"sip:%s@%s:%d",TransTo,host,port);
			osip_message_set_header(sip,"refer-to",buf);

			OSIP_Transaction *tr = new OSIP_Transaction(NICT);
			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(sip,NICT)) )
			{
			}
		}
	}
}

int OPPSipRegisterDev::Calc_Proxy_Auth(osip_uri_t *req_uri, osip_message_t *sip,char *result,int code )
{
	osip_proxy_authenticate_t *auth;
	char qop[24];
	char A2[255];
	char A3[255];
	char strNonece[255],strDomain[100],strRealm[100];
	char A1[100],tmp[200],B1[255];//,req_url[50];
	const char *strMethod;
	char  *cnonece = NULL;
	int  qop_present = 0;

	if(code == 407)
	{
		if(osip_list_eol(&sip->proxy_authenticates,0))
			return -1;

		auth = (osip_proxy_authenticate_t*)osip_list_get(&sip->proxy_authenticates,0);
	}
	else
	{
		if(osip_list_eol(&sip->www_authenticates,0))
			return -1;

		auth = (osip_proxy_authenticate_t*)osip_list_get(&sip->www_authenticates,0);
	}

	strcpy(strNonece,auth->nonce);
	trim(strNonece,'"');

	if(auth->qop_options != NULL)
	{
		strcpy(qop,auth->qop_options);
		trim(qop,'"');

		if(strncmp(qop,"auth-int",8) == 0 || strncmp(qop,"auth",4) == 0)
		{
			char* c = strchr(qop,',');
			if(c!=NULL) *c = 0;
			cnonece = new_random_string();
			sprintf(B1,"%s:%s:%s:%s",strNonece,"00000002",cnonece,qop);
			qop_present = 1;
		}
	}
	else
	{
		strcpy(B1,strNonece);
	}

/*	if(m_nPort != 5060)
		sprintf(strDomain,"sip:%s:%d",m_Domain,m_nPort);
	else
		sprintf(strDomain,"sip:%s",m_Domain);
*/
	strDomain[0] = 0;
	if(req_uri->username && req_uri->host && req_uri->port)
	{
		sprintf(strDomain,"sip:%s@%s:%s",req_uri->username,req_uri->host,req_uri->port);
	}
	else if(req_uri->username && req_uri->host)
	{
		sprintf(strDomain,"sip:%s@%s",req_uri->username,req_uri->host);
	}
	else if(req_uri->host && req_uri->port)
	{
		sprintf(strDomain,"sip:%s:%s",req_uri->host,req_uri->port);
	}
	else if(req_uri->host)
	{
		sprintf(strDomain,"sip:%s",req_uri->host);
	}
	else
	{
		return -1;
	}

	if(auth->realm == NULL)
	{
		strcpy(strRealm,"sip");
	}
	else
	{
		strcpy(strRealm,auth->realm);
		trim(strRealm,'"');
	}
	
	strMethod = sip->cseq->method;

	sprintf(tmp,"%s:%s:%s",m_AuthID,strRealm,m_Password);
	MD5_String(tmp,A1);

	sprintf(A2,"%s:%s",strMethod,strDomain);

	sprintf(A3,"%s:%s",B1,MD5_String(A2,tmp));
	sprintf(A2,"%s:%s",A1,A3);

	MD5_String(A2,A3);

	if(qop_present && cnonece)
	{
		if(auth->opaque != NULL)
		{
			sprintf(result,"Digest username=\"%s\",realm=\"%s\",nonce=\"%s\",response=\"%s\",uri=\"%s\",algorithm=MD5,opaque=%s,qop=%s,cnonce=\"%s\",nc=00000002",
				m_AuthID,strRealm,strNonece,A3,strDomain,auth->opaque,qop,cnonece );
		}
		else
		{
			sprintf(result,"Digest username=\"%s\",realm=\"%s\",nonce=\"%s\",response=\"%s\",uri=\"%s\",algorithm=MD5,qop=%s,cnonce=\"%s\",nc=00000002",
				m_AuthID,strRealm,strNonece,A3,strDomain,qop,cnonece );
		}
		osip_free(cnonece);
	}
	else
	{
		sprintf(result,"Digest username=\"%s\",realm=\"%s\",nonce=\"%s\",response=\"%s\",uri=\"%s\",algorithm=MD5",
			m_AuthID,strRealm,strNonece,A3,strDomain );
	}

	return 0;
}

int OPPSipRegisterDev::Register_With_Authentication( osip_message_t *sip )
{
	char				result[600];
	osip_message_t		*reg;
	char				from[384];
	char                contact[256];

	if(m_nPort != 5060)
		sprintf(from,"sip:%s@%s:%d",m_UserName,m_Domain,m_nPort);
	else
		sprintf(from,"sip:%s@%s",m_UserName,m_Domain);

	if(OPPSipService::sGetInstance()->GetListenPort() != 5060)
		sprintf(contact,"sip:%s@%s:%d",m_UserName,OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort());
	else
		sprintf(contact,"sip:%s@%s",m_UserName,OPPSipService::sGetInstance()->GetLocalHost());

	if( 0 != generating_request_out_of_dialog(&reg,"REGISTER",NULL,from,from,OPPSipService::sGetInstance()->GetLocalHost(),
		OPPSipService::sGetInstance()->GetListenPort(),contact,"UDP",(char*)GetOutboundIp(),GetOutboundPort()) )
	{
		NetDbg(DBG_ERROR,"generating register error!");
		return -1;
	}

	if(-1 == Calc_Proxy_Auth(reg->req_uri,sip,result,401)) 
	{
		osip_message_free(reg);
		return -1;
	}

	osip_message_set_authorization(reg,result);

	osip_message_set_expires(reg,"90");

	osip_free(reg->call_id->number);
	osip_call_id_set_number(reg->call_id,osip_strdup(m_CallID));

	char num[20];
	sprintf(num,"%d",m_nRegisterSeq++);

	osip_free(reg->cseq->number);
	osip_cseq_set_number(reg->cseq,osip_strdup(num));

	OSIP_Transaction *tr = new OSIP_Transaction(NICT);

	tr->SetUserData(this);

	if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(reg,NICT)) )
	{
		NetDbg(DBG_ERROR,"send register error!");
		return -1;
	}

	return 0;
}

int OPPSipRegisterDev::Register()
{
	osip_message_t *reg;
	char from[256];
	char contact[128];

	if(m_nPort != 5060)
		sprintf(from,"sip:%s@%s:%d",m_UserName,m_Domain,m_nPort);
	else
		sprintf(from,"sip:%s@%s",m_UserName,m_Domain);

	if(OPPSipService::sGetInstance()->GetListenPort() != 5060)
		sprintf(contact,"sip:%s@%s:%d",m_UserName,OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort());
	else
		sprintf(contact,"sip:%s@%s",m_UserName,OPPSipService::sGetInstance()->GetLocalHost());

	if( 0 != generating_request_out_of_dialog(&reg,"REGISTER",NULL,from,from,OPPSipService::sGetInstance()->GetLocalHost(),
		OPPSipService::sGetInstance()->GetListenPort(),contact,"UDP",(char*)GetOutboundIp(),GetOutboundPort()) )
	{
		NetDbg(DBG_ERROR,"generating register error!");
		return -1;
	}

	osip_message_set_expires(reg,"90");

	osip_free(reg->call_id->number);
	osip_call_id_set_number(reg->call_id,osip_strdup(m_CallID));

	char num[20];
	sprintf(num,"%d",m_nRegisterSeq++);

	osip_free(reg->cseq->number);
	osip_cseq_set_number(reg->cseq,osip_strdup(num));

	OSIP_Transaction *tr = new OSIP_Transaction(NICT);

	tr->SetUserData(this);

	if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(reg,NICT)) )
	{
		NetDbg(DBG_ERROR,"send register error!");
		return -1;
	}

	OPPTimerMonitor::sGetInstance()->AddTimer(300000,OPPSipChannel::TIMEOUT_REGISTER_NO_RESPONSE,static_cast<OPPTimerAware*>(this),NULL,&m_reg_on_fail);

	return 0;
}

void OPPSipRegisterDev::OnRegisterResult( OSIP_Transaction *tr,osip_message_t *sip )
{
	int nExp = 60000;

	if( m_reg_on_timeout.tv_sec < 0 && (sip->status_code == 401 || sip->status_code == 407) )
	{
		Register_With_Authentication(sip);
	}
	else if(sip->status_code >= 200 && sip->status_code < 300)
	{
		osip_contact_t *hd_contact;
		const char *exp = NULL;
		
		if( 0 == osip_message_get_contact(sip,0,&hd_contact) )
		{
			osip_generic_param_t *gen_para;
			if( 0 == osip_generic_param_get_byname(&hd_contact->gen_params,"expires",&gen_para) )
			{
				exp = gen_para->gvalue;				
			}
		}
		if(exp == NULL)
		{
			osip_header_t *hd_exp;
			if( 0 <= osip_message_get_expires(sip,0,&hd_exp) )
			{
				exp = hd_exp->hvalue;
			}
		}
		if(exp)
			nExp = osip_atoi(exp)*1000/2 + 1;
	}

	if(m_reg_on_fail.tv_sec > 0)
	{
		OPPTimerMonitor::sGetInstance()->CancelTimer(&m_reg_on_fail,OPPSipChannel::TIMEOUT_REGISTER_NO_RESPONSE,static_cast<OPPTimerAware*>(this));
		m_reg_on_fail.tv_sec = -1;
	}
	
	if(m_reg_on_timeout.tv_sec < 0)
	{
		OPPTimerMonitor::sGetInstance()->AddTimer(nExp,OPPSipChannel::TIMEOUT_REGISTER,static_cast<OPPTimerAware*>(this),NULL,&m_reg_on_timeout);
	}
}

extern unsigned long net_gethostbyname(const char *name,char *host,int i);

OPPSipRegisterDev::OPPSipRegisterDev( int nMaxChannels,const char *UserName,const char *Password,const char *Domain,int nPort,const char *OutboundIP,int OutboundPort ) 
:OPPSipDev(nMaxChannels,DEV_TYPE_REGISTER,OutboundIP,OutboundPort)
{
	m_UserName = osip_strdup(UserName);
	m_AuthID = osip_strdup(UserName);
	m_Password = osip_strdup(Password);

	m_Host = NULL;

	if(OutboundIP)
		m_Host = osip_strdup(OutboundIP);
	else if(Domain)
	{
		char host[36];
		if(net_gethostbyname(Domain,host,0))
			m_Host = osip_strdup(host);
	}
	
	/*if(nPort != 5060)
	{
		char Dom[256];
		sprintf(Dom,"%s:%d",Domain,nPort);
		m_Domain = osip_strdup(Dom);
	}
	else*/
	{
		m_Domain = osip_strdup(Domain);
	}

	m_CallID = new_random_string();
	m_nRegisterSeq = 1;
	m_nPort = nPort;
	m_reg_on_timeout.tv_sec = -1;
	m_reg_on_fail.tv_sec = -1;

	osip_uri_init(&m_uriContact);
	if(OutboundIP)
	{
		m_uriContact->host = osip_strdup(OutboundIP);
		if(OutboundPort != 5060)
		{
			char strPort[50];
			sprintf(strPort,"%d",OutboundPort);
			m_uriContact->port = osip_strdup(strPort);
		}
	}
	else
	{
		m_uriContact->host = osip_strdup(Domain);
		if(nPort != 5060)
		{
			char strPort[50];
			sprintf(strPort,"%d",nPort);
			m_uriContact->port = osip_strdup(strPort);
		}
	}
	m_uriContact->scheme = osip_strdup("sip");
}

OPPSipRegisterDev::~OPPSipRegisterDev()
{
	if(m_UserName)
		osip_free((void*)m_UserName);
	if(m_Password)
		osip_free((void*)m_Password);
	if(m_Domain)
		osip_free((void*)m_Domain);
	if(m_AuthID)
		osip_free((void*)m_AuthID);
	if(m_CallID)
		osip_free((void*)m_CallID);
	if(m_Host)
		osip_free((void*)m_Host);
}

void OPPSipRegisterDev::OnTimeout( int flag , void *para)
{
	if(flag == OPPSipChannel::TIMEOUT_REGISTER_NO_RESPONSE)
	{
		m_reg_on_fail.tv_sec = -1;
	}
	else if(flag == OPPSipChannel::TIMEOUT_REGISTER)
	{
		m_reg_on_timeout.tv_sec = -1;
	}
	else 
	{
		NetDbg(DBG_ERROR,"register timeout flag not defined\n");
	}

	Register();
}


void OPPSipDev::SendSipMessage( char *from_name,char *body,char *content_type )
{
	if(m_uriContact)
	{
		osip_message_t *msg;
		char contact[256];
		char from[256];
		char *to;

		osip_uri_to_str(m_uriContact,&to);

		if(OPPSipService::sGetInstance()->GetListenPort() != 5060)
			sprintf(from,"sip:%s@%s:%d",from_name,OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort());
		else
			sprintf(from,"sip:%s@%s",from_name,OPPSipService::sGetInstance()->GetLocalHost());

		if(OPPSipService::sGetInstance()->GetListenPort() != 5060)
			sprintf(contact,"sip:%s:%d",OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort());
		else
			sprintf(contact,"sip:%s",OPPSipService::sGetInstance()->GetLocalHost());

		if( 0 == generating_request_out_of_dialog(&msg,"MESSAGE",m_uriContact,from,to,
			OPPSipService::sGetInstance()->GetLocalHost(),
			OPPSipService::sGetInstance()->GetListenPort(),contact,"UDP",
			(char*)GetOutboundIp(),GetOutboundPort()) )
		{
			osip_message_set_body(msg,body,strlen(body));
			osip_message_set_content_type(msg,content_type);

			OSIP_Transaction *tr = new OSIP_Transaction(NICT);
			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(msg,NICT)) )
			{
				osip_free(to);
				return;
			}
		}
		osip_free(to);
	}
}

