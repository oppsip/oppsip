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

#include "OPPChannelManager.h"
#include "OPPDebug.h"

#include "OPPApi.h"
#include "OPPEvents.h"
#include "OPPSipChannel.h"
#include "OSIP_Dialog.h"
#include "OPPSipService.h"


#ifdef LINUX_OS
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#endif

#ifdef GW_MT
#define GW_LOCK(X) osip_mutex_lock(X)
#define GW_UNLOCK(X) osip_mutex_unlock(X)
#else
#define GW_LOCK(X) do{} while(0)
#define GW_UNLOCK(X) do{} while(0)
#endif

OPPChannelManager *OPPChannelManager::sGetInstance()
{
	static OPPChannelManager sg_ChManager;
	return &sg_ChManager;
}

OPPChannelManager::OPPChannelManager(void)
{
	memset(m_devSipEndPoint,0,sizeof(m_devSipEndPoint));
	memset(m_devSipRegister,0,sizeof(m_devSipRegister));
	memset(m_devSipRelay,0,sizeof(m_devSipRelay));
	m_nSipEndPointDevNum = 0;
	m_nSipRelayDevNum = 0;
	m_nSipRegisterDevNum = 0;
	m_nWebRtcDevNum = 0;
	m_pEventCallback = NULL;
}

OPPChannelManager::~OPPChannelManager(void)
{
}

int ReadFileBlock(const char *FileName,char **Block,int* len)
{
	FILE *fp;
	int totallen;
	char *buf;
	const int blockLen = 512;
	int readlen = 0;
	int ret;
	int leftlen;

	fp = fopen( FileName, "rb" );
	if( fp == NULL )
	{
		NetDbg(DBG_ERROR,"Open Dsp kernel file error!\n");
		return -1;
	}

	fseek( fp, 0, SEEK_END );
	totallen = ftell(fp);
	fseek( fp, 0, SEEK_SET );

	buf = (char *)osip_malloc(totallen + 1);
	if( buf == NULL )
	{
		fclose(fp);
		return -1;
	}

	while(readlen < totallen)
	{
		leftlen = (totallen-readlen > blockLen) ? blockLen : (totallen-readlen);

		ret = fread( buf+readlen, 1, leftlen, fp );
		if(ret != leftlen)
		{
			fclose(fp);
			osip_free(buf);
			return -1;
		}
		readlen += ret;
	}

	fclose(fp);

	*Block = buf;
	*len = totallen;

	return 0;
}

int OPPChannelManager::Init(InitParam_t *para)
{
	OPPSipService::sGetInstance()->Init(para->in.LocalDomain,para->in.nSip_Port);
	
	memcpy(&m_InitPara,para,sizeof(InitParam_t));

	m_pEventCallback = para->in.pCallBack;

	struct timeval tm;
	__osip_port_gettimeofday(&tm,NULL);

	srand(tm.tv_usec);

	return 0;
}

OPPSipDev *OPPChannelManager::InitDev( DevPara_t *para )
{
	OPPSipDev *dev = NULL;
	
	switch(para->nDevType)
	{
	case DEV_TYPE_SIP_ENDPOINT:
		if(m_nSipEndPointDevNum < MAX_SIP_END_POINT)
		{
			OPPSipEndPointDev *devEnd = new OPPSipEndPointDev(para->x_dev.sip_endpoint_dev.nChannels,
						para->x_dev.sip_endpoint_dev.UserName,para->x_dev.sip_endpoint_dev.Password);
			if(devEnd)
			{
				devEnd->SetUserData(para->UserData);
				m_devSipEndPoint[m_nSipEndPointDevNum] = devEnd;
				m_nSipEndPointDevNum++;
				dev = devEnd;
			}
		}
		break;
	case DEV_TYPE_SIP_REGISTER:
		if(m_nSipRegisterDevNum < MAX_SIP_REGISTER)
		{
			OPPSipRegisterDev* devReg = new OPPSipRegisterDev(para->x_dev.sip_register_dev.nChannels,para->x_dev.sip_register_dev.UserName,
				         para->x_dev.sip_register_dev.Password,para->x_dev.sip_register_dev.Domain,
						 para->x_dev.sip_register_dev.nPort,NULL,5060);
			if(devReg)
			{
				devReg->SetUserData(para->UserData);
				m_devSipRegister[m_nSipRegisterDevNum] = devReg;
				m_nSipRegisterDevNum++;
				dev = devReg;
				devReg->Register();
			}
		}
		break;
	case DEV_TYPE_SIP_RELAY:
		if(m_nSipRelayDevNum < MAX_SIP_RELAY)
		{
			OPPSipRelayDev *devRelay = new OPPSipRelayDev(para->x_dev.sip_relay_dev.nChannels,para->x_dev.sip_relay_dev.Domain,
									para->x_dev.sip_relay_dev.Host,para->x_dev.sip_relay_dev.nPort);
			if(devRelay)
			{
				devRelay->SetUserData(para->UserData);
				m_devSipRelay[m_nSipRelayDevNum] = devRelay;
				m_nSipRelayDevNum++;
				dev = devRelay;
			}
		}
		break;

	default:
		dev = NULL;
		break;
	}
	return dev;
}

