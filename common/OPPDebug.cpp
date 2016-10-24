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
 
#include "OPPDebug.h"

#ifdef LINUX_OS
#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h> 
#include <stdarg.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#elif defined(WIN32)
#include <winsock2.h>
#endif

static int dbg_sock = -1;
static struct sockaddr_in dest_addr;
static FILE *dbg_fp = NULL;

void (CALL_BACK_TYPE *OnLog)(const char *msg) = NULL;

int NetDebugInit(char *localIP,int localPort,char *remoteIP,int remotePort)
{
	if(localPort < 0)
	{
		osip_trace_initialize(END_TRACE_LEVEL,stderr);
		return -1;
	}

	dbg_sock = socket(AF_INET,SOCK_DGRAM,0);
	if(dbg_sock < 0) 
	{
		dbg_fp = fopen("log.txt","w");
		fprintf(dbg_fp,"log start: socket fail\n");
		return -1;
	}

	memset(&dest_addr,0,sizeof(dest_addr));
	dest_addr.sin_addr.s_addr = inet_addr(remoteIP);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(remotePort);

	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_addr.s_addr = 0;//inet_addr(localIP);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(localPort);

	if( 0 > bind(dbg_sock,(sockaddr*)&addr,sizeof(addr)) )
	{
		dbg_fp = fopen("log.txt","w");
		fprintf(dbg_fp,"log start: bind fail\n");
		closesocket(dbg_sock);
		dbg_sock = -1;
		osip_trace_initialize(END_TRACE_LEVEL,stderr);
		return -1;
	}
	else
	{
		osip_trace_initialize_func(END_TRACE_LEVEL,NetTrace);
		return 0;
	}
}

//typedef void osip_trace_func_t(char *fi, int li, osip_trace_level_t level, char *chfr, va_list ap)
/************************************************************************/
/* 输出参数长度不能大于512个字节，否则会溢出异常;输出长字符串可以用NetDbgBlock函数 */
/************************************************************************/

void NetTrace(char *fi,int li,osip_trace_level_t level, char *chfr, va_list ap)
{
	char buf[4096];
	buf[0] = 0;

	vsprintf(buf,chfr,ap);

	if(fi)
		sprintf(buf+strlen(buf),"\t--%s %d\n",fi,li);

	if(OnLog)
		OnLog(buf);
	else if(dbg_sock > 0)
		sendto(dbg_sock,buf,strlen(buf),0,(sockaddr*)&dest_addr,sizeof(dest_addr));
	else if(dbg_fp)
		fprintf(dbg_fp,buf);
	else
		printf(buf);
}

void NetDbg(int level,const char *format,...)
{
	char buf[4096];
	va_list ap;
	va_start(ap,format);
	vsprintf(buf,format,ap);

	if(OnLog)
		OnLog(buf);
	else if(dbg_sock > 0){
		sendto(dbg_sock,buf,strlen(buf),0,(sockaddr*)&dest_addr,sizeof(dest_addr));
	}else if(dbg_fp){
		fprintf(dbg_fp,buf);
	} else
		printf(buf);

	va_end(ap);
}


void NetDbgBlock(char *buf,int len)
{	
	/*struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_addr.s_addr = inet_addr("192.168.1.79");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(50000);*/
	sendto(dbg_sock,buf,len,0,(sockaddr*)&dest_addr,sizeof(dest_addr));
}

void NetDbgBlock2(char *buf,int len)
{	
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_addr.s_addr = inet_addr("192.168.1.120");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(50000);
	sendto(dbg_sock,buf,len,0,(sockaddr*)&addr,sizeof(addr));
}

int set_debug_cb(void (CALL_BACK_TYPE*fun)(const char *msg))
{
	if(fun == NULL){
		OnLog = NULL;
		return(0);
	}
	if(OnLog){
		NetDbg(DBG_DEBUG, "Overwrite debug cb\n");
	}
	OnLog = fun;
	printf("Set OnLog to %p\n", fun);
	return 0;
}
