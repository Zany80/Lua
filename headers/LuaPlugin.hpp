#pragma once

#include <Core/Time/Clock.h>
#include <Core/Containers/Array.h>
#include <Core/String/String.h>
#include <lua.hpp>
#include <LuaImGuiBindings.hpp>

#include <worker_thread.hpp>

using namespace Oryol;

class LuaPlugin {
public:
	/**
	 * The constructor makes use of Oryol's IO module. The plugins: assign
	 * references the plugins root. This makes it possible to seamlessly switch
	 * to online plugins or those in a custom location, even embedded into the
	 * application or directly from RAM!
	 * 
	 * It's highly recommended that you construct all plugins in parallel on a
	 * 
	 */
	LuaPlugin(String path);
	~LuaPlugin();
	bool IsValid();
	bool isRunning();
	String error();
	String getPath();
	String getEmittedFunctionName(int i);
	void frame();
	int countEmittedFunctions();
	void executeEmittedFunctions();
	String getName();
	bool getWaitFrame();
	int getMissedFrames();
private:
	friend bool Lua::ImGui::verifyOwnership(lua_State *lua, String window);
	TimePoint tp;
	lua_State *lua, *frame_stabilizer;
	Array<String> emitted_functions, function_names;
	worker_thread *thread;
	bool loadError;
	String errorMessage;
	String path, name;
	int missed_frames;
	bool wait_frame;
};

extern Array<LuaPlugin*> plugins; 
extern std::mutex plugins_mutex, renderer_mutex;
