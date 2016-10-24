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
 
#include "AppTimer.h"
#include "wsLuaHook.h"

AppTimer AppTimer::m_sInst;

AppTimer::AppTimer(void)
{
}

AppTimer::~AppTimer(void)
{
}

void AppTimer::OnNotify( opp_timer_t *para )
{
	struct my_para mp;
	mp.nFlag = para->flag;
	mp.p = para->Para;

	MyParaMap::iterator it = m_ParaMap.find(mp);
	if(it != m_ParaMap.end())
	{
		m_ParaMap.erase(it);
	}

	para->TimerInterface->OnTimeout(para->flag,para->Para);
}

int AppTimer::StartTimer( int nMSec,OPPTimerAware *Inter,int nFlag,void *Para )
{
	struct my_para mp;
	mp.p = Para;
	mp.nFlag = nFlag;

	struct timeval tv;
	AddTimer(nMSec,nFlag,Inter,Para,&tv);

	std::pair<MyParaMap::iterator,bool> ret;
	ret = m_ParaMap.insert(MyParaMap::value_type(mp,tv));
	if(ret.second == false)
	{
		CancelTimer(&tv,nFlag,Inter);
		return -1;
	}
	else
		return 0;
}

void AppTimer::StopTimer( int nFlag,void *Para ,OPPTimerAware *Inter)
{
	struct my_para mp;
	mp.nFlag = nFlag;
	mp.p = Para;

	MyParaMap::iterator it = m_ParaMap.find(mp);
	if(it != m_ParaMap.end())
	{
		CancelTimer(&(it->second),nFlag,Inter);
		m_ParaMap.erase(it);
	}
}


