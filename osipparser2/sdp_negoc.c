/*
The oSIP library implements the Session Initiation Protocol (SIP -rfc3261-)
Copyright (C) 2001,2002,2003  Aymeric MOIZARD jack@atosc.org

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


#include "sdp_negoc.h"
//#include "sip_port.h"
#include "time.h"
#include "osip_port.h"

/* this MUST be initialized through a call to sdp_config_init() */
//sdp_config_t *config = NULL;
#define MAX_PTIME_VALUE 30

//sdp_config_t *sdp_config_get()
//{
//	return config;


int sdp_context_init (sdp_context_t ** con)
{
	(*con) = (sdp_context_t *) osip_malloc (sizeof (sdp_context_t));
	if(*con == NULL)
		return -1;

	(*con)->remote = NULL;
	(*con)->local = NULL;
	(*con)->mycontext = NULL;

	return 0;
}

void sdp_context_free (sdp_context_t * con)
{
	if(con == NULL)
		return;

	if(con->remote != NULL)
		sdp_message_free (con->remote);

	if(con->local != NULL)
		sdp_message_free (con->local);

//	if(con->mycontext != NULL)
//		media_set_free(con->mycontext);

	osip_free(con);
}

/* this method is used by end-user application so any pointer can
be associated with this context (usefull to link with your own context */
int sdp_context_set_mycontext (sdp_context_t * con, void *my_instance)
{
	if(con == NULL)
		return -1;
	con->mycontext = my_instance;
	return 0;
}

void *sdp_context_get_mycontext (sdp_context_t * con)
{
	if(con == NULL)
		return NULL;
	return con->mycontext;
}

sdp_message_t *sdp_context_get_local_sdp (sdp_context_t * con)
{
	if(con == NULL)
		return NULL;
	return con->local;
}

int sdp_context_set_local_sdp (sdp_context_t * con, sdp_message_t * sdp)
{
	if(con == NULL)
		return -1;
	con->local = sdp;
	return 0;
}

sdp_message_t *sdp_context_get_remote_sdp (sdp_context_t * con)
{
	if(con == NULL)
		return NULL;
	return con->remote;
}

int sdp_context_set_remote_sdp (sdp_context_t * con, sdp_message_t * sdp)
{
	if (con == NULL)
		return -1;
	con->remote = sdp;
	return 0;
}

int
payload_init (payload_t ** payload)
{
	*payload = (payload_t *)osip_malloc (sizeof (payload_t));
	if (*payload == NULL)
		return -1;
	(*payload)->payload = NULL;
	(*payload)->number_of_port = NULL;
	(*payload)->proto = NULL;
	(*payload)->c_nettype = NULL;
	(*payload)->c_addrtype = NULL;
	(*payload)->c_addr = NULL;
	(*payload)->c_addr_multicast_ttl = NULL;
	(*payload)->c_addr_multicast_int = NULL;
	(*payload)->a_rtpmap = NULL;
	return 0;
}

void
payload_free (payload_t * payload)
{
	if (payload == NULL)
		return;
	osip_free (payload->payload);
	osip_free (payload->number_of_port);
	osip_free (payload->proto);
	osip_free (payload->c_nettype);
	osip_free (payload->c_addrtype);
	osip_free (payload->c_addr);
	osip_free (payload->c_addr_multicast_ttl);
	osip_free (payload->c_addr_multicast_int);
	osip_free (payload->a_rtpmap);

	osip_free(payload);
}

int sdp_config_init (sdp_config_t **cfg)
{
	sdp_config_t *config;

	config = (sdp_config_t *) osip_malloc (sizeof (sdp_config_t));
	if (config == NULL)
		return -1;

	config->o_username = NULL;
	config->o_session_id = NULL;
	config->o_session_version = NULL;
	config->o_nettype = NULL;
	config->o_addrtype = NULL;
	config->o_addr = NULL;

	config->c_nettype = NULL;
	config->c_addrtype = NULL;
	config->c_addr = NULL;

	config->c_nettype6 = NULL;
	config->c_addrtype6 = NULL;
	config->c_addr6 = NULL;

	config->c_addr_multicast_ttl = NULL;
	config->c_addr_multicast_int = NULL;

	/* supported codec for the SIP User Agent */
	osip_list_init(&config->audio_codec);
	osip_list_init(&config->video_codec);
	osip_list_init(&config->other_codec);

	config->fcn_set_info = NULL;
	config->fcn_set_uri = NULL;
	config->fcn_set_emails = NULL;
	config->fcn_set_phones = NULL;
	config->fcn_set_attributes = NULL;
	config->fcn_accept_audio_codec = NULL;
	config->fcn_accept_video_codec = NULL;
	config->fcn_accept_other_codec = NULL;
	config->fcn_get_c_addr = NULL;

	*cfg = config;

	return 0;
}

