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

#ifdef LINUX_OS
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

typedef struct sockaddr_in SOCKADDR_IN;
typedef fd_set  FD_SET;
typedef struct sockaddr SOCKADDR;
typedef struct hostent  HOSTENT;

#define closesocket(X) close(X)
#endif

class OSIP_Transaction;
class OSIP_Dialog;
class OSIP_Core
{
public:
	OSIP_Core(void);
	~OSIP_Core(void);

	typedef void (*osip_message_cb_t)(OSIP_Transaction *,osip_message_t *);

	int Init();

	int DoTimerCheck();

	int SetCallback(int type, osip_message_cb_t cb);

	int DoSipMsg(struct sockaddr_in *from_addr,char *buf,int len);

	int SendMsg(osip_message_t *sip, char *host, int port, int transport_type, int out_socket);

	void DoCallback(int type,OSIP_Transaction *tr,osip_message_t *sip);

	static OSIP_Core *sGetInstance();

	typedef enum osip_message_callback_type 
	{
		OSIP_ICT_INVITE_SENT = 0,			/**< INVITE MESSAGE SENT */
		OSIP_ICT_INVITE_SENT_AGAIN,			/**< INVITE MESSAGE RETRANSMITTED */
		OSIP_ICT_ACK_SENT,					/**< ACK MESSAGE SENT */
		OSIP_ICT_ACK_SENT_AGAIN,			/**< ACK MESSAGE RETRANSMITTED */
		OSIP_ICT_STATUS_1XX_RECEIVED,		/**< 1XX FOR INVITE RECEIVED */
		OSIP_ICT_STATUS_2XX_RECEIVED,		/**< 2XX FOR INVITE RECEIVED */
		OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN,	/**< 2XX FOR INVITE RECEIVED AGAIN */
		OSIP_ICT_STATUS_3XX_RECEIVED,		/**< 3XX FOR INVITE RECEIVED */
		OSIP_ICT_STATUS_4XX_RECEIVED,		/**< 4XX FOR INVITE RECEIVED */
		OSIP_ICT_STATUS_5XX_RECEIVED,		/**< 5XX FOR INVITE RECEIVED */
		OSIP_ICT_STATUS_6XX_RECEIVED,		/**< 6XX FOR INVITE RECEIVED */
		OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN,
		/**< RESPONSE RECEIVED AGAIN */

		OSIP_IST_INVITE_RECEIVED,			/**< INVITE MESSAGE RECEIVED */
		OSIP_IST_INVITE_RECEIVED_AGAIN,		/**< INVITE MESSAGE RECEIVED AGAN */
		OSIP_IST_ACK_RECEIVED,				/**< ACK MESSAGE RECEIVED */
		OSIP_IST_ACK_RECEIVED_AGAIN,		/**< ACK MESSAGE RECEIVED AGAIN */
		OSIP_IST_STATUS_1XX_SENT,			/**< 1XX FOR INVITE SENT */
		OSIP_IST_STATUS_2XX_SENT,			/**< 2XX FOR INVITE SENT */
		OSIP_IST_STATUS_2XX_SENT_AGAIN,		/**< 2XX FOR INVITE RETRANSMITTED */
		OSIP_IST_STATUS_3XX_SENT,			/**< 3XX FOR INVITE SENT */
		OSIP_IST_STATUS_4XX_SENT,			/**< 4XX FOR INVITE SENT */
		OSIP_IST_STATUS_5XX_SENT,			/**< 5XX FOR INVITE SENT */
		OSIP_IST_STATUS_6XX_SENT,			/**< 6XX FOR INVITE SENT */
		OSIP_IST_STATUS_3456XX_SENT_AGAIN,	/**< RESPONSE RETRANSMITTED */

		OSIP_NICT_REGISTER_SENT,			/**< REGISTER MESSAGE SENT */
		OSIP_NICT_BYE_SENT,					/**< BYE MESSAGE SENT */
		OSIP_NICT_OPTIONS_SENT,				/**< OPTIONS MESSAGE SENT */
		OSIP_NICT_INFO_SENT,				/**< INFO MESSAGE SENT */
		OSIP_NICT_CANCEL_SENT,				/**< CANCEL MESSAGE SENT */
		OSIP_NICT_NOTIFY_SENT,				/**< NOTIFY MESSAGE SENT */
		OSIP_NICT_SUBSCRIBE_SENT,			/**< SUBSCRIBE MESSAGE SENT */
		OSIP_NICT_UNKNOWN_REQUEST_SENT,		/**< UNKNOWN REQUEST MESSAGE SENT */
		OSIP_NICT_REQUEST_SENT_AGAIN,		/**< REQUEST MESSAGE RETRANMITTED */
		OSIP_NICT_STATUS_1XX_RECEIVED,		/**< 1XX FOR MESSAGE RECEIVED */
		OSIP_NICT_STATUS_2XX_RECEIVED,		/**< 2XX FOR MESSAGE RECEIVED */
		OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN,/**< 2XX FOR MESSAGE RECEIVED AGAIN */
		OSIP_NICT_STATUS_3XX_RECEIVED,		/**< 3XX FOR MESSAGE RECEIVED */
		OSIP_NICT_STATUS_4XX_RECEIVED,		/**< 4XX FOR MESSAGE RECEIVED */
		OSIP_NICT_STATUS_5XX_RECEIVED,		/**< 5XX FOR MESSAGE RECEIVED */
		OSIP_NICT_STATUS_6XX_RECEIVED,		/**< 6XX FOR MESSAGE RECEIVED */
		OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN,
		/**< RESPONSE RECEIVED AGAIN */

		OSIP_NIST_REGISTER_RECEIVED,		/**< REGISTER RECEIVED */
		OSIP_NIST_BYE_RECEIVED,				/**< BYE RECEIVED */
		OSIP_NIST_OPTIONS_RECEIVED,			/**< OPTIONS RECEIVED */
		OSIP_NIST_INFO_RECEIVED,			/**< INFO RECEIVED */
		OSIP_NIST_CANCEL_RECEIVED,			/**< CANCEL RECEIVED */
		OSIP_NIST_NOTIFY_RECEIVED,			/**< NOTIFY RECEIVED */
		OSIP_NIST_SUBSCRIBE_RECEIVED,		/**< SUBSCRIBE RECEIVED */

		OSIP_NIST_UNKNOWN_REQUEST_RECEIVED,	/**< UNKNWON REQUEST RECEIVED */
		OSIP_NIST_REQUEST_RECEIVED_AGAIN,	/**< UNKNWON REQUEST RECEIVED AGAIN */
		OSIP_NIST_STATUS_1XX_SENT,			/**< 1XX FOR MESSAGE SENT */
		OSIP_NIST_STATUS_2XX_SENT,			/**< 2XX FOR MESSAGE SENT */
		OSIP_NIST_STATUS_2XX_SENT_AGAIN,	/**< 2XX FOR MESSAGE RETRANSMITTED */
		OSIP_NIST_STATUS_3XX_SENT,			/**< 3XX FOR MESSAGE SENT */
		OSIP_NIST_STATUS_4XX_SENT,			/**< 4XX FOR MESSAGE SENT */
		OSIP_NIST_STATUS_5XX_SENT,			/**< 5XX FOR MESSAGE SENT */
		OSIP_NIST_STATUS_6XX_SENT,			/**< 6XX FOR MESSAGE SENT */
		OSIP_NIST_STATUS_3456XX_SENT_AGAIN,	/**< RESPONSE RETRANSMITTED */

		OSIP_ICT_STATUS_TIMEOUT,			/**< TIMER B EXPIRATION: NO REMOTE ANSWER  */
		OSIP_NICT_STATUS_TIMEOUT,			/**< TIMER F EXPIRATION: NO REMOTE ANSWER  */

		OSIP_MESSAGE_CALLBACK_COUNT			/**< END OF ENUM */
	} osip_message_callback_type_t;

private:
	
	osip_message_cb_t msg_callbacks[OSIP_MESSAGE_CALLBACK_COUNT];

	static OSIP_Core m_sInst;
};

enum osip_transport_type_t
{
	OSIP_UDP,
	OSIP_TCP
};

int complete_answer_that_establish_a_dialog(char *contact,osip_message_t *response, osip_message_t *request);
/**
* Search in a SIP response the destination where the message
* should be sent.
* @param response the message to work on.
* @param address a pointer to receive the allocated host address.
* @param portnum a pointer to receive the host port.
*/
void osip_response_get_destination(osip_message_t * response,char **address, int *portnum);
int generating_request_out_of_dialog(osip_message_t **dest, char *method_name,osip_uri_t *req_uri, char *from,char *to, char *local_host,int local_port,char *contact,char *transport,char *outbound_ip,int outbound_port);
int generating_response_default(osip_message_t **dest, OSIP_Dialog *dialog, int status, osip_message_t *request);
int generating_cancel(osip_message_t **dest, osip_message_t *request_cancelled);

