#include <Core/Main.h>
#include <Core/String/StringBuilder.h>
#include <Core/Time/Clock.h>
#include <Gfx/Gfx.h>
#include <Gfx/private/displayMgr.h>
#include <IMUI/IMUI.h>
#include <Input/Input.h>
#include <IO/IO.h>
#if ORYOL_EMSCRIPTEN
#include <HttpFS/HTTPFileSystem.h>
#else
#include <LocalFS/LocalFileSystem.h>
#endif
#if (ORYOL_D3D11 || ORYOL_METAL)
#elif (ORYOL_WINDOWS || ORYOL_MACOS || ORYOL_LINUX)
#include <GLFW/glfw3.h>
#include <glfw_icon.h>
#endif

#include <thread>
#include <chrono>

#include <lua.hpp>
#include <LuaPlugin.hpp>
#include <LuaImGuiBindings.hpp>
#include <config.h>

using namespace Oryol;

class Zany80 : public App {
public:
	AppState::Code OnInit();
	AppState::Code OnRunning();
	AppState::Code OnCleanup();
	//void warn(const char *msg);
private:
	void ToggleFullscreen();
	bool fullscreen;
	glm::vec4 windowed_position;
	TimePoint tp;
};

OryolMain(Zany80);

void Zany80::ToggleFullscreen() {
#if (ORYOL_D3D11 || ORYOL_METAL)
	//this->warn("Fullscreen mode is not supported on your platform.");
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
	//this->warn("Fullscreen mode is not supported on your platform.");
#endif
}

AppState::Code Zany80::OnInit() {
	this->fullscreen = false;
	IOSetup ioSetup;
#if ORYOL_EMSCRIPTEN
	ioSetup.FileSystems.Add("https", HTTPFileSystem::Creator());
#else
	ioSetup.FileSystems.Add("file", LocalFileSystem::Creator());
#endif
	IO::Setup(ioSetup);
	Gfx::Setup(GfxSetup::WindowMSAA4(800, 600, "Zany80 (Lua Edition) v" PROJECT_VERSION));
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
#elif ORYOL_EMSCRIPTEN
#ifdef FIPS_EMSCRIPTEN_USE_WASM
	IO::SetAssign("root:", "https://zany80.github.io/lua/wasm/");
#else
	IO::SetAssign("root:", "https://zany80.github.io/lua/emscripten/");
#endif
#endif
	IO::SetAssign("plugins:", "root:plugins/");
	Log::Dbg("Application URL: %s\n", IO::ResolveAssigns("root:").AsCStr());
	Log::Dbg("Plugin URL: %s\n", IO::ResolveAssigns("plugins:").AsCStr());
	Input::Setup();
	IMUI::Setup();
	this->tp = Clock::Now();
#if ORYOL_EMSCRIPTEN
// prevent massive slowdown via message spamming
	Log::SetLogLevel(Log::Level::Warn);
	// some http tests
	LuaPlugin::constructPlugin("plugins:NonExistent/Deliberate404");
	LuaPlugin::constructPlugin("http://google.com");
#endif
	LuaPlugin::constructPlugin("plugins:DynamicRecompiler/main.lua");
	return App::OnInit();
}

AppState::Code Zany80::OnRunning() {
	Gfx::BeginPass(PassAction::Clear(glm::vec4(0.1f, 0.8f, 0.6f, 1.0f)));
	IMUI::NewFrame(Clock::LapTime(this->tp));
	ImGui::Begin("Hub");
	ImGui::Text("Zany80 version: " PROJECT_VERSION);
	ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("%d plugins", plugins.Size());
	if (ImGui::Button("Toggle fullscreen"))
		this->ToggleFullscreen();
	ImGui::Text("Log level: %d", (int)Log::GetLogLevel());
	if(ImGui::Button("Mute"))
		Log::SetLogLevel(Log::Level::None);
	if(ImGui::Button("Error"))
		Log::SetLogLevel(Log::Level::Error);
	if(ImGui::Button("Warn"))
		Log::SetLogLevel(Log::Level::Warn);
	if(ImGui::Button("Info"))
		Log::SetLogLevel(Log::Level::Info);
	if(ImGui::Button("Debug"))
		Log::SetLogLevel(Log::Level::Dbg);
	ImGui::End();
	for (unsigned char i = 0; i < plugins.Size() && i < 255; i++) {
		char buf[16];
		sprintf(buf, "Plugin %d", i);
		ImGui::Begin(buf);
		ImGui::Text("Valid: %s", plugins[i] -> IsValid() ? "True" : "False");
		ImGui::Text("Path: %s", plugins[i] -> getPath().AsCStr());
		ImGui::Text("Missed frames: %d", plugins[i] -> getMissedFrames());
		if (plugins[i] -> IsValid()) {
			ImGui::Text("Emitted functions: %d", plugins[i]->countEmittedFunctions());
			for (int j = 1; j <= plugins[i]->countEmittedFunctions(); j++) {
				ImGui::Text("Function %d: %s", j, plugins[i] -> getEmittedFunctionName(j).AsCStr());
			}
		}
		else
			ImGui::Text("Error message: %s", plugins[i]->error().AsCStr());
		if (ImGui::Button("Reload")) {
			String path = plugins[i]->getPath();
			if (plugins[i]->IsValid())
				// clear the queue
				plugins[i]->executeEmittedFunctions();
			delete plugins[i];
			Log::Dbg("Resetting plugin %d out of %d\n", i + 1, plugins.Size() + 1);
			LuaPlugin::constructPlugin(path);
		}
		ImGui::End();
		if (plugins[i] -> IsValid())
			plugins[i]->executeEmittedFunctions();
	}
	if (Lua::ImGui::ends_required) {
		ImGui::Begin("Error!");
		ImGui::Text("Missed %d ImGui.End calls!\n", Lua::ImGui::ends_required);
		ImGui::End();
		Log::Warn("Missed %d ImGui.End calls!\n", Lua::ImGui::ends_required);
	}
	for (; Lua::ImGui::ends_required > 0; Lua::ImGui::ends_required--)
		ImGui::End();
	ImGui::Render();
	for (LuaPlugin *p : plugins)
		if (p->IsValid())
			p->frame();
	Gfx::EndPass();
	Gfx::CommitFrame();
	return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

AppState::Code Zany80::OnCleanup() {
	for (int i = 0; i < plugins.Size(); i++) {
		delete plugins[i];
	}
	plugins.Clear();
	IMUI::Discard();
	Input::Discard();
	Gfx::Discard();
	IO::Discard();
	return App::OnCleanup();
}
