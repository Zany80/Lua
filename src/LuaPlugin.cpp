#include <LuaPlugin.hpp>
#include <LuaImGuiBindings.hpp>
#include <LuaOryolBindings.hpp>

#include <IMUI/IMUI.h>
#include <IO/IO.h>
#include <Core/Core.h>

#include <thread>
#include <chrono>

#ifndef ORYOL_EMSCRIPTEN
#include <worker_thread.hpp>
#endif

String gettype(lua_State *lua, int i) {
	if (lua_isboolean(lua, i))
		return "Boolean";
	if (lua_iscfunction(lua, i))
		return "C Function";
	if (lua_isfunction(lua, i))
		return "Function";
	if (lua_islightuserdata(lua, i))
		return "Light User Data";
	if (lua_isnil(lua, i))
		return "Nil";
	return "Default";
}

String dump(lua_State *lua) {
	StringBuilder s = "State of Lua object at 0x";
	o_assert_dbg(s.AppendFormat(19, "%016llX: \n", (unsigned long long)lua));
	o_assert_dbg(s.AppendFormat(18, "Object count: %d\n", lua_gettop(lua)));
	for (int i = 1; i <= lua_gettop(lua); i++) {
		s.AppendFormat(64, "Object %d: %s\n\t", i, gettype(lua, i).AsCStr());
	}
	return s.GetString();
}

int emit(lua_State *lua) {
	if (lua_gettop(lua) != 3 || !lua_isfunction(lua, 1) || !lua_isstring(lua, 2) || !lua_isstring(lua, 3))
		return luaL_error(lua, "Usage: function:emit(string name, string fallback_command)");
	String name = lua_tostring(lua, 2);
	String fallback_command = lua_tostring(lua, 3);
	lua_pop(lua, 2);
	lua_getfield(lua, LUA_REGISTRYINDEX, "Emitted Functions");
	o_assert_dbg(lua_istable(lua, 2));
	lua_insert(lua, 1);
	o_assert_dbg(lua_isfunction(lua, 2));
	o_assert_dbg(lua_istable(lua, 1));
	lua_rawseti(lua, 1, lua_objlen(lua, 1) + 1);
	Log::Dbg("Emitted %s\n", name.AsCStr());
	lua_getfield(lua, LUA_REGISTRYINDEX, "Emitted Function Names");
	o_assert_dbg(lua_istable(lua, 2));
	lua_pushstring(lua, name.AsCStr());
	lua_rawseti(lua, 2, lua_objlen(lua, 1));
	lua_pop(lua, 1);
	lua_getfield(lua, LUA_REGISTRYINDEX, "Fallback Commands");
	o_assert_dbg(lua_istable(lua, 2));
	lua_pushstring(lua, fallback_command.AsCStr());
	lua_rawseti(lua, 2, lua_objlen(lua, 1));
	return 0;
}

String LuaPlugin::getName() {
	return name;
}

bool LuaPlugin::getWaitFrame() {
	return wait_frame;
}

constexpr const char *bind_string = 
"function bind(func, ...)\n"
"	local args={...}\n"
"	return function(...)\n"
"		local args2 = {...}\n"
"		return func(unpack(args), unpack(args2))\n"
"	end\n"
"end";

int bind(lua_State *lua) {
	luaL_loadstring(lua, bind_string);
	lua_call(lua, 0, 0);
	lua_getglobal(lua, "bind");
	lua_insert(lua, 1);
	lua_call(lua, lua_gettop(lua) - 1, 1);
	return 1;
}

// The following function is from the Lua 5.1 source.
int unpack (lua_State *L) {
	int i, e, n;
	luaL_checktype(L, 1, LUA_TTABLE);
	i = luaL_optint(L, 2, 1);
	e = luaL_opt(L, luaL_checkint, 3, luaL_getn(L, 1));
	if (i > e) return 0;  /* empty range */
	n = e - i + 1;  /* number of elements */
	if (n <= 0 || !lua_checkstack(L, n))  /* n <= 0 means arith. overflow */
		return luaL_error(L, "too many results to unpack");
	lua_rawgeti(L, 1, i);  /* push arg[i] (avoiding overflow problems) */
	while (i++ < e)  /* push arg[i + 1...e] */
		lua_rawgeti(L, 1, i);
	return n;
}

