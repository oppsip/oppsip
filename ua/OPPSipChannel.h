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

#include "osip_message.h"
#include "OSIP_Dialog.h"
#include "OPPChannelManager.h"
#include "OPPTimerMonitor.h"
#include "OSIP_Transaction.h"
#include "OSIP_Event.h"
#include "OPPSipService.h"

class OPPChannelSM;
class OSIP_Transaction;
class OSIP_Dialog;
class OPPSipEvent;
class OPPSipDev;

class OPPSipChannel : public OPPState , public OPPTimerAware 
{
public:
	OPPSipChannel(OPPSipDev *pDev,int nChNo);
	virtual ~OPPSipChannel();

	enum sip_state_t
	{
		STATE_Ready = STATE_Inital,
		STATE_Ring,
		STATE_Accept,
		STATE_Setup,
		STATE_CallOut,
		STATE_RingBack,
		STATE_Terminate,
		MAX_STATE
	};

	enum TimerFlag_t
	{
		FLAG_INCALL_TIMER,
		FLAG_SENDCALLERID_TIMER,
		TIMEOUT_2xxRETRY,
		TIMEOUT_REGISTER,
		TIMEOUT_REGISTER_NO_RESPONSE,
		TIMEOUT_CALLOUT,
		TIMEOUT_ALLOCMEDIA,
	};
	
	void SetICT(OSIP_Transaction *tr);
	void SetIST(OSIP_Transaction *tr);
	void SetNICT(OSIP_Transaction *tr);
	void SetNIST(OSIP_Transaction *tr);

	void SetFromUri(osip_from_t *From);
	void SetFinalResponse(osip_message_t *response);
	void SetAck(osip_message_t *ack);

	void Execute(OPPSipEvent *evt);

	virtual void DoInCall(InCallParam_t *para);
	virtual void DoAcceptCall(AcceptParam_t *para);

	virtual void DoRingBack();
	virtual void DoNotify(const char *msg);

	virtual void DoEndCall();

	virtual void DoBlindTransfer(const char *TransTo);

	virtual void OnTimeout(int flag,void *para);
	virtual void OnDTMF(int dtmf);

	virtual void DoInfo( int dtmf );

	virtual const unsigned char *GetAllocTransID()
	{
		return m_AllocTransID;
	}

	virtual OPPDev *GetDev()
 	{
 		return (OPPDev*)m_pSipDev;
 	}
	
	void FreeRes();

	void SetInCallParam(InCallParam_t *para)
	{
		if(m_InCallParam)
			delete m_InCallParam;

		if(para)
			m_InCallParam = para->Clone();
		else
			m_InCallParam = NULL;
	}

	void SetAcceptParam(AcceptParam_t *para)
	{
		if(m_AcceptParam)
			delete m_AcceptParam;

		if(para)
			m_AcceptParam = para->Clone();
		else
			m_AcceptParam = NULL;
	}

	int GetChannelNo()
	{
		return m_nChNo;
	}

	void SetMyBody(const char *body,const char *content_type)
	{
		if(m_MyBody)
			osip_free(m_MyBody);
		if(m_MyContentType)
			osip_free(m_MyContentType);

		m_MyBody = osip_strdup(body);
		m_MyContentType = osip_strdup(content_type);
	}

	int	GetMyBody(const char **body,const char **content_type)
	{
		if(m_MyBody && m_MyContentType)
		{
			*body = m_MyBody;
			*content_type = m_MyContentType;
			return 0;
		}
		else
			return -1;
	}

	void	SetMediaPara(const char *Host,int port);
	int		GetMediaPara(const char **Host,uint16 *port);
	
	void SetAllocHostPort(const char *host,int port)
	{		
		if(m_AllocHost)
		{
			osip_free((void*)m_AllocHost);
			m_AllocHost = NULL;
		}
		if(host)
		{
			m_AllocHost = osip_strdup(host);
			m_AllocPort = port;
		}
	}
	
