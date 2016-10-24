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

#include "wsLuaHook.h"
#include "stdlib.h"
#include "stdarg.h"
#include "OPPSipChannel.h"
#include "sdp_negoc.h"
#include "AppTimer.h"
#include "OPPChannelSM.h"


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

wsLuaHook wsLuaHook::m_sInst;

wsLuaHook::~wsLuaHook() { }
wsLuaHook::wsLuaHook() { }

void wsLuaHook::hServerStartup(wsServerInterface *server) {
    _server = server;
    
    // check if lua-main-script exists
    ifstream ifile("main.lua");
    if (!ifile) {
        cout << "FATAL Error: main.lua not found.\n";
        return;
    }

    ifile.close();
   
    _L = luaL_newstate();
    luaL_openlibs(_L);
    
    // register funcs
    lua_register(_L, "_send", lSend);
    lua_register(_L, "_sendAll", lSendAll);
	lua_register(_L, "_disconnect", lDisConnect);
	lua_register(_L, "_answer", lAnswer);
	lua_register(_L, "_outCall", lOutCall);
	lua_register(_L, "_reOutCall", lReOutCall);
	lua_register(_L, "_bye", lBye);
	lua_register(_L, "_ringback", lRingback);

	lua_register(_L, "_devInit",lDevInit);
	lua_register(_L, "_devGetLeg",lDevGetChannel);
	lua_register(_L, "_devGetState",lDevGetSipState);
	lua_register(_L, "_transfer",lTransfer);

	lua_register(_L, "_toUserdata",lToUserdata);
	lua_register(_L, "_startTimer",lStartTimer);
	lua_register(_L, "_cancelTimer",lCancelTimer);
	lua_register(_L, "_sysInit",lSysInit);

	lua_register(_L, "_sendInfo",lSendInfo);

	lua_register(_L, "_doDispatch",lDoDispatch);
	lua_register(_L, "_msSleep",lSleep);

    if(luaL_dofile(_L, "main.lua"))
		printf(lua_tostring(_L, -1));
}

void wsLuaHook::hServerShutdown() {
    // close lua
    lua_close(_L);
}

void wsLuaHook::hClientConnect(int i) {
    lua_getglobal(_L, "on_connect");
    
    if (!lua_isfunction(_L,lua_gettop(_L))) {
        cout << "FATAL ERROR: Missing on_connect() in lua-Script!";
        return;
    }
    
    lua_pushnumber(_L, i);
    
    lua_call(_L, 1, 0);
}

void wsLuaHook::hClientDisconnect(int i) {
    lua_getglobal(_L, "on_disconnect");
    
    if (!lua_isfunction(_L,lua_gettop(_L))) {
        cout << "FATAL ERROR: Missing on_disconnect() in lua-Script!";
        return;
    }
    
    lua_pushnumber(_L, i);
    
    lua_call(_L, 1, 0);
}

void wsLuaHook::hClientMessage(int i, uint8*msg, int len) {
    lua_getglobal(_L, "on_websocket_msg");
    
    if (!lua_isfunction(_L,lua_gettop(_L))) {
        cout << "FATAL ERROR: Missing main() in lua-Script!";
        return;
    }
    
    lua_pushnumber(_L, i);
    lua_pushstring(_L, (char*)msg);
    
    lua_call(_L, 2, 0);
}

void wsLuaHook::hClientRawMessage(int i, uint8*msg, int len) {

	lua_getglobal(_L, "on_raw_msg");

	if (!lua_isfunction(_L,lua_gettop(_L))) {
		cout << "FATAL ERROR: Missing on_raw_msg() in lua-Script!";
		return;
	}

	lua_pushnumber(_L, i);
	lua_pushstring(_L, (char*)msg);

	lua_call(_L, 2, 0);
}

void wsLuaHook::hClientBinary(int i, uint8*msg, int len) {
}