void LuaPlugin::constructPlugin(String path) {
	IO::Load(path, [path](IO::LoadResult res) {
		Log::Dbg("Plugin %s loaded. SOURCE %s ENDSOURCE \n", path.AsCStr(), (const char *)res.Data.Data());
		new LuaPlugin(path, (const char *)res.Data.Data(), res.Data.Size());
	});
}

LuaPlugin::LuaPlugin(String path, String source, int size) {
	this->path = path;
	#ifndef ORYOL_EMSCRIPTEN
	this->thread = nullptr;
	#endif
	this->frame_stabilizer = nullptr;
	this->missed_frames = 0;
	plugins_mutex.lock();
	plugins.Add(this);
	plugins_mutex.unlock();
	loadError = false;
	this->lua = luaL_newstate();
	o_assert(this->lua);
	lua_pushlightuserdata(lua, this);
	lua_setfield(lua, LUA_REGISTRYINDEX, "Plugin");
	lua_pushcfunction(lua, luaopen_base);
	lua_call(lua, 0, 0);
	lua_pushcfunction(lua, luaopen_string);
	lua_call(lua, 0, 0);
	lua_pushcfunction(lua, luaopen_math);
	lua_call(lua, 0, 0);
	lua_register(lua, "unpack", unpack);
	// Load in the bind function
	lua_pushcfunction(lua, bind);
	// Create a function metatable
	lua_newtable(lua);
	// make __index
	lua_newtable(lua);
	// Add bind function to __index
	lua_pushcfunction(lua, bind);
	lua_setfield(lua, -2, "bind");
	// Add emit function to __index
	lua_pushcfunction(lua, emit);
	lua_setfield(lua, -2, "emit");
	// Set __index into metatable
	lua_setfield(lua, -2, "__index");
	// Set table as function metatable
	lua_setmetatable(lua, -2);
	// Pop the bind function
	lua_pop(lua, 1);
	// Set up the emitted functions table in the registry
	lua_newtable(lua);
	lua_setfield(lua, LUA_REGISTRYINDEX, "Emitted Functions");
	lua_newtable(lua);
	lua_setfield(lua, LUA_REGISTRYINDEX, "Emitted Function Names");
	lua_newtable(lua);
	lua_setfield(lua, LUA_REGISTRYINDEX, "Fallback Commands");
	Lua::ImGui::expose(this->lua);
	Lua::Oryol::expose(this->lua);
	if (size == -1)
		size = source.Length();
	int loaded = luaL_loadbuffer(lua, source.AsCStr(), size, path.AsCStr());
	if (loaded != 0) {
		loadError = true;
		errorMessage = lua_tostring(lua, -1);
		return;
	}
	if (lua_pcall(lua, 0, 0, 0)) {
		Log::Error("Error: %s\n", lua_tostring(lua, -1));
		errorMessage = lua_tostring(lua, -1);
		loadError = true;
		return;
	}
	lua_getglobal(lua, "get_config");
	if (!lua_isfunction(lua, -1)) {
		loadError = true;
		errorMessage = "No function get_config defined!";
		return;
	}
	if (lua_pcall(lua, 0, 1, 0)) {
		errorMessage = lua_tostring(lua, -1);
		loadError = true;
		return;
	}
	if (!lua_istable(lua, -1)) {
		loadError = true;
		errorMessage = "Configuration must be a table!";
		return;
	}
	lua_getfield(lua, -1, "wait_frame");
	if (!lua_isboolean(lua, -1)) {
		loadError = true;
		errorMessage = "wait_frame must be a boolean!";
		return;
	}
	wait_frame = lua_toboolean(lua, -1);
	lua_pop(lua, 1);
	lua_getfield(lua, -1, "name");
	if (!lua_isstring(lua, -1)) {
		errorMessage = "Configuration must contain a string \"name\"!";
		loadError = true;
		return;
	}
	name = lua_tostring(lua, -1);
	lua_pop(lua, 2);
	frame_stabilizer = luaL_newstate();
	o_assert(this->frame_stabilizer);
	lua_pushlightuserdata(frame_stabilizer, this);
	lua_setfield(frame_stabilizer, LUA_REGISTRYINDEX, "Plugin");
	Lua::ImGui::expose(frame_stabilizer);
	lua_pushcfunction(frame_stabilizer, luaopen_string);
	lua_call(frame_stabilizer, 0, 0);
	this->tp = Clock::Now();
	#ifndef ORYOL_EMSCRIPTEN
	thread = new worker_thread([this]() {
		lua_getglobal(this->lua, "frame");
		lua_pushnumber(this->lua, Clock::LapTime(this->tp).AsMilliSeconds());
		if (lua_pcall(this->lua, 1, 0, 0)) {
			Log::Error("Error: %s\n", lua_tostring(this->lua, -1));
			this->loadError = true;
			this->errorMessage = lua_tostring(this->lua, -1);
			lua_pop(this->lua, 1);
		}
		this->thread->pause();
	}, false);
	#endif
}

