
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

#include "OSIP_Dialog.h"
#include "osip_parser.h"

OSIP_Dialog::OSIP_Dialog()
:call_id(NULL),local_tag(NULL),remote_tag(NULL),org_cseq_for_invite(-1),remote_uri(NULL),local_uri(NULL),remote_contact_uri(NULL)
{
}

OSIP_Dialog::~OSIP_Dialog()
{
	osip_dialog_free();
}

void OSIP_Dialog::osip_dialog_set_state(dialog_state_t sta)
{
	state = sta;
}

int OSIP_Dialog::osip_dialog_update_route_set_as_uas(osip_message_t * invite)
{
	osip_contact_t *contact;
	int i;

	if (invite == NULL)
		return OSIP_BADPARAMETER;

	if(osip_list_eol(&invite->contacts, 0)) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"missing a contact in invite!\n"));
	}
	else
	{
		if (remote_contact_uri != NULL)
			osip_contact_free(remote_contact_uri);
		
		remote_contact_uri = NULL;
		contact = (osip_contact_t*)osip_list_get(&invite->contacts, 0);
		i = osip_contact_clone(contact, &remote_contact_uri);
		if (i != 0)
			return i;
	}
	return OSIP_SUCCESS;
}

int OSIP_Dialog::osip_dialog_update_osip_cseq_as_uas(osip_message_t * invite)
{
	if (invite == NULL || invite->cseq == NULL || invite->cseq->number == NULL)
		return OSIP_BADPARAMETER;

	remote_cseq = osip_atoi(invite->cseq->number);

	return OSIP_SUCCESS;
}

int OSIP_Dialog::osip_dialog_update_route_set_as_uac(osip_message_t * response)
{
	/* only the remote target URI is updated here... */
	osip_contact_t *contact;
	int i;

	if(response == NULL)
		return OSIP_BADPARAMETER;

	if (osip_list_eol(&response->contacts, 0)) 
	{	/* no contact header in response? */
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"missing a contact in response!\n"));
	} 
	else
	{
		/* I personally think it's a bad idea to keep the old
		value in case the new one is broken... */
		if (remote_contact_uri != NULL)
			osip_contact_free(remote_contact_uri);
		
		remote_contact_uri = NULL;
		contact = (osip_contact_t*)osip_list_get(&response->contacts, 0);
		i = osip_contact_clone(contact, &remote_contact_uri);
		if (i != 0)
			return i;
	}

	if (state == DIALOG_EARLY && osip_list_size(&route_set) > 0) 
	{
		osip_list_special_free(&route_set,(void (*)(void *)) &osip_record_route_free);
		osip_list_init(&route_set);
	}

	if (state == DIALOG_EARLY && osip_list_size(&route_set) == 0) 
	{	/* update the route set */
		int pos = 0;

		while(!osip_list_eol(&response->record_routes, pos)) 
		{
			osip_record_route_t *rr;
			osip_record_route_t *rr2;

			rr = (osip_record_route_t *) osip_list_get(&response->record_routes,pos);
			i = osip_record_route_clone(rr, &rr2);
			if (i != 0)
				return i;
			osip_list_add(&route_set, rr2, 0);
			pos++;
		}
	}

	if (MSG_IS_STATUS_2XX(response))
		state = DIALOG_CONFIRMED;

	return OSIP_SUCCESS;
}

int OSIP_Dialog::osip_dialog_update_tag_as_uac(osip_message_t * response)
{
	osip_generic_param_t *tag;
	int i;

	if (response == NULL || response->to == NULL)
		return OSIP_BADPARAMETER;

	if(remote_tag != NULL) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"This dialog already have a remote tag: it can't be changed!\n"));
		return OSIP_WRONG_STATE;
	}

	i = osip_to_get_tag(response->to, &tag);
	if(i != 0 || tag == NULL || tag->gvalue == NULL) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Remote UA is not compliant: missing a tag in response!\n"));
		remote_tag = NULL;
	} 
	else
	{
		remote_tag = osip_strdup(tag->gvalue);
	}
	return OSIP_SUCCESS;
}

