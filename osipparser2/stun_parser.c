
#include "stun_parser.h"

#ifdef WIN32
#include "windows.h"
#elif defined LINUX_OS
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

void stun_msg_free(stun_msg_t *msg)
{
	if(msg->header != NULL)
		osip_free(msg->header);
	
	while(!osip_list_eol(&msg->attr_list,0))
	{
		stun_attr_t *attr = osip_list_get(&msg->attr_list,0);
		osip_list_remove(&msg->attr_list,0);
		osip_free(attr);
	}

	osip_free(msg);
}

int stun_attr_init(stun_attr_t **attr)
{
	*attr = osip_malloc(sizeof(stun_attr_t));
	if(*attr == NULL) return -1;

	memset((*attr),0,sizeof(stun_attr_t));

	return 0;
}

int stun_get_addr(stun_msg_t *stun_msg,uint32 *ip,uint16 *port)
{
	int pos = 0;
	osip_list_t *attr_list = &stun_msg->attr_list;

	if( attr_list == NULL ) return -1;

	while(!osip_list_eol(attr_list,pos))
	{
		stun_attr_t *attr = osip_list_get(attr_list,pos);

		if(attr == NULL) break;

		if(attr->type == 0x0001)
		{
			*ip = attr->addr.ipAddr;
			*port = attr->addr.port;
			return 0;
		}
		pos++;
	}

	return -1;
}

int stun_get_my_own_port(stun_msg_t *stun_msg,uint32 *addr,uint16* port1,uint16* port2)
{
	int pos = 0;
	osip_list_t *attr_list = &stun_msg->attr_list;

	if( attr_list == NULL ) return -1;

	while(!osip_list_eol(attr_list,pos))
	{
		stun_attr_t *attr = osip_list_get(attr_list,pos);

		if(attr == NULL) break;

		if(attr->type == 0x000c)
		{
			*port1 = attr->rec.port1;
			*port2 = attr->rec.port2;
			*addr = attr->rec.addr;
			return 0;
		}
		pos++;
	}

	return -1;
}

int stun_attr_parse(uint8 *buf,stun_attr_t **attr,uint8 **next)
{
	char s[20];
	int iRet = -1;
	short i = htons(0x0001);
	char *p = (char*)&i;
	sprintf(s,"%x  %x",p[0],p[1]);

	if( 0 != stun_attr_init(attr) ) return -1;

	(*attr)->type = ntohs(*(short*)buf);

	(*attr)->bodyLen = ntohs(*(short*)(buf+2));

	switch((*attr)->type)
	{
		case 0x0001://MAPPED-ADDRESS
		case 0x0002: //RESPONSE-ADDRESS
		case 0x0004: //SOURCE-ADDRESS
		case 0x0005: //CHANGED-ADDRESS
		{			 
			(*attr)->addr.padding = buf[4];
			(*attr)->addr.family = buf[5];
			(*attr)->addr.port = ntohs(*(short*)(buf+6));
			
			(*attr)->addr.ipAddr = ntohl(*(long*)(buf+8));
			
			iRet = 0;
			break;
		}
		case 0x0003: //CHANGE-REQUEST
		{
			memcpy((*attr)->flag.flag,&(buf[4]),4);
			iRet = 0;
			break;
		}
		case 0x000c: //my own type for record
		{
			(*attr)->rec.port1 = ntohs(*(uint16*)(buf+4));
			(*attr)->rec.port2 = ntohs(*(uint16*)(buf+6));
			(*attr)->rec.addr  = ntohl(*(uint32*)(buf+8));
			iRet = 0;
			break;
		}
		default:
		{
			iRet = -1;
			break;
		}
	}

	*next = buf + 4 + (*attr)->bodyLen;

	if(iRet != 0)
	{
		osip_free(*attr);
	}
	return iRet;
}

int stun_body_parse(uint8 *buf,uint16 bodyLen,osip_list_t *attr_list)
{
	stun_attr_t *attr;

	uint8		*next = buf;

	while( next < buf + bodyLen )
	{
		if(0 == stun_attr_parse(next,&attr,&next))
		{
			osip_list_add(attr_list,attr,-1);
		}
	}

	return 0;
}

int stun_header_init(stun_header_t **hdr)
{
	*hdr = osip_malloc(sizeof(stun_header_t));
	if(*hdr == NULL) return -1;

	memset(*hdr,0,sizeof(stun_header_t));

	return 0;
}

