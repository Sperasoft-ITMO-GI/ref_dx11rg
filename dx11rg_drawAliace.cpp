#include "dx11rg_drawData.h"

model_t* currentmodel;

vec3_t	shadevector;
float	shadelight[3];






/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel(entity_t* e) {
	int			i;
	float		an;
	vec3_t		bbox[8];
	image_t* skin;

	//if (!(e->flags & RF_WEAPONMODEL)) {
	//	if (R_CullAliasModel(bbox, e))
	//		return;
	//}

	if (e->flags & RF_WEAPONMODEL) {
		//if (r_lefthand->value == 2)
		//	return;
	}

	//paliashdr = (dmdl_t*)currentmodel->extradata;

	//
	// get lighting information
	//
	// PMM - rewrote, reordered to handle new shells & mixing
	//
	if (currententity->flags & (RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE)) {
		// PMM -special case for godmode
		if ((currententity->flags & RF_SHELL_RED) &&
			(currententity->flags & RF_SHELL_BLUE) &&
			(currententity->flags & RF_SHELL_GREEN)) {
			for (i = 0; i < 3; i++)
				shadelight[i] = 1.0;
		}
		else if (currententity->flags & (RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE)) {
			VectorClear(shadelight);

			if (currententity->flags & RF_SHELL_RED) {
				shadelight[0] = 1.0;
				if (currententity->flags & (RF_SHELL_BLUE | RF_SHELL_DOUBLE))
					shadelight[2] = 1.0;
			}
			else if (currententity->flags & RF_SHELL_BLUE) {
				if (currententity->flags & RF_SHELL_DOUBLE) {
					shadelight[1] = 1.0;
					shadelight[2] = 1.0;
				}
				else {
					shadelight[2] = 1.0;
				}
			}
			else if (currententity->flags & RF_SHELL_DOUBLE) {
				shadelight[0] = 0.9;
				shadelight[1] = 0.7;
			}
		}
		else if (currententity->flags & (RF_SHELL_HALF_DAM | RF_SHELL_GREEN)) {
			VectorClear(shadelight);
			// PMM - new colors
			if (currententity->flags & RF_SHELL_HALF_DAM) {
				shadelight[0] = 0.56;
				shadelight[1] = 0.59;
				shadelight[2] = 0.45;
			}
			if (currententity->flags & RF_SHELL_GREEN) {
				shadelight[1] = 1.0;
			}
		}
	}
	else if (currententity->flags & RF_FULLBRIGHT) {
		for (i = 0; i < 3; i++)
			shadelight[i] = 1.0;
	}
	else {
		R_LightPoint(currententity->origin, shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if (currententity->flags & RF_WEAPONMODEL) {
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if (shadelight[0] > shadelight[1]) {
				if (shadelight[0] > shadelight[2])
					r_lightlevel->value = 150 * shadelight[0];
				else
					r_lightlevel->value = 150 * shadelight[2];
			}
			else {
				if (shadelight[1] > shadelight[2])
					r_lightlevel->value = 150 * shadelight[1];
				else
					r_lightlevel->value = 150 * shadelight[2];
			}

		}

		if (false/*gl_monolightmap->string[0] != '0'*/) {
			float s = shadelight[0];

			if (s < shadelight[1])
				s = shadelight[1];
			if (s < shadelight[2])
				s = shadelight[2];

			shadelight[0] = s;
			shadelight[1] = s;
			shadelight[2] = s;
		}
	}

	if (currententity->flags & RF_MINLIGHT) {
		for (i = 0; i < 3; i++)
			if (shadelight[i] > 0.1)
				break;
		if (i == 3) {
			shadelight[0] = 0.1;
			shadelight[1] = 0.1;
			shadelight[2] = 0.1;
		}
	}

	if (currententity->flags & RF_GLOW) {	// bonus items will pulse with time
		float	scale;
		float	min;

		scale = 0.1 * sin(r_newrefdef.time * 7);
		for (i = 0; i < 3; i++) {
			min = shadelight[i] * 0.8;
			shadelight[i] += scale;
			if (shadelight[i] < min)
				shadelight[i] = min;
		}
	}

	// =================
	// PGM	ir goggles color override
	if (r_newrefdef.rdflags & RDF_IRGOGGLES && currententity->flags & RF_IR_VISIBLE) {
		shadelight[0] = 1.0;
		shadelight[1] = 0.0;
		shadelight[2] = 0.0;
	}
	// PGM	
	// =================

	//shadedots = r_avertexnormal_dots[((int)(currententity->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];

	an = currententity->angles[1] / 180 * M_PI;
	shadevector[0] = cos(-an);
	shadevector[1] = sin(-an);
	shadevector[2] = 1;
	VectorNormalize(shadevector);

	//
	// locate the proper data
	//

	c_alias_polys += currentmodel->num_tris;



	// select skin
	if (currententity->skin)
		skin = currententity->skin;	// custom player skin
	else {
		if (currententity->skinnum >= MAX_MD2SKINS)
			skin = currentmodel->skins[0].m_texid;
		else {
			skin = currentmodel->skins[currententity->skinnum].m_texid;
			if (!skin)
				skin = currentmodel->skins[0].m_texid;
		}
	}
	if (!skin)
		skin = r_notexture;	// fallback...
	//GL_Bind(skin->texnum);

	//// draw it

	//qglShadeModel(GL_SMOOTH);

	//GL_TexEnv(GL_MODULATE);
	//if (currententity->flags & RF_TRANSLUCENT)
	//{
	//	qglEnable(GL_BLEND);
	//}


	if ((currententity->frame >= currentmodel->num_frames)
		|| (currententity->frame < 0)) {
		ri.Con_Printf(PRINT_ALL, "R_DrawAliasModel %s: no such frame %d\n",
			currentmodel->name, currententity->frame);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ((currententity->oldframe >= currentmodel->num_frames)
		|| (currententity->oldframe < 0)) {
		ri.Con_Printf(PRINT_ALL, "R_DrawAliasModel %s: no such oldframe %d\n",
			currentmodel->name, currententity->oldframe);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	//if (!r_lerpmodels->value)
	//	currententity->backlerp = 0;


	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	e->angles[ROLL] = -e->angles[ROLL];		// sigh.

	//Transform(
	//{currententity->origin[0],currententity->origin[1], currententity->origin[2] },
	//{ currententity->angles[0], currententity->angles[1], currententity->angles[2] },
	//{ 1,1,1 });

	LerpModelDrawData data = {
		R_RotateForEntity(e, true), 
		R_RotateForEntity(e, false), 
		currentmodel->num_frames == 1,
		currententity->isGun,
		currententity->backlerp,
		currententity->old_backlerp,
		currententity->oldframe, currententity->frame,
		float4(),
		MBAD_UV | MLIGHTED };
	
	if (currentmodel->num_frames != 1) {
		data.flags |= MLERP;
	}

	float	alpha;
	if (currententity->flags & RF_TRANSLUCENT)
		alpha = currententity->alpha;
	else
		alpha = 1.0;
	data.color = float4(shadelight[0], shadelight[1], shadelight[2], alpha) * float4(colorBuf);

	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE)) {
		data.flags |= MNONORMAL;
	}


	
	RD.DrawFramedModel(currentmodel->modelId, skin->texnum, data);

	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	e->angles[ROLL] = -e->angles[ROLL];		// sigh.

	//GL_DrawAliasFrameLerp(paliashdr, currententity->backlerp);

	/*GL_TexEnv(GL_REPLACE);
	qglShadeModel(GL_FLAT);

	qglPopMatrix();*/

#if 0
	qglDisable(GL_CULL_FACE);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	qglDisable(GL_TEXTURE_2D);
	qglBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i < 8; i++) {
		qglVertex3fv(bbox[i]);
	}
	qglEnd();
	qglEnable(GL_TEXTURE_2D);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglEnable(GL_CULL_FACE);
#endif

	/*if ((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F))
	{
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
		qglCullFace(GL_FRONT);
	}

	if (currententity->flags & RF_TRANSLUCENT)
	{
		qglDisable(GL_BLEND);
	}

	if (currententity->flags & RF_DEPTHHACK)
		qglDepthRange(gldepthmin, gldepthmax);

#if 1
	if (gl_shadows->value && !(currententity->flags & (RF_TRANSLUCENT | RF_WEAPONMODEL)))
	{
		qglPushMatrix();
		R_RotateForEntity(e);
		qglDisable(GL_TEXTURE_2D);
		qglEnable(GL_BLEND);
		qglColor4f(0, 0, 0, 0.5);
		GL_DrawAliasShadow(paliashdr, currententity->frame);
		qglEnable(GL_TEXTURE_2D);
		qglDisable(GL_BLEND);
		qglPopMatrix();
	}
#endif
	qglColor4f(1, 1, 1, 1);*/
	colorBuf[0] = 1.0f;
	colorBuf[1] = 1.0f;
	colorBuf[2] = 1.0f;
	colorBuf[3] = 1.0f;
}