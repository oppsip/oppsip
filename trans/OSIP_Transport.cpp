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

#include "OSIP_Transport.h"
#include "OSIP_Core.h"
#include "errno.h"

OSIP_Transport OSIP_Transport::m_sInst;

OSIP_Transport::OSIP_Transport()
{
}

OSIP_Transport::~OSIP_Transport()
{
}

unsigned long net_gethostbyname(const char *name,char *host,int i)
{
	unsigned long *addr;
	HOSTENT *pHot = gethostbyname(name);
	if(pHot == NULL) return 0;
	addr = (unsigned long*)(pHot->h_addr_list[i]);
	if(addr == 0) return 0;
	if(host != NULL)
	{
		sprintf( host,"%u.%u.%u.%u",((unsigned int)((unsigned char*)pHot->h_addr_list[i])[0]),
			((unsigned int)((unsigned char*)pHot->h_addr_list[i])[1]),
			((unsigned int)((unsigned char*)pHot->h_addr_list[i])[2]),
			((unsigned int)((unsigned char*)pHot->h_addr_list[i])[3])   );
	}
	return *addr; //return as network byte order;
}

int OSIP_Transport::SendMsg(const uint8 *buf,int bufLen, char *host, int port, int transport_type)
{
	if(host == NULL || port <= 0)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"host or port invalid, port=%d\n",port));
	}

	if(transport_type == OSIP_UDP)
	{
		unsigned long lAddr;
		int j;
		SOCKADDR_IN addr;
		char str[20];
		int bIp = 1;
		size_t len = strlen(host);

		for(j=0;j<len;j++)
		{
			if(*(host+j) =='.') continue;
			if (*(host+j) < '0' || *(host+j) > '9')
			{
				bIp = 0;
				break;
			}
		}
		if(!bIp)
		{
			lAddr = net_gethostbyname(host,str,0);
			if(lAddr == 0) 
				return -1;
		}
		else
		{
			lAddr = inet_addr(host);
		}

		addr.sin_addr.s_addr = lAddr;
		addr.sin_port = htons((short)port);
		addr.sin_family  = AF_INET;

		if( 0 >	sendto(m_UdpSock,(const char*)buf, bufLen, 0,(SOCKADDR*)&addr,sizeof(addr)) )
			return -1;
	}
	else if(transport_type == OSIP_TCP)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"Transport in TCP is not implement\n"));
	}

	return 0;
}

void Init_Sock()
{
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions later    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */

	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 ) {
			/* Tell the user that we could not find a usable */
			/* WinSock DLL.                                  */
			WSACleanup( );
			return; 
		}
#endif
}
int OSIP_Transport::Init(int port,TransportCallBackInterface *in)
{
	m_Callback = in;

	Init_Sock();

	m_Port = port;

	m_UdpSock = socket(AF_INET,SOCK_DGRAM,0);
	if(m_UdpSock < 0)
		return -1;

	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = 0;
	addr.sin_port = htons(m_Port);

	m_buf = (uint8*)osip_malloc(SIP_MESSAGE_MAX_LENGTH);
	
	if(bind(m_UdpSock,(struct sockaddr*)&addr,sizeof(addr)) < 0)
		return -1;
	else
		return 0;

	//Start();
}

#ifdef WIN32
#ifndef socklen_t
typedef int socklen_t;
#endif
#endif

void OSIP_Transport::Run()
{
	SOCKADDR_IN from_addr;
	socklen_t from_len = sizeof(from_addr);
	FD_SET fd;
	struct timeval tv;
	int i;

	tv.tv_sec = 0;
	tv.tv_usec = 1000;

	FD_ZERO(&fd);
	FD_SET(m_UdpSock,&fd);

	while(1)
	{
		i = select(m_UdpSock+1,&fd,NULL,NULL,&tv);
		if(i < 0)
		{
			if(errno == EINTR)
				continue;
			else
				break;
		}
		else if(i > 0)
		{
			i = recvfrom(m_UdpSock,(char*)m_buf,SIP_MESSAGE_MAX_LENGTH,0,(SOCKADDR*)&from_addr,&from_len);
			if( i > 0 && m_Callback )
			{
				m_Callback->OnNewPacket(&from_addr,m_buf,i);
			}
			break;
		}
		else if(i == 0)
			break;
	}
}