	virtual int GetAllocHostPort(const char **host,int *port)
	{
		if(m_AllocHost)
		{
			*host = m_AllocHost;
			*port = m_AllocPort;
			return 0;
		}
		else
			return -1;
	}
	void SetReinviteFlag(int flag)
	{
		m_reInviteFlag = flag;
	}
	int GetReinviteFlag()
	{
		return m_reInviteFlag;
	}
	void	SetPeerBody(const char *body,const char *content_type);
	int		GetPeerBody(const char **body,const char **content_type);
	
public:
	OSIP_Dialog      *m_Dialog;
	int				  m_bFree;
	OPPSipDev         *m_pSipDev;
	unsigned char 	  m_AllocTransID[16];

	OSIP_Transaction *m_ICT;
	OSIP_Transaction *m_IST;
	OSIP_Transaction *m_NICT;
	OSIP_Transaction *m_NIST;
private:
	char			 *m_FromUri;
	osip_message_t   *m_FinalResponse;
	osip_message_t   *m_Ack;
	struct timeval    m_2xxRetryTimeout;
	int				  m_2xxRetryTimes;
	struct timeval    m_tv_call_out;
	struct timeval    m_alloc_tm;

	OPPChannelSM		 *m_SM;
	int               m_nChNo;

	InCallParam_t	 *m_InCallParam;
	AcceptParam_t    *m_AcceptParam;
	const char		 *m_MediaHost;
	uint16			 m_MediaPort;
	const char       *m_AllocHost;
	uint16            m_AllocPort;

	char			 *m_PeerBody;
	char			 *m_PeerContentType;
	char             *m_MyBody;
	char			 *m_MyContentType;
	int               m_reInviteFlag;

	friend class OPPSipService;
	friend class OPPChannelSM;
	friend class OPPSipDev;
};

class OPPSipDev : public OPPDev,public OPPTimerAware
{
public:
	typedef enum sip_dev_state_t
	{
		STATE_OffLine = STATE_Inital,
		STATE_OnLine
	}sip_dev_state_t;

	typedef enum { DEV_TYPE_ENDPOINT,
				   DEV_TYPE_RELAY,
				   DEV_TYPE_REGISTER
	}DevType_t;

	OPPSipDev(int nChannels,DevType_t type,const char *outbound_ip = NULL,int outbound_port = 5060)
		:m_uriContact(NULL),m_nChannels(nChannels),m_devType(type)
	{
		m_outbound_ip = osip_strdup(outbound_ip);
		m_outbound_port = outbound_port;
		m_reg_tv.tv_sec = -1;
		m_SipChannel = new OPPSipChannel*[nChannels];
		AllocSipChannel();
	}
	virtual ~OPPSipDev()
	{
		if(m_outbound_ip)
			osip_free((void*)m_outbound_ip);

		if(m_SipChannel)
			delete[] m_SipChannel;
	}

	virtual void SetUserData(void *Data)
	{
		m_UserData = Data;
	}

	virtual void *GetUserData()
	{
		return m_UserData;
	}

	DevType_t GetDevType()
	{
		return m_devType;
	}

	int IsVeryLateInviteRetransform(OSIP_Transaction *tr)
	{
		for(int i=0;i<m_nChannels;i++)
		{
			if(m_SipChannel[i]->m_IST && strcmp(m_SipChannel[i]->m_IST->GetKey(),tr->GetKey()) == 0)
			{
				return 1;
			}
		}
		return 0;
	}

	void SendSipMessage(char *from_name,char *body,char *content_type);

	virtual void DoSipMessage(osip_message_t *sip)
	{
		if(m_uriContact && sip)
		{
			osip_message_t *msg;
			if( 0 == osip_message_clone(sip,&msg) )
			{
				osip_uri_free(msg->req_uri);
				osip_uri_clone(m_uriContact,&msg->req_uri);

				OSIP_Transaction *tr = new OSIP_Transaction(NICT);
				if( 0 != tr->Execute(OSIP_MsgEvent::sCreate(msg,NICT)) )
					return;
			}
			else
			{
				OSIP_TRACE(osip_trace
				(__FILE__, __LINE__, OSIP_ERROR, NULL,
				"osip_message_clone: failed\n"));
			}
		}
		else
		{
			OSIP_TRACE(osip_trace
				(__FILE__, __LINE__, OSIP_ERROR, NULL,
				"DoSipMessage: m_uriContact=%p sip=%p\n",m_uriContact,sip));
		}
	}

