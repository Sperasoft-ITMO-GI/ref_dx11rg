#pragma once
#include "dx11rg_local.h"
#include "Window.h"

cvar_t* vid_fullscreen;
cvar_t* dx11rg_mode;

cvar_t* gl_skymip;
cvar_t* gl_picmip;


RenderDevice RD;

const char* compileHLSLComand = "Recompile";

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
	
	ReloadShaders();

	TextureData text = TextureData(10, 10);
	RD.RegisterTexture(1, text);

	RM.InitImages();
	RM.InitLocal();

	ri.Cmd_AddCommand("Recompile", ReloadShaders);



	r_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

	gl_picmip = ri.Cvar_Get("gl_picmip", "0", 0);
	gl_skymip = ri.Cvar_Get("gl_skymip", "0", 0);
	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);

	// init intensity conversions
	//intensity = ri.Cvar_Get("intensity", "2", 0);

	//if (intensity->value <= 1)
	//	ri.Cvar_Set("intensity", "1");

	inverse_intensity = 1 / 2.0;	

	return True;
};

void ReloadShaders() {
	printf("Shaders recompiled.\n");
	{
		RenderDevice::ShaderData shD;
		shD.type = RenderDevice::ShaderType::ModelsShader;
		shD.dataSize = ri.FS_LoadFile("ModelsShader.hlsl", (void**)&shD.data);
	
		RD.ReloadShader(shD);
		ri.FS_FreeFile(shD.data);
	}
	{
		RenderDevice::ShaderData shD;
		shD.type = RenderDevice::ShaderType::UIShader;
		shD.dataSize = ri.FS_LoadFile("UIShader.hlsl", (void**)&shD.data);

		RD.ReloadShader(shD);
		ri.FS_FreeFile(shD.data);
	}
	{
		RenderDevice::ShaderData shD;
		shD.type = RenderDevice::ShaderType::UPShader;
		shD.dataSize = ri.FS_LoadFile("BSPShader.hlsl", (void**)&shD.data);

		RD.ReloadShader(shD);
		ri.FS_FreeFile(shD.data);
	}
	{
		RenderDevice::ShaderData shD;
		shD.type = RenderDevice::ShaderType::ParticlesShader;
		shD.dataSize = ri.FS_LoadFile("ParticlesShader.hlsl", (void**)&shD.data);

		RD.ReloadShader(shD);
		ri.FS_FreeFile(shD.data);
	}
	{
		RenderDevice::ShaderData shD;
		shD.type = RenderDevice::ShaderType::SkyShader;
		shD.dataSize = ri.FS_LoadFile("SkyShader.hlsl", (void**)&shD.data);

		RD.ReloadShader(shD);
		ri.FS_FreeFile(shD.data);
	}
}

void R_Shutdown(void) {

	ri.Cmd_RemoveCommand("Recompile");
	RD.DestroyDevice();
	ReleaseWindow();
	ReleaseWindowClass();
};