OPPSipDev *OPPChannelManager::GetSipDev(const char *RemoteAddr,const char *Domain,const char *Name)
{
	int i;
	if(RemoteAddr)
	{
		for(i=0;i<m_nSipRelayDevNum;i++)
		{
			if(strcmp(m_devSipRelay[i]->m_Host,RemoteAddr) == 0)
				return m_devSipRelay[i];
		}
		for(i=0;i<m_nSipRegisterDevNum;i++)
		{
			if(strcmp(m_devSipRegister[i]->m_Host,RemoteAddr) == 0)
				return m_devSipRegister[i];
		}
	}
	/*if(Domain)
	{
		for(i=0;i<m_nSipRegisterDevNum;i++)
		{
			if(strcmp(m_devSipRegister[i]->m_Domain,Domain) == 0)
				return m_devSipRegister[i];
		}
	}*/
	if(Name)
	{
		for(i=0;i<m_nSipEndPointDevNum;i++)
		{
			if(strcmp(m_devSipEndPoint[i]->m_UserName,Name) == 0)
				return m_devSipEndPoint[i];
		}
	}
	return NULL;
}


OPPSipChannel *OPPChannelManager::GetSipChannelByDialogAsUAS(osip_message_t *sip)
{
	int i;
	OPPSipChannel *ch;
	for(i=0;i<m_nSipEndPointDevNum;i++)
	{
		ch = m_devSipEndPoint[i]->GetSipChannelByDialogAsUAS(sip);
		if(ch)
			return ch;
	}
	for(i=0;i<m_nSipRegisterDevNum;i++)
	{
		ch = m_devSipRegister[i]->GetSipChannelByDialogAsUAS(sip);
		if(ch)
			return ch;
	}
	for(i=0;i<m_nSipRelayDevNum;i++)
	{
		ch = m_devSipRelay[i]->GetSipChannelByDialogAsUAS(sip);
		if(ch)
			return ch;
	}
	return NULL;
}

OPPSipChannel* OPPChannelManager::GetSipChannelByDialog(osip_message_t *sip)
{
	int i;
	OPPSipChannel *ch;
	for(i=0;i<m_nSipEndPointDevNum;i++)
	{
		ch = m_devSipEndPoint[i]->GetSipChannelByDialog(sip);
		if(ch)
			return ch;
	}
	for(i=0;i<m_nSipRegisterDevNum;i++)
	{
		ch = m_devSipRegister[i]->GetSipChannelByDialog(sip);
		if(ch)
			return ch;
	}
	for(i=0;i<m_nSipRelayDevNum;i++)
	{
		ch = m_devSipRelay[i]->GetSipChannelByDialog(sip);
		if(ch)
			return ch;
	}
	return NULL;
}

OPPSipChannel *OPPChannelManager::GetSipChannelByDialogAsUAC(osip_message_t *sip)
{
	int i;
	OPPSipChannel *ch;
	for(i=0;i<m_nSipEndPointDevNum;i++)
	{
		ch = m_devSipEndPoint[i]->GetSipChannelByDialogAsUAC(sip);
		if(ch)
			return ch;
	}
	for(i=0;i<m_nSipRegisterDevNum;i++)
	{
		ch = m_devSipRegister[i]->GetSipChannelByDialogAsUAC(sip);
		if(ch)
			return ch;
	}
	for(i=0;i<m_nSipRelayDevNum;i++)
	{
		ch = m_devSipRelay[i]->GetSipChannelByDialogAsUAC(sip);
		if(ch)
			return ch;
	}
	return NULL;
}

void OPPChannelManager::DoSip2xxRetryMessage(osip_message_t *response)
{
	GW_LOCK(m_mutexSipChannel);

	OPPSipChannel *ch = GetSipChannelByDialogAsUAC(response);
	if(ch)
	{
		OPPSipRetry2xxEvent evt(response);
		ch->Execute(&evt);
	}

	GW_UNLOCK(m_mutexSipChannel);
}

OPPSipChannel *OPPChannelManager::GetSipChannelByCallLeg(const char *call_id,const char *remote_tag,const char *local_tag)
{
	int i;
	OPPSipChannel *ch;
	for(i=0;i<m_nSipEndPointDevNum;i++)
	{
		ch = m_devSipEndPoint[i]->GetSipChannelByCallLeg(call_id,remote_tag,local_tag);
		if(ch)
			return ch;
	}
	for(i=0;i<m_nSipRegisterDevNum;i++)
	{
		ch = m_devSipRegister[i]->GetSipChannelByCallLeg(call_id,remote_tag,local_tag);
		if(ch)
			return ch;
	}
	for(i=0;i<m_nSipRelayDevNum;i++)
	{
		ch = m_devSipRelay[i]->GetSipChannelByCallLeg(call_id,remote_tag,local_tag);
		if(ch)
			return ch;
	}
	return NULL;
}