	virtual void OnTimeout(int flag,void *para)
	{
		if(flag == OPPSipChannel::TIMEOUT_REGISTER)
		{
			if(GetState() != OPPSipDev::STATE_OffLine)
			{
				SetState(OPPSipDev::STATE_OffLine);
				OPPChannelManager::sGetInstance()->GetEventCallback()->OnDeviceOffline(this);
			}
		}
	}

	void AllocSipChannel()
	{
		for(int i=0;i<m_nChannels;i++)
			m_SipChannel[i] = new OPPSipChannel(this,i);
	}

	virtual OPPSipChannel *GetFreeSipChannel()
	{
		for(int i=0;i<m_nChannels;i++)
		{
			if(m_SipChannel[i]->m_bFree)
			{
				m_SipChannel[i]->m_bFree = 0;
				return m_SipChannel[i];
			}
		}
		return NULL;
	}

	OPPSipChannel *GetSipChannelByDialog(osip_message_t *sip)
	{
		OSIP_Dialog *dlg;
		for(int j=0;j<m_nChannels;j++)
		{
			dlg = m_SipChannel[j]->m_Dialog;
			if( dlg && (0 == dlg->osip_dialog_match_as_uac(sip) || 0 == dlg->osip_dialog_match_as_uas(sip)) )
				return m_SipChannel[j];
		}
		return NULL;
	}

	OPPSipChannel *GetSipChannelByDialogAsUAS(osip_message_t *sip)
	{
		OSIP_Dialog *dlg;

		for(int j=0;j<m_nChannels;j++)
		{
			dlg = m_SipChannel[j]->m_Dialog;
			if(dlg && 0 == dlg->osip_dialog_match_as_uas(sip))
				return m_SipChannel[j];
		}
		return NULL;
	}

	OPPSipChannel *GetSipChannelByDialogAsUAC(osip_message_t *sip)
	{
		OSIP_Dialog *dlg;

		for(int j=0;j<m_nChannels;j++)
		{
			dlg = m_SipChannel[j]->m_Dialog;
			if(dlg && 0 == dlg->osip_dialog_match_as_uac(sip))
				return m_SipChannel[j];
		}
		return NULL;
	}

	OPPSipChannel *GetSipChannelByCallLeg(const char *call_id,const char *remote_tag,const char *local_tag)
	{
		OSIP_Dialog *dlg;

		for(int j=0;j<m_nChannels;j++)
		{
			dlg = m_SipChannel[j]->m_Dialog;
			if(dlg && strcmp(dlg->GetCallId(),call_id) == 0 &&
				strcmp(dlg->GetRemoteTag(),remote_tag) == 0 && 
				strcmp(dlg->GetLocalTag(),local_tag) == 0 )
				return m_SipChannel[j];
		}
		return NULL;
	}

	void SetContactUri(osip_uri_t *uri)
	{
		if(m_uriContact)
			osip_uri_free(m_uriContact);
		
		osip_uri_clone(uri,&m_uriContact);
	}
	osip_uri_t *GetContactUri()
	{
		return m_uriContact;
	}
	struct timeval *GetRegTv()
	{
		return &m_reg_tv;
	}

	const char *GetOutboundIp()
	{
		return m_outbound_ip;
	}

	int GetOutboundPort()
	{
		return m_outbound_port;
	}

	OPPSipChannel *GetChannelByNo(int nChNo)
	{
		return m_SipChannel[nChNo];
	}

	virtual const char *GetDomain() = 0;
	
protected:
	OPPSipChannel **m_SipChannel;
	osip_uri_t *m_uriContact;
	int m_nChannels;
	void *m_UserData;
	const char *m_outbound_ip;
	int   m_outbound_port;
	struct timeval m_reg_tv;

	DevType_t m_devType;
};

class OPPSipRegisterDev : public OPPSipDev
{
public:
	OPPSipRegisterDev(int nMaxChannels,const char *UserName,const char *Password,const char *Domain,int nPort,const char *OutboundIP,int OutboundPort);

	~OPPSipRegisterDev();

	int Calc_Proxy_Auth(osip_uri_t *req_uri, osip_message_t *sip,char *result,int code);