int OSIP_Dialog::osip_dialog_match_as_uac(osip_message_t * answer)
{
	osip_generic_param_t *tag_param_local;
	osip_generic_param_t *tag_param_remote;
	char *tmp;
	int i;

	if (call_id == NULL)
		return OSIP_BADPARAMETER;

	if (answer == NULL || answer->call_id == NULL ||
		answer->from == NULL || answer->to == NULL)
		return OSIP_BADPARAMETER;

	OSIP_TRACE(osip_trace
		(__FILE__, __LINE__, OSIP_WARNING, NULL,
		"Using this method is discouraged. See source code explanations!\n"));
	/*
	When starting a new transaction and when receiving several answers,
	you must be prepared to receive several answers from different sources.
	(because of forking).

	Because some UAs are not compliant (a to tag is missing!), this method
	may match the wrong dialog when a dialog has been created with an empty
	tag in the To header.

	Personnaly, I would recommend to discard 1xx>=101 answers without To tags!
	Just my own feelings.
	*/
	osip_call_id_to_str(answer->call_id, &tmp);
	if (0 != strcmp(call_id, tmp)) 
	{
		osip_free(tmp);
		return OSIP_UNDEFINED_ERROR;
	}
	osip_free(tmp);

	/* for INCOMING RESPONSE:
	To: remote_uri;remote_tag
	From: local_uri;local_tag           <- LOCAL TAG ALWAYS EXIST
	*/
	i = osip_from_get_tag(answer->from, &tag_param_local);
	if (i != 0)
		return OSIP_SYNTAXERROR;
	if(local_tag == NULL)
		/* NOT POSSIBLE BECAUSE I MANAGE REMOTE_TAG AND I ALWAYS ADD IT! */
		return OSIP_SYNTAXERROR;
	if (0 != strcmp(tag_param_local->gvalue,local_tag))
		return OSIP_UNDEFINED_ERROR;

	i = osip_to_get_tag(answer->to, &tag_param_remote);
	if(i != 0 && remote_tag != NULL)	/* no tag in response but tag in dialog */
		return OSIP_SYNTAXERROR;	/* impossible... */

	if(i != 0 && remote_tag == NULL) 
	{	/* no tag in response AND no tag in dialog */
		if(0 == osip_from_compare((osip_from_t *)local_uri,(osip_from_t *) answer->from)
			&& 0 == osip_from_compare(remote_uri, answer->to))
			return OSIP_SUCCESS;
		return OSIP_UNDEFINED_ERROR;
	}

	if(remote_tag == NULL) 
	{	/* tag in response BUT no tag in dialog */
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Remote UA is not compliant: missing a tag in To fields!\n"));

		if (0 == osip_from_compare((osip_from_t *)local_uri,(osip_from_t *) answer->from)
			&& 0 == osip_from_compare(remote_uri, answer->to))
			return OSIP_SUCCESS;
		return OSIP_UNDEFINED_ERROR;
	}

	/* we don't have to compare
	remote_uri with from
	&& local_uri with to.    ----> we have both tag recognized, it's enough..
	*/
	if(0 == strcmp(tag_param_remote->gvalue, remote_tag))
		return OSIP_SUCCESS;

	return OSIP_UNDEFINED_ERROR;
}

int OSIP_Dialog::osip_dialog_match_as_uas(osip_message_t * request)
{
	osip_generic_param_t *tag_param_remote;
	osip_generic_param_t *tag_param_local;
	int i;
	char *tmp;

	if (call_id == NULL)
		return OSIP_BADPARAMETER;
	if (request == NULL || request->call_id == NULL ||
		request->from == NULL || request->to == NULL)
		return OSIP_BADPARAMETER;

	osip_call_id_to_str(request->call_id, &tmp);
	if(0 != strcmp(call_id, tmp)) 
	{
		osip_free(tmp);
		return OSIP_UNDEFINED_ERROR;
	}
	osip_free(tmp);

	/* for INCOMING REQUEST:
	To: local_uri;local_tag           <- LOCAL TAG ALWAYS EXIST
	From: remote_uri;remote_tag
	*/

	if(local_tag == NULL)
		/* NOT POSSIBLE BECAUSE I MANAGE REMOTE_TAG AND I ALWAYS ADD IT! */
		return OSIP_SYNTAXERROR;

	i = osip_to_get_tag(request->to, &tag_param_local);
	if(i != 0)	/* no tag in request but tag in dialog */
		return OSIP_SYNTAXERROR;	

	i = osip_from_get_tag(request->from, &tag_param_remote);
	if(i != 0 && remote_tag != NULL)	/* no tag in request but tag in dialog */
		return OSIP_SYNTAXERROR;	/* impossible... */
	if(i != 0 && remote_tag == NULL) 
	{	/* no tag in request AND no tag in dialog */
		if(0 ==osip_from_compare((osip_from_t *)remote_uri,(osip_from_t *) request->from)
			&& 0 == osip_from_compare(local_uri, request->to))
			return OSIP_SUCCESS;
		return OSIP_UNDEFINED_ERROR;
	}

	if(remote_tag == NULL)
	{	/* tag in response BUT no tag in dialog */
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"Remote UA is not compliant: missing a tag in To feilds!\n"));
		if (0 == osip_from_compare((osip_from_t *)remote_uri,(osip_from_t *) request->from)
			&& 0 == osip_from_compare(local_uri, request->to))
			return OSIP_SUCCESS;
		return OSIP_UNDEFINED_ERROR;
	}
	/* we don't have to compare
	remote_uri with from
	&& local_uri with to.    ----> we have both tag recognized, it's enough..
	*/
	if (0 == strcmp(tag_param_remote->gvalue,remote_tag) && 0 == strcmp(tag_param_local->gvalue,local_tag))
		return OSIP_SUCCESS;

	return OSIP_UNDEFINED_ERROR;
}

