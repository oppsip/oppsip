/*
The oSIP library implements the Session Initiation Protocol (SIP -rfc3261-)
Copyright (C) 2001,2002,2003,2004,2005,2006,2007 Aymeric MOIZARD jack@atosc.org

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#pragma once

#include "OSIP_Transaction.h"
#include <map>

class OSIP_MsgEvent;
class OSIP_TransManager
{
public:
	OSIP_TransManager(void);
	~OSIP_TransManager(void);

	int AddTrans( const char *key,OSIP_Transaction *tr );

	int RemoveTrans(OSIP_Transaction *tr);

	void EnumTrans(int fd);

	//OSIP_Transaction * GetTrans( const char *key,osip_fsm_type_t type );

	int FindTransaction_andExcuteEvent(OSIP_MsgEvent *evt);

	OSIP_Transaction *GetMatchedTransForCancel(osip_message_t *cancel);

	static OSIP_TransManager *sGetInstance();

private:
	class str_sort
	{
	public:
		bool operator () (const char *s1,const char *s2) const
		{
			return strcmp(s1,s2) < 0;
		}
	};

	typedef std::multimap<const char *,OSIP_Transaction*,str_sort> TransMap;

	TransMap m_ICTMap;
	TransMap m_NICTMap;
	TransMap m_ISTMap;
	TransMap m_NISTMap;

	TransMap *GetMap(osip_fsm_type_t ctx_type)
	{
		switch(ctx_type)
		{
		case ICT:
			return &m_ICTMap;
		case NICT:
			return &m_NICTMap;
		case IST:
			return &m_ISTMap;
		case NIST:
			return &m_NISTMap;
		}
		return NULL;
	}

	int osip_transaction_matching_response_osip_to_xict_17_1_3(OSIP_Transaction *tr,osip_message_t *response);
	int osip_transaction_matching_request_osip_to_xist_17_2_3(OSIP_Transaction *tr,osip_message_t *request);

	
	static OSIP_TransManager m_sInst;

#ifdef OSIP_MT
	osip_mutex_t *m_ict_fastmutex;
	osip_mutex_t *m_ist_fastmutex;
	osip_mutex_t *m_nict_fastmutex;
	osip_mutex_t *m_nist_fastmutex;
	osip_mutex_t *m_id_mutex;
#endif
};