int stun_header_parse(uint8 *buf,stun_header_t **hdr)
{
	if( 0 != stun_header_init(hdr) ) return -1;

	(*hdr)->type = buf[0];
	(*hdr)->type = ((*hdr)->type << 8) + buf[1];

	(*hdr)->bodyLen = ntohs(*(uint16*)(buf+2));

	memcpy((*hdr)->transID,buf+4,16);

	return 0;
}

int stun_msg_init(stun_msg_t **msg)
{
	*msg = osip_malloc(sizeof(stun_msg_t));
	if(*msg == NULL) return -1;

	(*msg)->header = NULL;

	osip_list_init(&(*msg)->attr_list);

	return 0;
}

int stun_msg_parse(stun_msg_t **msg,uint8 *buf,int bufLen)
{

	if( 0 != stun_msg_init(msg) ) return -1;
	
	if( 0 != stun_header_parse(buf,&((*msg)->header)) ) return -1;

	if( (*msg)->header->bodyLen + 20 != bufLen ) return -1;

	if((*msg)->header->bodyLen == 0) return 0;

	if( 0 != stun_body_parse(buf+20,(*msg)->header->bodyLen,&(*msg)->attr_list) ) return -1;

	return 0;
}
int stun_msg_is_response_for(uint8 *buf,uint8 code)
{
	return (buf[0] == 0x01 && buf[1] == code);
}
int stun_make_request(uint8 *buf,int bufLen,uint16 type,char *transID)
{
	int i;
	
	if(bufLen < 28) return -1;

	*(uint16*)buf = htons(type);

	*(uint16*)(buf+2) = htons(8);

	for(i=0;i<16;i++)
	{
		buf[4+i] = transID[i];
	}

	*(uint16*)(buf+20) = htons(0x0003);
	*(uint16*)(buf+22) = htons(0x0004);

	buf[24] = 0x00;
	buf[25] = 0x00;
	buf[26] = 0x00;
	buf[27] = 0x00;
	
	return 0;
}

int stun_make_request_for_mtcc(uint8 *buf,int bufLen,uint16 type,uint8 *transID,const char *tenant_prefix)
{
	int i;
	int len;

	if(bufLen < 38) return -1;
	
	len = strlen(tenant_prefix);
	if(len > 16)
		return -1;

	*(uint16*)buf = htons(type);

	*(uint16*)(buf+2) = htons(8);

	for(i=0;i<16;i++)
	{
		buf[4+i] = transID[i];
	}
	
	memcpy(buf+20,tenant_prefix,len);
	buf[20+len] = 0;

	return 0;
}
/*
int stun_make_request_for_record(uint8 *buf,int bufLen,uint16 type,uint8 *transID,uint32 dstAddr,uint16 dstport1,uint16 dstport2,uint32 org_addr,uint16 org_port)
{
	int i;

	if(bufLen < 38) return -1;

	*(uint16*)buf = htons(type);

	*(uint16*)(buf+2) = htons(8);

	for(i=0;i<16;i++)
	{
		buf[4+i] = transID[i];
	}

	*(uint16*)(buf+20) = htons(0x000c);
	*(uint16*)(buf+22) = htons(0x000e);

	*((uint16*)(buf+24)) = htons(dstport1);
	*((uint16*)(buf+26)) = htons(dstport2);
	*((uint32*)(buf+28)) = dstAddr; //dstAddr is network byte order;

	*((uint32*)(buf+32)) = org_addr;
	*((uint16*)(buf+36)) = htons(org_port);

	return 0;
}
*/
int stun_make_bridge_request(uint8 *buf,int bufLen,uint16 type,char *transID_from,char *transID_to,uint32 addr,uint16 port,uint8 beRecord)
{
	int i;

	if(bufLen < 43)
		return -1;

	*(uint16*)buf = htons(type);

	*(uint16*)(buf+2) = htons(8);

	for(i=0;i<16;i++)
		buf[4+i] = transID_from[i];

	if(transID_to) 
	{
		for(i=0;i<16;i++)
			buf[20+i] = transID_to[i];
	}
	else
	{
		for(i=0;i<16;i++)
			buf[20+i] =0;
	}

	*((uint32*)(buf+36)) = addr;
	*((uint16*)(buf+40)) = htons(port);
	buf[42] = beRecord;
	return 0;
}

