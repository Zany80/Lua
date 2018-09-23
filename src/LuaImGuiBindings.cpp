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
#include <mutex>

#include <LuaPlugin.hpp>

using namespace Oryol;

namespace Lua {
	
	namespace ImGui {
		
		#define ImGui ::ImGui
		
		Map<const lua_State *,String> windows{};
		
		bool verifyOwnership(lua_State *lua, String window) {
			bool found = false;
			std::for_each(windows.begin(), windows.end(), [window, &found, lua](KeyValuePair<const lua_State *, String> pair) {
				if (pair.key == lua && pair.value == window) {
					found = true;
					return;
				}
			});
			if (!found) {
				lua_getfield(lua, LUA_REGISTRYINDEX, "Plugin");
				LuaPlugin *plugin = (LuaPlugin*)lua_touserdata(lua, -1);
				lua_pop(lua, 1);
				lua = plugin->lua;
				std::for_each(windows.begin(), windows.end(), [window, &found, lua](KeyValuePair<const lua_State *, String> pair) {
					if (pair.key == lua && pair.value == window) {
						found = true;
						return;
					}
				});
			}
			return found;
		}
		
		bool isOwned(String window) {
			bool found;
			std::for_each(windows.begin(), windows.end(), [window, &found](KeyValuePair<const lua_State *, String> pair) {
				if (pair.value == window) {
					found = true;
					return;
				}
			});
			return found;
		}
		
		int requestWindow(lua_State *lua) {
			if (lua_gettop(lua) != 1) {
				return luaL_error(lua, "Invalid number of arguments: %d", lua_gettop(lua));
			}
			if (!lua_isstring(lua, 1)) {
				return luaL_error(lua, "Argument to ImGui.RequestWindow must be a string!");
			}
			lua_getglobal(lua, "get_config");
			if (lua_pcall(lua, 0, 1, 0)) {
				return luaL_error(lua, "Plugin's name must be a string!");
			}
			if (!lua_istable(lua, -1)) {
				return luaL_error(lua, "Configuration must be a table!");
			}
			lua_getfield(lua, -1, "name");
			if (!lua_isstring(lua, -1)) {
				return luaL_error(lua, "Configuration must contain a string \"name\"!");
			}
			char buf[128];
			sprintf(buf, "%s (%s)##0x%016llX", lua_tostring(lua, 1), lua_tostring(lua, -1), (unsigned long long)lua);
			String window = buf;
			if (isOwned(window)) {
				lua_pushboolean(lua, false);
				lua_pushstring(lua, "No need to call this function twice!");
			}
			else {
				windows.Add(lua, window);
				Log::Dbg("Window %s claimed.\n", window.AsCStr());
				lua_pushboolean(lua, true);
				lua_pushstring(lua, window.AsCStr());
			}
			return 2;
		}
		
		int ends_required = 0;
		
		int begin(lua_State *lua) {
			if (!lua_isstring(lua, 1)) {
				return luaL_error(lua, "Argument 1 to ImGui.begin must be a string!");
			}
			// TODO: automatically grant access to the fallback Lua instance and re-enable this
			//~ if (!verifyOwnership(lua, lua_tostring(lua, 1))) {
				//~ return luaL_error(lua, "Attempted to draw to other's window.");
			//~ }
			Log::Dbg("Began %s\n", lua_tostring(lua, 1));
			ImGui::Begin(lua_tostring(lua,1));
			ends_required++;
			return 0;
		}
		
		int end(lua_State *lua) {
			if (ends_required == 0)
				return 0;
			ends_required--;
			Log::Dbg("Ended.\n");
			ImGui::End();
			return 0;
		}
		
		int text(lua_State *lua) {
			lua_getglobal(lua, "string");
			o_assert(lua_istable(lua, -1));
			lua_getfield(lua, -1, "format");
			o_assert(lua_isfunction(lua, -1));
			// Move string.format to the bottom of the stack
			lua_insert(lua, 1);
			// Pop string table from stack
			lua_pop(lua, 1);
			// Stack now contains string.format at the bottom and all args above it in order
			lua_call(lua, lua_gettop(lua) - 1, 1);
			// Only thing still on stack is formatted text
			Log::Dbg("Text: %s\n", lua_tostring(lua, 1));
			ImGui::Text("%s",lua_tostring(lua, 1));
			return 0;
		}
		
		void expose(lua_State *lua) {
			lua_settop(lua, 0);
			// Create the ImGui table
			lua_newtable(lua);
			// Pop it from the stack and name it ImGui
			lua_setfield(lua, LUA_GLOBALSINDEX, "ImGui");
			// Put it back on the stack so we can work with it
			lua_getglobal(lua, "ImGui");
			lua_pushcfunction(lua, begin);
			lua_setfield(lua, 1, "Begin");
			lua_pushcfunction(lua, end);
			lua_setfield(lua, 1, "End");
			lua_pushcfunction(lua, requestWindow);
			lua_setfield(lua, 1, "RequestWindow");
			lua_pushcfunction(lua, text);
			lua_setfield(lua, 1, "Text");
			// Pop ImGui from the stack
			lua_pop(lua, 1);
		}
		
		#undef ImGui
		
	}
}
