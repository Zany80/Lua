#pragma once

#include <Core/Time/Clock.h>
#include <Core/Containers/Array.h>
#include <Core/String/String.h>
#include <lua.hpp>
#include <LuaImGuiBindings.hpp>

#ifndef ORYOL_EMSCRIPTEN
#include <worker_thread.hpp>
#endif

#include <mutex>

using namespace Oryol;

class LuaPlugin {
public:
	/**
	 * The constructor makes use of Oryol's IO module. The plugins: assign
	 * references the plugins root. This makes it possible to seamlessly switch
	 * to online plugins or those in a custom location, even embedded into the
	 * application or directly from RAM!
	 * 
	 */
	LuaPlugin(String path, String source, int size = -1);
	static void constructPlugin(String path);
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
	lua_State *lua;
	#ifndef ORYOL_EMSCRIPTEN
	lua_State *frame_stabilizer;
	Array<String> emitted_functions, function_names;
	worker_thread *thread;
	int missed_frames;
	bool wait_frame;
	#endif
	bool loadError;
	String errorMessage;
	String path, name;
};

extern Array<LuaPlugin*> plugins; 
extern std::mutex plugins_mutex, renderer_mutex;
