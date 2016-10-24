/**
 * ,.   ,   ,.   .   .---.         .       .  .---.                      
 * `|  /|  / ,-. |-. \___  ,-. ,-. | , ,-. |- \___  ,-. ,-. .  , ,-. ,-. 
 *  | / | /  |-' | |     \ | | |   |<  |-' |      \ |-' |   | /  |-' |   
 *  `'  `'   `-' ^-' `---' `-' `-' ' ` `-' `' `---' `-' '   `'   `-' ' 
 * 
 * Copyright 2012 by Alexander Thiemann <mail@agrafix.net>
 *
 * This file is part of WebSocketServer.
 *
 *  WebSocketServer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  WebSocketServer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with WebSocketServer.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef WSSERVER_H
#define	WSSERVER_H

#include <stdlib.h>

#include <string>
#include <sstream>
#include <iostream>

#ifdef linux
#include <netdb.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <fcntl.h>
typedef int SOCKET;
typedef fd_set FD_SET;
#define  INVALID_SOCKET (-1)
#define  SOCKET_ERROR (-1)
#else
#include <winsock2.h>
#endif

#include "./hybi10.h"
#include "./wsHandshake.h"
#include "./wsServerInterface.h"
#include "./wsHookInterface.h"
#include "osip_port.h"
#include <assert.h>

#define WS_MAX_CLIENTS (200)

#define WS_HAND_SHAKE  (0)
#define WS_CONTENT_TEXT (1)
#define WS_CONTENT_BIN  (2)
#define WS_CONTENT_UNKNOWN (3)
#define WS_CONTENT_NOT_COMPLETE (4)
#define WS_CONTROL_CLOSE (5)
#define WS_CONTROL_PING (6)
#define WS_CONTROL_PONG (7)
#define RAW_CONTENT_TEXT (8)

using namespace std;

#define MAXLINELENGTH 1024*5

typedef struct _MsgHead
{
	int m_lLen; 
	char m_szDesType[20];
	char m_szDesName[20];
	char m_szSurType[20];
	char m_szSurName[20];
}MsgHead;

typedef struct _MsgNode 
{
	MsgHead m_head;
	char m_buf[MAXLINELENGTH];
}MsgNode;

class Encoder
{
public:
	virtual int Encode(uint8 opcode,uint8* buf,uint64 bufLen,uint8 **out_buf,int *out_len) = 0;
};

class RawEncoder : public Encoder
{
	int Encode(uint8 opcode,uint8* buf,uint64 bufLen,uint8 **out_buf,int *out_len)
	{
		MsgHead head;
		strcpy(head.m_szDesName,"Agent");
		strcpy(head.m_szDesType,"Agent");
		strcpy(head.m_szSurName,"FlyCcs");
		strcpy(head.m_szSurType,"FlyCcs");
		head.m_lLen = bufLen;
		*out_len = sizeof(head)+bufLen;
		*out_buf = (uint8*)malloc(*out_len);
		memcpy(*out_buf,&head,sizeof(head));
		memcpy((*out_buf)+sizeof(head),buf,bufLen);
		return 0;
	}
};

class wsEncoder : public Encoder
{
public:
	wsEncoder()
	{
	}
	~wsEncoder()
	{
	}

	int Encode(uint8 opcode,uint8* buf,uint64 bufLen,uint8 **out_buf,int *out_len)
	{
		uint8 head[24];

		head[0] = 0x80 | (opcode & 0x0F);

		int payloadOffset = 0;
		
		if(bufLen > 65535)
		{
			head[1] = 127;
			for(int i=0;i<8;i++)
				head[2+i] = (bufLen >> (56-i*8)) & 0x00000000000000ffL;

			payloadOffset = 10;
		}
		else if(bufLen > 125)
		{
			head[1] = 126;
			head[2] = (bufLen >> 8) & 0x00000000000000ffL;
			head[3] = bufLen & 0x00000000000000ffL;

			payloadOffset = 4;
		}
		else
		{
			head[1] = bufLen;
			payloadOffset = 2;
		}

		*out_len = payloadOffset + bufLen;

		*out_buf = (uint8*)malloc(*out_len);

		memcpy(*out_buf,head,payloadOffset);
		memcpy((*out_buf)+payloadOffset,buf,bufLen);

		return 0;
	}
};

class Decoder 
{
public:
	virtual int GetBuffer(uint8 **Buf) = 0;
	virtual int Decode(int recvLen,uint8 **out,int *content_type,int *beMore) = 0;
	virtual void reset() = 0;
};

class RawDecoder : public Decoder
{
public:
	RawDecoder()
	{
		m_frame_len = 40959;
		m_frame = (uint8*)malloc(m_frame_len+1);

		m_dataLen = 0;
	};
	~RawDecoder()
	{
		if(m_frame)
			free(m_frame);
	};
	void reset()
	{
		m_dataLen = 0;
	}

	int GetBuffer(uint8 **Buf)
	{
		*Buf = m_frame + m_dataLen;
		return m_frame_len - m_dataLen;
	};

	virtual int Decode(int recvLen,uint8 **out,int *content_type,int *beMore)
	{
		m_dataLen += recvLen;
		MsgHead *head = (MsgHead*)m_frame;
		if(head->m_lLen > 512 || head->m_lLen < 4)
		{
			printf("invalid client:len=%d\n",head->m_lLen);
			return -1;
		}

		int i = 0;

		for(i=0;i<sizeof(head->m_szSurName);i++)
		{
			if(head->m_szSurName[i] == '\0')
				break;
		}

		if(i >= sizeof(head->m_szSurName))
		{
			printf("invalid client,reason: invalid m_szSurName\n");
			return -1;
		}

		char from[128];
		sprintf(from,"__from:%s",head->m_szSurName);
		
		if(m_dataLen >= (int)sizeof(MsgHead) + head->m_lLen)
		{
			int len = head->m_lLen+strlen(from)+1;

			uint8* bf = (uint8*)malloc(len);
			memcpy(bf,m_frame+sizeof(MsgHead),head->m_lLen);
			strcpy((char*)bf+head->m_lLen,from);
			bf[len-1] = 0;
			m_dataLen -= (int)sizeof(MsgHead) + head->m_lLen;
			printf("m_dataLen=%d\n",m_dataLen);
			if(m_dataLen > 0)
				memcpy(m_frame,m_frame+sizeof(MsgHead) + head->m_lLen,m_dataLen);
			
			*out = bf;
			*content_type = RAW_CONTENT_TEXT;
			if(m_dataLen > (int)sizeof(MsgHead))
				*beMore = 1;
			else
				*beMore = 0;
			return len;
		}
		else
		{
			printf("---m_dataLen=%d\n",m_dataLen);
			*content_type = WS_CONTENT_NOT_COMPLETE;
			*beMore = 0;
			return 0;
		}
	}
private:
	uint8 *m_frame;
	uint64 m_frame_len;
	int   m_dataLen;

};

class wsDecoder : public Decoder
{
public:
	wsDecoder()
	{
		m_frame_len = 40959;
		m_frame = (uint8*)malloc(m_frame_len+1);
		
		m_dataLen = 0;
		m_handshake_completed = 0;
	};
	~wsDecoder()
	{
		if(m_frame)
			free(m_frame);
	};

	void reset()
	{
		std::vector<frame_payload_t*>::iterator it;
		it = m_cache.begin();
		while( it != m_cache.end() )
		{
			free((*it)->payload);
			free(*it);
			it = m_cache.erase(it);
		}
		m_dataLen = 0;
		m_handshake_completed = 0;
	}

	int GetBuffer(uint8 **Buf)
	{
		*Buf = m_frame + m_dataLen;
		return m_frame_len - m_dataLen;
	};

	int Decode(int recvLen,uint8 **out,int *content_type,int *beMore)
	{
		m_dataLen += recvLen;

		unsigned char *data = m_frame;

		if( m_handshake_completed == 0 && data[0] == 'G' && data[1] == 'E' && data[2] == 'T') 
		{
			data[m_dataLen] = 0;

			wsHandshake handshake((char*)data);
			handshake.generateResponse();

			if (!handshake.isSuccess())
			{
				//log("handshake failed", i);
				m_dataLen = 0;
				return -1;
			}

			int r_len = handshake.getResponse().length();
			*out = (uint8*)malloc(r_len);
			memcpy(*out,handshake.getResponse().c_str(),r_len);

			//log("handshake complete", i);
			m_dataLen = 0;
			*content_type = WS_HAND_SHAKE;
			*beMore = 0;
			m_handshake_completed = 1;
			return r_len;
		}
		else if(m_handshake_completed)
		{
			bool fin = data[0]&0x80;
			int opcode = data[0] & 0x0f;

			bool masked = data[1] & 0x80;
			if(!masked)
			{
				m_dataLen = 0;
				return -1;
			}
			uint64 payloadLength = data[1] & 127;

			unsigned char mask[4];
			int payloadOffset;

			if (payloadLength == 126) {
				memcpy(mask,data+4,4);
				payloadOffset = 8;
				payloadLength = data[2]*pow(2,8) + data[3];
				printf("datalen=%d payloadLength=%d %d %d\n",m_dataLen,payloadLength,recvLen,opcode);
			} else if (payloadLength == 127) {
				memcpy(mask,data+10,4);
				payloadOffset = 14;
				for(int i=0;i<8;i++)
					payloadLength += data[2+i]*pow(2,56-i*8);
			} else {
				memcpy(mask,data+2,4);
				payloadOffset = 6;
			}

			if(payloadLength+payloadOffset > m_dataLen)
			{
				*content_type = WS_CONTENT_NOT_COMPLETE;
				*beMore = 0;
				return 0;
			}

			if(opcode == 8)
			{
				m_dataLen -= (payloadLength + payloadOffset);
				if(m_dataLen < 0)
				{
					m_dataLen = 0;
					return -1;
				}
				if(m_dataLen > 0)
					memcpy(m_frame,&m_frame[payloadLength+payloadOffset],m_dataLen);
				*content_type = WS_CONTROL_CLOSE;
				*beMore = 0;
				return 0;
			}
			else if(opcode == 9)
			{
				*content_type = WS_CONTROL_PING;
			}
			else if(opcode == 10)
			{
				m_dataLen -= (payloadLength + payloadOffset);
				if(m_dataLen < 0)
				{
					m_dataLen = 0;
					return -1;
				}

				if(m_dataLen > 0)
					memcpy(m_frame,&m_frame[payloadLength+payloadOffset],m_dataLen);
				*content_type = WS_CONTROL_PONG;
				if(m_dataLen > 5)
					*beMore = 1;
				return 0;
			}
			else if(opcode != 1 && opcode != 2 && opcode != 0)
			{
				m_dataLen -= (payloadLength + payloadOffset);
				if(m_dataLen < 0)
				{
					m_dataLen = 0;
					return -1;
				}
				if(m_dataLen > 0)
					memcpy(m_frame,&m_frame[payloadLength+payloadOffset],m_dataLen);
				*content_type = WS_CONTENT_UNKNOWN;
				*beMore = 0;
				return 0;
			}

			uint8 *buff = (uint8*)malloc(payloadLength);

			for(int i = 0;i < payloadLength;i++)
			{
				unsigned char d = data[i+payloadOffset];
				unsigned char m = mask[i % 4];
				unsigned char unmasked = d ^ m;
				buff[i] = unmasked;
			}

			if(!fin)
			{
				frame_payload_t *pat = (frame_payload_t*)malloc(sizeof(frame_payload_t));
				pat->opcode = opcode;
				pat->payload_len = payloadLength;
				pat->payload = buff;
				m_cache.push_back(pat);

				m_dataLen -= (payloadLength + payloadOffset);
				if(m_dataLen < 0)
				{
					m_dataLen = 0;
					return -1;
				}
				if(m_dataLen > 0)
					memcpy(m_frame,&m_frame[payloadLength+payloadOffset],m_dataLen);

				*content_type = WS_CONTENT_NOT_COMPLETE;
				if(m_dataLen > 5)
					*beMore = 1;
				return 0;
			}
			else
			{
				int total_len = payloadLength;
				std::vector<frame_payload_t*>::iterator it;
				for(it=m_cache.begin();it!=m_cache.end();it++)
					total_len += (*it)->payload_len;

				int len = 0;
				uint8 *bf = (uint8*)malloc(total_len+1);
				if(total_len > payloadLength)
				{
					for(it=m_cache.begin();it!=m_cache.end();it++)
					{
						memcpy(bf+len,(*it)->payload,(*it)->payload_len);
						len += (*it)->payload_len;
						if((*it)->opcode == 1)
							*content_type = WS_CONTENT_TEXT;
						else if((*it)->opcode == 2)
							*content_type = WS_CONTENT_BIN;
					}

					it = m_cache.begin();
					while( it != m_cache.end() )
					{
						free((*it)->payload);
						free(*it);
						it = m_cache.erase(it);
					}
				}
				else
				{
					if(opcode == 1)
						*content_type = WS_CONTENT_TEXT;
					else if(opcode == 2)
						*content_type = WS_CONTENT_BIN;
				}

				memcpy(bf+len,buff,payloadLength);
				bf[len+payloadLength] = '\0';

				free(buff);

				*out = bf;

				m_dataLen -= (payloadLength + payloadOffset);
				if(m_dataLen < 0)
				{
					m_dataLen = 0;
					return -1;
				}
				if(m_dataLen > 0)
					memcpy(m_frame,&m_frame[payloadLength+payloadOffset],m_dataLen);

				if(m_dataLen > 5)
					*beMore = 1;

				return total_len;
			}
		}
		else
		{
			m_dataLen = 0;
			return -1;
		}
	};
private:
	uint8 *m_frame;
	uint64 m_frame_len;
	int   m_dataLen;
	int    m_handshake_completed;

	typedef struct {
		int opcode;
		int payload_len;
		uint8 *payload;
	}frame_payload_t;
	std::vector<frame_payload_t*> m_cache;
};

class wsServer : public wsServerInterface {
public:
    wsServer(wsHookInterface *hook);
    virtual ~wsServer();
    
    virtual void run();
	virtual int  Init(int port);
    virtual void sendTo(int i, string message);
    virtual void sendBroadcast(string message);
	virtual void disconnect(int i);
	
    
private:
	int m_ListenPort;
    wsHookInterface *_hook;

	typedef struct {
		SOCKET sock;
		Decoder *decoder;
		Encoder *encoder;
	} CLIENT_DATA;
    
    SOCKET _sListen;
    CLIENT_DATA _sClients[WS_MAX_CLIENTS];
    
    FD_SET _fdSet;
    
    // init functions
	
    void initClients();
    bool winSock(int action);
    bool openSocket();
    bool setNonBlocking();
    bool bindTo(int port);
    bool startListen();
    
    // mainloop functions
    void acceptClients();
    void handleClients();
    void clientRecv(int i);
   // void handleRequest(int i, unsigned char *request,int len);
    
    // client functions
    void disconnectClient(int i);
    void disconnectClient(int, string reason);
    void sendRaw(int i, uint8* msg ,int len);
    //void sendHybi10(int i, string type, string message, bool masked);
    
    // log & error functions
    void error(string message);
    void log(string message);
    void log(string message, int clientId);
	void do_received_data(int i,int recvLen);
};

#endif	/* WSSERVER_H */