void
sdp_config_free (sdp_config_t *config)
{
	if (config == NULL)
		return;

	osip_free (config->o_username);
	osip_free (config->o_session_id);
	osip_free (config->o_session_version);
	osip_free (config->o_nettype);

	osip_free (config->o_addrtype);
	osip_free (config->o_addr);

	osip_free (config->c_nettype);
	osip_free (config->c_addrtype);
	osip_free (config->c_addr);

	osip_free (config->c_nettype6);
	osip_free (config->c_addrtype6);
	osip_free (config->c_addr6);

	osip_free (config->c_addr_multicast_ttl);
	osip_free (config->c_addr_multicast_int);

	osip_list_special_free(&config->audio_codec, (void (*)(void *)) &payload_free);

	osip_list_special_free (&config->video_codec, (void (*)(void *)) &payload_free);

	osip_list_special_free (&config->other_codec, (void (*)(void *)) &payload_free);

	/* other are pointer to func, they don't need free() calls */

	/* yes, this is done here... :) */
	osip_free (config);
}

int
sdp_config_set_o_username (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->o_username = tmp;
	return 0;
}

int
sdp_config_set_o_session_id (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->o_session_id = tmp;
	return 0;
}

int
sdp_config_set_o_session_version (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->o_session_version = tmp;
	return 0;
}

int
sdp_config_set_o_nettype (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->o_nettype = tmp;
	return 0;
}

int
sdp_config_set_o_addrtype (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->o_addrtype = tmp;
	return 0;
}

int
sdp_config_set_o_addr (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->o_addr = tmp;
	return 0;
}

int
sdp_config_set_c_nettype (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->c_nettype = tmp;
	return 0;
}

int
sdp_config_set_c_addrtype (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->c_addrtype = tmp;
	return 0;
}

int
sdp_config_set_c_addr (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	if(config->c_addr != NULL)
		osip_free(config->c_addr);

	config->c_addr = tmp;
	return 0;
}

int sdp_config_set_c_nettype6 (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->c_nettype6 = tmp;
	return 0;
}
int sdp_config_set_c_addrtype6 (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->c_addrtype6 = tmp;
	return 0;
}
int sdp_config_set_c_addr6 (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	if(config->c_addr6 != NULL)
	{
		osip_free(config->c_addr6);
	}
	config->c_addr6 = tmp;
	return 0;
}

int
sdp_config_set_c_addr_multicast_ttl (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->c_addr_multicast_ttl = tmp;
	return 0;
}

int
sdp_config_set_c_addr_multicast_int (sdp_config_t * config,char *tmp)
{
	if (config == NULL)
		return -1;
	config->c_addr_multicast_int = tmp;
	return 0;
}

int
sdp_config_add_support_for_audio_codec (sdp_config_t * config,char *payload, char *number_of_port,
										char *proto, char *c_nettype,
										char *c_addrtype, char *c_addr,
										char *c_addr_multicast_ttl,
										char *c_addr_multicast_int,
										char *a_rtpmap)
{
	int i;
	payload_t *my_payload;

	i = payload_init (&my_payload);
	if (i != 0)
		return -1;
	my_payload->payload = payload;
	my_payload->number_of_port = number_of_port;
	my_payload->proto = proto;
	my_payload->c_nettype = c_nettype;
	my_payload->c_addrtype = c_addrtype;
	my_payload->c_addr = c_addr;
	my_payload->c_addr_multicast_ttl = c_addr_multicast_ttl;
	my_payload->c_addr_multicast_int = c_addr_multicast_int;
	my_payload->a_rtpmap = a_rtpmap;
	osip_list_add (&config->audio_codec, my_payload, -1);
	return 0;
}

int
sdp_config_add_support_for_video_codec (sdp_config_t * config,char *payload, char *number_of_port,
										char *proto, char *c_nettype,
										char *c_addrtype, char *c_addr,
										char *c_addr_multicast_ttl,
										char *c_addr_multicast_int,
										char *a_rtpmap)
{
	int i;
	payload_t *my_payload;

	i = payload_init (&my_payload);
	if (i != 0)
		return -1;
	my_payload->payload = payload;
	my_payload->number_of_port = number_of_port;
	my_payload->proto = proto;
	my_payload->c_nettype = c_nettype;
	my_payload->c_addrtype = c_addrtype;
	my_payload->c_addr = c_addr;
	my_payload->c_addr_multicast_ttl = c_addr_multicast_ttl;
	my_payload->c_addr_multicast_int = c_addr_multicast_int;
	my_payload->a_rtpmap = a_rtpmap;
	osip_list_add(&config->video_codec, my_payload, -1);
	return 0;
}

int
sdp_config_add_support_for_other_codec (sdp_config_t * config,char *payload, char *number_of_port,
										char *proto, char *c_nettype,
										char *c_addrtype, char *c_addr,
										char *c_addr_multicast_ttl,
										char *c_addr_multicast_int,
										char *a_rtpmap)
{
	int i;
	payload_t *my_payload;

	i = payload_init (&my_payload);
	if (i != 0)
		return -1;
	my_payload->payload = payload;
	my_payload->number_of_port = number_of_port;
	my_payload->proto = proto;
	my_payload->c_nettype = c_nettype;
	my_payload->c_addrtype = c_addrtype;
	my_payload->c_addr = c_addr;
	my_payload->c_addr_multicast_ttl = c_addr_multicast_ttl;
	my_payload->c_addr_multicast_int = c_addr_multicast_int;
	my_payload->a_rtpmap = a_rtpmap;
	osip_list_add(&config->other_codec, my_payload, -1);
	return 0;
}

