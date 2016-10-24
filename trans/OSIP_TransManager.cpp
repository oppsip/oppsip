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

#include "OSIP_TransManager.h"
#include "OSIP_Event.h"
#include "osip_parser.h"
#include "OSIP_Core.h"

OSIP_TransManager OSIP_TransManager::m_sInst;

OSIP_TransManager::OSIP_TransManager(void)
{
}

OSIP_TransManager::~OSIP_TransManager(void)
{
}

//OSIP_Transaction * OSIP_TransManager::GetTrans( const char *key,osip_fsm_type_t type )
//{
//	OSIP_Transaction *tr = NULL;
//	TransMap *map;
//#ifdef OSIP_MT
//	smutex_t *mut;
//#endif
//
//	switch (type)
//	{
//	case ICT:
//		map = &m_ICTMap;
//#ifdef OSIP_MT
//		mut = m_ict_fastmutex;
//#endif
//		break;
//	case IST:
//		map = &m_ISTMap;
//#ifdef OSIP_MT
//		mut = m_ist_fastmutex;
//#endif
//		break;
//	case NICT:
//		map = &m_NICTMap;
//#ifdef OSIP_MT
//		mut = m_nict_fastmutex;
//#endif
//		break;
//	case NIST:
//		map = &m_NISTMap;
//#ifdef OSIP_MT
//		mut = m_nist_fastmutex;
//#endif
//		break;
//	default:
//		return NULL;
//	}
//
//	TransMap::iterator it;
//
//#ifdef OSIP_MT
//	smutex_lock(mut);
//#endif
//	it = map->find(key);
//	if(it != map->end())
//		tr = it->second;
//#ifdef OSIP_MT
//	smutex_unlock(mut);
//#endif
//	return tr;
//}

int show_osip(int fd, int argc, char *argv[])
{
	OSIP_TransManager::sGetInstance()->EnumTrans(fd);
	show_mem(fd);
	return 0;
}

void OSIP_TransManager::EnumTrans(int fd)
{
	TransMap::iterator it;
	OSIP_Transaction *tr;

	for(it=m_ISTMap.begin();it!=m_ISTMap.end();it++)
	{
		tr = it->second;
		if(tr)
		{
//			printf("Existing dev=%p %d %s\n",((GwSipChannel*)tr->GetUserData())->m_pSipDev,tr->GetState(),tr->orig_request->call_id->number);
//			write(fd,buf,strlen(buf));
		}
	}
}

int OSIP_TransManager::AddTrans( const char *key,OSIP_Transaction *tr )
{
	TransMap *map;
#ifdef OSIP_MT
	osip_mutex_t *mut;
#endif

	switch(tr->GetCtxType())
	{
	case ICT:
		map = &m_ICTMap;
#ifdef OSIP_MT
		mut = m_ict_fastmutex;
#endif
		break;
	case IST:
		map = &m_ISTMap;
#ifdef OSIP_MT
		mut = m_ist_fastmutex;
#endif
		break;
	case NICT:
		map = &m_NICTMap;
#ifdef OSIP_MT
		mut = m_nict_fastmutex;
#endif
		break;
	case NIST:
		map = &m_NISTMap;
#ifdef OSIP_MT
		mut = m_nist_fastmutex;
#endif
		break;
	default:
		return -1;
	}

	TransMap::iterator it;

#ifdef OSIP_MT
	osip_mutex_lock(mut);
#endif
	map->insert(TransMap::value_type(key,tr));//mutimap always return true;
#ifdef OSIP_MT
	osip_mutex_unlock(mut);
#endif
	return 0;
}

int OSIP_TransManager::RemoveTrans( OSIP_Transaction *tr )
{
	int ret = -1;
	TransMap *map;
#ifdef OSIP_MT
	osip_mutex_t *mut;
#endif

	if(tr->GetKey() == NULL)
		return ret;

	switch(tr->GetCtxType())
	{
	case ICT:
		map = &m_ICTMap;
#ifdef OSIP_MT
		mut = m_ict_fastmutex;
#endif
		break;
	case IST:
		map = &m_ISTMap;
#ifdef OSIP_MT
		mut = m_ist_fastmutex;
#endif
		break;
	case NICT:
		map = &m_NICTMap;
#ifdef OSIP_MT
		mut = m_nict_fastmutex;
#endif
		break;
	case NIST:
		map = &m_NISTMap;
#ifdef OSIP_MT
		mut = m_nist_fastmutex;
#endif
		break;
	default:
		return -1;
	}

	TransMap::iterator it;

#ifdef OSIP_MT
	osip_mutex_lock(mut);
#endif
	std::pair<TransMap::iterator,TransMap::iterator> pa;
	pa = map->equal_range(tr->GetKey());
	for(it = pa.first;it!=pa.second;)
	{
		if(it->second == tr)
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"--Trans:%p removed\n",tr));

			map->erase(it);
			ret = 0;
			break;
		}
		else
			it++;
	}
