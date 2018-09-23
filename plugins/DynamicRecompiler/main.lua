local version = "0.1.0"

-- Configuration *cannot* be changed later.
function get_config()
	return {
		name = "Dynamic Recompiler",
		wait_frame = false
	}
end

success, title = ImGui.RequestWindow("Hello!")
if not success then print(title) error("Unable to claim window!")end

total_time = 0
frames = 0

function frame(delta_time)
	total_time = total_time + delta_time
	frames = frames + 1
	local FPS = math.floor((frames / total_time) * 1000)
	FPS = FPS <= 60 and FPS or 60
	ImGui.Begin:bind(title):emit("Set up window", "ImGui.Begin(\""..title.."\")")
	ImGui.Text:bind("Dynamic Recompiler v" .. version):emit("Display version", "ImGui.Text(\"Dynamic Recompiler v"..version.."\")")
	ImGui.Text:bind("FPS: " .. FPS):emit("Display FPS", "ImGui.Text(\"FPS: \" .. "..FPS..")")
	ImGui.End:emit("Finalize window", "ImGui.End()")
end

function cleanup()
	Log.Dbg("Cleaning up "..get_config().name.."\n")
	
end