int
sdp_config_remove_audio_payloads (sdp_config_t * config)
{
	osip_list_special_free (&config->audio_codec, (void (*)(void *)) &payload_free);
	return 0;
}

int
sdp_config_remove_video_payloads (sdp_config_t * config)
{
	osip_list_special_free (&config->video_codec, (void (*)(void *)) &payload_free);
	return 0;
}

int
sdp_config_remove_other_payloads (sdp_config_t * config)
{
	osip_list_special_free (&config->other_codec, (void (*)(void *)) &payload_free);
	return 0;
}

payload_t *
sdp_config_find_audio_payload (sdp_config_t * config,char *payload)
{
	payload_t *my;
	size_t length;
	int pos = 0;

	if(payload == NULL)
		return NULL;

	length = strlen(payload);

	while(!osip_list_eol(&config->audio_codec, pos))
	{
		my = (payload_t *)osip_list_get(&config->audio_codec, pos);
		if(strlen (my->payload) == length && 0 == strncmp(my->payload, payload, length))
			return my;
		else
			pos++;
	}
	return NULL;
}

payload_t *
sdp_config_find_video_payload (sdp_config_t * config,char *payload)
{
	payload_t *my;
	size_t length;
	int pos = 0;

	if(payload == NULL)
		return NULL;

	length = strlen(payload);

	while(!osip_list_eol(&config->video_codec, pos))
	{
		my = (payload_t *) osip_list_get(&config->video_codec, pos);

		if(strlen (my->payload) == length && 0 == strncmp (my->payload, payload, length))
			return my;
		else
			pos++;
	}
	return NULL;
}

payload_t *
sdp_config_find_other_payload (sdp_config_t * config,char *payload)
{
	payload_t *my;
	size_t length;
	int pos = 0;

	if(payload == NULL)
		return NULL;

	length = strlen(payload);

	while(!osip_list_eol(&config->other_codec, pos))
	{
		my = (payload_t *) osip_list_get(&config->other_codec, pos);

		if(strlen(my->payload) == length && 0 == strncmp (my->payload, payload, length))
			return my;
		else
			pos++;
	}
	return NULL;
}

int
sdp_config_set_fcn_set_info (sdp_config_t * config,int (*fcn) (sdp_context_t *, sdp_message_t *))
{
	if(config == NULL)
		return -1;
	config->fcn_set_info = (int (*)(void *, sdp_message_t *)) fcn;
	return 0;
}

int
sdp_config_set_fcn_set_uri (sdp_config_t * config,int (*fcn) (sdp_context_t *, sdp_message_t *))
{
	if (config == NULL)
		return -1;
	config->fcn_set_uri = (int (*)(void *, sdp_message_t *)) fcn;
	return 0;
}

int
sdp_config_set_fcn_set_emails (sdp_config_t * config,int (*fcn) (sdp_context_t *, sdp_message_t *))
{
	if (config == NULL)
		return -1;
	config->fcn_set_emails = (int (*)(void *, sdp_message_t *)) fcn;
	return 0;
}

int
sdp_config_set_fcn_set_phones (sdp_config_t * config,int (*fcn) (sdp_context_t *, sdp_message_t *))
{
	if (config == NULL)
		return -1;
	config->fcn_set_phones = (int (*)(void *, sdp_message_t *)) fcn;
	return 0;
}

int
sdp_config_set_fcn_set_attributes (sdp_config_t * config,int (*fcn) (sdp_context_t *, sdp_message_t *, int))
{
	if (config == NULL)
		return -1;
	config->fcn_set_attributes = (int (*)(void *, sdp_message_t *, int)) fcn;
	return 0;
}
void sdp_config_set_ptime(sdp_config_t * config,int ptime)
{
	config->ptime = ptime;
}
int
sdp_config_set_fcn_accept_audio_codec (sdp_config_t * config,int (*fcn) (sdp_context_t *, char *,
									   char *, int, char *))
{
	if (config == NULL)
		return -1;
	config->fcn_accept_audio_codec = (int (*)(void *, char *,char *, int, char *)) fcn;
	return 0;
}

int
sdp_config_set_fcn_accept_video_codec (sdp_config_t * config,int (*fcn) (sdp_context_t *, char *,
									   char *, int, char *))
{
	if (config == NULL)
		return -1;
	config->fcn_accept_video_codec = (int (*)(void *, char *,char *, int, char *)) fcn;
	return 0;
}

