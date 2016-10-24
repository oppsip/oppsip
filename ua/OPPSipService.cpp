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

#include "OPPSipService.h"

#include "OPPChannelManager.h"
#include "OSIP_Transaction.h"
#include "OPPSipChannel.h"

#include "OSIP_Core.h"
#include "OSIP_Event.h"
#include "OPPEvents.h"
#include "OSIP_TransManager.h"
#include "osip_parser.h"
#include "osip_md5.h"
#include "stun_parser.h"

int genRandom36(char *Buffer,int len)
{
	int i;
	time_t tm;
	srand(time(&tm));

	for(i=0;i<len-1;i++)
	{
		unsigned rnd = abs(rand())%36;

		Buffer[i] = rnd < 10 ? (rnd+'0') : (rnd-10+'A');

	}

	Buffer[i] = '\0';

	return 0;
}

extern const char *MD5_String(const char *string,char *Dest);

extern void trim(char *pScr,char ch);

OPPSipService OPPSipService::m_sInst;

OPPSipService::OPPSipService()
{
}

OPPSipService::~OPPSipService(void)
{
}

void OPPSipService::cb_ict_rcv1xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	if(sip->status_code == 100)
		return;

	OPPSipChannel *ch = (OPPSipChannel*)tr->GetUserData();
	if(ch && ch->m_ICT == tr)
	{
		OPPSipRingBackEvent evt(tr,sip);
		ch->Execute(&evt);
	}
}

void OPPSipService::cb_ict_rcv2xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	OPPSipChannel *ch = (OPPSipChannel*)tr->GetUserData();
	if(ch && ch->m_ICT == tr)
	{
		OPPSipSetupEvent evt(tr,sip);
		ch->Execute(&evt);
	}
}

void OPPSipService::cb_ict_rcv2xx_again(OSIP_Transaction *tr,osip_message_t *sip)
{
	OPPChannelManager::sGetInstance()->DoSip2xxRetryMessage(sip);
}

void OPPSipService::cb_ict_rcv3xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	OPPSipChannel *ch = (OPPSipChannel*)tr->GetUserData();
	if(ch && ch->m_ICT == tr)
	{
		OPPSipErrorEvent evt(tr,sip);
		ch->Execute(&evt);
	}
}
void OPPSipService::cb_ict_rcv4xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	OPPSipChannel *ch = (OPPSipChannel*)tr->GetUserData();
	if(ch && ch->m_ICT == tr)
	{
		OPPSipErrorEvent evt(tr,sip);
		ch->Execute(&evt);
	}
}
void OPPSipService::cb_ict_rcv5xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	OPPSipChannel *ch = (OPPSipChannel*)tr->GetUserData();
	if(ch && ch->m_ICT == tr)
	{
		OPPSipErrorEvent evt(tr,sip);
		ch->Execute(&evt);
	}
}
void OPPSipService::cb_ict_rcv6xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	OPPSipChannel *ch = (OPPSipChannel*)tr->GetUserData();
	if(ch && ch->m_ICT == tr)
	{
		OPPSipErrorEvent evt(tr,sip);
		ch->Execute(&evt);
	}
}

void OPPSipService::cb_nict_rcv1xx(OSIP_Transaction *tr,osip_message_t *sip)
{

}
void OPPSipService::cb_nict_rcv2xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	if(MSG_IS_RESPONSE_FOR(sip,"REGISTER"))
	{
		((OPPSipRegisterDev*)tr->GetUserData())->OnRegisterResult(tr,sip);
	}
}

void OPPSipService::cb_nict_rcv3xx(OSIP_Transaction *tr,osip_message_t *sip)
{

}
void OPPSipService::cb_nict_rcv4xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	if(MSG_IS_RESPONSE_FOR(sip,"REGISTER"))
	{
		((OPPSipRegisterDev*)tr->GetUserData())->OnRegisterResult(tr,sip);
	}
}
void OPPSipService::cb_nict_rcv5xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	if(MSG_IS_RESPONSE_FOR(sip,"REGISTER"))
	{
		((OPPSipRegisterDev*)tr->GetUserData())->OnRegisterResult(tr,sip);
	}
}
void OPPSipService::cb_nict_rcv6xx(OSIP_Transaction *tr,osip_message_t *sip)
{
	if(MSG_IS_RESPONSE_FOR(sip,"REGISTER"))
	{
		((OPPSipRegisterDev*)tr->GetUserData())->OnRegisterResult(tr,sip);
	}
}