void wsLuaHook::OnEvent(std::string strEventType,const char *fmt,...)
{
	if(fmt == NULL) return;

	lua_getglobal(_L, "on_event");

	if (!lua_isfunction(_L,lua_gettop(_L))) {
		cout << "FATAL ERROR: Missing on_event() in lua-Script!";
		return;
	}

	lua_pushstring(_L,strEventType.c_str());

	int param_count = 1;

	va_list arg_ptr;
	va_start(arg_ptr,fmt);

	const char *p = fmt;
	while(NULL != (p = strchr(p,'%')))
	{
		p++;
		switch(*p)
		{
		case 'd':
			lua_pushnumber(_L,va_arg(arg_ptr,int));
			param_count++;
			break;
		case 's':
			lua_pushstring(_L,va_arg(arg_ptr,char*));
			param_count++;
			break;
		case 'p':
			lua_pushlightuserdata(_L,va_arg(arg_ptr,void*));
			param_count++;
			break;
		default:
			break;
		}
	}

	va_end(arg_ptr);

	lua_call(_L,param_count,0);
}

int wsLuaHook::lSend(lua_State *L) {
    int args = lua_gettop(L);
    
    if (args != 2) {
        return 0;
    }
    
    int clientId = lua_tonumber(L, 1);
    string message = lua_tostring(L, 2);
    
    m_sInst._server->sendTo(clientId, message);
    
    return 0;
}

int wsLuaHook::lSendAll(lua_State *L) {
    int args = lua_gettop(L);
    
    if (args != 1) {
        return 0;
    }
    
    string message = lua_tostring(L, 1);
    
    m_sInst._server->sendBroadcast(message);
    
    return 0;
}

int wsLuaHook::lDisConnect(lua_State *L) {
	int args = lua_gettop(L);   
    if (args != 1) {
        return 0;
    }
	int clientId = lua_tonumber(L,1);
	
	m_sInst._server->disconnect(clientId);
	return 0;
}

int wsLuaHook::lOutCall(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 5)
		return 0;

	OPPSipDev *dev = (OPPSipDev*)lua_touserdata(L,1);
	const char *Caller = lua_tostring(L,2);
	const char *Callee = lua_tostring(L,3);
	const char *content = lua_tostring(L,4);
	const char *content_type = lua_tostring(L,5);

	if(dev && dev->GetContactUri())
	{
		OPPSipChannel *ch = dev->GetFreeSipChannel();
		if(ch)
		{
			InCallParam_t para(Caller,Callee,content,content_type,dev->GetContactUri());
			ch->DoInCall(&para);

			lua_pushlightuserdata(L,ch);
			lua_pushnumber(L,ch->GetChannelNo());

			return 2;
		}
	}

	return 0;
}

int wsLuaHook::lReOutCall(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 6)
		return 0;

	OPPSipDev		*dev = (OPPSipDev*)lua_touserdata(L,1);
	OPPSipChannel    *ch = (OPPSipChannel*)lua_touserdata(L,2);
	const char *caller = lua_tostring(L,3);
	const char *callee = lua_tostring(L,4);
	const char *content = lua_tostring(L,5);
	const char *content_type = lua_tostring(L,6);
	
	if(dev && dev->GetContactUri() && ch)
	{
		InCallParam_t para(caller,callee,content,content_type,dev->GetContactUri());	
		
		ch->DoInCall(&para);

		lua_pushnumber(L,0);

		return 1;	
	}
	
	lua_pushnumber(L,-1);
	return 1;
}

int wsLuaHook::lToUserdata(lua_State *L)
{
	OPPSipChannel *ch = (OPPSipChannel*)lua_tointeger(L,1);
	lua_pushlightuserdata(L,ch);
	return 1;
}

int wsLuaHook::lAnswer(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 5)
		return 0;

	OPPSipChannel*ch = (OPPSipChannel*)lua_touserdata(L,1);

	const char *number = lua_tostring(L,2);
	int  code = lua_tonumber(L,3);
	const char *content = lua_tostring(L,4);
	const char *content_type = lua_tostring(L,5);

	AcceptParam_t para(code,number,content,content_type);

	ch->DoAcceptCall(&para);

	return 0;
}

int wsLuaHook::lRingback(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 1)
		return 0;

	OPPSipChannel *ch = (OPPSipChannel*)lua_touserdata(L,1);
	
	ch->DoRingBack();

	return 0;
}

int wsLuaHook::lBye(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 1)
		return 0;

	OPPSipChannel *ch = (OPPSipChannel*)lua_touserdata(L,1);

	ch->DoEndCall();

	return 0;
}