	int Register_With_Authentication(osip_message_t *sip);

	int Register();

	void OnRegisterResult(OSIP_Transaction *tr,osip_message_t *sip);

	virtual void OnTimeout(int flag,void *para);

	virtual const char *GetDomain()
	{
		return m_Domain;
	}

public:
	const char *m_UserName;
	const char *m_AuthID;
	const char *m_Password;
	const char *m_Domain;
	const char *m_CallID;
	int         m_nPort;
	int         m_nRegisterSeq;
	const char *m_Host;
	struct timeval m_reg_on_timeout;
	struct timeval m_reg_on_fail;
};

class OPPSipRelayDev : public OPPSipDev
{
public:
	OPPSipRelayDev(int nMaxChannels,const char *Domain,const char *Host,int nPort)
		:OPPSipDev(nMaxChannels,DEV_TYPE_RELAY,Host,nPort),m_nCurSel(0)
	{
		if(nPort != 5060)
		{
			char tmp[256];
			sprintf(tmp,"%s:%d",Domain,nPort);
			m_Domain = osip_strdup(tmp);
		}
		else
		{
			m_Domain = osip_strdup(Domain);
		}
		m_Host = osip_strdup(Host);
		m_nPort = nPort;

		osip_uri_init(&m_uriContact);
		m_uriContact->host = osip_strdup(m_Host);
		char strPort[50];
		sprintf(strPort,"%d",m_nPort);
		m_uriContact->port = osip_strdup(strPort);
		m_uriContact->scheme = osip_strdup("sip");
	}
	~OPPSipRelayDev()
	{
		if(m_Domain)
			osip_free((void*)m_Domain);
		if(m_Host)
			osip_free((void*)m_Host);
	}

	virtual OPPSipChannel *GetFreeSipChannel()
	{
		int i;

		for(i=m_nCurSel;i<m_nChannels;i++)
		{
			if(m_SipChannel[i]->m_bFree)
			{
				m_SipChannel[i]->m_bFree = 0;
				
				if(++m_nCurSel==m_nChannels)
					m_nCurSel = 0;

				return m_SipChannel[i];
			}
		}

		for(i=0;i<m_nCurSel;i++)
		{
			if(m_SipChannel[i]->m_bFree)
			{
				m_SipChannel[i]->m_bFree = 0;

				if(++m_nCurSel==m_nChannels)
					m_nCurSel = 0;

				return m_SipChannel[i];
			}
		}

		return NULL;
	}

	virtual const char *GetDomain()
	{
		return m_Domain;
	}

	const char *m_Domain;
	const char *m_Host;
	int         m_nPort;
	int m_nCurSel;
};

class OPPSipEndPointDev : public OPPSipDev
{
public:
	OPPSipEndPointDev(int nMaxChannels,const char *UserName,const char *Password)
		:OPPSipDev(nMaxChannels,DEV_TYPE_ENDPOINT)
	{
		m_UserName = osip_strdup(UserName);
		m_Password = osip_strdup(Password);
		if(OPPSipService::sGetInstance()->GetListenPort() != 5060)
		{
			char tmp[256];
			sprintf(tmp,"%s:%d",OPPSipService::sGetInstance()->GetLocalHost(),OPPSipService::sGetInstance()->GetListenPort());
			m_Domain = osip_strdup(tmp);
		}
		else
			m_Domain = osip_strdup(OPPSipService::sGetInstance()->GetLocalHost());
	}
	~OPPSipEndPointDev()
	{
		if(m_UserName)
			osip_free((void*)m_UserName);
		if(m_Password)
			osip_free((void*)m_Password);
		if(m_Domain)
			osip_free((void*)m_Domain);
	}

	/*virtual void OnTimeout(int flag)
	{
		if(flag == OPPSipChannel::TIMEOUT_REGISTER)
		{
			SetState(OPPSipDev::STATE_OffLine);
			OPPChannelManager::sGetInstance()->GetEventCallback()->OnDeviceOffline(this);
		}
	}*/

	virtual const char *GetDomain()
	{
		return m_Domain;
	};

public:
	const char *m_UserName;
	const char *m_Password;
	const char *m_Domain;

};