int
sdp_config_set_fcn_accept_other_codec (sdp_config_t * config,int (*fcn) (sdp_context_t *, char *,
									   char *, char *, char *))
{
	if (config == NULL)
		return -1;
	config->fcn_accept_other_codec = (int (*)(void *, char *,char *, char *, char *)) fcn;
	return 0;
}

int
sdp_config_set_fcn_get_audio_port (sdp_config_t * config,char *(*fcn) (sdp_context_t *, int))
{
	if (config == NULL)
		return -1;
	config->fcn_get_audio_port = (char *(*)(void *, int)) fcn;
	return 0;
}

int
sdp_config_set_fcn_get_video_port (sdp_config_t * config,char *(*fcn) (sdp_context_t *, int))
{
	if (config == NULL)
		return -1;
	config->fcn_get_video_port = (char *(*)(void *, int)) fcn;
	return 0;
}

int
sdp_config_set_fcn_get_other_port (sdp_config_t * config,char *(*fcn) (sdp_context_t *, int))
{
	if (config == NULL)
		return -1;
	config->fcn_get_other_port = (char *(*)(void *, int)) fcn;
	return 0;
}

int sdp_config_set_fcn_get_c_addr(sdp_config_t * config, char *(*fcn)(int ))
{
	if(config == NULL) return -1;

	config->fcn_get_c_addr = (char *(*)(int))fcn;

	return 0;
}

int
sdp_partial_clone (sdp_config_t *config,sdp_context_t *con, sdp_message_t * remote, const char *c_addr ,sdp_message_t **dest)
{
	int i;

	if(c_addr == NULL)
		c_addr = config->c_addr;

	sdp_message_v_version_set(*dest, osip_strdup("0"));

	/* those fields MUST be set */
	sdp_message_o_origin_set(*dest,
		osip_strdup (config->o_username),
		osip_strdup (config->o_session_id),
		osip_strdup (config->o_session_version),
		osip_strdup (config->o_nettype),
		osip_strdup (config->o_addrtype), 
		osip_strdup (config->o_addr));

	sdp_message_s_name_set (*dest, osip_strdup(remote->s_name));

	if (config->fcn_set_info != NULL)
		config->fcn_set_info (con, *dest);
	if (config->fcn_set_uri != NULL)
		config->fcn_set_uri (con, *dest);
	if (config->fcn_set_emails != NULL)
		config->fcn_set_emails (con, *dest);
	if (config->fcn_set_phones != NULL)
		config->fcn_set_phones (con, *dest);

	if (config->c_nettype != NULL)
		sdp_message_c_connection_add (*dest, -1,
		osip_strdup (config->c_nettype),
		osip_strdup (config->c_addrtype),
		osip_strdup (c_addr),
		osip_strdup (config->c_addr_multicast_ttl),
		osip_strdup (config->c_addr_multicast_int));

	if (config->c_nettype6 != NULL)
		sdp_message_c_connection_add(*dest, -1,
		osip_strdup (config->c_nettype6),
		osip_strdup (config->c_addrtype6),
		osip_strdup (config->c_addr6),
		osip_strdup (config->c_addr_multicast_ttl),
		osip_strdup (config->c_addr_multicast_int));

	{				/* offer-answer draft says we must copy the "t=" line */
		char *tmp = sdp_message_t_start_time_get (remote, 0);
		char *tmp2 = sdp_message_t_stop_time_get (remote, 0);

		if(tmp == NULL || tmp2 == NULL)
			return -1;		/* no t line?? */
		i = sdp_message_t_time_descr_add(*dest, osip_strdup(tmp), osip_strdup(tmp2));
		if(i != 0)
			return -1;
	}
	if(config->fcn_set_attributes != NULL)
		config->fcn_set_attributes(con, *dest, -1);
	return 0;
}

//int
//sdp_confirm_ptime(sdp_config_t * config,sdp_context_t *context, sdp_message_t *remote, sdp_message_t ** dest)
//{
//	char *tmp,*ptime;
//	int pos;
//
//	tmp = (char *)osip_malloc(25);
//	if(tmp == NULL) 
//		return -1;
//
//	pos = get_first_media_position_by_name(remote,"audio");
//	if(pos == -1) 
//	{
//		osip_free(tmp);
//		return -1;
//	}
//	ptime = sdp_attribute_get_value_by_field(remote,"ptime",pos);
//
//	if(ptime == NULL || MAX_PTIME_VALUE < osip_atoi(ptime)) 
//		sprintf(tmp,"%d",config->ptime);
//	else
//		strcpy(tmp,ptime);
//
//	pos = get_first_media_position_by_name(*dest,"audio");
//	if(pos == -1) 
//	{
//		osip_free(tmp);
//		return -1;
//	}
//
//	sdp_message_a_attribute_add(*dest,pos,osip_strdup("ptime"),tmp);
//
//	return 0;
//}

