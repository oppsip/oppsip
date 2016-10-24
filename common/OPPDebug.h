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

#include "osip_port.h"


enum 
{
	DBG_INFO = 1,
	DBG_WARNING,
	DBG_DEBUG,
	DBG_ERROR,
};

extern int debug;

#ifndef WIN32
#define CALL_BACK_TYPE 
#include <sys/time.h>
#else
#define CALL_BACK_TYPE __stdcall
#endif

void NetDbg(int level,const char *format,...);
int NetDebugInit(char *localIP,int localPort,char *remoteIP,int remotePort);

void NetDbgBlock(char *buf,int len);
void NetTrace(char *fi,int li,osip_trace_level_t level, char *chfr, va_list ap);
void NetDbgBlock2(char *buf,int len);
int set_debug_cb(void (CALL_BACK_TYPE*fun)(const char *msg));
