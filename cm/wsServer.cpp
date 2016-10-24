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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "wsServer.h"
#include "OPPApi.h"
#include "AppTimer.h"
#include <netinet/tcp.h>

/**
 * init the WebSocketServer
 */
wsServer::wsServer(wsHookInterface *hook) { 
    
    // store hook
    _hook = hook;

    // call server startup hook
    _hook->hServerStartup(this);
}

int wsServer::Init(int port)
{
	m_ListenPort = port;

	if(port > 0)
	{
    initClients();

    // wsaStartup
    if (!winSock(1)) {
        error("WSAStartup failed.");
    }

    // open socket
    if (!openSocket()) {
        error("Error opening the socket.");
    }

    // set it to nonblocking
    if (!setNonBlocking()) {
        error("Failed to set socket to nonblocking-io mode");
    }

    // bind to port 8085
		if (!bindTo(port)) {
			error("Failed to bind to port");
    }

    // listen
    if (!startListen()) {
			error("Failed to listen on port");
		}

		log("Server init succ."); 
    }
    return 0;
}
/**
 * stop server
 */
wsServer::~wsServer() { 
    // close socket
    closesocket(_sListen);
    
    // cleanup
    winSock(2);
    
    // server closed
    _hook->hServerShutdown();
}

/**
 * main server loop, is blocking and will block until the server crashes...
 */
void wsServer::run() {
	if(m_ListenPort > 0)
	{
	struct timeval tm;
    while (true) 
	{	
        FD_ZERO(&_fdSet);
        FD_SET(_sListen, &_fdSet);

		int max_fd = _sListen;

        // add valid sockets to fdSet
        for (int i = 0; i < WS_MAX_CLIENTS; i++) 
		{
            if(_sClients[i].sock != INVALID_SOCKET) 
			{
                FD_SET(_sClients[i].sock, &_fdSet);
				if(_sClients[i].sock > max_fd)
					max_fd = _sClients[i].sock;
            }
        }
		tm.tv_sec = 0;
		tm.tv_usec = 1000;
		int ret = select(max_fd+1,&_fdSet,NULL,NULL,&tm);
        if ( ret < 0) 
		{
            error("Error using select()");
			continue;
		}
		else if(ret == 0)
		{	//time out
			while(AppTimer::sGetInstance()->CheckExpires())
			{
			};
			API_DoDispatch();
			continue;
		}
		else if (FD_ISSET(_sListen, &_fdSet))
		{	// accept new clients
            acceptClients();
			continue;
        }
		else
		{
			handleClients();
		}        
    }
	}
	else //only deal with sip module
	{
		while(1)
		{
			while(AppTimer::sGetInstance()->CheckExpires())
			{
			};
			API_DoDispatch();
			usleep(1000);
		}
	}
}

/**
 * accept new clients
 */
void wsServer::acceptClients() 
{
    for (int i = 0; i < WS_MAX_CLIENTS; i++) 
	{
        if (_sClients[i].sock == INVALID_SOCKET) 
		{
            _sClients[i].sock = accept(_sListen, NULL, NULL);
            
            /* Detect dead connections */
            int   keepalive = 1 ;
            setsockopt(_sClients[i].sock, SOL_SOCKET, SO_KEEPALIVE,
                  (char *)&keepalive,
                  sizeof(keepalive)) ;
            
			int                 keepIdle = 50;
			int                 keepInterval = 50;
			int                 keepCount = 7;

			setsockopt(_sClients[i].sock, SOL_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle));
			setsockopt(_sClients[i].sock, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
			setsockopt(_sClients[i].sock,SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));
			
            log("new connection", i);
            break;
        }
    }
}

/**
 * handle clients
 */
void wsServer::handleClients() {
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        // skip empty slots and clients that didn't send anything
        if (_sClients[i].sock == INVALID_SOCKET) {
            continue;
        }
        
        if (FD_ISSET(_sClients[i].sock, &_fdSet)) 
		{
            //FD_CLR(_sClients[i].sock, &_fdSet);
            clientRecv(i);
        }
    }
}

