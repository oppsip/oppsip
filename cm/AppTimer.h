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
#include "OPPTimerMonitor.h"

class AppTimer :
	public OPPTimerMonitor
{
public:
	struct my_para {void *p;int nFlag;};
	class my_para_sort
	{
	public:
		bool operator () (const struct my_para &p1 ,const struct my_para &p2) const
		{
			if(p1.p < p2.p) 
				return true;
			else if(p1.p == p2.p)
				return (p1.nFlag < p2.nFlag);
			else 
				return false;
		}
	};

	AppTimer(void);
	~AppTimer(void);

	int StartTimer( int nMSec,OPPTimerAware *Inter,int nFlag,void *Para );
	void StopTimer( int nFlag,void *Para ,OPPTimerAware *Inter);
	virtual void OnNotify(zl_timer_t *para);

	static AppTimer *sGetInstance()
	{
		return &m_sInst;
	}

private:
	static AppTimer m_sInst;

	typedef std::map<struct my_para,struct timeval,my_para_sort> MyParaMap;

	MyParaMap m_ParaMap;

};