int
sdp_confirm_media(sdp_config_t * config,sdp_context_t * context, sdp_message_t *remote, sdp_message_t **dest)
{
	char *payload;
	char *tmp, *tmp2, *tmp3, *tmp4;
	int ret;
	int i;
	int k;
	int audio_qty = 0;		/* accepted audio line: do not accept more than one */
	int video_qty = 0;
	//int audio_payload_qty = 0;

	i = 0;
	while(!sdp_message_endof_media(remote, i))
	{
		tmp = sdp_message_m_media_get(remote, i);
		tmp2 = sdp_message_m_port_get(remote, i);
		tmp3 = sdp_message_m_number_of_port_get(remote, i);
		tmp4 = sdp_message_m_proto_get(remote, i);

		if(tmp == NULL)
			return -1;

		sdp_message_m_media_add(*dest, osip_strdup(tmp), osip_strdup("0"),NULL, osip_strdup(tmp4));
		k = 0;
		if(0 == strncmp (tmp, "audio", 5))
		{
			//audio_payload_qty = sdp_message_m_payload_num(remote,i);
			do
			{
				payload = sdp_message_m_payload_get(remote, i, k);
				if(payload != NULL)
				{
					payload_t *my_payload;

					my_payload = sdp_config_find_audio_payload(config,payload);
					if(my_payload != NULL)	/* payload is supported */
					{
						ret = -1;	/* somtimes, codec can be refused even if supported */
						if(config->fcn_accept_audio_codec != NULL)
							ret = config->fcn_accept_audio_codec(context, tmp2,tmp3, 0,payload);
						if(0 == ret)
						{
							sdp_message_m_payload_add(*dest, i,osip_strdup (payload));
							if(my_payload->a_rtpmap != NULL)
								sdp_message_a_attribute_add (*dest, i,osip_strdup("rtpmap"),osip_strdup(my_payload->a_rtpmap));

							if(my_payload->c_nettype != NULL)
							{
								sdp_media_t *med = osip_list_get(&((*dest)->m_medias), i);

								if(osip_list_eol(&med->c_connections, 0))
									sdp_message_c_connection_add(*dest,
									i,
									osip_strdup (my_payload->c_nettype),
									osip_strdup (my_payload->c_addrtype),
									osip_strdup (my_payload->c_addr),
									osip_strdup (my_payload->c_addr_multicast_ttl),
									osip_strdup (my_payload->c_addr_multicast_int));
							}
						}
					}
				}
				k++;
			}while(payload != NULL);

			if(NULL != sdp_message_m_payload_get (*dest, i, 0))
				audio_qty = 1;
		}
		else if(0 == strncmp (tmp, "video", 5))
		{
			do
			{
				payload = sdp_message_m_payload_get (remote, i, k);
				if(payload != NULL)
				{
					payload_t *my_payload =	sdp_config_find_video_payload (config,payload);
					if(my_payload != NULL)	/* payload is supported */
					{
						ret = -1;
						if(config->fcn_accept_video_codec != NULL)
							ret = config->fcn_accept_video_codec (context, tmp2, tmp3,video_qty, payload);
						if(0 == ret)
						{
							sdp_message_m_payload_add(*dest, i, osip_strdup(payload));
							/* TODO  set the attribute list (rtpmap..) */
							if(my_payload->a_rtpmap != NULL)
								sdp_message_a_attribute_add (*dest, i,
								osip_strdup ("rtpmap"),
								osip_strdup (my_payload->a_rtpmap));
							if(my_payload->c_nettype != NULL)
							{
								sdp_media_t *med = osip_list_get(&(*dest)->m_medias, i);

								if(osip_list_eol(&med->c_connections, 0))
									sdp_message_c_connection_add(*dest, i,
									osip_strdup(my_payload->c_nettype),
									osip_strdup(my_payload->c_addrtype),
									osip_strdup(my_payload->c_addr),
									osip_strdup(my_payload->c_addr_multicast_ttl),
									osip_strdup(my_payload->c_addr_multicast_int));
							}
						}
					}
				}
				k++;
			}while (payload != NULL);
			if(NULL != sdp_message_m_payload_get (*dest, i, 0))
				video_qty = 1;
		}
		else
		{
			do
			{
				payload = sdp_message_m_payload_get(remote, i, k);
				if(payload != NULL)
				{
					payload_t *my_payload =	sdp_config_find_other_payload (config,payload);
					if(my_payload != NULL)	/* payload is supported */
					{
						ret = -1;
						if (config->fcn_accept_other_codec != NULL)
							ret = config->fcn_accept_other_codec(context, tmp, tmp2,tmp3, payload);
						if(0 == ret)
						{
							sdp_message_m_payload_add(*dest, i, osip_strdup(payload));
							/* rtpmap has no meaning here! */
							if(my_payload->c_nettype != NULL)
							{
								sdp_media_t *med = osip_list_get(&((*dest)->m_medias), i);

								if(osip_list_eol(&med->c_connections, 0))
									sdp_message_c_connection_add(*dest, i,
									osip_strdup(my_payload->c_nettype),
									osip_strdup(my_payload->c_addrtype),
									osip_strdup(my_payload->c_addr),
									osip_strdup(my_payload->c_addr_multicast_ttl),
									osip_strdup(my_payload->c_addr_multicast_int));
							}
						}
					}
				}
				k++;
			}while (payload != NULL);
		}
		i++;
	}
	return 0;
}

