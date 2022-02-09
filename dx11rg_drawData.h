#pragma once

#include "dx11rg_local.h"
#include "dx11rg_common.h"

struct ViewData {
	vec3_t forward;
	vec3_t right;
	vec3_t up;

	matrix ViewMatrix;
	matrix ViewMatrixInv;
	matrix ProjectionMatrix;
	matrix ProjectionMatrixInv;
	matrix ViewProjectionMatrix;
	matrix ViewProjectionMatrixInv;
};

extern ViewData currentView;

extern model_t* r_worldmodel;

extern float		gldepthmin, gldepthmax;

extern image_t* r_notexture;		// use for bad textures
extern image_t* r_particletexture;	// little dot for particles

extern entity_t* currententity;
extern model_t* currentmodel;

extern cplane_t	frustum[4];

extern int			r_visframecount;	// bumped when going to a new PVS
extern int			r_framecount;		// used for dlight push checking

extern int			c_brush_polys, c_alias_polys;

extern float		v_blend[4];			// final blending color

extern void GL_Strings_f(void);

//
// view origin
//
extern vec3_t	vup;
extern vec3_t	vpn;
extern vec3_t	vright;
extern vec3_t	r_origin;

extern float	r_world_matrix[16];
extern float	r_base_world_matrix[16];

//
// screen size info
//
extern refdef_t	r_newrefdef;

extern int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern cvar_t* r_norefresh;
extern cvar_t* r_drawentities;
extern cvar_t* r_drawworld;
extern cvar_t* r_speeds;
extern cvar_t* r_fullbright;
extern cvar_t* r_novis;
extern cvar_t* r_nocull;
extern cvar_t* r_lerpmodels;
extern cvar_t* r_lefthand;

extern cvar_t* r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern cvar_t* gl_skymip;
extern cvar_t* gl_picmip;


extern cvar_t* dx11_mode;

extern cvar_t* vid_fullscreen;
extern cvar_t* vid_gamma;
extern cvar_t* vid_ref;

//dx11config_t dx11_config;
//dx11state_t  dx11_state;


extern cvar_t* gl_round_down;


const float Pi = 3.14159265359f;

inline float DegToRad(float deg) {
	return deg / 180.0f * Pi;
}



void R_RenderFrame(refdef_t* fd);


void R_SetupFrame(void);
void R_PushDlights(void);
void R_SetupDX(void);
void R_SetFrustum(void);
void R_SetupDX(void);

void R_DrawEntitiesOnList(void);
/* Models drawing */
void R_DrawAliasModel(entity_t* e);