void OPPSipService::cb_ist_rcv_invite(OSIP_Transaction *tr,osip_message_t *sip)
{
	osip_message_t *response;

	const char *callee = sip->req_uri->username;
	if(callee == NULL)
		callee = sip->to->url->username;

	const char *caller = sip->from->url->username;
	
	if(callee == NULL || caller == NULL)
	{
		if( 0 != generating_response_default(&response,NULL,420,sip) )
			return;

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.

		return;
	}
	
	OPPSipChannel *ch = OPPChannelManager::sGetInstance()->GetSipChannelByDialogAsUAS(sip);
	if(ch)		
	{   
		if( 0 != generating_response_default(&response,NULL,100,sip) )
			return;

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
	
		//对于ict和设备对外呼出的情况怎么处理？
		if(ch->GetState() == OPPSipChannel::STATE_Accept || ch->GetState() == OPPSipChannel::STATE_Setup )
		{
			tr->SetUserData(ch);
			ch->SetIST(tr);

			OPPSipInCallEvent evt(tr,sip);
			ch->Execute(&evt);
		}
		else //if(ch->m_IST && ch->m_IST->GetState() == OSIP_Transaction::STATE_Proceeding) // no final response yet
		{
			if( 0 != generating_response_default(&response,NULL,423,sip) )
				return;

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
				return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
		}
	}
	else
	{
		OPPSipDev *dev = OPPChannelManager::sGetInstance()->GetSipDev(sip->from_ip,NULL,NULL);
		if(dev)//does not need 407 authentication because the sip message comes from the authorized ip address 
		{
			process_invite(dev,tr,sip);
		}
		else
		{
			osip_proxy_authorization_t *auth;
			if( 0 > osip_message_get_proxy_authorization(sip,0,&auth) )
			{
				char sRnd[37];
				char Content[256];

				if( 0 != generating_response_default(&response,NULL,407,sip) )
					return;

				genRandom36(sRnd,sizeof(sRnd));

				sprintf(Content,"Digest realm=\"liaowj@ehangcom.com\",nonce=\"%s\",algorithm=MD5",sRnd);

				osip_message_set_proxy_authenticate(response,Content);

				if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
					return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
			}
			else if(auth->nonce == NULL || auth->response == NULL || auth->realm == NULL || auth->uri == NULL)
			{
				if( 0 != generating_response_default(&response,NULL,420,sip) )
					return;

				if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
					return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
			}
			else
			{
				OPPSipEndPointDev *dev = (OPPSipEndPointDev*)OPPChannelManager::sGetInstance()->GetSipDev(NULL,NULL,sip->from->url->username);
				if(dev == NULL)
					dev = (OPPSipEndPointDev*)OPPChannelManager::sGetInstance()->GetSipDev(NULL,NULL,callee);
				
				if(dev)
				{
					char *domain;
					char *realm;
					char *str_resp;
					const char *pass;
					char *nonce;
					char *buf1;
					char *buf2;
					char md5[35];
					char md5_1[35];

					domain = osip_strdup(auth->uri);
					realm = osip_strdup(auth->realm);
					str_resp = osip_strdup(auth->response);
					nonce = osip_strdup(auth->nonce);

					trim(str_resp,'"');
					trim(domain,'"');
					trim(realm,'"');
					trim(nonce,'"');

					pass = dev->m_Password;

					buf1 = (char*)osip_malloc(strlen(dev->m_UserName)+strlen(realm)+strlen(pass)+5);
					sprintf(buf1,"%s:%s:%s",dev->m_UserName,realm,pass);

					buf2 = (char*)osip_malloc(strlen(domain)+strlen(sip->cseq->method)+4);
					sprintf(buf2,"%s:%s",sip->cseq->method,domain);

					MD5_String(buf1,md5);
					MD5_String(buf2,md5_1);

					osip_free(buf1);

					buf1 = (char*)osip_malloc(strlen(md5)+strlen(nonce)+strlen(md5_1)+5);

					sprintf(buf1,"%s:%s:%s",md5,nonce,md5_1);

					MD5_String(buf1,md5);

					if(strcmp(md5,str_resp) == 0)
					{
						process_invite(dev,tr,sip);
					}
					else
					{
						char sRnd[37];
						char Content[256];

						if( 0 != generating_response_default(&response,NULL,407,sip) )
						{
							osip_free(buf1);
							osip_free(buf2);
							osip_free(domain);
							osip_free(nonce);
							osip_free(realm);
							osip_free(str_resp);
							return;
						}

						genRandom36(sRnd,sizeof(sRnd));

						sprintf(Content,"Digest realm=\"liaowj@ehangcom.com\",nonce=\"%s\",algorithm=MD5",sRnd);

						osip_message_set_proxy_authenticate(response,Content);

						if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
						{
							osip_free(buf1);
							osip_free(buf2);
							osip_free(domain);
							osip_free(nonce);
							osip_free(realm);
							osip_free(str_resp);
							return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
						}
					}

					osip_free(buf1);
					osip_free(buf2);
					osip_free(domain);
					osip_free(nonce);
					osip_free(realm);
					osip_free(str_resp);
				}
				else
				{
					printf("sip dev not found for %s\n",sip->from->url->username);
					if( 0 != generating_response_default(&response,NULL,404,sip) )
						return;

					if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
						return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
				}
			}
		}		
	}
}

