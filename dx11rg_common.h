#pragma once
#include "DX11RenderEngine/DX11RenderEngine/pch.h"

#ifndef _WIN32
#  error You should not be including this file on this platform
#endif

struct WinState{
	HINSTANCE hInstance;
	WNDPROC   wndproc;
	HWND      hWnd;			// handle to window

	size_t width;
	size_t height;
	size_t left;
	size_t top;
	bool fullscreen;
};

extern WinState windowState;