void wsServer::do_received_data(int i,int recvLen)
{
	int  bMore = 0;
	do 
	{
		uint8 *out_buf = NULL;
		int    content_type;
		bMore = 0;

		int out_len = _sClients[i].decoder->Decode(recvLen,&out_buf,&content_type,&bMore);
		recvLen = 0;
		if(out_len > 0 && content_type == WS_CONTENT_TEXT) {
			_hook->hClientMessage(i,out_buf,out_len);
		}else if(out_len > 0 && content_type == RAW_CONTENT_TEXT){
			_hook->hClientRawMessage(i,out_buf,out_len);
		}else if(out_len > 0 && content_type == WS_CONTENT_BIN) {
			_hook->hClientBinary(i,out_buf,out_len);
		} else if(out_len > 0 && content_type == WS_HAND_SHAKE) {
			sendRaw(i,out_buf,out_len);
			_hook->hClientConnect(i);
		} else if(out_len > 0 && content_type == WS_CONTROL_PING) {
			uint8 *buf;
			int len;
			_sClients[i].encoder->Encode(10,out_buf,out_len,&buf,&len);
			//wsEncoder::Encode(10,out_buf,out_len,&buf,&len);
			sendRaw(i,buf,len);
			//sendHybi10(i, "pong", out_buf, false);
		} else if(out_len == 0 && content_type == WS_CONTROL_CLOSE) {
			disconnectClient(i,"peer close");
		} else if(out_len == 0 && (content_type == WS_CONTROL_PONG || content_type == WS_CONTENT_NOT_COMPLETE)) {
			//do nothing;
		} else if(out_len == 0 && content_type == WS_CONTENT_UNKNOWN) {
			disconnectClient(i,"unknown opcode");
		} else if(out_len < 0) {
			disconnectClient(i);
		}

		if(out_buf)
			free(out_buf);

	} while (bMore);
}

/**
 * handle signle client
 * @param i
 */
void wsServer::clientRecv(int i) 
{
	uint8 *buf;
	int recvLen;
	if(_sClients[i].decoder == NULL)
	{
		uint8 data[1024];
		recvLen = recv(_sClients[i].sock,data,sizeof(data),0);
		if(recvLen <= 0)
		{
			cout << "Errno: " << errno << "\n";
			cout << "Client " << i << " quit. Status: 0\n";
			disconnectClient(i, "Client Status is 0");
		}
		else
		{
			if(data[0] == 'G' && data[1] == 'E' && data[2] == 'T')
			{
				_sClients[i].decoder = new wsDecoder();
				_sClients[i].encoder = new wsEncoder();
			}
			else
			{
				_sClients[i].decoder = new RawDecoder();
				_sClients[i].encoder = new RawEncoder();
			}
			int bufLen = _sClients[i].decoder->GetBuffer(&buf);
			if(bufLen < recvLen)
			{
				disconnectClient(i, "Client Status is 0");
			}
			else
			{
				memcpy(buf,data,recvLen);
				do_received_data(i,recvLen);
			}			
		}
	}
	else
	{
		int bufLen = _sClients[i].decoder->GetBuffer(&buf);
		assert(bufLen > 0);
		recvLen = recv(_sClients[i].sock,buf,bufLen,0);
		if (recvLen <= 0) 
		{
			cout << "Errno: " << errno << "\n";
			cout << "Client " << i << " quit. Status: 0\n";
			disconnectClient(i, "Client Status is 0");
		}
		else 
		{
			printf("socket recv:%d\n",recvLen);
			do_received_data(i,recvLen);
		}
	}
	
}

/**
 * broadcast message to all connected clients
 * @param message
 */
void wsServer::sendBroadcast(string message) {
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (_sClients[i].sock == INVALID_SOCKET) {
            continue;
        }
        
        sendTo(i, message);
    }
}

/**
 * send hyb10-encoded message to client
 * @param i
 * @param message
 */
void wsServer::sendTo(int i, string message) {
	
	uint8 *out_buf = NULL;
	int    out_len;
	if(i<0 || i>=WS_MAX_CLIENTS)
		return;

	if(_sClients[i].encoder)
	{
		printf("send to:%s\n",message.c_str());

		_sClients[i].encoder->Encode(1,(uint8*)message.c_str(),message.length(),&out_buf,&out_len);

		sendRaw(i,out_buf,out_len);

		if(out_buf)
			free(out_buf);
	}
}

/**
 * send data in hybi10 format
 * @param i
 * @param type
 * @param message
 * @param masked
 */