int wsLuaHook::lSendInfo(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 2)
		return 0;

	OPPSipChannel *ch = (OPPSipChannel*)lua_touserdata(L,1);
	int dtmf = lua_tonumber(L,2);

	ch->DoInfo(dtmf);

	return 0;
}

int wsLuaHook::lTransfer(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 2)
		return 0;

	OPPSipChannel *ch = (OPPSipChannel*)lua_touserdata(L,1);
	const char *trans_to = lua_tostring(L,2);

	ch->DoBlindTransfer(trans_to);

	return 0;
}

int wsLuaHook::lSysInit(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 5)
		return 0;

	const char *local_domain = lua_tostring(L,1);
	int local_port = lua_tonumber(L,2);

	const char *dbg_host = lua_tostring(L,3);
	int  dbg_port = lua_tonumber(L,4);
	int  control_port = lua_tonumber(L,5);

	m_sInst._server->Init(control_port);

	API_DebugInit((char*)dbg_host,dbg_port);

	InitParam_t para;
	para.in.nSip_Port = local_port;
	para.in.LocalDomain = osip_strdup(local_domain);
	para.in.pCallBack = &m_sInst;

	API_Init(&para);

	return 0;
}

int wsLuaHook::lDevInit(lua_State *L)
{
	DevPara_t dev;
	dev.nDevType = (dev_type_t)(int)lua_tonumber(L,1);
	
	if(dev.nDevType == DEV_TYPE_SIP_ENDPOINT)
	{
		dev.x_dev.sip_endpoint_dev.nChannels = lua_tonumber(L,2);
		strcpy(dev.x_dev.sip_endpoint_dev.UserName,lua_tostring(L,3));
		strcpy(dev.x_dev.sip_endpoint_dev.Password,lua_tostring(L,4));
	}
	else if(dev.nDevType == DEV_TYPE_SIP_RELAY)
	{
		dev.x_dev.sip_relay_dev.nChannels = lua_tonumber(L,2);
		strcpy(dev.x_dev.sip_relay_dev.Domain,lua_tostring(L,3));
		strcpy(dev.x_dev.sip_relay_dev.Host,lua_tostring(L,4));
		dev.x_dev.sip_relay_dev.nPort = lua_tonumber(L,5);
	}
	else if(dev.nDevType == DEV_TYPE_SIP_REGISTER)
	{
		dev.x_dev.sip_register_dev.nChannels = lua_tonumber(L,2);
		strcpy(dev.x_dev.sip_register_dev.Domain,lua_tostring(L,3));
		strcpy(dev.x_dev.sip_register_dev.UserName,lua_tostring(L,4));
		strcpy(dev.x_dev.sip_register_dev.Password,lua_tostring(L,5));
		dev.x_dev.sip_register_dev.nPort = lua_tonumber(L,6);
	}

	void *p = API_InitDev(&dev);

	lua_pushlightuserdata(L,p);

	return 1;
}

int wsLuaHook::lDevGetChannel(lua_State *L)
{
	OPPSipDev *dev = (OPPSipDev *)lua_touserdata(L,1);
	int nChNo = lua_tonumber(L,2);
	void *p = dev->GetChannelByNo(nChNo);
	lua_pushlightuserdata(L,p);

	return 1;
}

int wsLuaHook::lDevGetSipState(lua_State *L)
{
	OPPSipDev *dev = (OPPSipDev *)lua_touserdata(L,1);
	
	if(dev && dev->GetState() == OPPSipDev::STATE_OnLine)
		lua_pushstring(L,"online");
	else
		lua_pushstring(L,"offline");

	return 1;
}

int wsLuaHook::lStartTimer(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 3)
		return 0;

	int nMSec = lua_tonumber(L,1);
	void *Para = (void*)lua_touserdata(L,2);
	int nFlag = lua_tonumber(L,3);

	AppTimer::sGetInstance()->StartTimer(nMSec,&m_sInst,nFlag,Para);
	
	return 0;
}

int wsLuaHook::lCancelTimer(lua_State *L)
{
	int args = lua_gettop(L);
	if(args != 2)
		return 0;

	void *Para = (void*)lua_touserdata(L,1);
	int nFlag = lua_tonumber(L,2);

	AppTimer::sGetInstance()->StopTimer(nFlag,Para,&m_sInst);

	return 0;
}

