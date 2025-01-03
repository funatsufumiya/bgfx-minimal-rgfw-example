/*
 * Copyright 2011-2019 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */
#include <stdio.h>
#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#define RGFW_IMPLEMENTATION
#define RGFW_NO_API
#define RGFWDEF
#include "RGFW.h"

#include "logo.h"
#include <iostream>

static bool s_showStats = false;

void rgfw_keyCallback(RGFW_window* win, u32 key , u32 keyChar , char keyName[16], u8 lockState, u8 pressed) {
    if (key == RGFW_Escape && pressed) {
        RGFW_window_setShouldClose(win);
    } else if (key == RGFW_F1 && pressed) {
		s_showStats = !s_showStats;
	}
}

static bool shouldResize = false;
int newWidth, newHeight;

void rgfw_resizeCallback(RGFW_window* win, RGFW_rect rect) {
	shouldResize = true;
	newWidth = rect.w;
	newHeight = rect.h;
}

int main(int argc, char **argv)
{
	// RGFW_window* window = RGFW_createWindow("helloworld", RGFW_RECT(0, 0, 1024, 768), (u16)(RGFW_CENTER | RGFW_NO_RESIZE));
	RGFW_window* window = RGFW_createWindow("helloworld", RGFW_RECT(0, 0, 1024, 768), (u16)(RGFW_CENTER));

	if (!window) {
		return 1;
	}

	RGFW_setKeyCallback(rgfw_keyCallback);
	RGFW_setWindowResizeCallback(rgfw_resizeCallback);

	// Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
	// Most graphics APIs must be used on the same thread that created the window.
	bgfx::renderFrame();

	// Initialize bgfx using the native window handle and window resolution.
	bgfx::Init init;

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
	init.platformData.ndt = window->src.display;
	init.platformData.nwh = (void*)window->src.window;
#elif BX_PLATFORM_OSX
	init.platformData.nwh = window->src.window;
#elif BX_PLATFORM_WINDOWS
	init.platformData.nwh = window->src.window;
#endif

	int width = 1024;
	int height = 768;
	init.resolution.width = (uint32_t)width;
	init.resolution.height = (uint32_t)height;
	init.resolution.reset = BGFX_RESET_VSYNC;
	if (!bgfx::init(init))
		return 1;
	// Set view 0 to the same dimensions as the window and to clear the color buffer.
	const bgfx::ViewId kClearView = 0;
	bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR);
	bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);

	while (!RGFW_window_shouldClose(window)) {
		while(RGFW_window_checkEvent(window)) {
		}

		// Handle window resize.
		if (shouldResize) {
			shouldResize = false;
			bgfx::reset((uint32_t)newWidth, (uint32_t)newHeight, BGFX_RESET_VSYNC);
			bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
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
		bgfx::setDebug(s_showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
		// Advance to next frame. Process submitted rendering primitives.
		bgfx::frame();
	}

	bgfx::shutdown();
	RGFW_window_close(window);

	return 0;
}
