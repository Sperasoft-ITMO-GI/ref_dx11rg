#pragma once
#include "dx11rg_local.h"
#include "Window.h"

cvar_t* vid_fullscreen;
cvar_t* dx11rg_mode;

RenderDevice RD;

void GLimp_AppActivate(qboolean active) {
	if (active) {
		SetForegroundWindow(windowState.hWnd);
		ShowWindow(windowState.hWnd, SW_RESTORE);
	} else {
		if (windowState.fullscreen)
			ShowWindow(windowState.hWnd, SW_MINIMIZE);
	}
}

qboolean R_Init(void* hinstance, void* winProc) {

	int width, height;
	cvar_t* vid_xpos, * vid_ypos;

	InitWindowClass((WNDPROC)winProc, (HINSTANCE)hinstance);

	dx11rg_mode = ri.Cvar_Get("dx11rg_mode", "3", CVAR_ARCHIVE);
	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);

	vid_fullscreen->modified = False;
	dx11rg_mode->modified = False;

	int mode = dx11rg_mode->value;
	bool fullscreen = vid_fullscreen->value;
	vid_xpos = ri.Cvar_Get("vid_xpos", "0", 0);
	vid_ypos = ri.Cvar_Get("vid_ypos", "0", 0);
	int x = vid_xpos->value;
	int y = vid_ypos->value;

	ri.Vid_GetModeInfo(&width, &height, mode);
	/*
	D3D11 ERROR: ID3D11DeviceContext::OMSetRenderTargets: 
	The RenderTargetView at slot 0 is not compatible with the DepthStencilView. 
	DepthStencilViews may only be used with RenderTargetViews if the effective dimensions of the Views are equal, as well as the Resource types, multisample count, and multisample quality. 
	The RenderTargetView at slot 0 has (w:1024,h:676,as:1), while the Resource is a Texture2D with (mc:1,mq:0). 
	The DepthStencilView has           (w:1024,h:576,as:1), while the Resource is a Texture2D with (mc:1,mq:0).  
	[ STATE_SETTING ERROR #388: OMSETRENDERTARGETS_INVALIDVIEW]
	*/

	InitWindow(L"Quake 2", width, height, x, y, fullscreen);

	RD.InitDevice(windowState.hWnd, windowState.width, windowState.height);
	
	{
		RenderDevice::ShaderData shD;
		shD.type = RenderDevice::ShaderType::Shader3D;
		shD.dataSize = ri.FS_LoadFile("Shader3D.hlsl", (void**)&shD.data);
	
		RD.ReloadShader(shD);
		ri.FS_FreeFile(shD.data);
	}
	{
		RenderDevice::ShaderData shD;
		shD.type = RenderDevice::ShaderType::Shader2D;
		shD.dataSize = ri.FS_LoadFile("Shader2D.hlsl", (void**)&shD.data);

		RD.ReloadShader(shD);
		ri.FS_FreeFile(shD.data);
	}
	TextureData text = TextureData(10, 10);
	RD.RegisterTexture(1, text);

	RM.InitImages();
	RM.InitLocal();

	return True;
};



void R_Shutdown(void) {
	RD.DestroyDevice();
	ReleaseWindow();
	ReleaseWindowClass();
};