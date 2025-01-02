/*
 * Copyright 2011-2019 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */
#include <stdio.h>
#include <bx/bx.h>
#include <bx/spscqueue.h>
#include <bx/thread.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#define RGFW_IMPLEMENTATION
#define RGFW_NO_API
#define RGFWDEF
extern "C" {
#include "RGFW.h"
}

#include "logo.h"

static bx::DefaultAllocator s_allocator;
static bx::SpScUnboundedQueue s_apiThreadEvents(&s_allocator);

enum class EventType
{
	Exit,
	Key,
	Resize
};

struct ExitEvent
{
	EventType type = EventType::Exit;
};

struct KeyEvent
{
	EventType type = EventType::Key;
	int key;
	u8 pressed;
};

struct ResizeEvent
{
	EventType type = EventType::Resize;
	uint32_t width;
	uint32_t height;
};

void rgfw_keyCallback(RGFW_window* win, u32 key , u32 keyChar , char keyName[16], u8 lockState, u8 pressed) {
	auto keyEvent = new KeyEvent;
	keyEvent->key = key;
	keyEvent->pressed = pressed;
	s_apiThreadEvents.push(keyEvent);
}

void rgfw_resizeCallback(RGFW_window* win, RGFW_rect rect) {
	auto resizeEvent = new ResizeEvent;
	resizeEvent->width = rect.w;
	resizeEvent->height = rect.h;
	s_apiThreadEvents.push(resizeEvent);
}

struct ApiThreadArgs
{
	bgfx::PlatformData platformData;
	uint32_t width;
	uint32_t height;
};

static int32_t runApiThread(bx::Thread *self, void *userData)
{
	auto args = (ApiThreadArgs *)userData;
	// Initialize bgfx using the native window handle and window resolution.
	bgfx::Init init;
	init.platformData = args->platformData;
	init.resolution.width = args->width;
	init.resolution.height = args->height;
	init.resolution.reset = BGFX_RESET_VSYNC;
	if (!bgfx::init(init))
		return 1;
	// Set view 0 to the same dimensions as the window and to clear the color buffer.
	const bgfx::ViewId kClearView = 0;
	bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR);
	bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
	uint32_t width = args->width;
	uint32_t height = args->height;
	bool showStats = false;
	bool exit = false;
	while (!exit) {
		// Handle events from the main thread.
		while (auto ev = (EventType *)s_apiThreadEvents.pop()) {
			if (*ev == EventType::Key) {
				auto keyEvent = (KeyEvent *)ev;
				// if (keyEvent->key == GLFW_KEY_F1 && keyEvent->action == GLFW_RELEASE)
				// 	showStats = !showStats;
				if (keyEvent->key == RGFW_F1 && keyEvent->pressed) {
					showStats = !showStats;
				}
			}
			else if (*ev == EventType::Resize) {
				auto resizeEvent = (ResizeEvent *)ev;
				bgfx::reset(resizeEvent->width, resizeEvent->height, BGFX_RESET_VSYNC);
				bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
				width = resizeEvent->width;
				height = resizeEvent->height;
			} else if (*ev == EventType::Exit) {
				exit = true;
			}
			delete ev;
		}
		// This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view 0.
		bgfx::touch(kClearView);
		// Use debug font to print information about this example.
		bgfx::dbgTextClear();
		bgfx::dbgTextImage(bx::max<uint16_t>(uint16_t(width / 2 / 8), 20) - 20, bx::max<uint16_t>(uint16_t(height / 2 / 16), 6) - 6, 40, 12, s_logo, 160);
		bgfx::dbgTextPrintf(0, 0, 0x0f, "Press F1 to toggle stats.");
		bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");
		bgfx::dbgTextPrintf(80, 1, 0x0f, "\x1b[;0m    \x1b[;1m    \x1b[; 2m    \x1b[; 3m    \x1b[; 4m    \x1b[; 5m    \x1b[; 6m    \x1b[; 7m    \x1b[0m");
		bgfx::dbgTextPrintf(80, 2, 0x0f, "\x1b[;8m    \x1b[;9m    \x1b[;10m    \x1b[;11m    \x1b[;12m    \x1b[;13m    \x1b[;14m    \x1b[;15m    \x1b[0m");
		const bgfx::Stats* stats = bgfx::getStats();
		bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters.", stats->width, stats->height, stats->textWidth, stats->textHeight);
		// Enable stats or debug text.
		bgfx::setDebug(showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
		// Advance to next frame. Main thread will be kicked to process submitted rendering primitives.
		bgfx::frame();
	}
	bgfx::shutdown();
	return 0;
}

int main(int argc, char **argv)
{
	// RGFW_window* window = RGFW_createWindow("helloworld multithreaded", RGFW_RECT(0, 0, 1024, 768), (u16)(RGFW_CENTER | RGFW_NO_RESIZE));
	RGFW_window* window = RGFW_createWindow("helloworld multithreaded", RGFW_RECT(0, 0, 1024, 768), (u16)(RGFW_CENTER));

	if (!window) {
		return 1;
	}

	RGFW_setKeyCallback(rgfw_keyCallback);
	RGFW_setWindowResizeCallback(rgfw_resizeCallback);

	// Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
	// Most graphics APIs must be used on the same thread that created the window.
	bgfx::renderFrame();
	// Create a thread to call the bgfx API from (except bgfx::renderFrame).
	ApiThreadArgs apiThreadArgs;

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
	apiThreadArgs.platformData.ndt = window->src.display;
	apiThreadArgs.platformData.nwh = (void*)window->src.window;
#elif BX_PLATFORM_OSX
	apiThreadArgs.platformData.nwh = window->src.window;
#elif BX_PLATFORM_WINDOWS
	apiThreadArgs.platformData.nwh = window->src.window;
#endif

	int width = 1024;
	int height = 768;
	apiThreadArgs.width = (uint32_t)width;
	apiThreadArgs.height = (uint32_t)height;
	// apiThreadArgs.reset = BGFX_RESET_VSYNC;

	bx::Thread apiThread;
	apiThread.init(runApiThread, &apiThreadArgs);
	// Run GLFW message pump.
	bool exit = false;
	while (!exit) {
		while(RGFW_window_checkEvent(window)) {
		}
		// Send window close event to the API thread.
		if (RGFW_window_shouldClose(window)) {
			s_apiThreadEvents.push(new ExitEvent);
			exit = true;
		}
		// Wait for the API thread to call bgfx::frame, then process submitted rendering primitives.
		bgfx::renderFrame();
	}
	// Wait for the API thread to finish before shutting down.
	while (bgfx::RenderFrame::NoContext != bgfx::renderFrame()) {}
	apiThread.shutdown();
	RGFW_window_close(window);
	return apiThread.getExitCode();
}
