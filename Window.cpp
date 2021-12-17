#pragma once
#include "dx11rg_local.h"
#include "Window.h"

// Window Class Stuff

WinState windowState;

void InitWindowClass(WNDPROC winProc, HINSTANCE hInstance) {
	windowState.hInstance = hInstance;
	windowState.wndproc = winProc;
	WNDCLASS		wc;

	/* Register the frame class */
	wc.style = 0;
	wc.lpfnWndProc = windowState.wndproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = windowState.hInstance;
	wc.hIcon = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = 0;
	wc.lpszClassName = WINDOW_CLASS_NAME;

	if (!RegisterClass(&wc)) {
		// Get the most recent error
		DWORD error = GetLastError();

		// If the class exists, that's actually fine.  Otherwise,
		// we can't proceed with the next step.
		if (error != ERROR_CLASS_ALREADY_EXISTS)
			ri.Sys_Error(ERR_FATAL, "Couldn't create window class");
	}
}

void ReleaseWindowClass() {
	UnregisterClass(WINDOW_CLASS_NAME, windowState.hInstance);
}


bool InitWindow(LPCWSTR name, int width, int height, int left, int top, bool fullscreen) {
	RECT			r;
	cvar_t* vid_xpos, * vid_ypos;
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;

	if (fullscreen) {
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE;
	} else {
		exstyle = 0;
		stylebits = WINDOW_STYLE;
	}

	r.left = left;
	r.top = top;
	r.right = left + width;
	r.bottom = left + height;

	AdjustWindowRect(&r, stylebits, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	if (fullscreen) {
		x = 0;
		y = 0;
	} else {
		x = top;
		y = left;
	}



	try {
		windowState.hWnd = CreateWindowEx(
			exstyle,
			WINDOW_CLASS_NAME,
			L"Quake 2",
			stylebits,
			x, y, w, h,
			NULL,
			NULL,
			windowState.hInstance,
			NULL);
	}
	catch (...) {

	}

	windowState.fullscreen = fullscreen;
	windowState.width = w;
	windowState.height = h;
	windowState.top = x;
	windowState.left = y;

	if (!windowState.hWnd)
		ri.Sys_Error(ERR_FATAL, "Couldn't create window");

	ShowWindow(windowState.hWnd, SW_SHOW);
	UpdateWindow(windowState.hWnd);


	SetForegroundWindow(windowState.hWnd);
	SetFocus(windowState.hWnd);

	// let the sound and input subsystems know about the new window




	ri.Vid_NewWindow(width, height);



	return True;
}

void ResizeWindow(int width, int height, int left, int top, bool fullscreen) {

	int				stylebits;
	int				exstyle;

	if (fullscreen) {
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE;
	} else {
		exstyle = 0;
		stylebits = WINDOW_STYLE;
	}

	RECT r;
	r.left = left;
	r.top = top;
	r.right = width + left;
	r.bottom = height + top;

	AdjustWindowRectEx(&r, stylebits, FALSE, exstyle);
	SetWindowPos(windowState.hWnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW);

	windowState.fullscreen = fullscreen;
	windowState.width = r.right - r.left;
	windowState.height = r.bottom - r.top;
	windowState.top = r.top;
	windowState.left = r.left;

}

void ReleaseWindow() {
	DestroyWindow(windowState.hWnd);
}

void SetTitle(const std::string& title) {
	SetWindowTextA(windowState.hWnd, title.c_str());
}