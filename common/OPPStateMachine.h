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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define STATE_MACHINE_IMPL(E,S,I,Q) {m_pTable[I::STATE_##S][Q::EVENT_##E] = On##E##While##S;};

class OPPState;
class OPPEvent;

template <class S,class E>
class OPPStateMachine
{
public:
	explicit OPPStateMachine(int nMaxState,int nMaxEvent)
		:m_pTable(NULL)
	{
		
		m_pTable = new SM_FUNCTION_PTR*[nMaxState];
		for(int i=0;i<nMaxState;i++)
		{
			m_pTable[i] = new SM_FUNCTION_PTR[nMaxEvent];

			for(int j=0;j<nMaxEvent;j++)
			{
				m_pTable[i][j] = NULL;
			}
		}
	}
	
	virtual ~OPPStateMachine()
	{
		//if(m_pTable)
		//	delete[] m_pTable;
	}

	int ExecuteSM(S *state,E *evt)
	{
		SM_FUNCTION_PTR fun = m_pTable[state->GetState()][evt->GetType()];
		if(fun)
		{
			fun(state,evt);
			return 0;
		}
		else
		{
			return -1;
		}
	}

private:
	OPPStateMachine();
	typedef void (*SM_FUNCTION_PTR)(S *obj,E *evt);
protected:
	SM_FUNCTION_PTR **m_pTable;
};

