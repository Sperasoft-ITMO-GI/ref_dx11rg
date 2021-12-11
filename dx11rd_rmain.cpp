// dx11r_main.c

#include "dx11rg_local.h"

viddef_t	vid;

refimport_t	ri;

cvar_t* dx11_mode;
cvar_t* vid_fullscreen;
cvar_t* vid_gamma;
cvar_t* vid_ref;



// reading param from config.txt
void R_Register(void) {
	
}

bool R_SetMode(int* width, int* height) {

	return true;
}

/*
===============
R_Init
===============
*/
qboolean R_Init(void* hinstance, void* hWnd) {
	return True;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown(void) {

}

// ==================================================================================================================================
/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration(char* model) {
	return;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/
struct model_s* R_RegisterModel(char* name) {
	return NULL;
}

/*
===============
R_RegisterSkin
===============
*/
struct image_s* R_RegisterSkin(char* name) {
	return NULL;
}

/*
=============
Draw_FindPic
=============
*/
image_t* R_RegisterPic(char* name) {

	return NULL;
}

/*
============
R_SetSky
============
*/
// 3dstudio environment map names
char* suf[6] = { (char*)"rt", (char*)"bk", (char*)"lf", (char*)"ft", (char*)"up", (char*)"dn" };
void R_SetSky(char* name, float rotate, vec3_t axis) {
	return;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration(void) {
	return;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame(refdef_t* fd) {
	return;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize(int* w, int* h, char* pic) {
	return;
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, char* pic) {
	return;
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic(int x, int y, int w, int h, char* pic) {
	return;
}

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char(int x, int y, int num) {
	return;
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h, char* pic) {
	return;
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c) {
	
}

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void) {
	return;
}

/*
=============
Draw_StretchRaw
=============
*/
extern unsigned	r_rawpalette[256];

void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte* data) {
	return;
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette(const unsigned char* palette) {
	return;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame(float camera_separation) {

	return;
}

/*
** GLimp_EndFrame
**
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame(void) {
	return;
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate(qboolean active) {
	return;
}

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/

refexport_t GetRefAPI(refimport_t rimp) {
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = R_RegisterPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen = Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.CinematicSetPalette = R_SetPalette;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;

	re.AppActivate = GLimp_AppActivate;

	Swap_Init();

	return re;
}

#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error(char* error, ...) {
	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	ri.Sys_Error(ERR_FATAL, (char*)"%s", text);
}

void Com_Printf(char* fmt, ...) {
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

	ri.Con_Printf(PRINT_ALL, (char*)"%s", text);
}

#endif