int
sdp_context_execute_negociation(sdp_config_t * config,sdp_context_t *context,int aPort,int vPort,const char *c_addr)
{
	int m_lines_that_match = 0;
	sdp_message_t *remote;
	sdp_message_t *local;
	int i;

	if(context == NULL)
		return -1;
	remote = context->remote;
	if (remote == NULL)
		return -1;

	i = sdp_message_init(&local);
	if(i != 0)
		return -1;

	if(0 != strncmp (remote->v_version, "0", 1))
	{
		sdp_message_free(local);
		/*      sdp_context->fcn_wrong_version(context); */
		return 406;		/* Not Acceptable */
	}

	i = sdp_partial_clone(config,context, remote, c_addr, &local);
	if (i != 0)
	{
		sdp_message_free(local);
		return -1;
	}
	i = sdp_confirm_media(config,context, remote, &local);
	if(i != 0)
	{
		sdp_message_free(local);
		return i;
	}
	i = 0;
	while(!sdp_message_endof_media(local, i))
	{
		/* this is to refuse each line with no codec that matches! */
		if(NULL == sdp_message_m_payload_get(local, i, 0))
		{
			//sdp_media_t *med = osip_list_get(&(local->m_medias), i);
			char *str = sdp_message_m_payload_get(remote, i, 0);

			sdp_message_m_payload_add(local, i, osip_strdup(str));
			//osip_free(med->m_port);
			//med->m_port = osip_strdup("0");	/* refuse this line */
		}
		else
		{			/* number of "m" lines that match */
			sdp_media_t *med = osip_list_get(&local->m_medias, i);

			m_lines_that_match++;
			osip_free(med->m_port);
			/* AMD: use the correct fcn_get_xxx_port method: */
			if(0 == strcmp(med->m_media, "audio"))
			{
				med->m_port = osip_malloc(32);
				sprintf(med->m_port,"%d",aPort);
				
				/*if (config->fcn_get_audio_port != NULL)
				med->m_port = config->fcn_get_audio_port (context, i);
				else
				med->m_port = sgetcopy ("0");*/
			}
			else if(0 == strcmp(med->m_media, "video"))
			{
				med->m_port = osip_malloc(32);
				sprintf(med->m_port,"%d",vPort);

				/*if (config->fcn_get_video_port != NULL)
				med->m_port = config->fcn_get_video_port (context, i);
				else
				med->m_port = sgetcopy ("0");*/
			}
			else
			{
				if (config->fcn_get_other_port != NULL)
					med->m_port = config->fcn_get_other_port (context, i);
				else
					med->m_port = osip_strdup("0");	/* should never happen */
			}
		}
		i++;
	}
	if(m_lines_that_match > 0)
	{
		context->local = local;
		return 200;
	}
	else
	{
		sdp_message_free(local);
		return 415;
	}
}

