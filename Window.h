#pragma once
#include "dx11rg_common.h"

#define	WINDOW_CLASS_NAME	L"Quake 2"

void InitWindowClass(WNDPROC winProc, HINSTANCE hInstance);
void ReleaseWindowClass();
bool InitWindow(LPCWSTR name, int width, int height, int left, int top, bool fullscreen);
void ResizeWindow(int width, int height, int left, int top, bool fullscreen);
void ReleaseWindow();
void SetTitle(const std::string& title);

LRESULT CALLBACK ProcessMessagesDebug(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);