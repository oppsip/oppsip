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

#include "OPPApi.h"
#include "osip_message.h"
#include "OPPSession.h"

#define	MAX_SIP_END_POINT (10000)
#define	MAX_SIP_RELAY    (1000)
#define	MAX_SIP_REGISTER (1024)

int GetCurTimer();

typedef void (*Func_RecordCallback)(long DevID,char *data,int dataLen);

class OPPSipDev;
class OPPSipEndPointDev;
class OPPSipRegisterDev;
class OPPSipRelayDev;
class OPPSipChannel;
class OSIP_Transaction;

class OPPChannelManager
{
public:
	OPPChannelManager(void);
	~OPPChannelManager(void);

	int		Init(InitParam_t *para);
	OPPSipDev*	InitDev( DevPara_t *para );

	OPPSipChannel* GetSipChannelByDialog(osip_message_t *sip);
	OPPSipChannel *GetSipChannelByDialogAsUAS(osip_message_t *sip);
	OPPSipChannel *GetSipChannelByDialogAsUAC(osip_message_t *sip);
	OPPSipChannel *GetSipChannelByCallLeg(const char *call_id,const char *remote_tag,const char *local_tag);

	OPPSipDev *GetSipDev(const char *RemoteAddr,const char *Domain,const char *Name);

	const char *GetMediaRelayServer();
	int 		GetMediaRelayPort();
	const char *GetTenantPrefix();

	OPPSession *GetEventCallback() const
	{
		return m_pEventCallback;
	}

	void DoSip2xxRetryMessage(osip_message_t *response);

	static OPPChannelManager *sGetInstance();
	
private:
	OPPSession		*m_pEventCallback;
	OPPSipEndPointDev *m_devSipEndPoint[MAX_SIP_END_POINT];
	OPPSipRelayDev *m_devSipRelay[MAX_SIP_RELAY];
	OPPSipRegisterDev *m_devSipRegister[MAX_SIP_REGISTER];

	int  m_nSipEndPointDevNum;
	int  m_nSipRelayDevNum;
	int  m_nSipRegisterDevNum;
	int  m_nWebRtcDevNum;
	InitParam_t m_InitPara;
};

