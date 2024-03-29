#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>

#include <stdio.h>

#include <math.h>

// specific things for Q2_Engine
#include "../client/ref.h"
#include "../win32/winquake.h"


//#include "RenderEngine.h"
#include "Window.h"
#include "RenderEngine.h"

extern RenderDevice RD;
//#include "dx11_init.h"


extern cvar_t* gl_skymip;
extern cvar_t* gl_picmip;

extern cvar_t* r_norefresh;
extern cvar_t* r_drawentities;
extern cvar_t* r_speeds;
extern cvar_t* r_drawworld;
extern cvar_t* r_fullbright;
extern cvar_t* r_novis;
extern cvar_t* r_nocull;
extern cvar_t* r_lerpmodels;
extern cvar_t* r_lefthand;
extern cvar_t* r_lightlevel;	// FIXME: This is a HACK to get the client's light level

								//#define	REF_VERSION	"DX11 0.01"

//unsigned	d_8to24table[256];




/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

void ReloadShaders();

qboolean 	R_Init(void* hinstance, void* hWnd);
void		R_Shutdown(void);
void GLimp_AppActivate(qboolean active);

void	Draw_GetPicSize(int* w, int* h, char* name);
void	Draw_Pic(int x, int y, char* name);
void	Draw_StretchPic(int x, int y, int w, int h, char* name);
void	Draw_Char(int x, int y, int c);
void	Draw_TileClear(int x, int y, int w, int h, char* name);
void	Draw_Fill(int x, int y, int w, int h, int c);
void	Draw_FadeScreen(void);
void	Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte* data);

void	R_BeginFrame(float camera_separation);
void	R_SetPalette(const unsigned char* palette);

struct image_s* R_RegisterSkin(char* name);

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;