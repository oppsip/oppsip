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

#include "OPPState.h"
#include "osip_message.h"
#include "OPPStateMachine.h"
#include "OPPTimerMonitor.h"

typedef enum osip_fsm_type_t{
	ICT, /**< Invite Client (outgoing) Transaction */
	IST, /**< Invite Server (incoming) Transaction */
	NICT,/**< Non-Invite Client (outgoing) Transaction */
	NIST /**< Non-Invite Server (incoming) Transaction */
} osip_fsm_type_t;

#ifndef DEFAULT_T1
/**
* You can re-define the default value for T1. (T1 is defined in rfcxxxx)
* The default value is 500ms.
*/
#define DEFAULT_T1 500			/* 500 ms */
#endif
#ifndef DEFAULT_T2
/**
* You can re-define the default value for T2. (T2 is defined in rfcxxxx)
* The default value is 4000ms.
*/
#define DEFAULT_T2 4000			/* 4s */
#endif
#ifndef DEFAULT_T4
/**
* You can re-define the default value for T4. (T1 is defined in rfcxxxx)
* The default value is 5000ms.
*/
#define DEFAULT_T4 5000			/* 5s */
#endif

typedef struct osip_ict {
	int timer_a_length;		  /**@internal A=T1, A=2xT1... (unreliable only) */
	struct timeval timer_a_start;
	/**@internal */
	int timer_b_length;		  /**@internal B = 64* T1 */
	struct timeval timer_b_start;
	/**@internal fire when transaction timeouts */
	int timer_d_length;		  /**@internal D >= 32s for unreliable tr (or 0) */
	struct timeval timer_d_start;
	/**@internal should be equal to timer H */
	char *destination;		  /**@internal url used to send requests */
	int port;				  /**@internal port of next hop */
}osip_ict_t;

typedef struct osip_nict {
	int timer_e_length;		  /**@internal A=T1, A=2xT1... (unreliable only) */
	struct timeval timer_e_start;
	/**@internal */
	int timer_f_length;		  /**@internal B = 64* T1 */
	struct timeval timer_f_start;
	/**@internal fire when transaction timeouts */
	int timer_k_length;		  /**@internal K = T4 (else = 0) */
	struct timeval timer_k_start;
	/**@internal */
	char *destination;		  /**@internal url used to send requests */
	int port;				  /**@internal port of next hop */
}osip_nict_t;

typedef struct osip_ist {
	int timer_g_length;	 /**@internal G=MIN(T1*2,T2) for unreliable trans. */
	struct timeval timer_g_start;
	/**@internal 0 when reliable transport is used */
	int timer_h_length;		  /**@internal H = 64* T1 */
	struct timeval timer_h_start;
	/**@internal fire when if no ACK is received */
	int timer_i_length;		  /**@internal I = T4 for unreliable (or 0) */
	struct timeval timer_i_start;
	/**@internal absorb all ACK */
}osip_ist_t;

typedef struct osip_nist {
	int timer_j_length;		   /**@internal J = 64*T1 (else 0) */
	struct timeval timer_j_start;
}osip_nist_t;

typedef struct osip_srv_entry {
	char srv[512];
	int priority;
	int weight;
	int rweight;
	int port;
}osip_srv_entry_t;

typedef struct osip_srv_record {
	char name[512];
	char protocol[64];
	struct osip_srv_entry srventry[10];
}osip_srv_record_t;

typedef enum timeout_flag_t
{
	/* TIMEOUT EVENTS for ICT */
	TIMEOUT_A,	 /**< Timer A */
	TIMEOUT_B,	 /**< Timer B */
	TIMEOUT_D,	 /**< Timer D */

	/* TIMEOUT EVENTS for NICT */
	TIMEOUT_E,	 /**< Timer E */
	TIMEOUT_F,	 /**< Timer F */
	TIMEOUT_K,	 /**< Timer K */

	/* TIMEOUT EVENTS for IST */
	TIMEOUT_G,	 /**< Timer G */
	TIMEOUT_H,	 /**< Timer H */
	TIMEOUT_I,	 /**< Timer I */

	/* TIMEOUT EVENTS for NIST */
	TIMEOUT_J,	 /**< Timer J */
	TIMEOUT_UNKNOWN
}timeout_flag_t;

class OSIP_Event;
class OSIP_Transaction : public OPPState, public OPPTimerAware
{
public:
	explicit OSIP_Transaction(osip_fsm_type_t type);
	~OSIP_Transaction(void);

	typedef enum _state_t 
	{
		STATE_Ready = STATE_Inital,
		STATE_Trying,
		STATE_Proceeding,
		STATE_Completed,
		STATE_Confirmed,
		STATE_Terminated,
		MAX_STATE
	} state_t;

	virtual void OnTimeout(int flag,void *para);
	
	int Execute(OSIP_Event *evt);
	int DeleteTrans();
	void SetUserData(void *p);
	void *GetUserData() const;
	void AddRef();
	const char *GetKey() const
	{
		return m_MapKey;
	};
	osip_fsm_type_t GetCtxType() const
	{
		return ctx_type;
	};
private:
	int ict_context_init(osip_message_t * invite);
	int ict_context_free();
	int ict_set_destination(char *destination, int port);
	int ist_context_init(osip_message_t * invite);
	int ist_context_free();
	int nict_context_init(osip_message_t * request);
	int nict_context_free();
	int nict_set_destination(char *destination, int port);
	int nist_context_init(osip_message_t * invite);
	int nist_context_free();

	int InitRes(osip_message_t *request);
	void FreeRes();

	int SendResponse(osip_message_t * response);
	
public:
	osip_via_t *topvia;		/**< CALL-LEG definition (Top Via) */
	osip_from_t *from;		/**< CALL-LEG definition (From)    */
	osip_to_t *to;			/**< CALL-LEG definition (To)      */
	osip_call_id_t *callid;	/**< CALL-LEG definition (Call-ID) */
	osip_cseq_t *cseq;		/**< CALL-LEG definition (CSeq)    */

	osip_message_t *orig_request;
	osip_message_t *last_response;
	osip_message_t *ack;	   /**< ack request sent           */

	time_t birth_time;		/**< birth date of transaction        */
	time_t completed_time;	/**< end   date of transaction        */

	int in_socket;			/**< Optional socket for incoming message */
	int out_socket;			/**< Optional place for outgoing message */

	void *config;			/**@internal transaction is managed by osip_t  */

	osip_ict_t *ict_context;
	osip_nict_t *nict_context;
	osip_ist_t *ist_context;
	osip_nist_t *nist_context;
	osip_srv_record_t record;

private:
	void *user_instance;	/**< User Defined Pointer. */
	int transactionid;		/**< Internal Transaction Identifier. */
	osip_fsm_type_t ctx_type;
	OPPStateMachine<OSIP_Transaction,OSIP_Event> *m_StateMachine;
	char *m_MapKey;

	long m_refCount;

	friend class OSIP_IctSM;
	friend class OSIP_NictSM;
	friend class OSIP_IstSM;
	friend class OSIP_NistSM;
};

