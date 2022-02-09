#include "dx11rg_drawData.h"




model_t* r_worldmodel;

float		gldepthmin, gldepthmax;

image_t* r_notexture;		// use for bad textures
image_t* r_particletexture;	// little dot for particles

entity_t* currententity;


cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

float		v_blend[4];			// final blending color

void GL_Strings_f(void);

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t* r_norefresh;
cvar_t* r_drawentities;
cvar_t* r_drawworld;
cvar_t* r_speeds;
cvar_t* r_fullbright;
cvar_t* r_novis;
cvar_t* r_nocull;
cvar_t* r_lerpmodels;
cvar_t* r_lefthand;

cvar_t* r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t* gl_skymip;
cvar_t* gl_picmip;


cvar_t* dx11_mode;

cvar_t* vid_gamma;
cvar_t* vid_ref;

//dx11config_t dx11_config;
//dx11state_t  dx11_state;


cvar_t* gl_round_down;






/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame(refdef_t* fd) {

	//if (r_norefresh->value)
	//	return;

	r_newrefdef = *fd;

	//if (!r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	//	ri.Sys_Error(ERR_DROP, "R_RenderView: NULL worldmodel");
	
	//if (r_speeds->value) {
	//	c_brush_polys = 0;
	//	c_alias_polys = 0;
	//}
	
	//R_PushDlights();
	
	//if (gl_finish->value)
	//	qglFinish();

	R_SetupFrame();
	//
	R_SetFrustum();
	//
	R_SetupDX();
	//
	//R_MarkLeaves();	// done here so we know if we're in water
	//
	//R_DrawWorld();

	R_DrawEntitiesOnList();
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
	//RD.Present();
	return;
}



/*
=============
R_SetupFrame
=============
*/
void R_SetupFrame(void) {
	int i;
	//mleaf_t* leaf;

	r_framecount++;

	// build the transformation matrix for the given view angles
	VectorCopy(r_newrefdef.vieworg, r_origin);

	AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

	// current viewcluster
	//if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL)) {
	//	r_oldviewcluster = r_viewcluster;
	//	r_oldviewcluster2 = r_viewcluster2;
	//	leaf = Mod_PointInLeaf(r_origin, r_worldmodel);
	//	r_viewcluster = r_viewcluster2 = leaf->cluster;
	//
	//	// check above and below so crossing solid water doesn't draw wrong
	//	if (!leaf->contents) {	// look down a bit
	//		vec3_t	temp;
	//
	//		VectorCopy(r_origin, temp);
	//		temp[2] -= 16;
	//		leaf = Mod_PointInLeaf(temp, r_worldmodel);
	//		if (!(leaf->contents & CONTENTS_SOLID) &&
	//			(leaf->cluster != r_viewcluster2))
	//			r_viewcluster2 = leaf->cluster;
	//	}
	//	else {	// look up a bit
	//		vec3_t	temp;
	//
	//		VectorCopy(r_origin, temp);
	//		temp[2] += 16;
	//		leaf = Mod_PointInLeaf(temp, r_worldmodel);
	//		if (!(leaf->contents & CONTENTS_SOLID) &&
	//			(leaf->cluster != r_viewcluster2))
	//			r_viewcluster2 = leaf->cluster;
	//	}
	//}

	for (i = 0; i < 4; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) {
		//qglEnable(GL_SCISSOR_TEST);
		//qglClearColor(0.3, 0.3, 0.3, 1);
		//qglScissor(r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height);
		//qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//qglClearColor(1, 0, 0.5, 0.5);
		//qglDisable(GL_SCISSOR_TEST);
	}
}





/*
=============
R_PushDlights
=============
*/
void R_PushDlights(void) {
	//int		i;
	//dlight_t* l;
	//
	///*if (gl_flashblend->value)
	//	return;*/
	//
	//r_dlightframecount = r_framecount + 1;	// because the count hasn't
	//										//  advanced yet for this frame
	//l = r_newrefdef.dlights;
	//for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	//	R_MarkLights(l, 1 << i, r_worldmodel->nodes);
}

ViewData currentView;