int OSIP_Dialog::sCreate(OSIP_Dialog **dialog, osip_message_t * invite, osip_message_t * response,
								osip_from_t *local, osip_to_t *remote,osip_message_t * remote_msg)
{
	int i;
	int pos;
	osip_generic_param_t *tag;

	*dialog = NULL;
	if (response == NULL)
		return OSIP_BADPARAMETER;
	if (response->cseq == NULL || local == NULL || remote == NULL)
		return OSIP_SYNTAXERROR;

	(*dialog) = new OSIP_Dialog();
	if(*dialog == NULL)
		return OSIP_NOMEM;

	osip_list_init(&(*dialog)->route_set);

	(*dialog)->your_instance = NULL;

	if (MSG_IS_STATUS_2XX(response))
		(*dialog)->state = DIALOG_CONFIRMED;
	else						/* 1XX */
		(*dialog)->state = DIALOG_EARLY;

	i = osip_call_id_to_str(response->call_id, &((*dialog)->call_id));
	if(i != 0) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Could not establish dialog!\n"));

		delete (*dialog);
		*dialog = NULL;
		return i;
	}

	i = osip_to_get_tag(local, &tag);
	if(i != 0) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Could not establish dialog!\n"));

		delete (*dialog);
		*dialog = NULL;
		return i;
	}

	(*dialog)->local_tag = osip_strdup(tag->gvalue);

	i = osip_from_get_tag(remote, &tag);
	if(i == 0)
		(*dialog)->remote_tag = osip_strdup(tag->gvalue);

	pos = 0;
	while(!osip_list_eol(&response->record_routes, pos)) 
	{
		osip_record_route_t *rr;
		osip_record_route_t *rr2;

		rr = (osip_record_route_t *) osip_list_get(&response->record_routes, pos);
		i = osip_record_route_clone(rr, &rr2);
		if(i != 0) 
		{
			OSIP_TRACE(osip_trace
				(__FILE__, __LINE__, OSIP_ERROR, NULL,
				"Could not establish dialog!\n"));

			delete (*dialog);
			*dialog = NULL;
			return i;
		}
		if (invite == NULL)
			osip_list_add(&(*dialog)->route_set, rr2, 0);
		else
			osip_list_add(&(*dialog)->route_set, rr2, -1);

		pos++;
	}

	/* local_cseq is set to response->cseq->number for better
	handling of bad UA */
	(*dialog)->local_cseq = osip_atoi(response->cseq->number);

	i = osip_from_clone(remote, &((*dialog)->remote_uri));
	if(i != 0) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Could not establish dialog!\n"));

		delete (*dialog);
		*dialog = NULL;
		return i;
	}

	i = osip_to_clone(local, &((*dialog)->local_uri));
	if(i != 0) 
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Could not establish dialog!\n"));

		delete (*dialog);
		*dialog = NULL;
		return i;
	}

	{
		osip_contact_t *contact;

		if(!osip_list_eol(&remote_msg->contacts, 0)) 
		{
			contact = (osip_contact_t*)osip_list_get(&remote_msg->contacts, 0);
			i = osip_contact_clone(contact, &((*dialog)->remote_contact_uri));
			if(i != 0) 
			{
				OSIP_TRACE(osip_trace
					(__FILE__, __LINE__, OSIP_ERROR, NULL,
					"Could not establish dialog!\n"));

				delete (*dialog);
				*dialog = NULL;
				return i;
			}
		} 
		else
		{
			(*dialog)->remote_contact_uri = NULL;
			OSIP_TRACE(osip_trace
				(__FILE__, __LINE__, OSIP_WARNING, NULL,
				"Remote UA is not compliant: missing a contact in remote message!\n"));
		}
	}

	(*dialog)->secure = -1;		/* non secure */

	return OSIP_SUCCESS;
}

