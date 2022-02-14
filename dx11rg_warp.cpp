// gl_warp.c -- sky and water polygons

#include "dx11rg_local.h"
#include "dx11rg_model.h"
#include "dx11rg_drawData.h"
// gl_warp.c -- sky and water polygons


extern	model_t* loadmodel;

char	skyname[MAX_QPATH];
float	skyrotate;
vec3_t	skyaxis;
image_t* sky_images[6];

msurface_t* warpface;

#define	SUBDIVIDE_SIZE	64

void BoundPoly(int numverts, float* verts, vec3_t mins, vec3_t maxs) {
	int		i, j;
	float* v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;
	for (i = 0; i < numverts; i++)
		for (j = 0; j < 3; j++, v++) {
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
}

void SubdividePolygon(int numverts, float* verts) {
	int		i, j, k;
	vec3_t	mins, maxs;
	float	m;
	float* v;
	vec3_t	front[64], back[64];
	int		f, b;
	float	dist[64];
	float	frac;
	glpoly_t* poly;
	float	s, t;
	vec3_t	total;
	float	total_s, total_t;

	if (numverts > 60)
		ri.Sys_Error(ERR_DROP, "numverts = %i", numverts);

	BoundPoly(numverts, verts, mins, maxs);

	for (i = 0; i < 3; i++) {
		m = (mins[i] + maxs[i]) * 0.5;
		m = SUBDIVIDE_SIZE * floor(m / SUBDIVIDE_SIZE + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j = 0; j < numverts; j++, v += 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v -= i;
		VectorCopy(verts, v);

		f = b = 0;
		v = verts;
		for (j = 0; j < numverts; j++, v += 3) {
			if (dist[j] >= 0) {
				VectorCopy(v, front[f]);
				f++;
			}
			if (dist[j] <= 0) {
				VectorCopy(v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j + 1] == 0)
				continue;
			if ((dist[j] > 0) != (dist[j + 1] > 0)) {
				// clip point
				frac = dist[j] / (dist[j] - dist[j + 1]);
				for (k = 0; k < 3; k++)
					front[f][k] = back[b][k] = v[k] + frac * (v[3 + k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon(f, front[0]);
		SubdividePolygon(b, back[0]);
		return;
	}

	// add a point in the center to help keep warp valid
	poly = (glpoly_t*)Hunk_Alloc(sizeof(glpoly_t) + ((numverts - 4) + 2) * VERTEXSIZE * sizeof(float));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts + 2;
	VectorClear(total);
	total_s = 0;
	total_t = 0;
	for (i = 0; i < numverts; i++, verts += 3) {
		VectorCopy(verts, poly->verts[i + 1]);
		s = DotProduct(verts, warpface->texinfo->vecs[0]);
		t = DotProduct(verts, warpface->texinfo->vecs[1]);

		total_s += s;
		total_t += t;
		VectorAdd(total, verts, total);

		poly->verts[i + 1][3] = s;
		poly->verts[i + 1][4] = t;
	}

	VectorScale(total, (1.0 / numverts), poly->verts[0]);
	poly->verts[0][3] = total_s / numverts;
	poly->verts[0][4] = total_t / numverts;

	// copy first vertex to last
	memcpy(poly->verts[i + 1], poly->verts[1], sizeof(poly->verts[0]));
}

/*
================
GL_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
void GL_SubdivideSurface(msurface_t* fa) {
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float* vec;

	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i = 0; i < fa->numedges; i++) {
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy(vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon(numverts, verts[0]);
}

//=========================================================



// speed up sin calculations - Ed
float	r_turbsin[] =
{
	#include "../ref_gl/warpsin.h"
};
#define TURBSCALE (256.0 / (2 * M_PI))

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
void EmitWaterPolys(msurface_t* fa) {
	glpoly_t* p, * bp;
	float* v;
	int			i;
	float		s, t, os, ot;
	float		scroll;
	float		rdt = r_newrefdef.time;

	if (fa->texinfo->flags & SURF_FLOWING)
		scroll = -64 * ((r_newrefdef.time * 0.5) - (int)(r_newrefdef.time * 0.5));
	else
		scroll = 0;
	for (bp = fa->polys; bp; bp = bp->next) {
		p = bp;

		UPVertex vert = {};
		std::vector<UPVertex> vect;

		for (i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE) {
			os = v[3];
			ot = v[4];

			s = os + r_turbsin[Q_ftol(((ot * 0.125 + rdt) * TURBSCALE)) & 255];
			s += scroll;
			s *= (1.0 / 64);

			t = ot + r_turbsin[Q_ftol(((os * 0.125 + rdt) * TURBSCALE)) & 255];
			t *= (1.0 / 64);

			vert.position.x = v[0];
			vert.position.y = v[1];
			vert.position.z = v[2];
			vert.texcoord.x = s;
			vert.texcoord.y = t;

			vect.push_back(vert);
		}

		std::vector<uint16_t> indexes;

		for (i = 0; i < p->numverts; i++) {
			if (i % 2 == 0)
				indexes.push_back(i / 2);
			else
				indexes.push_back(p->numverts - 1 - i / 2);
		}



		UPModelData model = { Renderer::PrimitiveType::PRIMITIVETYPE_TRIANGLESTRIP,
			p->numverts - 2, vect,indexes };



		RD.DrawUserPolygon(model, fa->texinfo->image->texnum, Transform(), float4{ colorBuf },  UPWATER);

		
	}
}


//===================================================================


vec3_t	skyclip[6] = {
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1}
};
int	c_sky;

// 1 = s, 2 = t, 3 = 2048
int	st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down
};

// s = [0]/[2], t = [1]/[2]
int	vec_to_st[6][3] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}
};

float	skymins[2][6], skymaxs[2][6];
float	sky_min, sky_max;

void DrawSkyPolygon(int nump, vec3_t vecs) {
	int		i, j;
	vec3_t	v, av;
	float	s, t, dv;
	int		axis;
	float* vp;

	c_sky++;

	// decide which face it maps to
	VectorCopy(vec3_origin, v);
	for (i = 0, vp = vecs; i < nump; i++, vp += 3) {
		VectorAdd(vp, v, v);
	}
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2]) {
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0]) {
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else {
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i = 0; i < nump; i++, vecs += 3) {
		j = vec_to_st[axis][2];
		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];
		if (dv < 0.001)
			continue;	// don't divide by zero
		j = vec_to_st[axis][0];
		if (j < 0)
			s = -vecs[-j - 1] / dv;
		else
			s = vecs[j - 1] / dv;
		j = vec_to_st[axis][1];
		if (j < 0)
			t = -vecs[-j - 1] / dv;
		else
			t = vecs[j - 1] / dv;

		if (s < skymins[0][axis])
			skymins[0][axis] = s;
		if (t < skymins[1][axis])
			skymins[1][axis] = t;
		if (s > skymaxs[0][axis])
			skymaxs[0][axis] = s;
		if (t > skymaxs[1][axis])
			skymaxs[1][axis] = t;
	}
}

#define	ON_EPSILON		0.1			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64
void ClipSkyPolygon(int nump, vec3_t vecs, int stage) {
	float* norm;
	float* v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS - 2)
		ri.Sys_Error(ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS");
	if (stage == 6) {	// fully clipped, so draw it
		DrawSkyPolygon(nump, vecs);
		return;
	}

	front = back = False;
	norm = skyclip[stage];
	for (i = 0, v = vecs; i < nump; i++, v += 3) {
		d = DotProduct(v, norm);
		if (d > ON_EPSILON) {
			front = True;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON) {
			back = True;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back) {	// not clipped
		ClipSkyPolygon(nump, vecs, stage + 1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy(vecs, (vecs + (i * 3)));
	newc[0] = newc[1] = 0;

	for (i = 0, v = vecs; i < nump; i++, v += 3) {
		switch (sides[i]) {
		case SIDE_FRONT:
			VectorCopy(v, newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy(v, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy(v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy(v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i + 1]);
		for (j = 0; j < 3; j++) {
			e = v[j] + d * (v[j + 3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
	ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

/*
=================
R_AddSkySurface
=================
*/
void R_AddSkySurface(msurface_t* fa) {
	int			i;
	vec3_t		verts[MAX_CLIP_VERTS];
	glpoly_t* p;

	// calculate vertex values for sky box
	for (p = fa->polys; p; p = p->next) {
		for (i = 0; i < p->numverts; i++) {
			VectorSubtract(p->verts[i], r_origin, verts[i]);
		}
		ClipSkyPolygon(p->numverts, verts[0], 0);
	}
}


/*
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox(void) {
	int		i;

	for (i = 0; i < 6; i++) {
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
	}
}


void MakeSkyVec(float s, float t, int axis, std::vector<UPVertex>* vect) {
	vec3_t		v, b;
	int			j, k;

	b[0] = s * 2300;
	b[1] = t * 2300;
	b[2] = 2300;

	for (j = 0; j < 3; j++) {
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
	}

	// avoid bilerp seam
	s = (s + 1) * 0.5;
	t = (t + 1) * 0.5;

	if (s < sky_min)
		s = sky_min;
	else if (s > sky_max)
		s = sky_max;
	if (t < sky_min)
		t = sky_min;
	else if (t > sky_max)
		t = sky_max;

	t = 1.0 - t;
	//qglTexCoord2f(s, t);
	//qglVertex3fv(v);

	UPVertex vert = {};
	vert.position.x = v[0];
	vert.position.y = v[1];
	vert.position.z = v[2];
	vert.texcoord.x = s;
	vert.texcoord.y = t;

	vect->push_back(vert);
}

/*
==============
R_DrawSkyBox
==============
*/
int	skytexorder[6] = { 0,2,1,3,4,5 };
void R_DrawSkyBox(void) {
	/*
	int		i;

	if (skyrotate) {	// check for no sky at all
		for (i = 0; i < 6; i++)
			if (skymins[0][i] < skymaxs[0][i]
				&& skymins[1][i] < skymaxs[1][i])
				break;
		if (i == 6)
			return;		// nothing visible
	}

	for (i = 0; i < 6; i++) {
		if (skyrotate) {	// hack, forces full sky to draw when rotating
			skymins[0][i] = -1;
			skymins[1][i] = -1;
			skymaxs[0][i] = 1;
			skymaxs[1][i] = 1;
		}

		if (skymins[0][i] >= skymaxs[0][i]
			|| skymins[1][i] >= skymaxs[1][i])
			continue;

		//GL_Bind(sky_images[skytexorder[i]]->texnum);

		//qglBegin(GL_QUADS);
		using namespace DirectX;
		sky_renderer->is_exist = true;
		//IndexBuffer ib({ 2, 1, 0, 0, 3, 2 });
		ConstantBufferSkyBox cbsb;

		if ((skyaxis[0] == 0) && (skyaxis[0] == 0) && (skyaxis[0] == 0)) {
			cbsb.position_transform = XMMatrixRotationAxis({ 0.0f, 0.0f, 1.0f }, XMConvertToRadians(-90))
				* XMMatrixTranslation(r_origin[0], r_origin[1], r_origin[2])
				* renderer->GetModelView() * renderer->GetPerspective();
		}
		else {
			printf("POWERNULOS NEBO!!!!\n");
			cbsb.position_transform = XMMatrixRotationAxis({ skyaxis[0], skyaxis[1], skyaxis[2] }, XMConvertToRadians(r_newrefdef.time * skyrotate))
				* XMMatrixRotationAxis({ 0.0f, 0.0f, 1.0f }, XMConvertToRadians(-90))
				* XMMatrixTranslation(r_origin[0], r_origin[1], r_origin[2])
				* renderer->GetModelView() * renderer->GetPerspective();
		}

		sky_renderer->Add(cbsb);

		//std::vector<SkyVertex> vect;
		//vect.push_back(MakeSkyVec(skymins[0][i], skymins[1][i], i));
		//vect.push_back(MakeSkyVec(skymins[0][i], skymaxs[1][i], i));
		//vect.push_back(MakeSkyVec(skymaxs[0][i], skymaxs[1][i], i));
		//vect.push_back(MakeSkyVec(skymaxs[0][i], skymins[1][i], i));

		//SkyDefinitions sd{
		//	vect, { 2, 1, 0, 0, 3, 2 }, cbq, SKY_DEFAULT, sky_images[skytexorder[i]]->texnum
		//};
		//sky_renderer->Add(sd);


		//VertexBuffer vb(vect);
		//Quad sky_quad(cb, vb, ib, SKY_DEFAULT, sky_images[skytexorder[i]]->texnum);
		//sky_renderer->AddElement(sky_quad);
		//qglEnd();
	}
	*/
}


/*
============
R_SetSky
============
*/
// 3dstudio environment map names
char* suf[6] = { "rt", "bk", "lf", "ft", "up", "dn" };
void R_SetSky(char* name, float rotate, vec3_t axis) {
	int		i;
	char	pathname[MAX_QPATH];

	strncpy(skyname, name, sizeof(skyname) - 1);
	skyrotate = rotate;
	VectorCopy(axis, skyaxis);

	//renderer->CreateSkyBoxSRV();
	//sky_renderer->is_exist = true;

	for (i = 0; i < 6; i++) {
		// chop down rotating skies for less memory
		if (gl_skymip->value || skyrotate)
			gl_picmip->value++;


		Com_sprintf(pathname, sizeof(pathname), "env/%s%s.tga", skyname, suf[i]);

		sky_images[i] = RM.FindImage(pathname, it_sky);
		if (!sky_images[i])
			sky_images[i] = r_notexture;

		if (gl_skymip->value || skyrotate) {	// take less memory
			gl_picmip->value--;
			sky_min = 1.0 / 256;
			sky_max = 255.0 / 256;
		}
		else {
			sky_min = 1.0 / 512;
			sky_max = 511.0 / 512;
		}
	}
}
