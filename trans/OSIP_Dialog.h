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

#include "osip_message.h"

class OSIP_Dialog
{
public:
	~OSIP_Dialog();

	typedef enum dialog_state
	{
		DIALOG_EARLY,
		DIALOG_CONFIRMED,
		DIALOG_CLOSE
	}dialog_state_t;

	typedef enum _osip_dialog_type_t 
	{
		CALLER,
		CALLEE
	}osip_dialog_type_t;

	void osip_dialog_set_state(dialog_state_t sta);
	int osip_dialog_update_route_set_as_uas(osip_message_t * invite);
	int osip_dialog_update_osip_cseq_as_uas(osip_message_t * invite);
	int osip_dialog_update_route_set_as_uac(osip_message_t * response);
	int osip_dialog_update_tag_as_uac(osip_message_t * response);
	int osip_dialog_match_as_uac(osip_message_t * answer);
	int osip_dialog_match_as_uas(osip_message_t * request);
	int generating_request_within_dialog(osip_message_t **dest, char *method_name, char *local_host,int local_port,char *transport);
	static int sCreateAsUAC(OSIP_Dialog ** dialog, osip_message_t *org_request,osip_message_t * response);
	static int sCreateAsUacWithRemoteRequest(OSIP_Dialog ** dialog,osip_message_t * next_request,int local_cseq);
	static int sCreateAsUAS(OSIP_Dialog ** dialog, osip_message_t * invite, osip_message_t * response);
	void osip_dialog_free();

	const char *GetLocalTag()
	{
		return local_tag;
	}

	const char *GetRemoteTag()
	{
		return remote_tag;
	}

	const char *GetCallId()
	{
		return call_id;
	}

	osip_dialog_type_t GetType()
	{
		return type;
	}

private:

	OSIP_Dialog();
	static int sCreate(OSIP_Dialog **dialog, osip_message_t * invite, osip_message_t * response, osip_from_t *local, osip_to_t *remote,osip_message_t * remote_msg);
	int dialog_fill_route_set(osip_message_t *request);
	char *call_id;					 /**< Call-ID*/
	char *local_tag;				 /**< local tag */
	char *remote_tag;				 /**< remote tag */
	osip_list_t route_set;			/**< route set */
	int local_cseq;					 /**< last local cseq */
	int remote_cseq;				 /**< last remote cseq*/
	int org_cseq_for_invite;
	osip_to_t *remote_uri;			 /**< remote_uri */
	osip_from_t *local_uri;			 /**< local_uri */
	osip_contact_t *remote_contact_uri;	/**< remote contact_uri */

	int secure;						 /**< use secure transport layer */

	osip_dialog_type_t type;		 /**< type of dialog (CALLEE or CALLER) */
	dialog_state_t state;					 /**< DIALOG_EARLY || DIALOG_CONFIRMED || DIALOG_CLOSED */
	void *your_instance;			 /**< for application data reference */
};


