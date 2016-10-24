local call_session = require "call_session"
local b2b_ua = require "b2b_ua"
local sip_dev = require "sip_dev"

local sip_proxy_session = b2b_ua:new()

function sip_proxy_session:on_incall(devid,chno,caller,callee,content,content_type)
	b2b_ua.on_incall(self,devid,chno,caller,callee,content,content_type)
	dbg_log("on_incall",self,content,content_type)
	local dev_dest = sip_dev:get_by_uid(callee)
	if dev_dest then
		local s = b2b_ua:new()
		if 0 == s:out_call(dev_dest,caller,callee,content,content_type) then
			self:set_peer(s)
			s:set_peer(self)
		end
	end
end

function sip_proxy_session:on_peer_ringback(s)
	self:ringback()
end

function sip_proxy_session:on_peer_setup(s,content,content_type)
	self:answer(self.callee,200,content,content_type)
end

function sip_proxy_session:on_setup()
	b2b_ua.on_setup(self)
end

function sip_proxy_session:on_transfer(dest)
	b2b_ua.on_transfer(self,dest)
end

function sip_proxy_session:on_peer_bye(s)
	self:bye()
end

function sip_proxy_session:on_dtmf(dtmf)
	local peer = self:get_peer()
	if peer then
		peer:send_info(dtmf)
	end
end

function sip_proxy_session:on_bye()
	b2b_ua.on_bye(self)
end

function sip_proxy_session:on_peer_error(s,code)
	self:answer(self.callee,code)
	self:terminate()
end

return sip_proxy_session