/*
void wsServer::sendHybi10(int i, string type, string message, bool masked) {
    hybi10::request response;
    response.type = type;
    response.payload = message;
    
	std::string s = hybi10::encode(response, masked);
    sendRaw(i,(uint8*)s.c_str(),s.length());
}*/


/**
 * send a raw tcp packet to a client
 * @param i
 * @param message
 */
void wsServer::sendRaw(int i,uint8 *msg,int len) {
    
    if (i<0 || i>=WS_MAX_CLIENTS || _sClients[i].sock == INVALID_SOCKET)
        return;

    send(_sClients[i].sock, msg, len, 0);
}

void wsServer::disconnect(int i){
	disconnectClient(i);
}
/**
 * disconnect clients
 * @param i
 */
void wsServer::disconnectClient(int i) {
    disconnectClient(i, "-");
    /*log("disconnected", i);
    
    if (_sClients[i] != INVALID_SOCKET) {
        closesocket(_sClients[i]);
        _sClients[i] = INVALID_SOCKET;
    }
    
    _hook->hClientDisconnect(i);*/
}

/**
 * disconnect a client and indicate reason
 * @param i
 * @param reason
 */
void wsServer::disconnectClient(int i, string reason) {
    log("disconnected. Reason: " + reason, i);
    
    if (_sClients[i].sock != INVALID_SOCKET) {
        closesocket(_sClients[i].sock);
        _sClients[i].sock = INVALID_SOCKET;
		if(_sClients[i].decoder) {
			_sClients[i].decoder->reset();
			delete _sClients[i].decoder;
			_sClients[i].decoder = NULL;
		}
		if(_sClients[i].encoder) {
			delete _sClients[i].encoder;
			_sClients[i].encoder = NULL;
		}
    }

    _hook->hClientDisconnect(i);
}

/**
 * start to listen for data
 * @return false on failure, true on success
 */
bool wsServer::startListen() {
    if(listen(_sListen,10) != 0)
    {
        return false;
    }
    
    return true;
}

/**
 * bind socket to a port
 * @param port
 * @return false on failure, true on success
 */
bool wsServer::bindTo(int port) {
    
    sockaddr_in local;
    
    // set addr
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons((u_short)port);
    
    if(bind(_sListen,(sockaddr*)&local,sizeof(local))!=0)
    {
        return false;
    }
    
    return true;
}

/**
 * open socket
 * @return false on failure, true on success
 */
bool wsServer::openSocket() {
    _sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	int opt = 1;
	setsockopt(_sListen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (_sListen == SOCKET_ERROR) {
        return false;
    }
    
    return true;
}

/**
 * set the socket to nonblocking-IO
 * @return false on failure, true on success
 */
bool wsServer::setNonBlocking() {
#ifndef linux
    u_long iMode = 1;
    int iResult = ioctlsocket(_sListen, FIONBIO, &iMode);
    if (iResult != NO_ERROR) {
        return false;
    }
#else
    int x=fcntl(_sListen, F_GETFL, 0);  
    fcntl( _sListen, F_SETFL, x | O_NONBLOCK );
#endif
    
    return true;
}

/**
 * set all client sockets to INVALID_SOCKET
 */
void wsServer::initClients() {
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        _sClients[i].sock = INVALID_SOCKET;
		_sClients[i].decoder = NULL;
		_sClients[i].encoder = NULL;
    }
}

/**
 * WSA function, only needed on windows systems
 * @param action 1: startup, 2: cleanup
 * @return false on error, true on success
 */
bool wsServer::winSock(int action) {
#ifndef linux
    if (action == 1) {
        WSADATA w;
        if(int result = WSAStartup(MAKEWORD(2,2), &w) != 0)
        {
            cout << "Winsock 2 konnte nicht gestartet werden! Error #" << result << endl;
            return false;
        }
    }
    else if (action == 2) {
        WSACleanup();
    }
#endif
    return true;
}

/**
 * Handle an error
 * @param message
 */
void wsServer::error(string message) {
    cout << "[ERROR] " << message << "\n";
    exit(1);
}

/**
 * handle a message
 * @param message
 */
void wsServer::log(string message) {
    cout << "[MESSAGE] " << message << "\n";
}

/**
 * handle a message belonging to a client
 * @param message
 * @param clientId
 */
void wsServer::log(string message, int clientId) {
    cout << "[MESSAGE] [CLI: " << clientId << "] " << message << "\n";
}
