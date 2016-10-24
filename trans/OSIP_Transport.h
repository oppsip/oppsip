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
#include "osip_port.h"

class TransportCallBackInterface
{
public:
	TransportCallBackInterface(){};
	~TransportCallBackInterface(){};

	virtual void OnNewPacket(struct sockaddr_in *from_addr,uint8 *buf,int len) = 0;
};

class OSIP_Transport 
{
public:
	OSIP_Transport();
	~OSIP_Transport();

	int Init(int port,TransportCallBackInterface* in);

	int SendMsg(const uint8 *buf,int len, char *host, int port, int transport_type);
	static OSIP_Transport *sGetInstance()
	{
		return &m_sInst;
	}

	void Run();
	
private:
	TransportCallBackInterface *m_Callback;
	int m_UdpSock;
	int m_Port;
	uint8 *m_buf;

	static OSIP_Transport m_sInst;
};

