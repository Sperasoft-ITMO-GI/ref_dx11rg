// GL_RSURF.C: surface-related refresh code
#include <assert.h>
#include <algorithm>

#include "dx11rg_local.h"
#include "dx11rg_model.h"
#include "dx11rg_drawData.h"
#include "RegistrationManager.h"

// GL_RSURF.C: surface-related refresh code
#include <assert.h>
#include <algorithm>


static vec3_t	modelorg;		// relative to viewpoint

int  lightmap_textures;
float inverse_intensity;

msurface_t* r_alpha_surfaces;

#define DYNAMIC_LIGHT_WIDTH  128
#define DYNAMIC_LIGHT_HEIGHT 128

#define LIGHTMAP_BYTES 4

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define	MAX_LIGHTMAPS	32

int		c_visible_lightmaps;
int		c_visible_textures;


TextureData dynamicLightMap = {128*MAX_LIGHTMAPS, 128};

#define GL_LIGHTMAP_FORMAT GL_RGBA

typedef struct {
	int internal_format;
	int	current_lightmap_texture;

	msurface_t* lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	//TextureData lightmap_buffer = TextureData{ BLOCK_WIDTH , BLOCK_HEIGHT };
	byte		lightmap_buffer[4 * BLOCK_WIDTH * BLOCK_HEIGHT];
} gllightmapstate_t;

static gllightmapstate_t gl_lms;


static void		LM_InitBlock(void);
static void		LM_UploadBlock(qboolean dynamic);
static qboolean	LM_AllocBlock(int w, int h, int* x, int* y);

extern void R_SetCacheState(msurface_t* surf);
extern void R_BuildLightMap(msurface_t* surf, byte* dest, int stride);

float colorBuf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
bool multiTexture = true;

