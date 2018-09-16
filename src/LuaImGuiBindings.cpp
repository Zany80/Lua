#include <LuaImGuiBindings.hpp>

#include <lua.hpp>

#include <Core/Core.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/Map.h>
#include <Core/String/String.h>
#include <Core/String/StringBuilder.h>
#include <Core/Time/Clock.h>

#include <IMUI/IMUI.h>

#include <stdlib.h>

using namespace Oryol;

namespace ImGui {
	
	namespace Lua {
		
		//~ bool verifyOwnership(lua_State *lua, String window) {
			//~ bool found = false;
			//~ std::for_each(windows.begin(), windows.end(), [window, &found, lua](KeyValuePair<const lua_State *, String> pair) {
				//~ if (pair.key == lua && pair.value == window) {
					//~ found = true;
					//~ return;
				//~ }
			//~ });
			//~ return found;
		//~ }
		
		//~ int requestWindow(lua_State *lua) {
			//~ if (lua_gettop(lua) != 1) {
				//~ return luaL_error(lua, "Invalid number of arguments: %d", lua_gettop(lua));
			//~ }
			//~ if (!lua_isstring(lua, 1)) {
				//~ return luaL_error(lua, "Argument to ImGui.requestWindow must be a string!");
			//~ }
			//~ StringBuilder window = lua_tostring(lua, 1);
			//~ lua_getglobal(lua, "get_name");
			//~ lua_call(lua, 0, 1);
			//~ if (!lua_isstring(lua, -1)) {
				//~ return luaL_error(lua, "Plugin's name must be a string!");
			//~ }
			//~ window.Append(" (");
			//~ window.Append(lua_tostring(lua, -1));
			//~ window.Append(")");
			//~ lua_pop(lua, 1);
			//~ if (isOwned(window.GetString())) {
				//~ lua_pushboolean(lua, false);
				//~ lua_pushstring(lua, "Window already claimed!");
			//~ }
			//~ else {
				//~ windows.Add(lua, window.GetString());
				//~ Log::Dbg("Window %s claimed.\n", window.GetString().AsCStr());
				//~ lua_pushboolean(lua, true);
				//~ lua_pushstring(lua, window.GetString().AsCStr());
			//~ }
			//~ return 2;
		//~ }
		
		int begin(lua_State *lua) {
			if (!lua_isstring(lua, 1)) {
				return luaL_error(lua, "Argument 1 to ImGui.begin must be a string!");
			}
			//~ if (!verifyOwnership(lua, lua_tostring(lua, 1))) {
				//~ return luaL_error(lua, "You don't own that window!");
			//~ }
			Log::Dbg("Began %s\n", lua_tostring(lua, 1));
			ImGui::Begin(lua_tostring(lua,1));
			return 0;
		}
		
		int end(lua_State *lua) {
			ImGui::End();
			return 0;
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
		
		int emit(lua_State *lua) {
			if (lua_gettop(lua) != 1 || !lua_isfunction(lua, 1))
				return luaL_error(lua, "Usage: function:emit()");
			lua_getfield(lua, LUA_REGISTRYINDEX, "Emitted Functions");
			o_assert(lua_istable(lua, 2));
			lua_insert(lua, 1);
			o_assert(lua_isfunction(lua, 2));
			o_assert(lua_istable(lua, 1));
			lua_rawseti(lua, 1, lua_objlen(lua, 1) + 1);
			return 0;
		}
		
		void expose(lua_State *lua) {
			lua_settop(lua, 0);
			// Create the ImGui table
			lua_newtable(lua);
			// Pop it from the stack and name it ImGui
			lua_setglobal(lua, "ImGui");
			// Put it back on the stack so we can work with it
			lua_getglobal(lua, "ImGui");
			lua_pushcfunction(lua, begin);
			lua_setfield(lua, 1, "Begin");
			lua_pushcfunction(lua, end);
			lua_setfield(lua, 1, "End");
			// Pop ImGui from the stack
			lua_pop(lua, 1);
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
			o_assert(!luaL_dostring(lua, "ImGui.Begin:bind(\"Hello\"):emit()\nImGui.End:emit()"));
		}
		
	}
}