LuaPlugin::~LuaPlugin() {
	/* If a plugin errors out mid-execution, the thread exists. If it errors out
	 * in the constructor, it's nullptr. As such, an assertion would cause an
	 * unwanted crash here, and checking IsValid() would not always be correct.
	 */
	#ifndef ORYOL_EMSCRIPTEN
	if (thread != nullptr) {
		thread->finalize();
	}
	#endif
	o_assert(this->lua);
	int index = plugins.FindIndexLinear(this);
	o_assert(index != InvalidIndex);
	plugins_mutex.lock();
	plugins.Erase(index);
	plugins_mutex.unlock();
	#ifndef ORYOL_EMSCRIPTEN
	if (thread != nullptr) {
		// The destructor calls wait(), no reason to call it here
		delete thread;
		thread = nullptr;
	}
	#endif
	lua_getglobal(this->lua, "cleanup");
	if (lua_pcall(this->lua, 0, 0, 0))
		Log::Error("Error in plugin cleanup: %s\n", lua_tostring(this->lua, -1));
	lua_close(this->lua);
	this->lua = nullptr;
	if (frame_stabilizer != nullptr) {
		lua_close(frame_stabilizer);
		frame_stabilizer = nullptr;
	}
}

int LuaPlugin::countEmittedFunctions() {
	#ifndef ORYOL_EMSCRIPTEN
	o_assert(this->thread);
	if (thread->running())
		return function_names.Size();
	#endif
	o_assert(this->lua);
	lua_getfield(this->lua, LUA_REGISTRYINDEX, "Emitted Functions");
	o_assert(lua_istable(lua, -1));
	int count = lua_objlen(lua, -1);
	lua_pop(lua, 1);
	return count;
}

String LuaPlugin::getEmittedFunctionName(int i) {
	#ifndef ORYOL_EMSCRIPTEN
	o_assert_dbg(this->thread);
	if (thread->running())
		return function_names.Size() > (i-1) ? function_names[i-1] : "Out of bounds";
	#endif
	o_assert_dbg(this->lua);
	lua_getfield(this->lua, LUA_REGISTRYINDEX, "Emitted Function Names");
	o_assert_dbg(lua_istable(lua, -1));
	lua_rawgeti(lua, -1, i);
	o_assert_dbg(!lua_isnumber(lua, -1));
	o_assert_dbg(lua_isstring(lua, -1));
	String name = lua_tostring(lua, -1);
	lua_pop(lua, 2);
	return name;
}

int LuaPlugin::getMissedFrames() {
	return missed_frames;
}

