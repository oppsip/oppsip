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

#include <map>
#include <time.h>
#include "osip_port.h"

class OPPTimerAware
{
public:
	virtual void OnTimeout(int flag,void *para) = 0;
};

class OPPTimerMonitor
{
private:
	class timeval_sort
	{
	public:
		bool operator () (const struct timeval &tv1,const struct timeval &tv2) const
		{
			if(tv1.tv_sec < tv2.tv_sec) 
				return true;
			else if(tv1.tv_sec == tv2.tv_sec)
				return (tv1.tv_usec < tv2.tv_usec);
			else 
				return false;
		}
	};

protected:
	typedef struct opp_timer_t {OPPTimerAware *TimerInterface;int flag;void *Para;} zl_timer_t;

	typedef std::multimap<struct timeval,opp_timer_t,timeval_sort> TimerMap;

public:
	OPPTimerMonitor();
	virtual ~OPPTimerMonitor();

	int AddTimer(int nMSec,int nFlag,OPPTimerAware *TimerInterface,void *Para,struct timeval *tv);
	int CancelTimer( timeval *tv,int nFlag,OPPTimerAware *TimerInterface);
	int DelTimer(OPPTimerAware *TimerInterface);

	int CheckExpires();

	virtual void OnNotify(opp_timer_t *para);

	static OPPTimerMonitor *sGetInstance();

	TimerMap m_TimerMap;
private:
	
	static OPPTimerMonitor m_sInst;
};