int
sdp_build_offer (sdp_config_t * config, sdp_message_t ** sdp, char *audio_port,
				 char *video_port,char *c_addr)
{
	int i;
	int media_line = 0;
	if(config == NULL) return  -1;

	if(audio_port == NULL && video_port == NULL) return -1;

	i = sdp_message_init(sdp);
	if (i != 0)
		return -1;

	sdp_message_v_version_set(*sdp, osip_strdup("0"));

	/* those fields MUST be set */
	sdp_message_o_origin_set(*sdp,
		osip_strdup(config->o_username),
		osip_strdup(config->o_session_id),
		osip_strdup(config->o_session_version),
		osip_strdup(config->o_nettype),
		osip_strdup(config->o_addrtype), 
		osip_strdup(config->o_addr));

	sdp_message_s_name_set(*sdp, osip_strdup("A call"));

	//	if (config->fcn_set_info != NULL)
	//		config->fcn_set_info (con, *sdp);
	//	if (config->fcn_set_uri != NULL)
	//		config->fcn_set_uri (con, *sdp);
	//	if (config->fcn_set_emails != NULL)
	//		config->fcn_set_emails (con, *sdp);
	//	if (config->fcn_set_phones != NULL)
	//		config->fcn_set_phones (con, *sdp);

	if(config->c_nettype != NULL)
	{
		char *c_ip = c_addr ? c_addr : config->c_addr;

		sdp_message_c_connection_add(*sdp, -1,
			osip_strdup(config->c_nettype),
			osip_strdup(config->c_addrtype),
			osip_strdup(c_ip),
			osip_strdup(config->c_addr_multicast_ttl),
			osip_strdup(config->c_addr_multicast_int));
	}

	if(config->c_nettype6 != NULL)
	{
		sdp_message_c_connection_add(*sdp, -1,
			osip_strdup(config->c_nettype6),
			osip_strdup(config->c_addrtype6),
			osip_strdup(config->c_addr6),
			osip_strdup(config->c_addr_multicast_ttl),
			osip_strdup(config->c_addr_multicast_int));
	}

	{				/* offer-answer draft says we must copy the "t=" line */
		//		time_t now = stime (&now);
		char *tmp = osip_malloc(25);
		char *tmp2 = osip_malloc(25);

		sprintf (tmp, "%i", 0);
		sprintf (tmp2, "%i", 0);

		i = sdp_message_t_time_descr_add(*sdp, tmp, tmp2);
		if (i != 0)
			return -1;
	}
	//	if (config->fcn_set_attributes != NULL)
	//		config->fcn_set_attributes (con, *sdp, -1);

	/* add all audio codec */
	if(audio_port != NULL && !osip_list_eol(&config->audio_codec, 0))
	{
		int pos = 0;
		payload_t *my = (payload_t *)osip_list_get(&config->audio_codec, pos);

		/* all media MUST have the same PROTO, PORT. */
		sdp_message_m_media_add(*sdp, osip_strdup("audio"), osip_strdup(audio_port),
			osip_strdup(my->number_of_port), osip_strdup(my->proto));

		while(!osip_list_eol(&config->audio_codec, pos))
		{
			my = (payload_t *)osip_list_get(&config->audio_codec, pos);

			sdp_message_m_payload_add(*sdp, media_line, osip_strdup(my->payload));

			if(my->a_rtpmap != NULL)
				sdp_message_a_attribute_add(*sdp, media_line, osip_strdup("rtpmap"),osip_strdup(my->a_rtpmap));

			pos++;
		}
		/*
		while(!list_eol(config->audio_codec,pos)) 
		{
		bAdd = 1;
		pos++;
		}
		if(bAdd) 
		{
		my = (payload_t *) list_get (config->audio_codec, pos-1);
		if(strcmp(my->payload,"0") != 0 && strcmp(my->payload,"8") != 0 &&
		strcmp(my->payload,"18") != 0 && strcmp(my->payload,"4") != 0)
		{//is 2833 PT
		sdp_m_payload_add (*sdp, media_line, sgetcopy (my->payload));
		if (my->a_rtpmap != NULL)
		sdp_a_attribute_add (*sdp, media_line, sgetcopy ("rtpmap"),
		sgetcopy (my->a_rtpmap));
		}
		}*/

		media_line++;
	}

	/* add all video codec */

	if(video_port != NULL && !osip_list_eol(&config->video_codec, 0))
	{
		int pos = 0;
		payload_t *my = (payload_t *)osip_list_get(&config->video_codec, pos);

		/* all media MUST have the same PROTO, PORT. */
		sdp_message_m_media_add(*sdp, osip_strdup("video"), osip_strdup(video_port),
			osip_strdup(my->number_of_port), osip_strdup(my->proto));

		while(!osip_list_eol(&config->video_codec, pos))
		{
			my = (payload_t *)osip_list_get(&config->video_codec, pos);
			sdp_message_m_payload_add(*sdp, media_line, osip_strdup(my->payload));
			if (my->a_rtpmap != NULL)
				sdp_message_a_attribute_add(*sdp, media_line, osip_strdup("rtpmap"),osip_strdup(my->a_rtpmap));
			pos++;
		}
		media_line++;
	}
	return 0;
}