void LuaPlugin::executeEmittedFunctions() {
	#ifndef ORYOL_EMSCRIPTEN
	o_assert(this->thread);
	// If thread is still running, abort
	if (this->thread->running()) {
		if (this->getWaitFrame()) {
			this->thread->waitFrame();
		}
		else {
			this->missed_frames++;
			Log::Warn("Frame dropped! Re-executing %d functions\n", this->emitted_functions.Size());
			// Re-render previously emitted functions
			for (int i = 0; i < emitted_functions.Size(); i++) {
				if (luaL_dostring(frame_stabilizer, emitted_functions[i].AsCStr()))
					Log::Error("Error stabilizing frame: %s\n", lua_tostring(frame_stabilizer, -1));
			}
			return;
		}
	}
	#endif
	o_assert(this->lua);
	lua_getfield(this->lua, LUA_REGISTRYINDEX, "Emitted Functions");
	o_assert(lua_istable(lua, -1));
	for (size_t i = 1; i <= lua_objlen(lua, -1);i++) {
		lua_rawgeti(lua, -1, i);
		o_assert(lua_isfunction(lua, -1));
		if (lua_pcall(lua, 0, 0, 0)) {
			Log::Error("Error: %s\n", lua_tostring(lua, -1));
			lua_pop(lua, 1);
		}
	}
	for (size_t i = lua_objlen(lua, -1);i >= 1;i--) {
		lua_pushnil(lua);
		lua_rawseti(lua, -2, i);
	}
	o_assert(lua_objlen(lua, -1) == 0);
	lua_pop(lua, 1);
	// Copy over the previously emitted functions
	emitted_functions.Clear();
	lua_getfield(lua, LUA_REGISTRYINDEX, "Fallback Commands");
	o_assert(lua_istable(lua, -1));
	for (size_t i = 1; i <= lua_objlen(lua, -1); i++) {
		lua_rawgeti(lua, -1, i);
		if (lua_isstring(lua, -1))
			emitted_functions.Add(lua_tostring(lua, -1));
		lua_pop(lua, 1);
	}
	for (size_t i = lua_objlen(lua, -1);i >= 1;i--) {
		lua_pushnil(lua);
		lua_rawseti(lua, -2, i);
	}
	o_assert(lua_objlen(lua, -1) == 0);
	lua_pop(lua, 1);
	// Copy over function names
	function_names.Clear();
	lua_getfield(lua, LUA_REGISTRYINDEX, "Emitted Function Names");
	o_assert(lua_istable(lua, -1));
	for (size_t i = 1; i <= lua_objlen(lua, -1); i++) {
		lua_rawgeti(lua, -1, i);
		if (lua_isstring(lua, -1))
			function_names.Add(lua_tostring(lua, -1));
		lua_pop(lua, 1);
	}
	for (size_t i = lua_objlen(lua, -1);i >= 1;i--) {
		lua_pushnil(lua);
		lua_rawseti(lua, -2, i);
	}
	o_assert(lua_objlen(lua, -1) == 0);
	lua_pop(lua, 1);
}

bool LuaPlugin::IsValid() {
	return !loadError;
}

String LuaPlugin::error() {
	return errorMessage;
}

void LuaPlugin::frame() {
	#if ORYOL_EMSCRIPTEN
	lua_getglobal(this->lua, "frame");
	lua_pushnumber(this->lua, Clock::LapTime(this->tp).AsMilliSeconds());
	if (lua_pcall(this->lua, 1, 0, 0)) {
		Log::Error("Error: %s\n", lua_tostring(this->lua, -1));
		this->loadError = true;
		this->errorMessage = lua_tostring(this->lua, -1);
		lua_pop(this->lua, 1);
	}
	#else
	o_assert(this->thread);
	if (!this->thread->running()) {
		lua_getfield(this->lua, LUA_REGISTRYINDEX, "Emitted Functions");
		o_assert_dbg(lua_istable(this->lua, -1));
		bool play = lua_objlen(this->lua, -1) == 0;
		lua_pop(this->lua, 1);
		if (play)
			this->thread->play();
	}
	#endif
}

String LuaPlugin::getPath() {
	return path;
}

Array<LuaPlugin*> plugins;
std::mutex plugins_mutex;
