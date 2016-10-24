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

local call_session = {}

call_session.t_call_legs = {}

function call_session:push_self(legid)
	if legid then
		self.legid = legid
		call_session.t_call_legs[legid] = self
	elseif self.legid then
		call_session.t_call_legs[self.legid] = self
	end
end

function call_session:pop_self()
	if self.legid then
		call_session.t_call_legs[self.legid] = nil
	end
end

function call_session:get_by_legid(legid)
	if legid then
		return call_session.t_call_legs[legid]
	end
end

math.randomseed(os.time())

function call_session:new(legid,s)
	s = s or {}
	setmetatable(s,self)
	self.__index = self

	s.call_id = new_call_id() 
	s.legid = legid

	return s
end

function call_session:update_class(s)
	setmetatable(self,s)
	s.__index = s
end

function call_session:answer(number,code,content,content_type)
	_answer(self.legid,number,code,content,content_type)
end

function call_session:out_call(devid,caller,callee,content,content_type)
	legid,chno = _outCall(devid,caller,callee,content,content_type)
	if legid == nil then
		dbg_log("out_call failed",caller,callee)
		return -1
	end
	
	self.legid = legid
	self.devid = devid
	self.chno  = chno
	self.caller = caller
	self.callee = callee
	
	self.start_time = os.time()
	self:push_self(legid)
	
	return 0
end

function call_session:recall(content,content_type)
	_reOutCall(self.devid,self.legid,self.caller,self.callee,content,content_type)
end

function call_session:ringback()
	_ringback(self.legid)
end	

function call_session:bye()
	if self.legid then
		_bye(self.legid)
	end
	self:terminate()
end

function call_session:transfer(to)
	_transfer(self.legid,to)
end

function call_session:send_info(dtmf)
	_sendInfo(self.legid,dtmf)
end

function call_session:start_timer(nMs,nFlag)
	_startTimer(nMs,self.legid,nFlag)
end

function call_session:cancel_timer(nFlag)
	_cancelTimer(self.legid,nFlag)
end

function call_session:terminate()
	self.end_time = os.time()
	self:set_peer(nil)
	self:pop_self()
end


function call_session:on_incall(devid,chno,caller,callee,content,content_type)
	self.devid = devid
	self.chno = chno
	self.callee = callee
	self.caller = caller

	self.start_time = os.time()
end

function call_session:on_ringback(content,content_type)

end

function call_session:on_setup(content,content_type)
	self.setup_time = os.time()
end

function call_session:on_bye()
	self:terminate()
end

function call_session:on_reincall(content,content_type)

end

function call_session:on_transfer(dest)

end

function call_session:on_error(code)
	self:terminate()
end

function call_session:on_timeout(flag)
	self:bye()
end

function call_session:on_app_timer(nFlag)

end

function call_session:on_dtmf(dtmf)

end

return call_session