void OPPSipService::process_invite(OPPSipDev *dev,OSIP_Transaction *tr,osip_message_t *sip)
{
	osip_message_t *response;
	if( 0 != generating_response_default(&response,NULL,100,sip) )
		return;

	if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
		return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.

	if(dev)
	{
		if(dev->IsVeryLateInviteRetransform(tr))
		{
			if( 0 != generating_response_default(&response,NULL,423,sip) )
				return;

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
				return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
		}
		else
		{
			OPPSipChannel *ch = dev->GetFreeSipChannel();
			if(ch)
			{
				tr->SetUserData(ch);
				ch->SetIST(tr);

				OPPSipInCallEvent evt(tr,sip);
				ch->Execute(&evt);
			}
			else
			{
				if( 0 != generating_response_default(&response,NULL,500,sip) )
					return;
				if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
					return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
			}
		}
	}
	else
	{
		if( 0 != generating_response_default(&response,NULL,500,sip) )
			return;
		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
	}
}

void OPPSipService::cb_ist_rcv_ack(OSIP_Transaction *tr,osip_message_t *sip)
{
	if(tr == NULL)
	{
		OPPSipChannel *ch = OPPChannelManager::sGetInstance()->GetSipChannelByDialogAsUAS(sip);
		if(ch)
		{
			OPPSipSetupEvent evt(NULL,sip);
			ch->Execute(&evt);
		}
	}
}

int unescape_string(char *src,char **dst)
{
	if(src == NULL || dst == NULL)
		return -1;

	char *buf = (char*)osip_malloc(strlen(src)+1);
	*dst = buf;
	while(*src != '\0')
	{
		if(*src == '%' && *(src+1) != '\0' && *(src+2) != '\0')
		{
			char asc[3];
			strncpy(asc,src+1,2);
			asc[2] = '\0';
			*buf = (char)osip_atoi(asc);
			buf++;
			src+=3;
		}
		else
		{
			*buf = *src;
			buf++;
			src++;
		}
	}
	return 0;
}