#ifdef OSIP_MT
	osip_mutex_unlock(mut);
#endif
	return ret;
}

OSIP_TransManager * OSIP_TransManager::sGetInstance()
{
	return &m_sInst;
}

int OSIP_TransManager::osip_transaction_matching_response_osip_to_xict_17_1_3(OSIP_Transaction *tr,osip_message_t *response)
{
	osip_generic_param_t *b_request;
	osip_generic_param_t *b_response;
	osip_via_t *topvia_response;

	/* some checks to avoid crashing on bad requests */
	if(tr == NULL || (tr->ict_context == NULL && tr->nict_context == NULL) ||
		/* only ict and nict can match a response */
		response == NULL || response->cseq == NULL
		|| response->cseq->method == NULL)
		return OSIP_BADPARAMETER;

	topvia_response = (osip_via_t*)osip_list_get(&response->vias, 0);
	if(topvia_response == NULL) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Remote UA is not compliant: missing a Via header!\n"));
		return OSIP_SYNTAXERROR;
	}

	osip_via_param_get_byname(tr->topvia, "branch", &b_request);
	if(b_request == NULL) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_BUG, NULL,
			"You created a transaction without any branch! THIS IS NOT ALLOWED\n"));
		return OSIP_SYNTAXERROR;
	}

	osip_via_param_get_byname(topvia_response, "branch", &b_response);
	if(b_response == NULL) 
	{
#ifdef FWDSUPPORT
		/* the from tag (unique) */
		if(from_tag_match(tr->from, response->from) != 0)
			return OSIP_UNDEFINED_ERROR;
		/* the Cseq field */
		if(cseq_match(tr->cseq, response->cseq) != 0)
			return OSIP_UNDEFINED_ERROR;
		/* the To field */
		if(response->to->url->username == NULL && tr->from->url->username != NULL)
			return OSIP_UNDEFINED_ERROR;
		if(response->to->url->username != NULL && tr->from->url->username == NULL)
			return OSIP_UNDEFINED_ERROR;
		if(response->to->url->username != NULL && tr->from->url->username != NULL) 
		{
			if(strcmp(response->to->url->host, tr->from->url->host) ||
				strcmp(response->to->url->username, tr->from->url->username))
				return OSIP_UNDEFINED_ERROR;
		} 
		else
		{
			if (strcmp(response->to->url->host, tr->from->url->host))
				return OSIP_UNDEFINED_ERROR;
		}

		/* the Call-ID field */
		if(call_id_match(tr->callid, response->call_id) != 0)
			return OSIP_UNDEFINED_ERROR;
		return OSIP_SUCCESS;
#else
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_BUG, NULL,
			"Remote UA is not compliant: missing a branch parameter in  Via header!\n"));
		return OSIP_SYNTAXERROR;
#endif
	}

	/*
	A response matches a client transaction under two
	conditions:

	1.   If the response has the same value of the branch parameter
	in the top Via header field as the branch parameter in the
	top Via header field of the request that created the
	transaction.
	*/
	if(0 != strcmp(b_request->gvalue, b_response->gvalue))
		return OSIP_UNDEFINED_ERROR;
	/*  
	2.   If the method parameter in the CSeq header field matches
	the method of the request that created the transaction. The
	method is needed since a CANCEL request constitutes a
	different transaction, but shares the same value of the
	branch parameter.
	AMD NOTE: cseq->method is ALWAYS the same than the METHOD of the request.
	*/
	if(0 == strcmp(response->cseq->method, tr->cseq->method))	/* general case */
		return OSIP_SUCCESS;

	return OSIP_UNDEFINED_ERROR;
}

