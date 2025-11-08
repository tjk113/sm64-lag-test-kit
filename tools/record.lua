local warp_param_blocks = {
	{ 0x8033B248, 8 }, -- sWarpDest
	{ 0x8033BAC8, 2 }, -- gCurrActNum
	{ 0x8032DDE8, 1 }, -- gWarpTransRed
	{ 0x8032DDEC, 1 }, -- gWarpTransGreen
	{ 0x8032DDF0, 1 }  -- gWarpTransBlue
}
local state_blocks = {
	{ 0x80207700, 0x200 },	-- gSaveBuffer
	{ 0x8032D5D4, 4 },		-- gGlobalTimer
	{ 0x8032D93C, 0xC8 },	-- gMarioState
	{ 0x8032DD34, 2 },		-- sSwimStrength
	{ 0x8032DD80, 0x18 },	-- save_file.o
	{ 0x8032DDF4, 2 },		-- gCurrSaveFileNum
	{ 0x80330F3C, 4},		-- gPaintingMarioYEntry
	{ 0x80331370, 0x368 },	-- ingame_menu.o
	{ 0x8033B170, 0xC8 },	-- gMarioStates
	{ 0x8033C684, 2 },		-- sSelectionFlags
	{ 0x80361258, 2 },		-- gTTCSpeedSetting
	{ 0x8038EEE0, 2 },		-- gRandomSeed16
}
	
local is_recording = false
local warp_params = {}
local start_state = {}
local recording_inputs = {}
local last_cam_yaw = 0
local last_cam_movement_flags = 0
local last_global_timer = -1

function getCamYaw()
	local mario_area = memory.readdword(0x8033B200)
	if (mario_area == 0) then
		return 0
	end
	local area_cam = memory.readdword(mario_area + 0x24)
	if (area_cam == 0) then
		return 0
	end
	return memory.readword(area_cam + 2)
end

function updateCam()
	last_cam_yaw = getCamYaw()
	last_cam_movement_flags = memory.readword(0x8033C848)
end

function addRecordingFrame()
	local global_timer = memory.readdword(0x8032D5D4)
	if ((last_global_timer ~= -1) and (global_timer ~= (last_global_timer + 1))) then
		print("Global timer jumped. Stopping recording.")
		stop()
	else
		last_global_timer = global_timer
		local cur_input_b = memory.readdword(0x8033AFF8)
		local cam_yaw = last_cam_yaw
		local cam_movement_flags = last_cam_movement_flags
		table.insert(recording_inputs,
			string.pack(">I4", cur_input_b) ..
			string.pack(">I2", cam_yaw) ..
			string.pack(">I2", cam_movement_flags))
	end
end


local read_sizes = { 4, 2, 1 }
local num_read_sizes = #read_sizes
function readMemoryBlock(addr, size)
	local data_block = ""
	for i=1,num_read_sizes do
		local cur_read_size = read_sizes[i]
		while (size >= cur_read_size) do
			if (cur_read_size == 4) then
				data_block = data_block .. string.pack(">I4", memory.readdword(addr))
			elseif (cur_read_size == 2) then
				data_block = data_block .. string.pack(">I2", memory.readword(addr))
			else
				data_block = data_block .. string.pack("B", memory.readbyte(addr))
			end
			addr = addr + cur_read_size
			size = size - cur_read_size
		end
	end
	return data_block
end

function readMemBlocks(data, blocks)
	for i=1,#blocks do
		local addr = blocks[i][1]
		local size = blocks[i][2]
		data[i] = readMemoryBlock(addr, size)
	end
end

function updateStartState()
	readMemBlocks(warp_params, warp_param_blocks)
	readMemBlocks(start_state, state_blocks)
end

function checkRecordingStart()
	if (#start_state > 0) then
		local warp_dest_type = memory.readbyte(0x8033B248)
		local last_frame_warp_dest_type = string.byte(warp_params[1])
		if ((last_frame_warp_dest_type == 1) and (warp_dest_type == 0)) then -- level warp
			is_recording = true
			addRecordingFrame()
			print("Recording started.")
			return
		end
	end
	updateStartState()
end


function update()
	if (is_recording) then
		addRecordingFrame()
	else
		checkRecordingStart()
	end
	updateCam()
end


function save()
	if (not is_recording) then
		return
	end

	local path = iohelper.filediag("*.dat", 1)
	if ((path == nil) or (path == "")) then
		print("Invalid path entered.")
		return
	end

	local file = io.open(path, "wb")
    if (file == nil) then
        print(string.format("Could not open file at \"%s\".", path))
        return
    end

    for i=1,#warp_param_blocks do
    	file:write(warp_params[i])
    end

    local num_state_blocks = #state_blocks
    file:write(string.pack("I2", num_state_blocks))
    for i=1,num_state_blocks do
    	file:write(string.pack("I4", state_blocks[i][1]))
    	file:write(string.pack("I4", state_blocks[i][2]))
    end
    for i=1,num_state_blocks do
    	file:write(start_state[i])
    end

    local recording_length = #recording_inputs
    file:write(string.pack("I2", recording_length))
    for i=1,recording_length do
    	file:write(recording_inputs[i])
    end

    file:close()
    print("Recording saved.")
end


emu.atinput(update)
emu.atstop(save)