/* build the SDP packet with only one audio codec and one video codec.
* - Usefull if you don't want to restrict proposal to one codec only -
* - Limitation, only one codec will be proposed
*/
int
__sdp_build_offer (sdp_config_t *config,sdp_context_t * con, sdp_message_t ** sdp, char *audio_port,
				   char *video_port, char *audio_codec, char *video_codec)
{
	int i;
	int media_line = 0;

	i = sdp_message_init(sdp);
	if (i != 0)
		return -1;

	sdp_message_v_version_set (*sdp, osip_strdup("0"));

	/* those fields MUST be set */
	sdp_message_o_origin_set(*sdp,
		osip_strdup(config->o_username),
		osip_strdup(config->o_session_id),
		osip_strdup(config->o_session_version),
		osip_strdup(config->o_nettype),
		osip_strdup(config->o_addrtype), 
		osip_strdup(config->o_addr));

	sdp_message_s_name_set(*sdp, osip_strdup("A call"));

	if (config->fcn_set_info != NULL)
		config->fcn_set_info (con, *sdp);
	if (config->fcn_set_uri != NULL)
		config->fcn_set_uri (con, *sdp);
	if (config->fcn_set_emails != NULL)
		config->fcn_set_emails (con, *sdp);
	if (config->fcn_set_phones != NULL)
		config->fcn_set_phones (con, *sdp);

	if (config->c_nettype != NULL)
		sdp_message_c_connection_add(*sdp, -1,
		osip_strdup(config->c_nettype),
		osip_strdup(config->c_addrtype),
		osip_strdup(config->c_addr),
		osip_strdup(config->c_addr_multicast_ttl),
		osip_strdup(config->c_addr_multicast_int));

	{				/* offer-answer draft says we must copy the "t=" line */
		//    time_t now = stime (&now);
		char *tmp = osip_malloc(25);
		char *tmp2 = osip_malloc(25);

		sprintf (tmp, "%i", 0);
		sprintf (tmp2, "%i", 0);

		i = sdp_message_t_time_descr_add(*sdp, tmp, tmp2);
		if (i != 0)
			return -1;
	}
	if (config->fcn_set_attributes != NULL)
		config->fcn_set_attributes (con, *sdp, -1);

	/* add all audio codec */
	if(audio_codec!=NULL)
	{
		if(!osip_list_eol(&config->audio_codec, 0))
		{
			int pos = 0;
			payload_t *my = (payload_t *)osip_list_get(&config->audio_codec, pos);

			while(!osip_list_eol(&config->audio_codec, pos))
			{
				my = (payload_t *)osip_list_get(&config->audio_codec, pos);
				if(0==strcmp(audio_codec, my->payload))
				{	  
					/* all media MUST have the same PROTO, PORT. */
					sdp_message_m_media_add(*sdp, osip_strdup ("audio"), osip_strdup (audio_port),
						osip_strdup(my->number_of_port), osip_strdup (my->proto));
					sdp_message_m_payload_add(*sdp, media_line, osip_strdup(my->payload));
					if (my->a_rtpmap != NULL)
						sdp_message_a_attribute_add (*sdp, media_line, osip_strdup("rtpmap"),osip_strdup(my->a_rtpmap));
					media_line++;
					break;
				}
				pos++;
			}
		}
	}

	/* add all video codec */
	if(video_codec!=NULL)
	{
		if(!osip_list_eol(&config->video_codec, 0))
		{
			int pos = 0;
			payload_t *my = (payload_t *)osip_list_get(&config->video_codec, pos);

			while(!osip_list_eol(&config->video_codec, pos))
			{
				my = (payload_t *)osip_list_get(&config->video_codec, pos);
				if(0==strcmp(video_codec, my->payload))
				{
					/* all media MUST have the same PROTO, PORT. */
					sdp_message_m_media_add(*sdp, osip_strdup ("video"), osip_strdup (video_port),
						osip_strdup(my->number_of_port), osip_strdup(my->proto));

					sdp_message_m_payload_add(*sdp, media_line, osip_strdup(my->payload));
					if (my->a_rtpmap != NULL)
						sdp_message_a_attribute_add(*sdp, media_line, osip_strdup("rtpmap"),osip_strdup(my->a_rtpmap));
					media_line++;
					break;
				}
				pos++;
			}
		}
	}
	return 0;
}


int sdp_message_put_on_hold (sdp_message_t * sdp)
{
	int pos;
	int pos_media = -1;
	char *rcvsnd;
	int recv_send = -1;

	pos = 0;
	rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
	while (rcvsnd != NULL)
	{
		if(rcvsnd != NULL && 0 == strcmp(rcvsnd, "sendonly"))
		{
			recv_send = 0;
		}
		else if (rcvsnd != NULL && 0 == strcmp (rcvsnd, "recvonly"))
		{
			recv_send = 0;
			sprintf (rcvsnd, "sendonly");
		}
		pos++;
		rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
	}

	pos_media = 0;
	while (!sdp_message_endof_media (sdp, pos_media))
	{
		pos = 0;
		rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
		while (rcvsnd != NULL)
		{
			if(rcvsnd != NULL && 0 == strcmp (rcvsnd, "sendonly"))
			{
				recv_send = 0;
			}
			else if (rcvsnd != NULL && (0 == strcmp (rcvsnd, "recvonly") || 0 == strcmp(rcvsnd,"sendrecv")))
			{
				recv_send = 0;
				sprintf (rcvsnd, "sendonly");
			}
			pos++;
			rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
		}
		pos_media++;
	}

	if (recv_send == -1)
	{
		/* we need to add a global attribute with a feild set to "sendonly" */
		sdp_message_a_attribute_add (sdp, 0, osip_strdup("sendonly"), NULL);
	}

	return 0;
}

int sdp_message_put_off_hold (sdp_message_t * sdp)
{
	int pos;
	int pos_media = -1;
	char *rcvsnd;

	pos = 0;
	rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
	while (rcvsnd != NULL)
	{
		if(0 == strcmp (rcvsnd, "sendonly")	|| 0 == strcmp (rcvsnd, "recvonly"))
		{
			sprintf (rcvsnd, "sendrecv");
		}
		pos++;
		rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
	}

	pos_media = 0;
	while (!sdp_message_endof_media (sdp, pos_media))
	{
		pos = 0;
		rcvsnd = sdp_message_a_att_field_get (sdp, pos_media, pos);
		while (rcvsnd != NULL)
		{
			if(0 == strcmp (rcvsnd, "sendonly") || 0 == strcmp (rcvsnd, "recvonly"))
			{
				sprintf (rcvsnd, "sendrecv");
			}
			pos++;
			rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
		}
		pos_media++;
	}

	return 0;
}





