int OSIP_TransManager::osip_transaction_matching_request_osip_to_xist_17_2_3(OSIP_Transaction *tr,osip_message_t *request)
{
	osip_generic_param_t *b_origrequest;
	osip_generic_param_t *b_request;
	osip_via_t *topvia_request;
	size_t length_br;
	size_t length_br2;

	/* some checks to avoid crashing on bad requests */
	if(tr == NULL || (tr->ist_context == NULL && tr->nist_context == NULL) ||
		/* only ist and nist can match a request */
		request == NULL || request->cseq == NULL || request->cseq->method == NULL)
		return OSIP_BADPARAMETER;

	topvia_request = (osip_via_t*)osip_list_get(&request->vias, 0);
	if(topvia_request == NULL) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Remote UA is not compliant: missing a Via header!\n"));
		return OSIP_SYNTAXERROR;
	}
	osip_via_param_get_byname(topvia_request, "branch", &b_request);
	osip_via_param_get_byname(tr->topvia, "branch", &b_origrequest);

	if((b_origrequest == NULL && b_request != NULL) ||
		(b_origrequest != NULL && b_request == NULL))
		return OSIP_SYNTAXERROR;	/* one request is compliant, the other one is not... */

	/* Section 17.2.3 Matching Requests to Server Transactions:
	"The branch parameter in the topmost Via header field of the request
	is examined. If it is present and begins with the magic cookie
	"z9hG4bK", the request was generated by a client transaction
	compliant to this specification."
	*/

	if(b_origrequest != NULL && b_request != NULL)
		/* case where both request contains a branch */
	{
		if (!b_origrequest->gvalue)
			return OSIP_UNDEFINED_ERROR;
		if (!b_request->gvalue)
			return OSIP_UNDEFINED_ERROR;

		length_br = strlen(b_origrequest->gvalue);
		length_br2 = strlen(b_request->gvalue);
		if (length_br != length_br2)
			return OSIP_UNDEFINED_ERROR;

		/* can't be the same */
		if(0 == strncmp(b_origrequest->gvalue, "z9hG4bK", 7) && 0 == strncmp(b_request->gvalue, "z9hG4bK", 7)) 
		{
				/* both request comes from a compliant UA */
				/* The request matches a transaction if the branch parameter
				in the request is equal to the one in the top Via header
				field of the request that created the transaction, the
				sent-by value in the top Via of the request is equal to
				the one in the request that created the transaction, and in
				the case of a CANCEL request, the method of the request
				that created the transaction was also CANCEL.
			 */
				if (0 != strcmp(b_origrequest->gvalue, b_request->gvalue))
					return OSIP_UNDEFINED_ERROR;	/* branch param does not match */
				{
					/* check the sent-by values */
					char *b_port = via_get_port(topvia_request);
					char *b_origport = via_get_port(tr->topvia);
					char *b_host = via_get_host(topvia_request);
					char *b_orighost = via_get_host(tr->topvia);

					if ((b_host == NULL || b_orighost == NULL))
						return OSIP_UNDEFINED_ERROR;
					if (0 != strcmp(b_orighost, b_host))
						return OSIP_UNDEFINED_ERROR;

					if (b_port != NULL && b_origport == NULL
						&& 0 != strcmp(b_port, "5060"))
						return OSIP_UNDEFINED_ERROR;
					else if (b_origport != NULL && b_port == NULL
						&& 0 != strcmp(b_origport, "5060"))
						return OSIP_UNDEFINED_ERROR;
					else if (b_origport != NULL && b_port != NULL
						&& 0 != strcmp(b_origport, b_port))
						return OSIP_UNDEFINED_ERROR;
				}
#ifdef AC_BUG
				/* audiocodes bug (MP108-fxs-SIP-4-0-282-380) */
				if (0 != osip_from_tag_match(tr->from, request->from))
					return OSIP_UNDEFINED_ERROR;
#endif
				if(/* MSG_IS_CANCEL(request)&& <<-- BUG from the spec?
					I always check the CSeq */
					(!(0 == strcmp(tr->cseq->method, "INVITE") &&
					0 == strcmp(request->cseq->method, "ACK")))
					&& 0 != strcmp(tr->cseq->method, request->cseq->method))
					return OSIP_UNDEFINED_ERROR;

				return OSIP_SUCCESS;
			}
	}

	/* Back to the old backward compatibilty mechanism for matching requests */
	if(0 != osip_call_id_match(tr->callid, request->call_id))
		return OSIP_UNDEFINED_ERROR;
	if(MSG_IS_ACK(request))
	{
		osip_generic_param_t *tag_from1;
		osip_generic_param_t *tag_from2;

		osip_from_param_get_byname(tr->to, "tag", &tag_from1);
		osip_from_param_get_byname(request->to, "tag", &tag_from2);
		if(tag_from1 == NULL && tag_from2 != NULL)
		{	/* do not check it as it can be a new tag when the final
														answer has a tag while an INVITE doesn't have one */
		}
		else if (tag_from1 != NULL && tag_from2 == NULL)
		{
			return OSIP_UNDEFINED_ERROR;
		}
		else
		{
			if (0 != osip_to_tag_match(tr->to, request->to))
				return OSIP_UNDEFINED_ERROR;
		}
	} 
	else
	{
		if(0 != osip_to_tag_match(tr->to, request->to))
			return OSIP_UNDEFINED_ERROR;
	}
	if(0 != osip_from_tag_match(tr->from, request->from))
		return OSIP_UNDEFINED_ERROR;
	if(0 != osip_cseq_match(tr->cseq, request->cseq))
		return OSIP_UNDEFINED_ERROR;
	if(0 != osip_via_match(tr->topvia, topvia_request))
		return OSIP_UNDEFINED_ERROR;
	return OSIP_SUCCESS;
}

