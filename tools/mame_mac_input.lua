-- Input/scheduling adapted from Alone in the Dark; logical low-memory reads for IIci.

local mac = {}

local prog = manager.machine.devices[":maincpu"].spaces["program"]

local CurApName = 0x910   -- Str31: frontmost application name
local RawMouse  = 0x82C   -- Point (v, h): the cursor, in global coordinates
local MacJmp    = 0x120   -- non-zero when a debugger is installed

function mac.pstring(addr, max)
	local n = prog:readv_u8(addr)
	if n > (max or 31) then return nil end
	local s = ""
	for i = 1, n do s = s .. string.char(prog:readv_u8(addr + i)) end
	return s
end

function mac.frontmost()  return mac.pstring(CurApName) end
function mac.mouse()      return prog:readv_i16(RawMouse + 2), prog:readv_i16(RawMouse) end  -- h, v
function mac.debugger()   return prog:readv_u32(MacJmp) end


local function find_field(name)
	local tries = { name }
	if #name == 1 then tries[#tries + 1] = name .. "  " .. name:upper() end
	for _, want in ipairs(tries) do
		for _, port in pairs(manager.machine.ioport.ports) do
			local f = port.fields[want]
			if f then return f end
		end
	end
	error("no ioport field named " .. name)
end

local HOLD, GAP = 4, 4    -- ⚠ ADB is polled, so a 1-frame press can be missed entirely

function mac.press(name, mods)
	local held = {}
	for _, m in ipairs(mods or {}) do
		local f = find_field(m); f:set_value(1); held[#held + 1] = f
	end
	local f = find_field(name)
	mac.wait(2)
	f:set_value(1);  mac.wait(HOLD)
	f:set_value(0)
	for _, h in ipairs(held) do h:set_value(0) end
	mac.wait(GAP)
end

function mac.key_down(name) find_field(name):set_value(1) end
function mac.key_up(name) find_field(name):set_value(0) end

mac.CMD = "Command / Open Apple"

local TYPE_HOLD, TYPE_GAP = 3, 2

local PUNCT = { [" "] = "Space", ["."] = ".  >", ["!"] = "1  !", ["-"] = "-  _" }

function mac.type(text)
	for i = 1, #text do
		local c = text:sub(i, i)
		local name = PUNCT[c] or (c:match("%a") and c:lower() .. "  " .. c:upper())
			or (c:match("%d") and nil) or nil
		if not name and c:match("%d") then
			for _, port in pairs(manager.machine.ioport.ports) do
				for fname, _ in pairs(port.fields) do
					if fname:sub(1, 1) == c and fname:sub(2, 2) == " " then name = fname end
				end
			end
		end
		if not name then error("mac.type: no key for " .. c) end
		local f = find_field(name)
		f:set_value(1); mac.wait(TYPE_HOLD)
		f:set_value(0); mac.wait(TYPE_GAP)
	end
end

local acc_x, acc_y = 0, 0

function mac.mouse_to(target_h, target_v, tol, tries)
	tol = tol or 3
	local fx, fy = find_field("Mouse X"), find_field("Mouse Y")
	local gain, last = { 1.5, 1.5 }, { 0, 0 }
	for _ = 1, tries or 400 do
		local h, v = mac.mouse()
		local dh, dv = target_h - h, target_v - v
		if math.abs(dh) <= tol and math.abs(dv) <= tol then
			mac.wait(6)
			h, v = mac.mouse()
			if math.abs(target_h - h) <= tol and math.abs(target_v - v) <= tol then
				return true
			end
		end
		local step = function(axis, d)
			if last[axis] * d < 0 then gain[axis] = math.max(0.2, gain[axis] / 2) end
			last[axis] = d
			if math.abs(d) <= tol then return 0 end
			local mag = math.max(1, math.min(40, math.floor(math.abs(d) * gain[axis])))
			return d > 0 and mag or -mag
		end
		acc_x = (acc_x + step(1, dh)) % 256
		acc_y = (acc_y + step(2, dv)) % 256
		fx:set_value(acc_x); fy:set_value(acc_y)
		mac.wait(1)
	end
	local h, v = mac.mouse()
	print(string.format("REF WARN mouse_to(%d,%d) ended at (%d,%d)", target_h, target_v, h, v))
	return false
end

function mac.menu(title_h, item_v, title_v)
	local b = find_field("Mouse Button 0")
	assert(mac.mouse_to(title_h, title_v or 7))
	b:set_value(1)
	mac.wait(20)
	assert(mac.mouse_to(title_h, item_v))
	mac.wait(20)
	b:set_value(0)
	mac.wait(30)
end

function mac.mouse_down() find_field("Mouse Button 0"):set_value(1) end
function mac.mouse_up() find_field("Mouse Button 0"):set_value(0) end

function mac.click(n)
	local b = find_field("Mouse Button 0")
	for _ = 1, n or 1 do
		b:set_value(1); mac.wait(3)
		b:set_value(0); mac.wait(3)      -- ⚠ inside the double-click time, so keep it tight
	end
	mac.wait(GAP)
end


local co
local frames = 0

function mac.wait(n)
	for _ = 1, n or 1 do coroutine.yield() end
end

function mac.frames() return frames end

function mac.wait_for(what, f, limit)
	for _ = 1, limit or 3600 do
		local v = f()
		if v then return v end
		coroutine.yield()
	end
	print("REF TIMEOUT waiting for " .. what)
	return nil
end

function mac.shot() manager.machine.video:snapshot() end

function mac.run(fn)
	co = coroutine.create(fn)
	emu.register_frame_done(function()
		frames = frames + 1
		if co and coroutine.status(co) ~= "dead" then
			local ok, err = coroutine.resume(co)
			if not ok then print("REF ERROR " .. tostring(err)) end
		end
	end)
end

return mac
