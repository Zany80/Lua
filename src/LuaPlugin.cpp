#include <LuaPlugin.hpp>
#include <LuaImGuiBindings.hpp>

#include <IMUI/IMUI.h>

LuaPlugin::LuaPlugin(String path) {
	this->lua = luaL_newstate();
	o_assert(this->lua);
	ImGui::Lua::expose(this->lua);
	plugins.Add(this);
}

LuaPlugin::~LuaPlugin() {
	o_assert(this->lua);
	lua_close(this->lua);
	this->lua = nullptr;
	int index = plugins.FindIndexLinear(this);
	o_assert(index != InvalidIndex);
	plugins.Erase(index);
}

int LuaPlugin::countEmittedFunctions() {
	o_assert(this->lua);
	lua_getfield(this->lua, LUA_REGISTRYINDEX, "Emitted Functions");
	o_assert(lua_istable(lua, -1));
	int count = lua_objlen(lua, -1);
	lua_pop(lua, 1);
	return count;
}

void LuaPlugin::executeEmittedFunctions() {
	o_assert(this->lua);
	lua_getfield(this->lua, LUA_REGISTRYINDEX, "Emitted Functions");
	o_assert(lua_istable(lua, -1));
	for (size_t i = 1; i <= lua_objlen(lua, -1);i++) {
		lua_rawgeti(lua, -1, i);
		o_assert(lua_isfunction(lua, -1));
		lua_call(lua, 0, 0);
	}
	for (size_t i = 1; i <= lua_objlen(lua, -1);i++) {
		lua_pushnil(lua);
		lua_rawseti(lua, -2, i);
	}
	lua_pop(lua, 1);
}

Array<LuaPlugin*> plugins;
