#pragma once

#include <Core/String/String.h>
using namespace Oryol;

#include <lua.hpp>

namespace Lua {
	namespace ImGui {
		/**
		 * @brief Load ImGui Lua bindings into the Lua instance.
		 * 
		 */
		void expose(lua_State *lua);
		/**
		 * 
		 * @brief Requests ownership of a window.
		 * 
		 * This function is used to request ownership of a window. An example
		 * call from Lua is `success, title = ImGui.requestWindow("Hello!")`.
		 * There is no guarantee that the returned title will be the one
		 * specified, and only the returned title can be used.
		 * 
		 * Plugins should only ever use the returned title. Anything else will cause a
		 * security error.
		 * 
		 * If success is false, the message will be in place of the title.
		 */
		int requestWindow(lua_State *lua);
		/**
		 * @brief This function checks if a window is owned by any plugins yet.
		 */
		bool isOwned(String window);
		/**
		 * @brief This function checks how many windows are already owned.
		 * @see ownedWindows(lua_State *);
		 */
		int ownedWindows();
		/**
		 * @brief This function checks how many windows a given plugins owns.
		 */
		int ownedWindows(lua_State *lua);
		/**
		 * @brief This function verifies that a plugin owns a window.
		 */
		bool verifyOwnership(lua_State *lua, String window);
		/**
		 * This function wraps around ImGui::Begin. It first ensures that the
		 * plugin has permission to use the specified window, then calls the
		 * ImGui::Begin function.
		 */
		int begin(lua_State *lua);
		/**
		 * This function wraps around ImGui::End.
		 */
		int end(lua_State *lua);
		/**
		 * This function wraps around ImGui::Text
		 */
		int text(lua_State *lua);
		extern int ends_required;
	}
}