void OPPSipService::cb_nist_rcv_unknown_request(OSIP_Transaction *tr,osip_message_t *sip)
{
	osip_message_t *response;

	if(MSG_IS_REFER(sip))
	{
		osip_header_t *refer_to = NULL;

		osip_uri_t *uri_refer_to = NULL;

		OPPSipChannel *refer_chan = NULL;

		OPPSipChannel *ch = OPPChannelManager::sGetInstance()->GetSipChannelByDialogAsUAS(sip);

		if(ch && 0 <= osip_message_header_get_byname(sip,"refer-to",0,&refer_to))
		{
			if( 0 != generating_response_default(&response,ch->m_Dialog,202,sip) )
				return;

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
				return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.

			if(refer_to && refer_to->hvalue)
			{
				osip_from_t *hd;
				osip_from_init(&hd);
				if( 0 == osip_from_parse(hd,refer_to->hvalue) && hd->url )
				{
					osip_uri_param_t *uri_para;

					osip_uri_clone(hd->url,&uri_refer_to);

					if( 0 == osip_uri_param_get_byname(&hd->url->url_params,"replaces",&uri_para) && uri_para->gvalue )
					{
						char *call_id = NULL;
						char *remote_tag = NULL;
						char *local_tag = NULL;
						char *dst;

						unescape_string(uri_para->gvalue,&dst);

						call_id = dst;

						remote_tag = strchr(call_id,';');
						if(remote_tag != NULL)
						{
							*remote_tag = '\0';
							remote_tag++;
							remote_tag = strchr(remote_tag,'=');
							if(remote_tag != NULL)
							{
								remote_tag++;
								local_tag = strchr(remote_tag,';');
								if(local_tag != NULL)
								{
									*local_tag = '\0';
									local_tag++;
									local_tag = strchr(local_tag,'=');
									if(local_tag != NULL)
										local_tag++;
								}
							}
						}

						if(call_id && remote_tag && local_tag)
							refer_chan = OPPChannelManager::sGetInstance()->GetSipChannelByCallLeg(call_id,remote_tag,local_tag);
						
						osip_free(dst);
					}

				}

				osip_from_free(hd);
			}

			OPPChannelManager::sGetInstance()->GetEventCallback()->OnTransfer(ch,uri_refer_to->username,refer_chan);
			osip_uri_free(uri_refer_to);
		}
		else
		{
			if( 0 != generating_response_default(&response,NULL,400,sip) )
				return;

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
				return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
		}
	}
	else if(MSG_IS_MESSAGE(sip))
	{
		OPPSipDev *pDev = OPPChannelManager::sGetInstance()->GetSipDev(NULL,NULL,sip->to->url->username);
		if(pDev)
		{
			pDev->DoSipMessage(sip);

			if( 0 != generating_response_default(&response,NULL,200,sip) )
				return;
		}
		else
		{
			if( 0 != generating_response_default(&response,NULL,404,sip) )
				return;
		}

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
	}
	else
	{
		osip_message_t *response;

		generating_response_default(&response,NULL,405,sip);

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;
	}
}

void OPPSipService::cb_nist_rcv_bye(OSIP_Transaction *tr,osip_message_t *sip)
{
	OPPSipChannel *ch = OPPChannelManager::sGetInstance()->GetSipChannelByDialog(sip);
	if(ch)
	{
		osip_message_t *response;
		if( 0 != generating_response_default(&response,ch->m_Dialog,200,sip) )
			return;

		complete_answer_that_establish_a_dialog(OPPSipService::sGetInstance()->m_ContactUri,response,sip);

		OPPSipEndCallEvent evt(tr,sip);
		ch->Execute(&evt);

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
	}
	else
	{
		osip_message_t *response;
		if( 0 != generating_response_default(&response,NULL,481,sip) )
			return;

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
	}
}

