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

#include "OPPTimerMonitor.h"
#include "osip_port.h"

OPPTimerMonitor OPPTimerMonitor::m_sInst;

OPPTimerMonitor::OPPTimerMonitor()
{
}

OPPTimerMonitor::~OPPTimerMonitor()
{
}

int OPPTimerMonitor::AddTimer(int nMSec,int nFlag,OPPTimerAware *TimerInterface,void *Para,struct timeval *tv)
{
	if(tv == NULL)
		return -1;

	__osip_port_gettimeofday(tv,NULL);

	tv->tv_sec += nMSec/1000;
	tv->tv_usec += (nMSec%1000)*1000;

	tv->tv_sec += tv->tv_usec / 1000000;

	opp_timer_t timer;
	timer.TimerInterface = TimerInterface;
	timer.flag = nFlag;
	timer.Para = Para;

	m_TimerMap.insert(TimerMap::value_type(*tv,timer));

	return 0;
}

int OPPTimerMonitor::CheckExpires()
{
	TimerMap::iterator it;
	timeval_sort ts;

	struct timeval tv;
	opp_timer_t opt;

	__osip_port_gettimeofday(&tv,NULL);

	it = m_TimerMap.begin();

	if(it != m_TimerMap.end() && ts.operator ()(it->first,tv))
	{
		opt = it->second;

		m_TimerMap.erase(it);

		OnNotify(&opt);

		return 1;
	}
	else
		return 0;
}

int OPPTimerMonitor::CancelTimer( timeval *tv,int nFlag,OPPTimerAware *Inter )
{
	TimerMap::iterator it;

	std::pair<TimerMap::iterator,TimerMap::iterator> ret;

	ret = m_TimerMap.equal_range(*tv);

	printf("CancelTimer :11111\n");
	for(it=ret.first; it!=ret.second;)
	{
		printf("CancelTimer :22222\n");
		if(it->second.flag == nFlag && it->second.TimerInterface == Inter)
		{
			printf("CancelTimer :%p\n",Inter);
			m_TimerMap.erase(it++);
			tv->tv_sec = -1;
		}
		else
			it++;
	}

	return 0;
}

int OPPTimerMonitor::DelTimer( OPPTimerAware *TimerInterface )
{
	TimerMap::iterator it;

	it = m_TimerMap.begin();

	while(it != m_TimerMap.end())
	{
		if(it->second.TimerInterface == TimerInterface)
		{
			m_TimerMap.erase(it++);
		}
		else 
			++it;
	}
	return 0;
}

void OPPTimerMonitor::OnNotify( opp_timer_t *opt )
{
	opt->TimerInterface->OnTimeout(opt->flag,opt->Para);
}

OPPTimerMonitor * OPPTimerMonitor::sGetInstance()
{
	return &m_sInst;
}

