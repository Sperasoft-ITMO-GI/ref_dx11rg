#pragma once
#include "winHandler.h"

#ifndef _WIN32
#  error You should not be including this file on this platform
#endif

struct WinState{
	HINSTANCE hInstance;
	WNDPROC   wndproc;
	HWND      hWnd1;			// handle to window
	HWND      hWnd2;			// handle to window

	size_t width;
	size_t height;
	size_t left;
	size_t top;
	bool fullscreen;
};

extern WinState windowState;