void wsLuaHook::OnDeviceOnline(OPPSipDev *dev)
{
	OnEvent("E_Online","%p",dev);
}

void wsLuaHook::OnDeviceOffline(OPPSipDev *dev)
{
	OnEvent("E_Offline","%p",dev);
}

void wsLuaHook::OnTransfer( OPPSipChannel *leg,const char *dest,OPPSipChannel *legTo )
{
	OnEvent("E_Transfer","%p%p%p%s",leg,leg->m_pSipDev,legTo,dest);
}
void wsLuaHook::OnDTMF(OPPSipChannel *leg,int Event)
{
	OnEvent("E_Dtmf","%p%p%d",leg,leg->GetDev(),Event);
}
void wsLuaHook::OnIncall( OPPSipChannel *ch )
{
	osip_message_t* sip = ch->m_IST->orig_request;
	const char *callee = sip->req_uri->username;
	if(callee == NULL)
		callee = sip->to->url->username;
	const char *caller = sip->from->url->username;
	const char *body = NULL;
	const char *content_type = NULL;
	if(ch->GetPeerBody(&body,&content_type) == 0)
	{
		OnEvent("E_InCall","%p%p%d%s%s%s%s",ch,ch->m_pSipDev,ch->GetChannelNo(),caller,callee,body,content_type);
	}
	else
	{
		OnEvent("E_InCall","%p%p%d%s%s",ch,ch->m_pSipDev,ch->GetChannelNo(),caller,callee);
	}
}

void wsLuaHook::OnOutCallSetup( OPPSipChannel *ch )
{
	const char *body = NULL;
	const char *content_type = NULL;
	if(ch->GetPeerBody(&body,&content_type) == 0)
	{
		OnEvent("E_Setup","%p%p%s%s",ch,ch->m_pSipDev,body,content_type);
	}
	else
	{
		OnEvent("E_Setup","%p%p",ch,ch->m_pSipDev);
	}
}

void wsLuaHook::OnInCallSetup( OPPSipChannel *ch )
{
	OnEvent("E_Setup","%p%p",ch,ch->m_pSipDev);
}

void wsLuaHook::OnEndCall( OPPSipChannel *ch )
{
	OnEvent("E_Bye","%p%p",ch,ch->m_pSipDev);
}

void wsLuaHook::OnError( OPPSipChannel *ch, int code )
{
	OnEvent("E_Error","%p%p%d",ch,ch->m_pSipDev,code);
}

void wsLuaHook::OnTimeout( OPPSipChannel *ch,int nFlag )
{
	OnEvent("E_Timeout","%p%p%d",ch,ch->m_pSipDev,nFlag);
}

void wsLuaHook::OnReIncall( OPPSipChannel *ch )
{
	const char *body = NULL;
	const char *content_type = NULL;
	if(ch->GetPeerBody(&body,&content_type) == 0)
	{
		OnEvent("E_ReIncall","%p%p%s%s",ch,ch->m_pSipDev,body,content_type);
	}
	else
	{
		OnEvent("E_ReIncall","%p%p",ch,ch->m_pSipDev);
	}
}

void wsLuaHook::OnRingBack( OPPSipChannel *ch )
{
	if(ch)
	{
		const char *body = NULL;
		const char *content_type = NULL;
		if(ch->GetPeerBody(&body,&content_type) == 0)
		{
			OnEvent("E_RingBack","%p%p%s%s",ch,ch->m_pSipDev,body,content_type);//has early media
		}
		else
		{
			OnEvent("E_RingBack","%p%p",ch,ch->m_pSipDev);//no early media
		}
	}
}

void wsLuaHook::OnTimeout(int nFlag,void *para)//OnAppTimer( void *Para,int nFlag )
{
	OnEvent("E_Timer","%p%d",para,nFlag);
}

int wsLuaHook::lDoDispatch(lua_State *L)
{
	m_sInst._server->run();
	return 0;
}

int wsLuaHook::lSleep(lua_State *L)
{
	int args = lua_gettop(L);
	if(args == 1)
	{
		int ms = lua_tonumber(L,1);
		usleep(ms*1000);
	}
	
	return 0;
}
