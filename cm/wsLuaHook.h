
#ifndef WSLUAHOOK_H
#define	WSLUAHOOK_H

#include <stdio.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "./wsHookInterface.h"
#include "./wsServerInterface.h"
#include "OPPSipChannel.h"
#include "OPPSession.h"

using namespace std;

class wsLuaHook : public wsHookInterface, public OPPSession , public OPPTimerAware
{
public:
    virtual ~wsLuaHook();

	static wsLuaHook *sGetInst()
	{
		return &m_sInst;
	}
    
    virtual void hServerStartup(wsServerInterface *server);
    virtual void hServerShutdown();
    
    virtual void hClientConnect(int i);
    virtual void hClientDisconnect(int i);
    virtual void hClientMessage(int i, uint8* buf,int len);
	virtual void hClientRawMessage(int i, uint8*msg, int len);
	virtual void hClientBinary(int i,uint8* buf,int len);

	virtual void OnIncall(OPPSipChannel *ch);
	virtual void OnOutCallSetup(OPPSipChannel *ch);
	virtual void OnInCallSetup(OPPSipChannel *ch);
	virtual void OnEndCall(OPPSipChannel *ch);
	virtual void OnError(OPPSipChannel *ch,int code);
	virtual void OnTimeout(OPPSipChannel *ch,int nFlag);
	virtual void OnReIncall(OPPSipChannel *ch);
	virtual void OnRingBack(OPPSipChannel *ch);
	virtual void OnTransfer(OPPSipChannel *leg,const char *dest,OPPSipChannel *legTo);
	virtual void OnDTMF(OPPSipChannel *leg,int Event);

	virtual void OnDeviceOnline(OPPSipDev *dev);
	virtual void OnDeviceOffline(OPPSipDev *dev);

	//overide OPPTimerAware
	virtual void OnTimeout(int nFlag,void *para);

private:
	wsLuaHook();
    lua_State* _L;
	struct timeval m_tv;
    wsServerInterface* _server;
	static wsLuaHook m_sInst;
    
    // functions to be called from lua
    static int lSend(lua_State *L);
    static int lSendAll(lua_State *L);
	static int lDisConnect(lua_State *L);

	static int lOutCall(lua_State *L);
	static int lReOutCall(lua_State *L);
	static int lAnswer(lua_State *L);
	static int lRingback(lua_State *L);
	static int lBye(lua_State *L);
	static int lTransfer(lua_State *L);

	static int lDevInit(lua_State *L);
	static int lDevGetChannel(lua_State *L);
	static int lDevGetSipState(lua_State *L);

	static int lToUserdata(lua_State *L);
	static int lSendInfo(lua_State *L);

	static int lStartTimer(lua_State *L);
	static int lCancelTimer(lua_State *L);
	static int lSysInit(lua_State *L);

	static int lDoDispatch(lua_State *L);
	static int lSleep(lua_State *L);
	void OnEvent(std::string strEventType,const char *fmt,...);
	
};

#endif	/* WSLUAHOOK_H */

