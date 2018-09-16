#pragma once

#include <Core/Containers/Array.h>
#include <Core/String/String.h>
#include <lua.hpp>

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
	int countEmittedFunctions();
	void executeEmittedFunctions();
private:
	lua_State *lua;
};

extern Array<LuaPlugin*> plugins; 