int OSIP_TransManager::FindTransaction_andExcuteEvent(OSIP_MsgEvent *evt)
{
	osip_generic_param_t *b_request;
	osip_via_t *topvia_request;

	osip_message_t *sip = evt->GetSipMsg();

	topvia_request = (osip_via_t*)osip_list_get(&sip->vias, 0);
	if(topvia_request == NULL) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Remote UA is not compliant: missing a Via header!\n"));
		evt->FreeSipMsg();
		delete evt;
		return OSIP_SYNTAXERROR;
	}

	osip_via_param_get_byname(topvia_request, "branch", &b_request);
	if(b_request == NULL)
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Remote UA is not compliant: missing a branch!\n"));
		evt->FreeSipMsg();
		delete evt;
		return OSIP_SYNTAXERROR;
	}

	TransMap *map = GetMap(evt->GetCtxType());
	if(map)
	{
		OSIP_Transaction *tr;
		TransMap::iterator it;
		std::pair<TransMap::iterator,TransMap::iterator> ret;
		ret = map->equal_range(b_request->gvalue);
		for(it = ret.first;it!=ret.second;++it)
		{
			tr = it->second;
			if(MSG_IS_REQUEST(sip) && 0 == osip_transaction_matching_request_osip_to_xist_17_2_3(tr,sip))
			{
				tr->Execute(evt);
				return OSIP_SUCCESS;
			}
			else if(MSG_IS_RESPONSE(sip) && 0 == osip_transaction_matching_response_osip_to_xict_17_1_3(tr,sip))
			{
				tr->Execute(evt);
				return OSIP_SUCCESS;
			}
		}
		//Not found any transaction
		if(MSG_IS_REQUEST(sip))
		{
			if(MSG_IS_ACK(sip))
			{
				OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_IST_ACK_RECEIVED, NULL, sip);
			}
			else
			{
				tr = new OSIP_Transaction(evt->GetCtxType());
				tr->Execute(evt);
				return OSIP_SUCCESS;
			}
		}
		else if(MSG_IS_RESPONSE_FOR(sip,"INVITE") && MSG_IS_STATUS_2XX(sip))
		{
			OSIP_Core::sGetInstance()->DoCallback(OSIP_Core::OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN, NULL, sip);
		}
	}

	evt->FreeSipMsg();
	delete evt;
	return OSIP_NOTFOUND;
}

OSIP_Transaction *OSIP_TransManager::GetMatchedTransForCancel(osip_message_t *cancel)
{
	osip_via_t *top_via;
	osip_generic_param_t *b_request;

	if( 0 > osip_message_get_via(cancel,0,&top_via) )
		return NULL;

	if( 0 == osip_via_param_get_byname(top_via, "branch", &b_request) && b_request && b_request->gvalue )
	{
		TransMap *map = GetMap(IST);
		if(map)
		{
			OSIP_Transaction *tr;
			TransMap::iterator it;
			std::pair<TransMap::iterator,TransMap::iterator> ret;

			ret = map->equal_range(b_request->gvalue);
			for(it = ret.first;it!=ret.second;++it)
			{
				tr = it->second;
				if((0 == osip_call_id_match(tr->callid,cancel->call_id)) &&
				   (0 == osip_from_tag_match(tr->from,cancel->from)) &&
				   (tr->cseq && tr->cseq->number && cancel->cseq && cancel->cseq->number) && 
				   (0 == strcmp(tr->cseq->number,cancel->cseq->number)))
				{
					/* check the sent-by values */
					char *b_port = via_get_port(top_via);
					char *b_origport = via_get_port(tr->topvia);
					char *b_host = via_get_host(top_via);
					char *b_orighost = via_get_host(tr->topvia);

					if(b_host && b_orighost && 0 == strcmp(b_orighost, b_host))
					{
						if(b_port == NULL)
							b_port = "5060";
						if(b_origport == NULL)
							b_origport = "5060";

						if(0 == strcmp(b_port,b_origport))
							return tr;
					}
				}
			}
		}
	}

	return NULL;
}




