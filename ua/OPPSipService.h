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
#include "OSIP_Transport.h"

class OSIP_Transaction;
class OPPSipDev;

class OPPSipService : public TransportCallBackInterface
{
public:
	~OPPSipService();

	static OPPSipService *sGetInstance();

	int Init(char *LocalHost,int ListenPort);

	void SetLocalHostPort( char *LocalHost,int ListenPort );

	void SetSipCallbacks();

	char *GetLocalHost()
	{
		return m_LocalHost;
	}

	int GetListenPort()
	{
		return m_ListenPort;
	}

	char *GetContact()
	{
		return m_ContactUri;
	}

	virtual void OnNewPacket(struct sockaddr_in *from_addr,uint8 *buf,int len);

private:
	static void cb_ict_rcv1xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_ict_rcv2xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_ict_rcv2xx_again(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_ict_rcv3xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_ict_rcv4xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_ict_rcv5xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_ict_rcv6xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nict_rcv1xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nict_rcv2xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nict_rcv3xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nict_rcv4xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nict_rcv5xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nict_rcv6xx(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_ist_rcv_invite(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_ist_rcv_ack(OSIP_Transaction *tr,osip_message_t *sip);
	
	static void cb_nist_rcv_bye(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nist_rcv_register(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nist_rcv_options(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nist_rcv_info(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nist_rcv_cancel(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nist_rcv_notify(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nist_rcv_subscribe(OSIP_Transaction *tr,osip_message_t *sip);
	static void cb_nist_rcv_unknown_request(OSIP_Transaction *tr,osip_message_t *sip);

	static void process_invite(OPPSipDev *dev,OSIP_Transaction *tr,osip_message_t *sip);

	OPPSipService();
	
	static OPPSipService m_sInst;

	char *m_LocalHost;
	int   m_ListenPort;
	char  m_ContactUri[128];
	char  m_FromUri[128];

	friend class OPPChannelSM;
};



