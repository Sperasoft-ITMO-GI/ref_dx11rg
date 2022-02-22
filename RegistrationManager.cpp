#include "RegistrationManager.h"
#include "dx11rg_local.h"
#include	<fstream>
#define _CRT_SECURE_NO_WARNINGS

RegistrationManager::RegistrationManager() {
	for (int i = 0; i < MAX_DXTEXTURES; i++) {
		dxTextures[i] = new image_t();
	}
	for (int i = 0; i < MAX_MOD_KNOWN; i++) {
		dxModels[i] = new model_t();
	}
}

void RegistrationManager::InitImages() {

	registration_sequence = 1;

	// init intensity conversions
	//intensity = ri.Cvar_Get("intensity", "2", 0);
	//
	//if (intensity->value <= 1)
	//	ri.Cvar_Set("intensity", "1");
	//
	//dx11_state.inverse_intensity = 1 / intensity->value;
	{
		int		r, g, b;
		unsigned	v;
		byte* pic, * pal;
		int		width, height;

		// get the palette

		LoadPCX("pics/colormap.pcx", &pic, &pal, &width, &height);
		if (!pal)
			ri.Sys_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");

		for (int i = 0; i < 256; i++) {
			r = pal[i * 3 + 0];
			g = pal[i * 3 + 1];
			b = pal[i * 3 + 2];

			v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
			d_8to24table[i] = LittleLong(v);
		}

		d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

		free(pic);
		free(pal);
	}

	ri.FS_LoadFile("pics/16to8.dat", (void**)&(d_16to8table));
	if (!d_16to8table)
		ri.Sys_Error(ERR_FATAL, "Couldn't load pics/16to8.pcx");

	float g = 0;

	for (int i = 0; i < 256; i++) {
		if (g == 1) {
			gammatable[i] = i;
		}
		else {
			float inf;

			inf = 255 * pow((i + 0.5) / 255.5, g) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	//for (int i = 0; i < 256; i++) {
	//	int j = i * intensity->value;
	//	if (j > 255)
	//		j = 255;
	//	intensitytable[i] = j;
	//}
}

void RegistrationManager::InitLocal(void) {
	draw_chars = FindImage("pics/conchars.pcx", it_pic);
}

void RegistrationManager::BeginRegistration(const char* map) {
	char	fullname[MAX_QPATH];
	cvar_t* flushmap;

	registration_sequence++;
	r_oldviewcluster = -1;		// force markleafs

	Com_sprintf(fullname, sizeof(fullname), "maps/%s.bsp", map);

	// this guarantees that mod_known[0] is the world map
	// explicitly free the old map if different
	flushmap = ri.Cvar_Get("flushmap", "0", 0);
	if (strcmp(dxModels[0]->name, fullname) || flushmap->value)
		Mod_Free(dxModels[0]);
	r_worldmodel = FindModel(fullname, True);
	
	r_viewcluster = -1;
}

image_t* RegistrationManager::LoadWal(char* name) {
	miptex_t* mt;
	int			width, height, ofs;
	image_t* image;

	ri.FS_LoadFile(name, (void**)&mt);
	if (!mt) {
		ri.Con_Printf(PRINT_ALL, "GL_FindImage: can't load %s\n", name);
		return r_notexture;
	}

	width = LittleLong(mt->width);
	height = LittleLong(mt->height);
	ofs = LittleLong(mt->offsets[0]);

	image = LoadPic(name, (byte*)mt + ofs, width, height, it_wall, 8);

	ri.FS_FreeFile((void*)mt);

	return image;
}



void RegistrationManager::SetPalette(const unsigned char* palette) {
	int		i;

	byte* rp = (byte*)rawPalette;

	if (palette) {
		for (i = 0; i < 256; i++) {
			rp[i * 4 + 0] = palette[i * 3 + 0];
			rp[i * 4 + 1] = palette[i * 3 + 1];
			rp[i * 4 + 2] = palette[i * 3 + 2];
			rp[i * 4 + 3] = 0xff;
		}
	}
	else {
		for (i = 0; i < 256; i++) {
			rp[i * 4 + 0] = d_8to24table[i] & 0xff;
			rp[i * 4 + 1] = (d_8to24table[i] >> 8) & 0xff;
			rp[i * 4 + 2] = (d_8to24table[i] >> 16) & 0xff;
			rp[i * 4 + 3] = 0xff;
		}
	}
	// setting texture palette
	// 
	//int i;
	//unsigned char temptable[768];
	//
	//for (i = 0; i < 256; i++) {
	//	temptable[i * 3 + 0] = (palette[i] >> 0) & 0xff;
	//	temptable[i * 3 + 1] = (palette[i] >> 8) & 0xff;
	//	temptable[i * 3 + 2] = (palette[i] >> 16) & 0xff;
	//}

}

void RegistrationManager::EndRegistration() {

	int		i;


	for (i = 0; i < numModels; i++) {
		model_t* mod = dxModels[i];
		if (!mod->name[0])
			continue;
		if (mod->registration_sequence != registration_sequence) {	// don't need this model
			delete[] (mod->skins);
			if (mod->extradata != nullptr)
				Hunk_Free(mod->extradata);
			memset(mod, 0, sizeof(*mod));
		}
	}


	image_t* image;

	// never free r_notexture or particle texture
	//r_notexture->registration_sequence = registration_sequence;
	//r_particletexture->registration_sequence = registration_sequence;

	for (int i = 0; i < numTextures; i++) {

		if (dxTextures[i]->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (dxTextures[i]->registration_sequence == -1)
			continue;		// free image_t slot
		if (dxTextures[i]->type == it_pic)
			continue;		// don't free pics
		// free it
		RD.ReleaseTexture(dxTextures[i]->texnum);
		dxTextures[i]->texnum = -1;
	}
	RD.Flush();
}

image_t* RegistrationManager::FindImage(const char* name, imagetype_t type) {
	image_t* image = nullptr;
	int		i, len;
	byte* pic, * palette;
	int		width, height;

	if (!name)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");
	len = strlen(name);
	if (len < 5)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

	for (i = 0; i < numTextures; i++) {
		if (!strcmp(name, dxTextures[i]->name)) {
			dxTextures[i]->registration_sequence = registration_sequence;
			return dxTextures[i];
		}
	}



	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;
	if (!strcmp(name + len - 4, ".pcx")) {
		LoadPCX(name, &pic, &palette, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
		image = LoadPic((char*)name, pic, width, height, type, 8);
	}
	else if (!strcmp(name + len - 4, ".wal")) {
		image = LoadWal((char*)name);
	}
	else if (!strcmp(name + len - 4, ".tga")) {
		LoadTGA(name, &pic, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
		image = LoadPic((char*)name, pic, width, height, type, 32);
	}
	else
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name);


	if (pic)
		free(pic);
	if (palette)
		free(palette);

	return image;
}

image_t* RegistrationManager::FindImage(int id) {
	return dxTextures[id];
}

model_t* loadmodel;

model_t* RegistrationManager::FindModel(int id, qboolean crash) {
	return dxModels[id];
}

model_t* RegistrationManager::FindModel(const char* name, qboolean crash) {
	model_t* mod;
	void* buf;
	int		i;

	if (!name[0])
		ri.Sys_Error(ERR_DROP, "Mod_ForName: NULL name");

	//
	// inline models are grabbed only from worldmodel
	
	if (name[0] == '*') {
		i = atoi(name + 1);
		if (i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels)
			ri.Sys_Error(ERR_DROP, "bad inline model number");
		return &mod_inline[i];
	}

	//
	// search the currently loaded models
	//
	for (i = 0; i < numModels; i++) {
		if (!dxModels[i]->name[0])
			continue;
		if (!strcmp(dxModels[i]->name, name))
			return dxModels[i];
	}

	//
	// find a free model slot spot
	//
	for (i = 0; i < numModels; i++) {
		if (!dxModels[i]->name[0])
			break;	// free spot
	}

	if (i == numModels) {
		if (numModels == MAX_MOD_KNOWN)
			ri.Sys_Error(ERR_DROP, "mod_numknown == MAX_MOD_KNOWN");
		numModels++;
	}
	mod = dxModels[i];


	strcpy_s<MAX_QPATH>(mod->name, name);

	//
	// load the file
	//
	int modfilelen = ri.FS_LoadFile(mod->name, &buf);
	if (!buf) {
		if (crash)
			ri.Sys_Error(ERR_DROP, "Mod_NumForName: %s not found", mod->name);
		memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}

	loadmodel = mod;

	//
	// fill it in
	//


	// call the apropriate loader

	switch (LittleLong(*(unsigned*)buf)) {
	case IDALIASHEADER:
		RD.RegisterFramedModel(i, RM.LoadAliasModel(loadmodel, buf));
		loadmodel->modelId = i;
		mod->type = mod_alias;
		break;

	case IDSPRITEHEADER:
		//loadmodel->extradata = Hunk_Begin(0x10000);
		//Mod_LoadSpriteModel(mod, buf);
		break;

	case IDBSPHEADER:
		loadmodel->extradata = Hunk_Begin(0x1000000);
		LoadBrushModel(mod, buf);
		mod->type == mod_brush;
		break;

	default:
		ri.Sys_Error(ERR_DROP, "Mod_NumForName: unknown fileid for %s", mod->name);
		break;
	}

	//loadmodel->extradatasize = Hunk_End();

	ri.FS_FreeFile(buf);

	return mod;
}



FramedModelData RegistrationManager::LoadAliasModel(model_t* mod, void* file) {

	FramedModelData result;

	// check for the ident and the version number

	dmdl_t* header = (dmdl_t*)file;			// md2 header 
	
	if ((header->ident != IDALIASHEADER) && (header->version != ALIAS_VERSION)) {
		// this is not a MD2 model
		//file.close();
		printf(mod->name);
		printf("\n");
		printf("Bad model");
		return result;
	}

	/////////////////////////////////////////////


	// initialize member variables
	int num_frames = header->num_frames;
	int num_xyz = header->num_xyz;
	mod->num_frames = header->num_frames;

	// allocate memory
	dstvert_t* uvCoordinates = new dstvert_t [header->num_st]; /////////////////////////////////
	dstvert_t* pinst = (dstvert_t*)((byte*)file + header->ofs_st);

	for (int i = 0; i < header->num_st; i++) {
		uvCoordinates[i].s = LittleShort(pinst[i].s);
		uvCoordinates[i].t = LittleShort(pinst[i].t);
	}


	//
	// load triangle lists
	//
	dtriangle_t* pintri = (dtriangle_t*)((byte*)file + header->ofs_tris);
	dtriangle_t* triangles = new dtriangle_t[header->num_tris]; //////////////////////////////
	mod->num_tris = header->num_tris;

	for (int i = 0; i < header->num_tris; i++) {
		for (int j = 0; j < 3; j++) {
			triangles[i].index_xyz[j] = LittleShort(pintri[i].index_xyz[j]);
			triangles[i].index_st[j] = LittleShort(pintri[i].index_st[j]);
		}
	}


	// register all skins
	//char* skinBuff = new char[header.num_skins * MAX_SKINNAME];
	mod->skins = new skinInfo[header->num_skins];
	mod->num_skins = header->num_skins;
	for (int i = 0; i < header->num_skins; i++) {
		memcpy(mod->skins[i].name, (char*)file + header->ofs_skins + i * MAX_SKINNAME, MAX_SKINNAME);
		mod->skins[i].m_texid = FindImage(mod->skins[i].name, it_skin);
	}


	// vertex array initialization

	vec3_t* m_vertices = new vec3_t [num_xyz* num_frames];
	int* m_lightnormals  = new int[num_xyz * num_frames];
	char* buffer = (char*)file + header->ofs_frames;
	for (int frameIndex = 0; frameIndex < num_frames; frameIndex++) {
		// ajust pointers
		daliasframe_t* frame = (daliasframe_t*)&buffer[header->framesize * frameIndex];
		vec3_t* ptrverts = &m_vertices[num_xyz * frameIndex];
		int* ptrnormals = &m_lightnormals[num_xyz * frameIndex];
		for (int i = 0; i < num_xyz; i++) {
			ptrverts[i][0] = (frame->verts[i].v[0] * frame->scale[0]) + frame->translate[0];
			ptrverts[i][1] = (frame->verts[i].v[1] * frame->scale[1]) + frame->translate[1];
			ptrverts[i][2] = (frame->verts[i].v[2] * frame->scale[2]) + frame->translate[2];

			ptrnormals[i] = frame->verts[i].lightnormalindex;
		}
	}


	int index = 0;
	result.pt = Renderer::PrimitiveType::PRIMITIVETYPE_TRIANGLELIST;
	result.frames.resize(num_frames);
	//mod->realModel.frames.shrink_to_fit();;
	//for (int frameIndex = 0; frameIndex < num_frames; frameIndex++) {
	//	dx11Model.frames[frameIndex].resize(num_xyz);
	//	dx11Model.frames[frameIndex].shrink_to_fit();
	//	for (int i = 0; i < num_xyz; i++) {
	//		dx11Model.frames[frameIndex][i] = float3(ptrverts[i][0], ptrverts[i][1], ptrverts[i][2]);
	//	}
	//}

	result.pt = Renderer::PrimitiveType::PRIMITIVETYPE_TRIANGLELIST;
	//dx11Model.indexes.resize(header.num_tris * 3);
	//dx11Model.verticies.resize(header.num_tris * 3);
	//dx11Model.primitiveCount = header.num_tris;
	std::vector<Frame> verticies3(num_frames);
	verticies3.shrink_to_fit();
	for (int frameIndex = 0; frameIndex < num_frames; frameIndex++) {
		verticies3[frameIndex].resize(3);
		verticies3[frameIndex].shrink_to_fit();
	}



	for (int i = 0; i < header->num_tris; i++) {
		int index1 = triangles[i].index_xyz[0];
		int index2 = triangles[i].index_xyz[1];
		int index3 = triangles[i].index_xyz[2];

		for (int frameIndex = 0; frameIndex < num_frames; frameIndex++) {
			//verticies3[frameIndex].resize(3);
			vec3_t* ptrverts = &m_vertices[num_xyz * frameIndex];
			//ptrnormals = &m_lightnormals[num_xyz * frameIndex];
			int* ptrnormals = &m_lightnormals[num_xyz * frameIndex];
			verticies3[frameIndex][0].position = float3(ptrverts[index1][0], ptrverts[index1][1], ptrverts[index1][2]);
			verticies3[frameIndex][1].position = float3(ptrverts[index2][0], ptrverts[index2][1], ptrverts[index2][2]);
			verticies3[frameIndex][2].position = float3(ptrverts[index3][0], ptrverts[index3][1], ptrverts[index3][2]);
			

			verticies3[frameIndex][0].normal = float3(r_avertexnormals[ptrnormals[index1]]);
			verticies3[frameIndex][1].normal = float3(r_avertexnormals[ptrnormals[index2]]);
			verticies3[frameIndex][2].normal = float3(r_avertexnormals[ptrnormals[index3]]);

			//verticies3[frameIndex][0].normalDot = (shadedots[ptrnormals[index1]]);
			//verticies3[frameIndex][1].normalDot = (shadedots[ptrnormals[index2]]);
			//verticies3[frameIndex][2].normalDot = (shadedots[ptrnormals[index3]]);
		}

		FramedModelCommon uvs[3];

		//float2 WH = float2(frameSkins[0].m_texid->width, frameSkins[0].m_texid->height);
		uvs[0].texcoord = float2(uvCoordinates[triangles[i].index_st[0]].s, uvCoordinates[triangles[i].index_st[0]].t);
		uvs[1].texcoord = float2(uvCoordinates[triangles[i].index_st[1]].s, uvCoordinates[triangles[i].index_st[1]].t);
		uvs[2].texcoord = float2(uvCoordinates[triangles[i].index_st[2]].s, uvCoordinates[triangles[i].index_st[2]].t);

		//uvNormals[1]->s = pstverts[ptri->index_st[1]].s << 16;
		//uvNormals[1]->t = pstverts[ptri->index_st[1]].t << 16;
		//
		//uvNormals[2]->s = pstverts[ptri->index_st[2]].s << 16;
		//uvNormals[2]->t = pstverts[ptri->index_st[2]].t << 16;

		result.AddTriangle(verticies3, uvs);
	}


	// free buffer's memory
	delete[] m_vertices;
	delete[] m_lightnormals;
	delete[] uvCoordinates;
	delete[] triangles;

	return result;
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

byte* mod_base;


/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting(lump_t* l) {
	if (!l->filelen) {
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = (byte*)Hunk_Alloc(l->filelen);
	memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility(lump_t* l) {
	int		i;

	if (!l->filelen) {
		loadmodel->vis = NULL;
		return;
	}
	loadmodel->vis = (dvis_t*)Hunk_Alloc(l->filelen);
	memcpy(loadmodel->vis, mod_base + l->fileofs, l->filelen);

	loadmodel->vis->numclusters = LittleLong(loadmodel->vis->numclusters);
	for (i = 0; i < loadmodel->vis->numclusters; i++) {
		loadmodel->vis->bitofs[i][0] = LittleLong(loadmodel->vis->bitofs[i][0]);
		loadmodel->vis->bitofs[i][1] = LittleLong(loadmodel->vis->bitofs[i][1]);
	}
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes(lump_t* l) {
	dvertex_t* in;
	mvertex_t* out;
	int			i, count;

	in = (dvertex_t*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mvertex_t*)Hunk_Alloc(count * sizeof(*out));

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for (i = 0; i < count; i++, in++, out++) {
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs) {
	int		i;
	vec3_t	corner;

	for (i = 0; i < 3; i++) {
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength(corner);
}


/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels(lump_t* l) {
	dmodel_t* in;
	mmodel_t* out;
	int			i, j, count;

	in = (dmodel_t*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mmodel_t*)Hunk_Alloc(count * sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++) {
		for (j = 0; j < 3; j++) {	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		out->radius = RadiusFromBounds(out->mins, out->maxs);
		out->headnode = LittleLong(in->headnode);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges(lump_t* l) {
	dedge_t* in;
	medge_t* out;
	int 	i, count;

	in = (dedge_t*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (medge_t*)Hunk_Alloc((count + 1) * sizeof(*out));

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for (i = 0; i < count; i++, in++, out++) {
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo(lump_t* l) {
	texinfo_t* in;
	mtexinfo_t* out, * step;
	int 	i, j, count;
	char	name[MAX_QPATH];
	int		next;

	in = (texinfo_t*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mtexinfo_t*)Hunk_Alloc(count * sizeof(*out));

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for (i = 0; i < count; i++, in++, out++) {
		for (j = 0; j < 8; j++)
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);

		out->flags = LittleLong(in->flags);
		next = LittleLong(in->nexttexinfo);
		if (next > 0)
			out->next = loadmodel->texinfo + next;
		else
			out->next = NULL;
		Com_sprintf(name, sizeof(name), "textures/%s.wal", in->texture);

		out->image = RM.FindImage(name, it_wall);
		if (!out->image) {
			ri.Con_Printf(PRINT_ALL, "Couldn't load %s\n", name);
			out->image = r_notexture;
		}
	}

	// count animation frames
	for (i = 0; i < count; i++) {
		out = &loadmodel->texinfo[i];
		out->numframes = 1;
		for (step = out->next; step && step != out; step = step->next)
			out->numframes++;
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents(msurface_t* s) {
	float	mins[2], maxs[2], val;
	int		i, j, e;
	mvertex_t* v;
	mtexinfo_t* tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++) {
		e = loadmodel->surfedges[s->firstedge + i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j = 0; j < 2; j++) {
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0; i < 2; i++) {
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

		//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256 */ )
		//			ri.Sys_Error (ERR_DROP, "Bad surface extents");
	}
}



void GL_BuildPolygonFromSurface(msurface_t* fa);
void GL_CreateSurfaceLightmap(msurface_t* surf);
void GL_EndBuildingLightmaps(void);
void GL_BeginBuildingLightmaps(model_t* m);

/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces(lump_t* l) {
	dface_t* in;
	msurface_t* out;
	int			i, count, surfnum;
	int			planenum, side;
	int			ti;

	in = (dface_t*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t*)Hunk_Alloc(count * sizeof(*out));

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	currentmodel = loadmodel;

	GL_BeginBuildingLightmaps(loadmodel);

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++) {
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->flags = 0;
		out->polys = NULL;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		ti = LittleShort(in->texinfo);
		if (ti < 0 || ti >= loadmodel->numtexinfo)
			ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: bad texinfo number");
		out->texinfo = loadmodel->texinfo + ti;

		CalcSurfaceExtents(out);

		// lighting info

		for (i = 0; i < MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;

		// set the drawing flags

		if (out->texinfo->flags & SURF_WARP) {
			out->flags |= SURF_DRAWTURB;
			for (i = 0; i < 2; i++) {
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface(out);	// cut up polygon for warps
		}

		// create lightmaps and polygons
		if (!(out->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
			GL_CreateSurfaceLightmap(out);

		if (!(out->texinfo->flags & SURF_WARP))
			GL_BuildPolygonFromSurface(out);

	}

	GL_EndBuildingLightmaps();
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent(mnode_t* node, mnode_t* parent) {
	node->parent = parent;
	if (node->contents != -1)
		return;
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes(lump_t* l) {
	int			i, j, count, p;
	dnode_t* in;
	mnode_t* out;

	in = (dnode_t*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mnode_t*)Hunk_Alloc(count * sizeof(*out));

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (i = 0; i < count; i++, in++, out++) {
		for (j = 0; j < 3; j++) {
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j = 0; j < 2; j++) {
			p = LittleLong(in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t*)(loadmodel->leafs + (-1 - p));
		}
	}

	Mod_SetParent(loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs(lump_t* l) {
	dleaf_t* in;
	mleaf_t* out;
	int			i, j, count, p;
	//	glpoly_t	*poly;

	in = (dleaf_t*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mleaf_t*)Hunk_Alloc(count * sizeof(*out));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++) {
		for (j = 0; j < 3; j++) {
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleShort(in->firstleafface);
		out->nummarksurfaces = LittleShort(in->numleaffaces);

		// gl underwater warp
#if 0
		if (out->contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_THINWATER)) {
			for (j = 0; j < out->nummarksurfaces; j++) {
				out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				for (poly = out->firstmarksurface[j]->polys; poly; poly = poly->next)
					poly->flags |= SURF_UNDERWATER;
			}
		}
#endif
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces(lump_t* l) {
	int		i, j, count;
	short* in;
	msurface_t** out;

	in = (short*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t**)Hunk_Alloc(count * sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (i = 0; i < count; i++) {
		j = LittleShort(in[i]);
		if (j < 0 || j >= loadmodel->numsurfaces)
			ri.Sys_Error(ERR_DROP, "Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges(lump_t* l) {
	int		i, count;
	int* in, * out;

	in = (int*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	if (count < 1 || count >= MAX_MAP_SURFEDGES)
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: bad surfedges count in %s: %i",
			loadmodel->name, count);

	out = (int*)Hunk_Alloc(count * sizeof(*out));

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for (i = 0; i < count; i++)
		out[i] = LittleLong(in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes(lump_t* l) {
	int			i, j;
	cplane_t* out;
	dplane_t* in;
	int			count;
	int			bits;

	in = (dplane_t*)((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		ri.Sys_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (cplane_t*)Hunk_Alloc(count * 2 * sizeof(*out));

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for (i = 0; i < count; i++, in++, out++) {
		bits = 0;
		for (j = 0; j < 3; j++) {
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);
		out->signbits = bits;
	}
}




void RegistrationManager::LoadBrushModel(model_t* mod, void* buffer) {
	int			i;
	dheader_t* header;
	mmodel_t* bm;

	loadmodel->type = mod_brush;
	if (loadmodel != dxModels[0])
		ri.Sys_Error(ERR_DROP, "Loaded a brush model after the world");

	header = (dheader_t*)buffer;

	i = LittleLong(header->version);
	if (i != BSPVERSION)
		ri.Sys_Error(ERR_DROP, "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);

	// swap all the lumps
	mod_base = (byte*)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int*)header)[i] = LittleLong(((int*)header)[i]);

	// load into heap

	Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges(&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces(&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces(&header->lumps[LUMP_LEAFFACES]);
	Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs(&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes(&header->lumps[LUMP_NODES]);
	Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);
	mod->num_frames = 2;		// regular and alternate animation

//
// set up the submodels
//
	for (i = 0; i < mod->numsubmodels; i++) {
		model_t* starmod;

		bm = &mod->submodels[i];
		starmod = &mod_inline[i];

		*starmod = *loadmodel;

		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces = bm->numfaces;
		starmod->firstnode = bm->headnode;
		if (starmod->firstnode >= loadmodel->numnodes)
			ri.Sys_Error(ERR_DROP, "Inline model %i has bad firstnode", i);

		VectorCopy(bm->maxs, starmod->maxs);
		VectorCopy(bm->mins, starmod->mins);
		starmod->radius = bm->radius;

		if (i == 0)
			*loadmodel = *starmod;

		starmod->numleafs = bm->visleafs;
	}
}

void RegistrationManager::Mod_Free(model_t* mod) {
	{
		Hunk_Free(mod->extradata);
		if (mod->skins != nullptr)
			delete mod->skins;
		memset(mod, 0, sizeof(*mod));
	}
}


const int		gl_solid_format = 3;
const int		gl_alpha_format = 4;
int		upload_width, upload_height;
bool uploaded_paletted;

void  RegistrationManager::BuildPalettedTexture(unsigned char* paletted_texture, unsigned char* scaled, int scaled_width, int scaled_height) {
	int i;

	for (i = 0; i < scaled_width * scaled_height; i++) {
		unsigned int r, g, b, c;

		r = (scaled[0] >> 3) & 31;
		g = (scaled[1] >> 2) & 63;
		b = (scaled[2] >> 3) & 31;

		c = r | (g << 5) | (b << 11);

		paletted_texture[i] = d_16to8table[c];

		scaled += 4;
	}
}

void RegistrationManager::CreateLightmap(int width, int height, int bits, unsigned char* data, int texNum, bool mipmap) {
	
}

RegistrationManager::~RegistrationManager() {
	for (int i = 0; i < MAX_DXTEXTURES; i++) {
		delete dxTextures[i];
	}
	for (int i = 0; i < MAX_MOD_KNOWN; i++) {
		delete dxModels[i];
	}
}



TextureData  RegistrationManager::GL_Upload32(uint32_t* data, int width, int height, bool mipmap) {
	int			samples;
	unsigned	scaled[256 * 256];
	unsigned char paletted_texture[256 * 256];
	int			scaled_width, scaled_height;
	int			i, c;
	byte* scan;
	int comp;

	uploaded_paletted = False;

	for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
		;
	//if (gl_round_down->value && scaled_width > width && mipmap)
	//	scaled_width >>= 1;
	for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
		;
	//if (gl_round_down->value && scaled_height > height && mipmap)
	//	scaled_height >>= 1;

	//// let people sample down the world textures for speed
	//if (mipmap) {
	//	scaled_width >>= (int)gl_picmip->value;
	//	scaled_height >>= (int)gl_picmip->value;
	//}

	// don't ever bother with >256 textures
	if (scaled_width > 256)
		scaled_width = 256;
	if (scaled_height > 256)
		scaled_height = 256;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled) / 4)
		ri.Sys_Error(ERR_DROP, "GL_Upload32: too big");

	// scan the texture for any non-255 alpha
	c = width * height;
	scan = ((byte*)data) + 3;
	samples = gl_solid_format;
	for (i = 0; i < c; i++, scan += 4) {
		if (*scan != 255) {
			samples = gl_alpha_format;
			break;
		}
	}

	//if (samples == gl_solid_format)
	//	comp = gl_tex_solid_format;
	//else if (samples == gl_alpha_format)
	//	comp = gl_tex_alpha_format;
	//else {
	//	ri.Con_Printf(PRINT_ALL,
	//		"Unknown number of texture components %i\n",
	//		samples);
	//	comp = samples;
	//}


	//if (scaled_width == width && scaled_height == height) {
	//
	//	Texture tex = Texture(scaled_width, scaled_height);
	//	for (size_t w = 0; w < scaled_width; w++) {
	//		for (size_t h = 0; h < scaled_height; h++) {
	//			tex.PutPixel(w, h, data[w + h * scaled_width]);
	//		}
	//	}
	//	return tex;
	//	//qglTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//
	//
	////memcpy(scaled, data, width * height * 4);
	//} else
	//	GL_ResampleTexture(data, width, height, scaled, scaled_width, scaled_height);
	//
	//GL_LightScaleTexture(scaled, scaled_width, scaled_height, !mipmap);

	//if (qglColorTableEXT && gl_ext_palettedtexture->value && (samples == gl_solid_format)) {
	//	uploaded_paletted = True;
	//	GL_BuildPalettedTexture(paletted_texture, (unsigned char*)scaled, scaled_width, scaled_height);
	//	qglTexImage2D(GL_TEXTURE_2D,
	//		0,
	//		GL_COLOR_INDEX8_EXT,
	//		scaled_width,
	//		scaled_height,
	//		0,
	//		GL_COLOR_INDEX,
	//		GL_UNSIGNED_BYTE,
	//		paletted_texture);
	//} else {
	//	qglTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	//}


	TextureData tex = TextureData(width, height);
	for (size_t w = 0; w < width; w++) {
		for (size_t h = 0; h < height; h++) {

			auto c = TextureData::Color(data[w + h * width]);
			uint8_t b = c.GetR();
			c.SetR(c.GetB());
			c.SetB(b);
			tex.PutPixel(w, h, data[w + h * width]);
			//tex.PutPixel(w, h, data[w + h*width]);
		}
	}
	return tex;
}

TextureData RegistrationManager::GL_Upload8(byte* data, int width, int height, bool mipmap, bool is_sky) {
	unsigned	trans[512 * 256];
	int			i, s;
	int			p;

	s = width * height;

	for (i = 0; i < s; i++) {
		p = data[i];
		trans[i] = d_8to24table[p];

		if (p == 255) {	// transparent, so scan around for another color
			// to avoid alpha fringes
			// FIXME: do a full flood fill so mips work...
			if (i > width && data[i - width] != 255)
				p = data[i - width];
			else if (i < s - width && data[i + width] != 255)
				p = data[i + width];
			else if (i > 0 && data[i - 1] != 255)
				p = data[i - 1];
			else if (i < s - 1 && data[i + 1] != 255)
				p = data[i + 1];
			else
				p = 0;
			// copy rgb components
			((byte*)&trans[i])[0] = ((byte*)&d_8to24table[p])[0];
			((byte*)&trans[i])[1] = ((byte*)&d_8to24table[p])[1];
			((byte*)&trans[i])[2] = ((byte*)&d_8to24table[p])[2];
		}
	}

	return GL_Upload32(trans, width, height, mipmap);

}


image_t* RegistrationManager::LoadPic(char* name, byte* pic, int width, int height, imagetype_t type, int bits) {
	image_t* image;
	int			i;

	// find a free image_t
	for (i = 0; i < numTextures; i++) {
		if (dxTextures[i]->texnum == -1)
			break;
	}

	if (i == numTextures) {
		if (numTextures == MAX_DXTEXTURES)
			ri.Sys_Error(ERR_DROP, "MAX_GLTEXTURES");
		numTextures++;
	}
	image = dxTextures[i];

	if (strlen(name) >= sizeof(image->name))
		ri.Sys_Error(ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	strcpy_s<256>(image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	//if (type == it_skin && bits == 8)
	//	R_FloodFillSkin(pic, width, height);

	// load little pics into the scrap
	{
		image->texnum = i;
		//GL_Bind(image->texnum);
		TextureData tex = TextureData(0, 0);
		if (bits == 8)
			tex = GL_Upload8(pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_sky);
		else
			tex = GL_Upload32((unsigned*)pic, width, height, (image->type != it_pic && image->type != it_sky));
			// after power of 2 and scales
		
		//image->paletted = uploaded_paletted;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
		RD.RegisterTexture(i, tex);
	}

	return image;
}

void RegistrationManager::LoadPCX(const char* filename, byte** pic, byte** palette, int* width, int* height) {
	byte* raw;
	pcx_t* pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte* out, * pix;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = ri.FS_LoadFile((char*)filename, (void**)&raw);
	if (!raw) {
		ri.Con_Printf(PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t*)raw;

	pcx->xmin = LittleShort(pcx->xmin);
	pcx->ymin = LittleShort(pcx->ymin);
	pcx->xmax = LittleShort(pcx->xmax);
	pcx->ymax = LittleShort(pcx->ymax);
	pcx->hres = LittleShort(pcx->hres);
	pcx->vres = LittleShort(pcx->vres);
	pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
	pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480) {
		ri.Con_Printf(PRINT_ALL, "Bad pcx file %s\n", filename);
		return;
	}

	out = (byte*)malloc((pcx->ymax + 1) * (pcx->xmax + 1));

	*pic = out;

	pix = out;

	if (palette) {
		*palette = (byte*)malloc(768);
		memcpy(*palette, (byte*)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax + 1;
	if (height)
		*height = pcx->ymax + 1;

	for (y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1) {
		for (x = 0; x <= pcx->xmax; ) {
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0) {
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if (raw - (byte*)pcx > len) {
		ri.Con_Printf(PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		free(*pic);
		*pic = NULL;
	}

	ri.FS_FreeFile(pcx);
}

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;

void RegistrationManager::LoadTGA(const char* name, byte** pic, int* width, int* height) {
	int		columns, rows, numPixels;
	byte* pixbuf;
	int		row, column;
	byte* buf_p;
	byte* buffer;
	int		length;
	TargaHeader		targa_header;
	byte* targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_LoadFile((char*)name, (void**)&buffer);
	if (!buffer) {
		ri.Con_Printf(PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort(*((short*)tmp));
	buf_p += 2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort(*((short*)tmp));
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.y_origin = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.width = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.height = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type != 2
		&& targa_header.image_type != 10)
		ri.Sys_Error(ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");

	if (targa_header.colormap_type != 0
		|| (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
		ri.Sys_Error(ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = (byte*)malloc(numPixels * 4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment

	if (targa_header.image_type == 2) {  // Uncompressed, RGB images
		for (row = rows - 1; row >= 0; row--) {
			pixbuf = targa_rgba + row * columns * 4;
			for (column = 0; column < columns; column++) {
				unsigned char red, green, blue, alphabyte;
				switch (targa_header.pixel_size) {
				case 24:

					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;
				}
			}
		}
	}
	else if (targa_header.image_type == 10) {   // Runlength encoded RGB images
		unsigned char red, green, blue, alphabyte, packetHeader, packetSize, j;
		for (row = rows - 1; row >= 0; row--) {
			pixbuf = targa_rgba + row * columns * 4;
			for (column = 0; column < columns; ) {
				packetHeader = *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						break;
					}

					for (j = 0; j < packetSize; j++) {
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if (column == columns) { // run spans across rows
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else {                            // non run-length packet
					for (j = 0; j < packetSize; j++) {
						switch (targa_header.pixel_size) {
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
						}
						column++;
						if (column == columns) { // pixel packet run spans across rows
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
		breakOut:;
		}
	}

	ri.FS_FreeFile(buffer);
}