void OPPSipService::cb_nist_rcv_register(OSIP_Transaction *tr,osip_message_t *sip)
{
	osip_message_t *response;
	osip_contact_t *hd_contact;

	OSIP_TRACE(osip_trace
		(__FILE__, __LINE__, TRACE_LEVEL1, NULL,
		"cb_nist_rcv_register!\n"));

	if( 0 != osip_message_get_contact(sip,0,&hd_contact) || sip->from->url->username == NULL )
	{
		if( 0 != generating_response_default(&response,NULL,400,sip) )
			return;

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
	}
	else
	{
		osip_contact_t *hd_contact2;
		osip_generic_param_t *gen_para;
		const char *exp = NULL;
		int nExp = 0;

		if( 0 != osip_generic_param_get_byname(&hd_contact->gen_params,"expires",&gen_para) )
		{
			osip_header_t *hd_exp;

			if( 0 > osip_message_get_expires(sip,0,&hd_exp) )
			{
				if( 0 != generating_response_default(&response,NULL,400,sip) )
					return;

				tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType()));
				
				return;
			}
			else
			{
				exp = hd_exp->hvalue;
			}
		}
		else
		{
			exp = gen_para->gvalue;
		}

		if(exp)
			nExp = osip_atoi(exp);

		osip_authorization_t *auth;
		if( 0 > osip_message_get_authorization(sip,0,&auth) )
		{
			char sRnd[37];
			char Content[256];

			if( 0 != generating_response_default(&response,NULL,401,sip) )
				return;

			genRandom36(sRnd,sizeof(sRnd));

			sprintf(Content,"Digest realm=\"liaowj@ehangcom.com\",nonce=\"%s\",algorithm=MD5",sRnd);

			osip_message_set_www_authenticate(response,Content);

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
				return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
		}
		else if(auth->nonce == NULL || auth->response == NULL || auth->realm == NULL || auth->uri == NULL)
		{
			if( 0 != generating_response_default(&response,NULL,420,sip) )
				return;

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
				return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
		}
		else
		{
			OPPSipEndPointDev *dev = (OPPSipEndPointDev*)OPPChannelManager::sGetInstance()->GetSipDev(NULL,NULL,sip->from->url->username);
			if(dev)
			{
				char *domain;
				char *realm;
				char *str_resp;
				const char *pass;
				char *user_name;
				char *nonce;
				char *buf1;
				char *buf2;
				char md5[35];
				char md5_1[35];

				domain = osip_strdup(auth->uri);
				realm = osip_strdup(auth->realm);
				str_resp = osip_strdup(auth->response);
				nonce = osip_strdup(auth->nonce);
				user_name = osip_strdup(auth->username);

				trim(str_resp,'"');
				trim(domain,'"');
				trim(realm,'"');
				trim(nonce,'"');
				trim(user_name,'"');

				pass = dev->m_Password;

				buf1 = (char*)osip_malloc(strlen(user_name)+strlen(realm)+strlen(pass)+5);
				sprintf(buf1,"%s:%s:%s",user_name,realm,pass);

				buf2 = (char*)osip_malloc(strlen(domain)+strlen(sip->cseq->method)+4);
				sprintf(buf2,"%s:%s",sip->cseq->method,domain);

				MD5_String(buf1,md5);
				MD5_String(buf2,md5_1);

				osip_free(buf1);

				buf1 = (char*)osip_malloc(strlen(md5)+strlen(nonce)+strlen(md5_1)+5);

				sprintf(buf1,"%s:%s:%s",md5,nonce,md5_1);

				MD5_String(buf1,md5);

				if(strcmp(md5,str_resp) == 0)
				{
					if( 0 != generating_response_default(&response,NULL,200,sip) )
					{
						osip_free(buf1);
						osip_free(buf2);
						osip_free(domain);
						osip_free(nonce);
						osip_free(realm);
						osip_free(str_resp);
						osip_free(user_name);
						return;
					}

					osip_contact_clone(hd_contact,&hd_contact2);

					osip_list_add(&response->contacts,hd_contact2,0);

					osip_message_set_expires(response,"90");

					if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
					{
						osip_free(buf1);
						osip_free(buf2);
						osip_free(domain);
						osip_free(nonce);
						osip_free(realm);
						osip_free(str_resp);
						osip_free(user_name);
						return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
					}

					dev->SetContactUri(hd_contact->url);

					if(dev->GetRegTv()->tv_sec > 0)
						OPPTimerMonitor::sGetInstance()->CancelTimer(dev->GetRegTv(),OPPSipChannel::TIMEOUT_REGISTER,static_cast<OPPTimerAware*>(dev));

					if(nExp > 0)
					{
						OPPTimerMonitor::sGetInstance()->AddTimer(90000,OPPSipChannel::TIMEOUT_REGISTER,static_cast<OPPTimerAware*>(dev),NULL,dev->GetRegTv());

						if(dev->GetState() != OPPSipDev::STATE_OnLine)
						{
							dev->SetState(OPPSipDev::STATE_OnLine);
							OPPChannelManager::sGetInstance()->GetEventCallback()->OnDeviceOnline(dev);
						}
					}
					else
					{
						if(dev->GetState() != OPPSipDev::STATE_OffLine)
						{
							dev->SetState(OPPSipDev::STATE_OffLine);
							OPPChannelManager::sGetInstance()->GetEventCallback()->OnDeviceOffline(dev);
						}
					}			
				}
				else
				{
					char sRnd[37];
					char Content[256];

					if( 0 != generating_response_default(&response,NULL,401,sip) )
					{
						osip_free(buf1);
						osip_free(buf2);
						osip_free(domain);
						osip_free(nonce);
						osip_free(realm);
						osip_free(str_resp);
						osip_free(user_name);
						return;
					}

					genRandom36(sRnd,sizeof(sRnd));

					sprintf(Content,"Digest realm=\"liaowj@ehangcom.com\",nonce=\"%s\",algorithm=MD5",sRnd);

					osip_message_set_www_authenticate(response,Content);

					if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
					{
						osip_free(buf1);
						osip_free(buf2);
						osip_free(domain);
						osip_free(nonce);
						osip_free(realm);
						osip_free(str_resp);
						osip_free(user_name);
						return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
					}
				}

				osip_free(buf1);
				osip_free(buf2);
				osip_free(domain);
				osip_free(nonce);
				osip_free(realm);
				osip_free(str_resp);
				osip_free(user_name);
			}
			else
			{
				if( 0 != generating_response_default(&response,NULL,404,sip) )
					return;

				if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
					return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
			}
		}
	}
}

