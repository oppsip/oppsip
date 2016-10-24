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


local sip_dev = {}

sip_dev.t_dev = {}

sip_dev.DEV_TYPE_ENDPOINT = 2
sip_dev.DEV_TYPE_RELAY = 1
sip_dev.DEV_TYPE_REGISTER = 0

sip_dev.t_endpoint_devs = {
	--{dev_name="1000",uid="1000",pass="1000",authid="1000",max_channels=2,status="offline"}
}
sip_dev.t_relay_devs = {
	--{dev_name="outgoing_gw",uid="outgoing_gw",callerid="52388216",domain="222.35.92.109",host="222.35.92.109",port=5060,max_channels=10,status="online"}
}
sip_dev.t_register_devs = {
	--{dev_name="server3",domain="192.168.1.200",port=5060,uid="2002",pass="2000",auth="2002",max_channels=2,status="offline"}
}

function sip_dev:create_endpoint_devs(base,count)	
	for i=1,count do
		local t = {}
		t.dev_name = tostring(base+i-1)
		t.uid = t.dev_name
		t.pass = t.uid
		t.authid = t.uid
		t.max_channels = 2
		t.status = "offline"
		self.t_endpoint_devs[#self.t_endpoint_devs+1] = t
	end
end

function sip_dev:dev_init()
	for i,v in ipairs(self.t_endpoint_devs) do
		local devid = _devInit(sip_dev.DEV_TYPE_ENDPOINT,v.max_channels,v.uid,v.pass)
		if devid ~= nil then
			self.t_dev[devid] = {}
			self.t_dev[devid].dev_type = sip_dev.DEV_TYPE_ENDPOINT
			self.t_dev[devid].dev_value = v
		end
	end
	
	for i,v in ipairs(self.t_relay_devs) do
		local devid = _devInit(sip_dev.DEV_TYPE_RELAY,v.max_channels,v.domain,v.host,v.port)
		if devid ~= nil then
			self.t_dev[devid] = {}
			self.t_dev[devid].dev_type = sip_dev.DEV_TYPE_RELAY
			self.t_dev[devid].dev_value = v
		end		
	end
	
	for i,v in ipairs(self.t_register_devs) do
		local devid = _devInit(sip_dev.DEV_TYPE_REGISTER,v.max_channels,v.domain,v.uid,v.pass,v.port)
		if devid ~= nil then
			self.t_dev[devid] = {}
			self.t_dev[devid].dev_type = sip_dev.DEV_TYPE_REGISTER
			self.t_dev[devid].dev_value = v
		end
	end
end

function sip_dev:get(devid)
	return self.t_dev[devid]
end

function sip_dev:get_by_uid(uid)
	for id,v in pairs(self.t_dev) do
		if v.dev_value.uid == uid then
			return id
		end
	end
end

function sip_dev:get_by_name(dev_name)
	for id,v in pairs(self.t_dev) do
		if v.dev_value.dev_name == dev_name then
			return id
		end
	end
end

function sip_dev:get_online_status(devid)
	if devid then
		return self.t_dev[devid].dev_value.status
	end
end

function sip_dev:set_online_status(devid,status)
	if devid then
		self.t_dev[devid].dev_value.status = status
	end
end

function sip_dev:get_max_channels(devid)
	if devid then
		return self.t_dev[devid].dev_value.max_channels
	end
end

return sip_dev