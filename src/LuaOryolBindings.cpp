#include <LuaOryolBindings.hpp>
#include <Core/Core.h>

using namespace Oryol;

namespace Lua {
	
	namespace Oryol {
		
		#define Oryol ::Oryol
		
		void _sformat(lua_State *lua) {
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
			// Only the string remains.
		}
		
		void expose (lua_State *lua) {
			lua_newtable(lua);
			lua_pushcfunction(lua, [](lua_State *lua) -> int {
				_sformat(lua);
				Log::Error(lua_tostring(lua, -1));
				return 0;
			});
			lua_setfield(lua, -2, "Error");
			lua_pushcfunction(lua, [](lua_State *lua) -> int {
				_sformat(lua);
				Log::Dbg(lua_tostring(lua, -1));
				return 0;
			});
			lua_setfield(lua, -2, "Dbg");
			lua_setglobal(lua, "Log");
		}
		
		#undef Oryol
		
	}
}
