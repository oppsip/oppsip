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


local call_session = require "call_session"
local b2b_ua = call_session:new()

function b2b_ua:set_peer(s)
	self.peer_session = s
end

function b2b_ua:get_peer()
	return self.peer_session
end

function b2b_ua:on_ringback(content,content_type)
	local peer = self:get_peer()
	if peer then
		peer:on_peer_ringback(self,content,content_type)
	end
end

function b2b_ua:on_setup(content,content_type)
	self.setup_time = os.time()
	local peer = self:get_peer()
	if peer then
		peer:on_peer_setup(self,content,content_type)
	end
end

function b2b_ua:on_bye()
	local peer = self:get_peer()
	if peer then
		peer:on_peer_bye(self)
	end
	self:terminate()
end

function b2b_ua:on_reincall(content,content_type)
	local peer = self:get_peer()
	if peer then
		peer:on_peer_reincall(self,content,content_type)
	end
end

function b2b_ua:on_transfer(dest)
	local peer = self:get_peer()
	if peer then
		peer:on_peer_transfer(self,dest)
	end
end

function b2b_ua:on_error(code)
	local peer = self:get_peer()
	if peer then
		peer:on_peer_error(self,code)
	end
	self:terminate()
end

function b2b_ua:on_timeout(flag)
	local peer = self:get_peer()
	if peer then
		peer:on_peer_timeout(self,flag)
	end
	self:bye()
end

function b2b_ua:on_dtmf(dtmf)
	local peer = self:get_peer()
	if peer then
		peer:on_peer_dtmf(self,dtmf)
	end
end

function b2b_ua:on_peer_ringback(s,content,content_type)

end

function b2b_ua:on_peer_setup(s,content,content_type)
end

function b2b_ua:on_peer_reincall(peer,content,content_type)
	
end

function b2b_ua:on_peer_bye(s)
	self:bye()
end

function b2b_ua:on_peer_transfer(s,dest)
end

function b2b_ua:on_peer_error(s,code)
	self:bye()
end

function b2b_ua:on_peer_dtmf(s,dtmf)
end

function b2b_ua:on_peer_timeout(s,flag)
end

return b2b_ua
