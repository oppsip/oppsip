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

typedef enum _CallDirection
{
	CallDirection_Outgoing = 0,
	CallDirection_Incoming = 1
}CallDirection_t;

class OPPSipChannel;
class OPPSipDev;

class OPPSession
{
public:

	OPPSession()
	{
	}
	virtual ~OPPSession()
	{
	}

	virtual void OnIncall(OPPSipChannel *ch) = 0;
	virtual void OnOutCallSetup(OPPSipChannel *ch) = 0;
	virtual void OnInCallSetup(OPPSipChannel *ch) = 0;
	virtual void OnEndCall(OPPSipChannel *ch) = 0;
	virtual void OnError(OPPSipChannel *ch,int code) = 0;
	virtual void OnTimeout(OPPSipChannel *ch,int nFlag) = 0;
	virtual void OnReIncall(OPPSipChannel *ch) = 0;
	virtual void OnRingBack(OPPSipChannel *ch) = 0;
	virtual void OnDTMF(OPPSipChannel *ch,int Event) = 0;
	
	virtual void OnTransfer(OPPSipChannel *leg,const char *dest,OPPSipChannel *legTo) = 0;
	virtual void OnDeviceOnline(OPPSipDev *dev) = 0;
	virtual void OnDeviceOffline(OPPSipDev *dev) = 0;
};

