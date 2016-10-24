--[[
 * ,--. ,--, ,--, .--. ,-  ,--,
 * |  | |__| |__| |__   |  |__|
 * |  | |    |       |  |  |
 * '--' '    '    `--` --- '
 *
 * Copyright 2016 by Li Yucun <liycliyc@hotmail.com>
 * 
 * This file is part of OPPSIP.
 * 
 *  OPPSIP is free software:  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OPPSIP is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OPPSIP.  If not, see <http://www.gnu.org/licenses/>.
 *
]]--

function _new_call_id()
	local call_seq = 0
	local str_date = os.date("%Y%m%d%H%M%S")
	return function () 
		local cur_date = os.date("%Y%m%d%H%M%S")
		if cur_date == str_date then
			call_seq = call_seq + 1 
		else
			str_date = cur_date
			call_seq = 1
		end
		return  cur_date .. string.format("%03d",call_seq) 
	end
end

function dbg_log(...)
	local cur_date = os.date("%m-%d %H:%M:%S")
	print(cur_date,...)
end

new_call_id = _new_call_id()

local cjson = require "cjson"
local db_mysql = require "luasql_mysql"
local sip_proxy_session = require "sip_proxy_session"
local sip_dev = require "sip_dev"
local ws_client = require "websocket_client"
local call_session = require "call_session"

WORK_STATUS_OFFLINE = 0
WORK_STATUS_IDLE = 1
WORK_STATUS_BUSY = 2

t_work_numbers = {
	--{number="3311",pass="3311",dev_uid="1000",clientId=-1,work_status=WORK_STATUS_BUSY,consults={}},
	--{number="3312",pass="3312",dev_uid="1001",clientId=-1,work_status=WORK_STATUS_BUSY,consults={}}
}

function work_table_get_by_workid(workid)
	for i,v in ipairs(t_work_numbers) do
		if v.number == workid then
			return v
		end
	end
end

function work_table_get_by_clientId(clientId)
	for i,v in ipairs(t_work_numbers) do
		if v.clientId == clientId then
			return v
		end
	end
end

function work_table_get_by_uid(uid)
	for i,v in ipairs(t_work_numbers) do
		if v.dev_uid == uid then
			return v
		end
	end
end

-- functions
function empty_table(t)
	while table.remove(t) do
	end 
end

function on_online(...)
	local devid = ...
	sip_dev:set_online_status(devid,"online")
	dbg_log("device online:",devid)
end

function on_offline(...)
	local devid = ...
	sip_dev:set_online_status(devid,"offline")
	dbg_log("device offline:",devid)
end	

function on_dtmf(legid,devid,dtmf)
	local s = call_session:get_by_legid(legid)
	if s then
		s:on_dtmf(dtmf)
	else
		dbg_log("missing dtmf event")
	end
end

function on_incall(legid,devid,chno,caller,callee,body,content_type)
	local s = call_session:get_by_legid(legid)
	if s then
		_answer(legid,callee,600)
		return
	end
	
	local s = sip_proxy_session --session_get_by_router(caller,callee)
	if s then
		local ss = s:new(legid)
		ss:push_self(legid)
		ss:on_incall(devid,chno,caller,callee,body,content_type)
	else
		_answer(legid,callee,600)
	end
end

function on_error(legid,devid,code)
	local s = call_session:get_by_legid(legid)	
	if s then
		s:on_error(code)
	else
		dbg_log("error!!! on_error")
	end
end

function on_ringback(legid,devid,content,content_type)
	local s = call_session:get_by_legid(legid)
	if s then
		s:on_ringback(content,content_type)
	else
		dbg_log("error!!! on_ringback")
	end
end

function on_setup(legid,devid,content,content_type)
	local s = call_session:get_by_legid(legid)
	if s then
		s:on_setup(content,content_type)	
	else
		dbg_log("error!!! on_setup")
	end
end

function on_bye(legid,devid)
	local s = call_session:get_by_legid(legid)
	if s then
		s:on_bye()
	else
		dbg_log("error!!! on_bye",legid)
	end
end

function on_reincall(legid,devid,content,content_type)
	local s = call_session:get_by_legid(legid)
	if s then
		s:on_reincall(content,content_type)
	else
		dbg_log("error!!! on_reincall")
	end
end

function on_timeout(legid,devid,flag)
	local s = call_session:get_by_legid(legid)
	if s then
		s:on_timeout(flag)
	else
		dbg_log("warning! got timeout event when leg is null")
	end
end

function on_transfer(legid,devid,legidTo,dest)
	local s = call_session:get_by_legid(legid)
	if s then
		s:on_transfer(dest)
	else
		dbg_log("error!!! on_transfer")
	end
end

function on_timer(legid,flag)
	local s = call_session:get_by_legid(legid)
	if s then
		s:on_app_timer(flag)
	else
		dbg_log("missing timer event")
	end
end

t_event_method = {	["E_InCall"]=on_incall,
					["E_RingBack"]=on_ringback,
					["E_Setup"]=on_setup,
					["E_Error"]=on_error,
					["E_Bye"]=on_bye,
					["E_Transfer"]=on_transfer,
					["E_Online"]=on_online,
					["E_Offline"]=on_offline,
					["E_Dtmf"]=on_dtmf,
					["E_Timeout"]=on_timeout,
					["E_Timer"]=on_timer,
					["E_ReIncall"]=on_reincall					
				}	
				
function on_event(event_type,...)
	if t_event_method[event_type] ~= nil then
		t_event_method[event_type](...)
	end
end

function on_raw_msg (clientId, message)	
	dbg_log("on_raw_msg",clientId,message)
end

function on_websocket_msg (clientId, message)
	dbg_log("websocket message:",clientId,message)
	
end

function on_disconnect(clientId)
    dbg_log("clientID " .. clientId .. "quit.")
	ws_client:set_client(clientId,nil)
end

function on_connect(clientId)
    dbg_log("clientID ".. clientId .. " connected.")
	ws_client:set_client(clientId,{name = 'noname'})
end

function sys_init()
	_sysInit("192.168.1.201",5060,"192.168.1.201",1234,1010)
end

sys_init()
sip_dev:create_endpoint_devs(8000,100)
sip_dev:dev_init()

while true do
	_doDispatch()
end
