
#ifndef __STUN_PARSER_H_930KD783Y78JHT790_INCLUDED__
#define __STUN_PARSER_H_930KD783Y78JHT790_INCLUDED__

#include "osip_port.h"

typedef struct tag_stun_header
{
	uint16 type;
	uint16 bodyLen;
	uint8  transID[16];
}stun_header_t;


typedef struct tag_stun_address
{
	uint8	padding;
	uint8	family;
	uint16	port;
	uint32	ipAddr;
}stun_address_t;

typedef struct tag_stun_change_request
{
	uint8  flag[4];
}stun_change_request_t;

typedef struct tag_stun_my_own_record
{
	uint16 port1;
	uint16 port2;
	uint32 addr;
}stun_my_own_record_t;

typedef struct tag_stun_msg
{
	stun_header_t *header;
	osip_list_t	  attr_list;
}stun_msg_t;

typedef struct tag_stun_attr
{
	uint16 type;
	uint16 bodyLen;
	union
	{
		stun_address_t addr;
		stun_change_request_t flag;
		stun_my_own_record_t rec;
	};
}stun_attr_t;

#ifdef __cplusplus
extern "C"{
#endif

int stun_attr_init(stun_attr_t **attr);
int stun_attr_parse(uint8 *buf,stun_attr_t **attr,uint8 **next);
int stun_body_parse(uint8 *buf,uint16 bodyLen,osip_list_t *attr_list);
int stun_header_init(stun_header_t **hdr);
int stun_header_parse(uint8 *buf,stun_header_t **hdr);
int stun_msg_init(stun_msg_t **msg);
int stun_msg_parse(stun_msg_t **msg,uint8 *buf,int bufLen);
void stun_msg_free(stun_msg_t *msg);
int stun_msg_is_response_for(uint8 *buf,uint8 code);
int stun_get_addr(stun_msg_t *stun_msg,uint32 *ip,uint16 *port);
int stun_make_request(uint8 *buf,int bufLen,uint16 type,char *transID);
int stun_get_my_own_port(stun_msg_t *stun_msg,uint32 *addr,uint16* port1,uint16* port2);
int stun_make_request_for_mtcc(uint8 *buf,int bufLen,uint16 type,uint8 *transID,const char *tenant_prefix);
//int stun_make_request_for_record(uint8 *buf,int bufLen,uint16 type,uint8 *transID,uint32 dstAddr,uint16 dstport1,uint16 dstport2,uint32 org_addr,uint16 org_port);
int stun_make_bridge_request(uint8 *buf,int bufLen,uint16 type,char *transID_from,char *transID_to,uint32 addr,uint16 port,uint8 beRecord);
#ifdef __cplusplus
}
#endif

#endif