void SmartTriangulation(std::vector<uint32_t>* ind, int num) {
	for (int i = 1; i < num - 1; i++) {
		ind->push_back(0);
		ind->push_back(i + 1);
		ind->push_back(i);
	}
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t* R_TextureAnimation(mtexinfo_t* tex) {
	int		c;

	if (!tex->next)
		return tex->image;

	c = currententity->frame % tex->numframes;
	while (c) {
		tex = tex->next;
		c--;
	}

	return tex->image;
}

Transform hashedTransform;

/*
================
DrawGLPoly
================
*/
void DrawGLPoly(glpoly_t* p, int texNum, int ligntTexShift, uint64_t defines, float2 texOffsets = float2{ 0,0 }, int lightTex = -1, ImageUpdate*  updateLM= nullptr) {
	int		i;
	float* v;

	if (p->savedData.indexOffset == -1) {

		UPVertex vert = {};
		std::vector<UPVertex> vect;

		v = p->verts[0];
		for (i = 0; i < p->numverts; i++, v += VERTEXSIZE) {
			//qglTexCoord2f(v[3], v[4]);
			//qglVertex3fv(v);

			vert.position.x = v[0];
			vert.position.y = v[1];
			vert.position.z = v[2];
			vert.texcoord.x = v[3];
			vert.texcoord.y = v[4];
			vert.lightTexcoord.x = v[5];
			vert.lightTexcoord.y = v[6];
			vert.dynamicLightTexcoord.x = (v[5]+ ligntTexShift)/ MAX_LIGHTMAPS;
			vert.dynamicLightTexcoord.y = v[6];

			vect.push_back(vert);
		}

		std::vector<uint32_t> indexes;

		SmartTriangulation(&indexes, p->numverts);\
		for (int i = 0; i < indexes.size()/3; i++)
		{
			float3	v1 = vect[indexes[i*3]].position;
			float3	v2 = vect[indexes[i*3+1]].position;
			float3	v3 = vect[indexes[i*3+2]].position;
			float3 normal = (v1-v2).Cross(v1 - v3);
			normal.Normalize();
			if (normal.Length() > 0.00001)
			{
				vect[indexes[i*3]].normal   = normal;
				vect[indexes[i*3+1]].normal = normal;
				vect[indexes[i*3+2]].normal = normal;
			} else
			{
				int a = 1;
				a++;
			}
		}

		UPModelMesh model = { EPrimitiveType::PRIMITIVETYPE_TRIANGLELIST,
			p->numverts - 2, vect, indexes };


		p->savedData = RD.RegisterUserPolygon(model, false);
	}

	UPDrawData data = { hashedTransform , texOffsets, float4{ colorBuf }, false, updateLM != nullptr, updateLM,  defines };
	if (lightTex == -1)

		RD.DrawUserPolygon(p->savedData, texNum, data);
	else
		RD.DrawUserPolygon(p->savedData, texNum, lightTex, data);

	//texNum, hashedTransform, , ;
}

//============
//PGM
/*
================
DrawGLFlowingPoly -- version of DrawGLPoly that handles scrolling texture
================
*/
void DrawGLFlowingPoly(msurface_t* fa, int texNum, uint64_t defines) {
	int		i;
	float* v;
	//glpoly_t* p;
	float	scroll;
	//p = fa->polys;

	scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
	if (scroll == 0.0)
		scroll = -64.0;

	DrawGLPoly(fa->polys, texNum, fa->lightmaptexturenum, defines, float2{ scroll , 0 });
	return;
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly(msurface_t* fa) {
	int		 maps = 0;
	image_t* image;
	qboolean is_dynamic = False;

	c_brush_polys++;

	image = R_TextureAnimation(fa->texinfo);

	if (fa->flags & SURF_DRAWTURB) {
		//GL_Bind(image->texnum);

		// warp texture, no lightmaps
		//qglColor4f(dx11_state.inverse_intensity, dx11_state.inverse_intensity, dx11_state.inverse_intensity, 1.0F);
		colorBuf[0] = inverse_intensity;
		colorBuf[1] = inverse_intensity;
		colorBuf[2] = inverse_intensity;
		colorBuf[3] = 1.0f;
		EmitWaterPolys(fa);

		return;
	}
	else {
		//GL_Bind(image->texnum);

		//GL_TexEnv(GL_REPLACE);
	}

	//======
	//PGM
	if (fa->texinfo->flags & SURF_FLOWING)
		DrawGLFlowingPoly(fa, image->texnum, 0);
	else
		DrawGLPoly(fa->polys, image->texnum, fa->lightmaptexturenum, 0);
	//PGM
	//======

	/*
	** check for lightmap modification
	*/
	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++) {
		if (r_newrefdef.lightstyles[fa->styles[maps]].white != fa->cached_light[maps])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ((fa->dlightframe == r_framecount)) {
	dynamic:
		if (/*gl_dynamic->value*/true) {
			if (!(fa->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))) {
				is_dynamic = True;
			}
		}
	}

	if (is_dynamic) {
		if ((fa->styles[maps] >= 32 || fa->styles[maps] == 0) && (fa->dlightframe != r_framecount)) {
			unsigned	temp[34 * 34];
			int			smax, tmax;

			smax = (fa->extents[0] >> 4) + 1;
			tmax = (fa->extents[1] >> 4) + 1;

			R_BuildLightMap(fa, (byte*)(void*)temp, smax * 4);
			R_SetCacheState(fa);

			uint8_t* data = new uint8_t[34 * 34 * 4];
			for (int i = 0; i < BLOCK_WIDTH * BLOCK_HEIGHT; i++) {
				((uint32_t*)data)[i] = temp[i];
			}

			ImageUpdate updateData{
				0,
				ImageBox{fa->light_s, fa->light_t, smax, tmax},
				34, 34,
				0, data
			};

			
			updateData.id = lightmap_textures + fa->lightmaptexturenum;

			RD.UpdateTexture(updateData);

			delete[] data;
			//renderer->UpdateTextureInSRV(smax, tmax, fa->light_s, fa->light_t, 32,
			//	(unsigned char*)temp, dx11_state.lightmap_textures + fa->lightmaptexturenum);

			fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
			gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else {
			fa->lightmapchain = gl_lms.lightmap_surfaces[0];
			gl_lms.lightmap_surfaces[0] = fa;
		}
	}
	else {
		fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
		gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}


/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces(void) {
	msurface_t* s;
	float		intens;

	//
	// go back to the world matrix
	//
	//qglLoadMatrixf(r_world_matrix);

	//qglEnable(GL_BLEND);
	//GL_TexEnv(GL_MODULATE);

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	intens = inverse_intensity;

	for (s = r_alpha_surfaces; s; s = s->texturechain) {
		//GL_Bind(s->texinfo->image->texnum);
		c_brush_polys++;
		if (s->texinfo->flags & SURF_TRANS33) {
			//qglColor4f(intens, intens, intens, 0.33);
			colorBuf[0] = intens;
			colorBuf[1] = intens;
			colorBuf[2] = intens;
			colorBuf[3] = 0.33;
		}
		else if (s->texinfo->flags & SURF_TRANS66) {
			//qglColor4f(intens, intens, intens, 0.66);
			colorBuf[0] = intens;
			colorBuf[1] = intens;
			colorBuf[2] = intens;
			colorBuf[3] = 0.66f;
		}
		else {
			//qglColor4f(intens, intens, intens, 1);
			colorBuf[0] = intens;
			colorBuf[1] = intens;
			colorBuf[2] = intens;
			colorBuf[3] = 1.0f;
		}
		if (s->flags & SURF_DRAWTURB)
			EmitWaterPolys(s);
		else
			DrawGLPoly(s->polys, s->lightmaptexturenum, s->texinfo->image->texnum, UPALPHA);
	}

	//qglColor4f(1, 1, 1, 1);
	colorBuf[0] = 1.0f;
	colorBuf[1] = 1.0f;
	colorBuf[2] = 1.0f;
	colorBuf[3] = 1.0f;

	r_alpha_surfaces = NULL;
}

/*
================
DrawTextureChains
================
*/
void DrawTextureChains(void) {
	int		i;
	msurface_t* s;
	image_t* image;

	c_visible_textures = 0;

	if (!multiTexture) {
		for (i = 0; i < RM.numTextures; i++) {
			image = RM.FindImage(i);
			if (!image->registration_sequence)
				continue;
			s = image->texturechain;
			if (!s)
				continue;
			c_visible_textures++;

			for (; s; s = s->texturechain)
				R_RenderBrushPoly(s);

			image->texturechain = NULL;
		}
	}
	else {
		for (i = 0; i < RM.numTextures; i++) {
			image = RM.FindImage(i);
			if (!image->registration_sequence)
				continue;
			if (!image->texturechain)
				continue;
			c_visible_textures++;

			for (s = image->texturechain; s; s = s->texturechain) {
				if (!(s->flags & SURF_DRAWTURB))
					R_RenderBrushPoly(s);
			}
		}

		for (i = 0; i < RM.numTextures; i++) {
			image = RM.FindImage(i);
			if (!image->registration_sequence)
				continue;
			s = image->texturechain;
			if (!s)
				continue;

			for (; s; s = s->texturechain) {
				if (s->flags & SURF_DRAWTURB)
					R_RenderBrushPoly(s);
			}

			image->texturechain = NULL;
		}
	}
}

size_t lightmapTex = 0;

static void GL_RenderLightmappedPoly(msurface_t* surf, uint64_t defines) {
	int		i, nv = surf->polys->numverts;
	int		map;
	float* v;
	image_t* image = R_TextureAnimation(surf->texinfo);
	qboolean is_dynamic = False;
	unsigned lmtex = surf->lightmaptexturenum;
	glpoly_t* p;

	for (map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++) {
		if (r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ((surf->dlightframe == r_framecount)) {
	dynamic:
		if (true/*gl_dynamic->value*/) {
			if (!(surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))) {
				is_dynamic = True;
			}
		}
	}

	if (is_dynamic) {
		unsigned	temp[128 * 128];
		int			smax, tmax;
		ImageUpdate* updateData = nullptr;
		uint dFlag = 0;;
		if ((surf->styles[map] >= 32 || surf->styles[map] == 0) && (surf->dlightframe != r_framecount)) {

			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			R_BuildLightMap(surf, (byte*)(void*)temp, smax * 4);
			R_SetCacheState(surf);

			//GL_MBind(GL_TEXTURE1_SGIS, dx11_state.lightmap_textures + surf->lightmaptexturenum);
			lmtex = lightmap_textures + surf->lightmaptexturenum;


			uint8_t* data = new uint8_t[smax * tmax * 4];
			for (int i = 0; i < smax * tmax * 4; i++) {
				(data)[i] = ((uint8_t*)temp)[i];
			}

			
			updateData = new ImageUpdate();
			*updateData = ImageUpdate{
				lmtex,
				ImageBox{surf->light_s, surf->light_t, smax, tmax},
				128, 128,0,data
			};
			
		}
		else {
			// TODO: Here is a bug with dynamic lightmap update

			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			R_BuildLightMap(surf, (byte*)(void*)(temp), smax * 4);

			//GL_MBind(GL_TEXTURE1_SGIS, dx11_state.lightmap_textures + 0);

			lmtex = lightmap_textures;
			//uint8_t* data = new uint8_t[smax * tmax * 4];
			
			for (int x = 0; x < smax; x++)
			{
				for (int y = 0; y <  tmax; y++) {
					dynamicLightMap.PutPixel(surf->light_s+x + 128 * surf->lightmaptexturenum, surf->light_t+y, (temp)[y*smax+x]);
				}
			}
			dFlag = 1;
			if (!dynamicLightMapUpdateFlag)
			{
				dynamicLightMapUpdateFlag = 1;
				updateData = new ImageUpdate();
				*updateData = ImageUpdate{
					lmtex,
					ImageBox{0, 0, 128*MAX_LIGHTMAPS, 128},
					128*MAX_LIGHTMAPS, 128,0,(byte*)(void*)(dynamicLightMap.GetBufferPtr())
				};
			}
			//RD.UpdateTexture(*updateData);
		}

		c_brush_polys++;

		if (surf->texinfo->flags & SURF_FLOWING) {
			float scroll;

			scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
			if (scroll == 0.0)
				scroll = -64.0;

			for (p = surf->polys; p; p = p->chain) {
				DrawGLPoly(p, image->texnum, surf->lightmaptexturenum, defines | UPLIGHTMAPPED | (dFlag* DYNAMIC), float2{ scroll , 0 }, lmtex, updateData);
				updateData = nullptr;
			}
		}
		else {
			for (p = surf->polys; p; p = p->chain) {
				DrawGLPoly(p, image->texnum, surf->lightmaptexturenum, defines | UPLIGHTMAPPED | (dFlag* DYNAMIC), float2{ 0 , 0 }, lmtex, updateData);
				updateData = nullptr;
			}
		}
		//PGM
		//==========
	}
	else {
		c_brush_polys++;

		//GL_MBind(GL_TEXTURE0_SGIS, image->texnum);
		//GL_MBind(GL_TEXTURE1_SGIS, dx11_state.lightmap_textures + lmtex);

		//==========
		//PGM
		if (surf->texinfo->flags & SURF_FLOWING) {

			float scroll;

			scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
			if (scroll == 0.0)
				scroll = -64.0;

			for (p = surf->polys; p; p = p->chain) {
				DrawGLPoly(p, image->texnum, surf->lightmaptexturenum , defines | UPLIGHTMAPPED, float2{ scroll , 0 }, lightmap_textures + surf->lightmaptexturenum);
			}
		}
		else {
			//PGM
			//==========
			for (p = surf->polys; p; p = p->chain) {
				DrawGLPoly(p, image->texnum, surf->lightmaptexturenum, defines | UPLIGHTMAPPED, float2{ 0 , 0 }, lightmap_textures + surf->lightmaptexturenum);
			}
			//==========
			//PGM
		}
		//PGM
		//==========
	}
}

/*
=================
R_DrawInlineBModel
=================
*/
void R_DrawInlineBModel(void) {
	int			i, k;
	cplane_t* pplane;
	float		dot;
	msurface_t* psurf;
	dlight_t* lt;

	// calculate dynamic lighting for bmodel
	if (true/*!gl_flashblend->value*/) {
		lt = r_newrefdef.dlights;
		for (k = 0; k < r_newrefdef.num_dlights; k++, lt++) {
			R_MarkLights(lt, 1 << k, currentmodel->nodes + currentmodel->firstnode);
		}
	}

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	// if translucent
	if (currententity->flags & RF_TRANSLUCENT) {
		//qglColor4f(1, 1, 1, 0.25);
		colorBuf[0] = 1.0f;
		colorBuf[1] = 1.0f;
		colorBuf[2] = 1.0f;
		colorBuf[3] = 0.25f;
	}
	else {
		//qglColor4f(1, 1, 1, 1);
		colorBuf[0] = 1.0f;
		colorBuf[1] = 1.0f;
		colorBuf[2] = 1.0f;
		colorBuf[3] = 1.0f;
	}

	//
	// draw texture
	//
	for (i = 0; i < currentmodel->nummodelsurfaces; i++, psurf++) {
		// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON))) {
			if (psurf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66)) {	// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else if (multiTexture && !(psurf->flags & SURF_DRAWTURB)) {
				if (currententity->flags & RF_TRANSLUCENT)
					GL_RenderLightmappedPoly(psurf, UPALPHA);
				else
					GL_RenderLightmappedPoly(psurf, 0);
			}
			else {
				//GL_EnableMultitexture(False);
				R_RenderBrushPoly(psurf);
				//GL_EnableMultitexture(True);
			}
		}
	}

	
}

/*
=================
R_DrawBrushModel
=================
*/
int	currenttextures[2];

qboolean R_CullBox(vec3_t mins, vec3_t maxs) {
	int		i;

	if (r_nocull->value)
		return False;

	for (i = 0; i < 4; i++)
		if (BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return True;
	return False;
}

void R_DrawBrushModel(entity_t* e) {
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;
	currenttextures[0] = currenttextures[1] = -1;

	if (e->angles[0] || e->angles[1] || e->angles[2]) {
		rotated = True;
		for (i = 0; i < 3; i++) {
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else {
		rotated = False;
		VectorAdd(e->origin, currentmodel->mins, mins);
		VectorAdd(e->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;

	//qglColor3f(1, 1, 1);
	colorBuf[0] = 1.0f;
	colorBuf[1] = 1.0f;
	colorBuf[2] = 1.0f;
	colorBuf[3] = 1.0f;
	memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	VectorSubtract(r_newrefdef.vieworg, e->origin, modelorg);
	if (rotated) {
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	//qglPushMatrix();
	//auto saveMatrix = renderer->GetModelView();	

	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	e->angles[ROLL] = -e->angles[ROLL];		// sigh.
	hashedTransform = Transform(R_RotateForEntity(e, false));
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	e->angles[ROLL] = -e->angles[ROLL];		// sigh.

	//GL_EnableMultitexture(True);
	//GL_SelectTexture(GL_TEXTURE0_SGIS);
	//GL_TexEnv(GL_REPLACE);
	//GL_SelectTexture(GL_TEXTURE1_SGIS);
	//GL_TexEnv(GL_MODULATE);

	R_DrawInlineBModel();
	//GL_EnableMultitexture(False);

	//qglPopMatrix();
	//renderer->SetModelViewMatrix(saveMatrix);
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode(mnode_t* node) {
	int			c, side, sidebit;
	cplane_t* plane;
	msurface_t* surf, ** mark;
	mleaf_t* pleaf;
	float		dot;
	image_t* image;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;
	if (R_CullBox(node->minmaxs, node->minmaxs + 3))
		return;

	// if a leaf node, draw stuff
	if (node->contents != -1) {
		pleaf = (mleaf_t*)node;

		// check for door connected areas
		if (r_newrefdef.areabits) {
			if (!(r_newrefdef.areabits[pleaf->area >> 3] & (1 << (pleaf->area & 7))) && (!r_worldmodel->firstFrame))
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c) {
			do {
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	plane = node->plane;

	switch (plane->type) {
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct(modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0) {
		side = 0;
		sidebit = 0;
	}
	else {
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNode(node->children[side]);

	// draw stuff
	for (c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++) {
		if (surf->visframe != r_framecount)
			continue;

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY) {	// just adds to visible sky bounds
			//R_AddSkySurface(surf);
		}
		else if (surf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66)) {	// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else {
			if (multiTexture && !(surf->flags & SURF_DRAWTURB)) {
				GL_RenderLightmappedPoly(surf, 0);
			}
			else {
				// the polygon is visible, so add it to the texture
				// sorted chain
				// FIXME: this is a hack for animation
				image = R_TextureAnimation(surf->texinfo);
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode(node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld(void) {
	entity_t	ent;

	hashedTransform = Transform();

	if (!r_drawworld->value)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	currentmodel = r_worldmodel;

	VectorCopy(r_newrefdef.vieworg, modelorg);

	// auto cycle the world frame for texture animation
	memset(&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time * 2);
	currententity = &ent;

	currenttextures[0] = currenttextures[1] = -1;

	//qglColor3f(1, 1, 1);
	colorBuf[0] = 1.0f;
	colorBuf[1] = 1.0f;
	colorBuf[2] = 1.0f;
	colorBuf[3] = 1.0f;
	memset(gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));
	lightmapTex = 0;
	R_ClearSkyBox();

	if (multiTexture) {
		R_RecursiveWorldNode(r_worldmodel->nodes);
	}
	else {
		R_RecursiveWorldNode(r_worldmodel->nodes);
	}

	r_worldmodel->firstFrame = false;
	/*
	** theoretically nothing should happen in the next two functions
	** if multitexture is enabled
	*/

	R_DrawSkyBox();

	//R_DrawTriangleOutlines();
}

/*
===================
Mod_DecompressVis
===================
*/
byte* Mod_DecompressVis(byte* in, model_t* model) {
	static byte	decompressed[MAX_MAP_LEAFS / 8];
	int		c;
	byte* out;
	int		row;

	row = (model->vis->numclusters + 7) >> 3;
	out = decompressed;

	if (!in) {	// no vis info, so make all visible
		while (row) {
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do {
		if (*in) {
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c) {
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

byte	mod_novis[MAX_MAP_LEAFS / 8];

/*
==============
Mod_ClusterPVS
==============
*/
byte* Mod_ClusterPVS(int cluster, model_t* model) {
	if (cluster == -1 || !model->vis)
		return mod_novis;
	return Mod_DecompressVis((byte*)model->vis + model->vis->bitofs[cluster][DVIS_PVS],
		model);
}



/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/

void R_MarkLeaves(void) {
	byte* vis;
	byte	fatvis[MAX_MAP_LEAFS / 8];
	mnode_t* node;
	int		i, c;
	mleaf_t* leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	/*if (gl_lockpvs->value)
		return;*/

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis) {
		// mark everything
		for (i = 0; i < r_worldmodel->numleafs; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i = 0; i < r_worldmodel->numnodes; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS(r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster) {
		memcpy(fatvis, vis, (r_worldmodel->numleafs + 7) / 8);
		vis = Mod_ClusterPVS(r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs + 31) / 32;
		for (i = 0; i < c; i++)
			((int*)fatvis)[i] |= ((int*)vis)[i];
		vis = fatvis;
	}

	for (i = 0, leaf = r_worldmodel->leafs; i < r_worldmodel->numleafs; i++, leaf++) {
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster >> 3] & (1 << (cluster & 7))) {
			node = (mnode_t*)leaf;
			do {
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock(void) {
	memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));
}

static void LM_UploadBlock(qboolean dynamic) {
	int texture;
	int height = 0;

	if (dynamic) {
		texture = 0;
	}
	else {
		texture = gl_lms.current_lightmap_texture;
	}


	if (dynamic) {
		int i;

		for (i = 0; i < BLOCK_WIDTH; i++) {
			if (gl_lms.allocated[i] > height)
				height = gl_lms.allocated[i];
		}


		uint8_t* data = new uint8_t[BLOCK_WIDTH * BLOCK_HEIGHT * 4];
		for (int i = 0; i < BLOCK_WIDTH * BLOCK_HEIGHT * 4; i++) {
			data[i] = gl_lms.lightmap_buffer[i];
		}
		
		ImageUpdate updateData {
			lightmap_textures + texture,
			ImageBox{0, 0, 32, height},
			128, 128, 0, data
		};

		RD.UpdateTexture(updateData);
		delete[] data;
	}
	else {
		RD.RegisterTexture(lightmap_textures + texture, BLOCK_WIDTH, BLOCK_HEIGHT, gl_lms.lightmap_buffer, false);

		if (++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS)
			ri.Sys_Error(ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n");
	}


}

// returns a texture number and the position inside it
static qboolean LM_AllocBlock(int w, int h, int* x, int* y) {
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	for (i = 0; i < BLOCK_WIDTH - w; i++) {
		best2 = 0;

		for (j = 0; j < w; j++) {
			if (gl_lms.allocated[i + j] >= best)
				break;
			if (gl_lms.allocated[i + j] > best2)
				best2 = gl_lms.allocated[i + j];
		}
		if (j == w) {	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return False;

	for (i = 0; i < w; i++)
		gl_lms.allocated[*x + i] = best + h;

	return True;
}

/*
================
GL_BuildPolygonFromSurface
================
*/
void GL_BuildPolygonFromSurface(msurface_t* fa) {
	int			i, lindex, lnumverts;
	medge_t* pedges, * r_pedge;
	int			vertpage;
	float* vec;
	float		s, t;
	glpoly_t* poly;
	vec3_t		total;

	// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	VectorClear(total);
	//
	// draw texture
	//
	poly = (glpoly_t*)Hunk_Alloc(sizeof(glpoly_t) + (lnumverts - 4) * VERTEXSIZE * sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	poly->savedData = MeshHashData{ -1,-1,-1 };
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0; i < lnumverts; i++) {
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0) {
			r_pedge = &pedges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else {
			r_pedge = &pedges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		VectorAdd(total, vec, total);
		VectorCopy(vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;

}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap(msurface_t* surf) {
	int		smax, tmax;
	byte* base;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;

	if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t)) {
		LM_UploadBlock(False);
		LM_InitBlock();
		if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t)) {
			ri.Sys_Error(ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax);
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = (byte*)gl_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState(surf);
	R_BuildLightMap(surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES);
}
unsigned dynamicLightMapUpdateFlag;
/*
==================
GL_BeginBuildingLightmaps

==================
*/
void GL_BeginBuildingLightmaps(model_t* m) {
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;

	memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));

	r_framecount = 1;		// no dlightcache

	//GL_EnableMultitexture(True);
	//GL_SelectTexture(GL_TEXTURE1_SGIS);

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i = 0; i < MAX_LIGHTSTYLES; i++) {
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	if (!lightmap_textures) {
		lightmap_textures = TEXNUM_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;

	/*
	** if mono lightmaps are enabled and we want to use alpha
	** blending (a,1-a) then we're likely running on a 3DLabs
	** Permedia2.  In a perfect world we'd use a GL_ALPHA lightmap
	** in order to conserve space and maximize bandwidth, however
	** this isn't a perfect world.
	**
	** So we have to use alpha lightmaps, but stored in GL_RGBA format,
	** which means we only get 1/16th the color resolution we should when
	** using alpha lightmaps.  If we find another board that supports
	** only alpha lightmaps but that can at least support the GL_ALPHA
	** format then we should change this code to use real alpha maps.
	*/
	
	RD.RegisterTexture(lightmap_textures + 0, BLOCK_WIDTH*MAX_LIGHTMAPS, BLOCK_HEIGHT, dynamicLightMap.GetBufferPtr(), true);

}

/*
=======================
GL_EndBuildingLightmaps
=======================
*/
void GL_EndBuildingLightmaps(void) {
	LM_UploadBlock(False);
	//GL_EnableMultitexture(False);
}



mleaf_t* Mod_PointInLeaf(vec3_t p, model_t* model) {
	mnode_t* node;
	float		d;
	cplane_t* plane;

	if (!model || !model->nodes)
		ri.Sys_Error(ERR_DROP, "Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1) {
		if (node->contents != -1)
			return (mleaf_t*)node;
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}