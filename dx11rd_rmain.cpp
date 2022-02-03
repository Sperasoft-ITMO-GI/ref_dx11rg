// dx11r_main.c

#include "dx11rg_local.h"

refimport_t	ri;




/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame(refdef_t* fd) {

	//if (r_norefresh->value)
	//	return;

	//r_newrefdef = *fd;

	//if (!r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	//	ri.Sys_Error(ERR_DROP, "R_RenderView: NULL worldmodel");
	//
	//if (r_speeds->value) {
	//	c_brush_polys = 0;
	//	c_alias_polys = 0;
	//}
	//
	//R_PushDlights();
	//
	//if (gl_finish->value)
	//	qglFinish();

	//R_SetupFrame();
	//
	//R_SetFrustum();
	//
	//R_SetupGL();
	//
	//R_MarkLeaves();	// done here so we know if we're in water
	//
	//R_DrawWorld();
	
	//R_DrawEntitiesOnList();
	//
	//R_RenderDlights();
	//
	//R_DrawParticles();
	//
	//R_DrawAlphaSurfaces();
	//
	//R_Flash();
	//
	//if (r_speeds->value) {
	//	ri.Con_Printf(PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
	//		c_brush_polys,
	//		c_alias_polys,
	//		c_visible_textures,
	//		c_visible_lightmaps);
	//}
	return;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame(float camera_separation) {
	RD.BeginFrame();

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
	RD.Clear(0,0, 0);
	
	RD.Present();

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