void UpdateViewData() {
	using namespace DirectX;
	float AspectRatioXOverY = float(r_newrefdef.width) / float(r_newrefdef.height);

	const float4 Up = float4(0.0f, 0.0f, 1.0f, 0.0f);
	const float4 Pos = float4(r_newrefdef.vieworg[0], r_newrefdef.vieworg[1], r_newrefdef.vieworg[2], 1.0f);

	AngleVectors(r_newrefdef.viewangles, currentView.forward, currentView.right, currentView.up);
	const float4 LookAtPos = float4(r_newrefdef.vieworg[0] + currentView.forward[0], r_newrefdef.vieworg[1] + currentView.forward[1], r_newrefdef.vieworg[2] + currentView.forward[2], 1.0f);

#if 0
	View.ViewMatrix = XMMatrixLookAtLH(Pos, LookAtPos, Up); // missing space conversion
#else

	// Convert Q2 to D3d space
	float4x4 D3dMat0 = float4x4::CreateScale(1.0f, 1.0f, -1.0f);
	float4x4 D3dMat1 = float4x4::CreateRotationX(DegToRad(-90.0f));
	float4x4 D3dMat2 = float4x4::CreateRotationZ(DegToRad(90.0f));
	// Q2 camera transform
	float4x4 RotXMat = float4x4::CreateRotationX(DegToRad(-r_newrefdef.viewangles[2]));
	float4x4 RotYMat = float4x4::CreateRotationY(DegToRad(-r_newrefdef.viewangles[0]));
	float4x4 RotZMat = float4x4::CreateRotationZ(DegToRad(-r_newrefdef.viewangles[1]));
	float4x4 TranMat = float4x4::CreateTranslation(-r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

	currentView.ViewMatrix = XMMatrixMultiply(D3dMat1, D3dMat0);
	currentView.ViewMatrix = XMMatrixMultiply(D3dMat2, currentView.ViewMatrix);
	currentView.ViewMatrix = XMMatrixMultiply(RotXMat, currentView.ViewMatrix);
	currentView.ViewMatrix = XMMatrixMultiply(RotYMat, currentView.ViewMatrix);
	currentView.ViewMatrix = XMMatrixMultiply(RotZMat, currentView.ViewMatrix);
	currentView.ViewMatrix = XMMatrixMultiply(TranMat, currentView.ViewMatrix);
#endif

	float4 ViewViewMatrixDet = XMMatrixDeterminant(currentView.ViewMatrix);
	currentView.ViewMatrixInv = currentView.ViewMatrix.Invert();

	currentView.ProjectionMatrix = float4x4::CreatePerspectiveFieldOfView(DegToRad(r_newrefdef.fov_y), AspectRatioXOverY, 0.1f, 20000.0f);
	float4 ViewProjectionMatrixDet = XMMatrixDeterminant(currentView.ProjectionMatrix);
	currentView.ProjectionMatrixInv = currentView.ProjectionMatrix.Invert();


	currentView.ViewProjectionMatrix = XMMatrixMultiply(currentView.ViewMatrix, currentView.ProjectionMatrix);
	float4 ViewViewProjectionMatrixDet = XMMatrixDeterminant(currentView.ViewProjectionMatrix);
	currentView.ViewProjectionMatrixInv = currentView.ViewProjectionMatrix.Invert();

}
/*
=============
R_SetupDX
=============
*/
void R_SetupDX(void) {

	UpdateViewData();

	RD.SetPerspectiveMatrix(currentView.ProjectionMatrix.Transpose());
	RD.SetViewMatrix(currentView.ViewMatrix.Transpose()	);
	// Тут каждый раз устанавливается новый вьюпорт
	// Не ясно, нужно ли это делать

	//
	// set up viewport
	//
	//x = floor(r_newrefdef.x * windowState.width / windowState.width);
	//x2 = ceil((r_newrefdef.x + r_newrefdef.width) * windowState.width / windowState.width);
	//y = floor(windowState.height - r_newrefdef.y * windowState.height / windowState.height);
	//y2 = ceil(windowState.height - (r_newrefdef.y + r_newrefdef.height) * windowState.height / windowState.height);
	//
	//w = x2 - x;
	//h = y - y2;

	//qglViewport(x, y2, w, h);

	////
	//// set up projection matrix
	////

	//screenaspect = (float)r_newrefdef.width / r_newrefdef.height;

	//qglMatrixMode(GL_PROJECTION);
	//qglLoadIdentity();
	//MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4, 4096);

	//qglCullFace(GL_FRONT);

	// Тут считаем матрицу моделвью

	//qglMatrixMode(GL_MODELVIEW);
	//qglLoadIdentity();

	// id's system:
	//	- X axis = Left/Right
	//	- Y axis = Forward/Backward
	//	- Z axis = Up/Down
	//
	// DirectX coordinate system:
	//	- X axis = Left/Right
	//	- Y axis = Up/Down
	//	- Z axis = Forward/Backward

	// vieworg - postition
	// viewangles - point view

	using namespace DirectX;
	XMMATRIX model_view = XMMatrixIdentity();
	/*model_view *= XMMatrixRotationX(XMConvertToRadians(-90.0f));
	model_view *= XMMatrixRotationZ(XMConvertToRadians(90.0f));

	model_view *= XMMatrixRotationX(XMConvertToRadians(-r_newrefdef.viewangles[2]));
	model_view *= XMMatrixRotationY(XMConvertToRadians(-r_newrefdef.viewangles[0]));
	model_view *= XMMatrixRotationZ(XMConvertToRadians(-r_newrefdef.viewangles[1]));

	model_view *= XMMatrixTranslation(-r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);*/


	model_view *= XMMatrixTranslation(-r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

	model_view *= XMMatrixRotationZ(XMConvertToRadians(-r_newrefdef.viewangles[1]));
	model_view *= XMMatrixRotationY(XMConvertToRadians(-r_newrefdef.viewangles[0]));
	model_view *= XMMatrixRotationX(XMConvertToRadians(-r_newrefdef.viewangles[2]));

	model_view *= XMMatrixRotationZ(XMConvertToRadians(90.0f));
	model_view *= XMMatrixRotationX(XMConvertToRadians(-90.0f));

	model_view = XMMatrixTranspose(model_view);

	RD.SetViewMatrix(model_view);
	//bsp_renderer->InitCB();

	//qglRotatef(-90, 1, 0, 0);	    // put Z going up
	//qglRotatef(90, 0, 0, 1);	    // put Z going up
	//qglRotatef(-r_newrefdef.viewangles[2], 1, 0, 0);
	//qglRotatef(-r_newrefdef.viewangles[0], 0, 1, 0);
	//qglRotatef(-r_newrefdef.viewangles[1], 0, 0, 1);
	//qglTranslatef(-r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

	// Записываем моделвью матрицу в ворлд матрицу
	// Может быть нужно траспанировать

	//for (int i = 0; i < 4; i++) {
	//	for (int j = 0; j < 4; j++) {
	//		r_world_matrix[(i * 4) + j] = model_view.m[i][j];
	//	}
	//}

	//qglGetFloatv(GL_MODELVIEW_MATRIX, r_world_matrix);

	////
	//// set drawing parms
	////
	//if (gl_cull->value)
	//	qglEnable(GL_CULL_FACE);
	//else
	//	qglDisable(GL_CULL_FACE);

	//qglDisable(GL_BLEND);
	//qglDisable(GL_ALPHA_TEST);
	//qglEnable(GL_DEPTH_TEST);
}


int SignbitsForPlane(cplane_t* out) {
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j = 0; j < 3; j++) {
		if (out->normal[j] < 0)
			bits |= 1 << j;
	}
	return bits;
}

void R_SetFrustum(void) {
	int		i;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90 - r_newrefdef.fov_x / 2));
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector(frustum[1].normal, vup, vpn, 90 - r_newrefdef.fov_x / 2);
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector(frustum[2].normal, vright, vpn, 90 - r_newrefdef.fov_y / 2);
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90 - r_newrefdef.fov_y / 2));

	for (i = 0; i < 4; i++) {
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane(&frustum[i]);
	}
}




/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList(void) {
	int		i;
	//
	//if (!r_drawentities->value)
	//	return;

	// draw non-transparent first
	for (i = 0; i < r_newrefdef.num_entities; i++) {
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// solid

		if (currententity->flags & RF_BEAM) {
			//R_DrawBeam(currententity);
		}
		else {
			currentmodel = currententity->model;
			if (!currentmodel) {
				//R_DrawNullModel();
				continue;
			}
			switch (currentmodel->type) {
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_brush:
				//R_DrawBrushModel(currententity);
				break;
			case mod_sprite:
				//R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	//qglDepthMask(0);		// no z writes
	for (i = 0; i < r_newrefdef.num_entities; i++) {
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if (currententity->flags & RF_BEAM) {
			//R_DrawBeam(currententity);
		}
		else {
			currentmodel = currententity->model;

			if (!currentmodel) {
				//R_DrawNullModel();
				continue;
			}
			switch (currentmodel->type) {
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_brush:
				//R_DrawBrushModel(currententity);
				break;
			case mod_sprite:
				//R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	//qglDepthMask(1);		// back to writing

}