int OSIP_Dialog::sCreateAsUAC(OSIP_Dialog **dialog, osip_message_t *org_request,osip_message_t * response)
{
	int i;

	i = sCreate(dialog,	NULL, response, response->from, response->to, response);
	if (i != 0) 
	{
		*dialog = NULL;
		return i;
	}

	(*dialog)->type = CALLER;
	(*dialog)->remote_cseq = -1;

	if(MSG_IS_INVITE(org_request))
		(*dialog)->org_cseq_for_invite = osip_atoi (org_request->cseq->number);	
	else
		(*dialog)->org_cseq_for_invite = -1;

	return OSIP_SUCCESS;
}

/* SIPIT13 */
int OSIP_Dialog::sCreateAsUacWithRemoteRequest(OSIP_Dialog ** dialog,osip_message_t * next_request,int local_cseq)
{
	int i;

	*dialog = NULL;
	if(next_request->cseq == NULL)
		return OSIP_BADPARAMETER;

	i = sCreate(dialog,next_request,next_request,next_request->to, next_request->from, next_request);
	if(i != 0) 
	{
		*dialog = NULL;
		return i;
	}

	(*dialog)->type = CALLER;
	(*dialog)->state = DIALOG_CONFIRMED;

	(*dialog)->local_cseq = local_cseq;	/* -1 osip_atoi (xxx->cseq->number); */
	(*dialog)->remote_cseq = osip_atoi(next_request->cseq->number);

	return OSIP_SUCCESS;
}

int OSIP_Dialog::sCreateAsUAS(OSIP_Dialog ** dialog, osip_message_t * invite,	osip_message_t * response)
{
	int i;

	*dialog = NULL;
	if (response->cseq == NULL)
		return OSIP_SYNTAXERROR;

	i = sCreate(dialog,invite, response, response->to, response->from, invite);
	if(i != 0) 
	{
		*dialog = NULL;
		return i;
	}

	(*dialog)->type = CALLEE;
	(*dialog)->remote_cseq = osip_atoi(response->cseq->number);

	return OSIP_SUCCESS;
}

void OSIP_Dialog::osip_dialog_free()
{
	osip_contact_free(remote_contact_uri);
	osip_from_free(local_uri);
	osip_to_free(remote_uri);
	osip_list_special_free(&route_set,(void (*)(void *)) &osip_record_route_free);
	osip_free(remote_tag);
	osip_free(local_tag);
	osip_free(call_id);
}

int OSIP_Dialog::dialog_fill_route_set(osip_message_t *request)
{
	/* if the pre-existing route set contains a "lr" (compliance
	with bis-08) then the rquri should contains the remote target
	URI */
	int i;
	int pos=0;
	int last_pos = 0;
	osip_uri_param_t *lr_param;
	osip_route_t *route;
	char *last_route;
	/* AMD bug: fixed 17/06/2002 */

/*	if(type==CALLER)
	{
		pos = osip_list_size(&route_set)-1;
		route = (osip_route_t*)osip_list_get(&route_set, pos);
	}
	else*/
		route = (osip_route_t*)osip_list_get(&route_set, 0);

	osip_uri_uparam_get_byname(route->url, "lr", &lr_param);
	if(lr_param!=NULL) /* the remote target URI is the rquri!! */
	{
		i = osip_uri_clone(remote_contact_uri->url,&(request->req_uri));
		if(i!=0)
			return -1;
		/* "[request] MUST includes a Route header field containing
		the route set values in order." */
		pos=0; /* first element is at index 0 */
		while(!osip_list_eol(&route_set, pos))
		{
			osip_route_t *route2;
			route = (osip_route_t*)osip_list_get(&route_set, pos);
			i = osip_route_clone(route, &route2);
			if(i!=0)
				return -1;
			/*if(type==CALLER)
				osip_list_add(&request->routes, route2, 0);
			else*/
				osip_list_add(&request->routes, route2, -1);
			pos++;
		}
	}
	else
	{
		/* if the first URI of route set does not contain "lr", the rquri
		is set to the first uri of route set */
		i = osip_uri_clone(route->url, &(request->req_uri));
		if(i!=0) return -1;
		/* add the route set */
		/* "The UAC MUST add a route header field containing
		the remainder of the route set values in order. */
		pos=0;
		last_pos = osip_list_size(&route_set) -1;
		while(!osip_list_eol(&route_set, pos)) /* not the first one in the list */
		{
			osip_route_t *route2;
			route = (osip_route_t*)osip_list_get(&route_set, pos);
			/*if(type==CALLER)
			{
				if(pos!=last_pos)
				{
					i = osip_route_clone(route, &route2);
					if(i!=0)
						return -1;
					osip_list_add(&request->routes, route2, 0);
				}
			}
			else*/
			{
				if(pos > 0)
				{      
					i = osip_route_clone(route, &route2);
					if(i!=0) 
						return -1;
					osip_list_add(&request->routes, route2, -1);
				}
			}
			pos++;
		}
		/* The UAC MUST then place the remote target URI into
		the route header field as the last value */
		i = osip_uri_to_str(remote_contact_uri->url, &last_route);
		if(i!=0)
			return -1;
		i = osip_message_set_route(request, last_route);
		osip_free(last_route);
		if(i!=0) 
			return -1;

//		request->rquri_as_destination = 1; // add by lyc 2003/9/16
	}
	return 0;
}