void OPPSipService::cb_nist_rcv_options(OSIP_Transaction *tr,osip_message_t *sip)
{
	osip_message_t *response;

	generating_response_default(&response,NULL,200,sip);

	if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
		return;
}

void OPPSipService::cb_nist_rcv_info(OSIP_Transaction *tr,osip_message_t *sip)
{
	OPPSipChannel *ch = OPPChannelManager::sGetInstance()->GetSipChannelByDialog(sip);
	if(ch)
	{
		osip_message_t *response;
		if( 0 != generating_response_default(&response,ch->m_Dialog,200,sip) )
			return;

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.

		osip_body_t *bodyt;
		if( 0 == osip_message_get_body(sip,0,&bodyt) )
		{
			char *signal = strstr(bodyt->body,"Signal=");
			if(signal)
			{	
				char *end = strchr(signal,'\r');
				if(end && end > signal+7 && end-signal-7 < 4)
				{
					char tmp[20];
					strncpy(tmp,signal+7,end-signal-7);
					tmp[end-signal-7] = '\0';
					if(tmp[0] == '*'){
						ch->OnDTMF(10);
					}
					else if(tmp[0] == '#'){
						ch->OnDTMF(11);
					}
					else{
						ch->OnDTMF(osip_atoi(tmp));
					}
				}
			}
		}
	}
	else
	{
		osip_message_t *response;
		generating_response_default(&response,NULL,200,sip);
		
		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;
	}
}

void OPPSipService::cb_nist_rcv_cancel(OSIP_Transaction *tr,osip_message_t *sip)
{
	OSIP_Transaction *ist;
	osip_message_t *response;
	ist = OSIP_TransManager::sGetInstance()->GetMatchedTransForCancel(sip);
	if(ist == NULL)
	{
		OPPSipChannel *ch = OPPChannelManager::sGetInstance()->GetSipChannelByDialogAsUAS(sip);
		if(ch)
		{
			generating_response_default(&response,NULL,200,sip);

			OPPSipCancelCallEvent evt(tr,sip);
			ch->Execute(&evt);

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
				return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
		}
		else
		{
			generating_response_default(&response,NULL,481,sip);

			if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
				return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
		}
	}
	else
	{
		OPPSipChannel *ch = (OPPSipChannel*)ist->GetUserData();
		if(ch && ch->m_IST == ist)
		{
			OPPSipCancelCallEvent evt(tr,sip);
			ch->Execute(&evt);
		}

		generating_response_default(&response,NULL,200,sip);

		if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
			return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
	}
}

