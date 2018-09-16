#include <Core/Main.h>
#include <Core/String/StringBuilder.h>
#include <Core/Time/Clock.h>
#include <Gfx/Gfx.h>
#include <Gfx/private/displayMgr.h>
#include <IMUI/IMUI.h>
#include <Input/Input.h>
#include <IO/IO.h>
#include <LocalFS/LocalFileSystem.h>
#if (ORYOL_D3D11 || ORYOL_METAL)
#elif (ORYOL_WINDOWS || ORYOL_MACOS || ORYOL_LINUX)
#include <GLFW/glfw3.h>
#include <glfw_icon.h>
#endif

#include <lua.hpp>
#include <LuaPlugin.hpp>

using namespace Oryol;

class Zany80 : public App {
public:
	AppState::Code OnInit();
	AppState::Code OnRunning();
	AppState::Code OnCleanup();
	void warn(const char *msg);
private:
	void ToggleFullscreen();
	bool fullscreen;
	glm::vec4 windowed_position;
	TimePoint tp;
};

OryolMain(Zany80);

void Zany80::ToggleFullscreen() {
#if (ORYOL_D3D11 || ORYOL_METAL)
	this->warn("Fullscreen mode is not supported on your platform.");
#elif (ORYOL_WINDOWS || ORYOL_MACOS || ORYOL_LINUX)
	this->fullscreen = !this->fullscreen;
	int monitor_count;
	GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);
	o_assert (monitor_count != 0);
	const GLFWvidmode* mode = glfwGetVideoMode(monitors[0]);
	if (this->fullscreen) {
		int width, height, xpos, ypos;
		glfwGetWindowSize(Oryol::_priv::glfwDisplayMgr::glfwWindow, &width, &height);
		glfwGetWindowPos(Oryol::_priv::glfwDisplayMgr::glfwWindow, &xpos, &ypos);
		windowed_position = glm::vec4(xpos, ypos, width, height);
		glfwSetWindowMonitor(Oryol::_priv::glfwDisplayMgr::glfwWindow, monitors[0], 0, 0, mode->width, mode->height, mode->refreshRate);
	}
	else {
		glfwSetWindowMonitor(Oryol::_priv::glfwDisplayMgr::glfwWindow, NULL, windowed_position.x, windowed_position.y, windowed_position.z, windowed_position.w, mode->refreshRate);
	}
#else
	this->warn("Fullscreen mode is not supported on your platform.");
#endif
}

AppState::Code Zany80::OnInit() {
	this->fullscreen = false;
	IOSetup ioSetup;
	ioSetup.FileSystems.Add("file", LocalFileSystem::Creator());
	IO::Setup(ioSetup);
	Gfx::Setup(GfxSetup::WindowMSAA4(800, 600, "Zany80 (Lua Edition)"));
#if (ORYOL_D3D11 || ORYOL_METAL)
#elif (ORYOL_WINDOWS || ORYOL_MACOS || ORYOL_LINUX)
	glfwSetWindowIcon(Oryol::_priv::glfwDisplayMgr::glfwWindow, 1, &icon);
#endif
#if ORYOL_LINUX
	if (IO::ResolveAssigns("root:") == "file:////usr/bin/")
		IO::SetAssign("root:", "/usr/share/zany80/");
	else if (IO::ResolveAssigns("root:") == "file:////usr/local/bin/")
		IO::SetAssign("root:", "/usr/local/share/zany80/");
	else {
		StringBuilder s = IO::ResolveAssigns("root:");
		for (int i = 0; i < 5; i++) {
			int j = s.FindLastOf(0, EndOfString, "/");
			o_assert(j != InvalidIndex);
			s.Set(s.GetSubString(0, j));
		}
		s.Append("/");
		IO::SetAssign("root:", s.GetString());
	}
#endif
	IO::SetAssign("plugins:", "root:plugins/");
	Log::Dbg("Application URL: %s\n", IO::ResolveAssigns("root:").AsCStr());
	Log::Dbg("Plugin URL: %s\n", IO::ResolveAssigns("plugins:").AsCStr());
	Input::Setup();
	IMUI::Setup();
	new LuaPlugin("test");
	return App::OnInit();
}

AppState::Code Zany80::OnRunning() {
	Gfx::BeginPass(PassAction::Clear(glm::vec4(0.1f, 0.8f, 0.6f, 1.0f)));
	IMUI::NewFrame(Clock::LapTime(this->tp));
	ImGui::Text("%d emitted functions", plugins[0]->countEmittedFunctions());
	//~ if (ImGui::Button("Execute?"))
		plugins[0]->executeEmittedFunctions();
	ImGui::Render();
	Gfx::EndPass();
	Gfx::CommitFrame();
	return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

AppState::Code Zany80::OnCleanup() {
	IMUI::Discard();
	Input::Discard();
	Gfx::Discard();
	IO::Discard();
	return App::OnCleanup();
}