int OSIP_Dialog::generating_request_within_dialog(osip_message_t **dest, char *method_name,
													char *local_host,int local_port,char *transport)
{
	int i;
	osip_message_t *request;

	if(remote_contact_uri == NULL)
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"remote_contact_uri == NULL\n"));
		return -1;
	}

	i = osip_message_init(&request);
	if(i!=0)
		return -1;

	/* prepare the request-line */
	request->sip_method  = osip_strdup(method_name);
	request->sip_version = osip_strdup("SIP/2.0");
	request->status_code = 0;
	request->reason_phrase = NULL;

	/* and the request uri???? */
	if(osip_list_eol(&route_set, 0))
	{
		/* The UAC must put the remote target URI (to field) in the rquri */
		i = osip_uri_clone(remote_contact_uri->url, &(request->req_uri));
		if(i!=0) 
		{
			OSIP_TRACE(osip_trace
				(__FILE__, __LINE__, OSIP_ERROR, NULL,
				"osip_uri_clone error\n"));
			osip_message_free(request);
			return -1;
		}
//		request->rquri_as_destination = 1;
	}
	else
	{
		/* fill the request-uri, and the route headers. */
		dialog_fill_route_set(request);
	}
	/* To and From already contains the proper tag! */
	i = osip_to_clone(remote_uri, &(request->to));
	if(i!=0)
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"osip_to_clone error\n"));

		osip_message_free(request);
		return -1;
	}
	i = osip_from_clone(local_uri, &(request->from));
	if(i!=0)
	{
		OSIP_TRACE(osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"osip_from_clone error\n"));
		osip_message_free(request);
		return -1;
	}

	/* set the cseq and call_id header */
	osip_message_set_call_id(request, call_id);

	if(0==strcmp("ACK", method_name))
	{
		//osip_cseq_t *cseq;
		char *tmp;
		/*if(org_cseq_for_invite < 0)
		{
			OSIP_TRACE(osip_trace
				(__FILE__, __LINE__, OSIP_ERROR, NULL,
				"org_cseq_for_invite=%d\n",org_cseq_for_invite));

			osip_message_free(request);
			return -1;
		}*/
		i = osip_cseq_init(&request->cseq);
		if(i!=0) 
		{
			osip_message_free(request);
			return -1;
		}
		tmp = (char*)osip_malloc(20);
		sprintf(tmp,"%d", local_cseq);
		osip_cseq_set_number(request->cseq, tmp);
		osip_cseq_set_method(request->cseq, osip_strdup(method_name));
	}
	else
	{
		char *tmp;
		i = osip_cseq_init(&request->cseq);
		if(i!=0) 
		{
			osip_message_free(request);
			return -1;
		}
		local_cseq++; /* we should we do that?? */
		tmp = (char*)osip_malloc(20);
		sprintf(tmp,"%d", local_cseq);
		osip_cseq_set_number(request->cseq, tmp);
		osip_cseq_set_method(request->cseq, osip_strdup(method_name));
	}

	osip_message_set_max_forwards(request,"70");

	/* even for ACK for 2xx (ACK within a dialog), the branch ID MUST
	be a new ONE! */
	{
		char tmp[128];
		sprintf(tmp, "SIP/2.0/%s %s:%d;branch=z9hG4bK%u;rport", transport,local_host ,local_port,osip_build_random_number());
		osip_message_set_via(request, tmp);
	}

	//request->outgoing_request_within_dialog = 1;

	*dest = request;
	return 0;
}