void OPPSipService::cb_nist_rcv_notify(OSIP_Transaction *tr,osip_message_t *sip)
{
	osip_message_t *response;

	generating_response_default(&response,NULL,200,sip);

	if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
		return;
}

void OPPSipService::cb_nist_rcv_subscribe(OSIP_Transaction *tr,osip_message_t *sip)
{
	osip_message_t *response;

	generating_response_default(&response,NULL,200,sip);

	if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(response,tr->GetCtxType())) )
		return;//'tr' has been deleted, MUST return to avoid 'tr' to be use later.
}

void OPPSipService::SetSipCallbacks()
{
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_ICT_STATUS_1XX_RECEIVED,cb_ict_rcv1xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_ICT_STATUS_2XX_RECEIVED,cb_ict_rcv2xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN,cb_ict_rcv2xx_again);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_ICT_STATUS_3XX_RECEIVED,cb_ict_rcv3xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_ICT_STATUS_4XX_RECEIVED,cb_ict_rcv4xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_ICT_STATUS_5XX_RECEIVED,cb_ict_rcv5xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_ICT_STATUS_6XX_RECEIVED,cb_ict_rcv6xx);
	
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NICT_STATUS_1XX_RECEIVED,cb_nict_rcv1xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NICT_STATUS_2XX_RECEIVED,cb_nict_rcv2xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NICT_STATUS_3XX_RECEIVED,cb_nict_rcv3xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NICT_STATUS_4XX_RECEIVED,cb_nict_rcv4xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NICT_STATUS_5XX_RECEIVED,cb_nict_rcv5xx);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NICT_STATUS_6XX_RECEIVED,cb_nict_rcv6xx);

	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_IST_INVITE_RECEIVED,cb_ist_rcv_invite);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_IST_ACK_RECEIVED,cb_ist_rcv_ack);

	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NIST_REGISTER_RECEIVED,cb_nist_rcv_register);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NIST_BYE_RECEIVED,cb_nist_rcv_bye);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NIST_OPTIONS_RECEIVED,cb_nist_rcv_options);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NIST_INFO_RECEIVED,cb_nist_rcv_info);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NIST_CANCEL_RECEIVED,cb_nist_rcv_cancel);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NIST_NOTIFY_RECEIVED,cb_nist_rcv_notify);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NIST_SUBSCRIBE_RECEIVED,cb_nist_rcv_subscribe);
	OSIP_Core::sGetInstance()->SetCallback(OSIP_Core::OSIP_NIST_UNKNOWN_REQUEST_RECEIVED,cb_nist_rcv_unknown_request);
}

OPPSipService *OPPSipService::sGetInstance()
{
	return &m_sInst;
}

int OPPSipService::Init(char *LocalHost,int ListenPort)
{
	SetLocalHostPort(LocalHost,ListenPort);
	
	OSIP_Core::sGetInstance()->Init();

	SetSipCallbacks();

	if( 0 != OSIP_Transport::sGetInstance()->Init(ListenPort,this) )
		return -1;

	return 0;
}

void OPPSipService::SetLocalHostPort( char *LocalHost,int ListenPort )
{
	m_LocalHost = osip_strdup(LocalHost);
	m_ListenPort = ListenPort;

	sprintf(m_ContactUri,"sip:%s:%d",m_LocalHost,m_ListenPort);
}

void OPPSipService::OnNewPacket( struct sockaddr_in *from_addr,uint8 *buf,int len )
{
	OSIP_Core::sGetInstance()->DoSipMsg(from_addr,(char*)buf,len);
}

