#include "RegistrationManager.h"
#include "dx11rg_local.h"
#include	<fstream>

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
	//cvar_t* flushmap;

	registration_sequence++;
	//r_oldviewcluster = -1;		// force markleafs

	//Com_sprintf(fullname, sizeof(fullname), "maps/%s.bsp", map);

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	//flushmap = ri.Cvar_Get("flushmap", "0", 0);
	//if (strcmp(mod_known[0].name, fullname) || flushmap->value)
	//	Mod_Free(&mod_known[0]);
	//r_worldmodel = Mod_ForName(fullname, True);
	//
	//r_viewcluster = -1;
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
			delete (mod->realModel);
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
		//image = GL_LoadWal(name);
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

model_t* loadmodel;

model_t* RegistrationManager::FindModel(const char* name, qboolean crash) {
	model_t* mod;
	void* buf;
	int		i;

	if (!name[0])
		ri.Sys_Error(ERR_DROP, "Mod_ForName: NULL name");

	//
	// inline models are grabbed only from worldmodel
	//
	//if (name[0] == '*') {
	//	i = atoi(name + 1);
	//	if (i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels)
	//		ri.Sys_Error(ERR_DROP, "bad inline model number");
	//	return &mod_inline[i];
	//}

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


	strcpy(mod->name, name);

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
		loadmodel->realModel = new CMD2Model();
		loadmodel->realModel->LoadModel(buf);
		printf("%i\n",i);
		if (i == 20) {
			RD.RegisterFramedModel(i, loadmodel->realModel->dx11Model);
			while (true) {
				RD.BeginFrame();
				RD.Clear(0, 0, 0);
				RD.DrawFramedModel(i, loadmodel->realModel->frameSkins[0].m_texid->texnum, Transform({ 0,0,0 }, { 0,0,0 }, { 100,100,100 }), 0, 1, 0.0, LERP);
				RD.Present();
			}
		}
		//LoadAliasModel(mod, buf);
		break;

	case IDSPRITEHEADER:
		//loadmodel->extradata = Hunk_Begin(0x10000);
		//Mod_LoadSpriteModel(mod, buf);
		break;

	case IDBSPHEADER:
		//loadmodel->extradata = Hunk_Begin(0x1000000);
		//Mod_LoadBrushModel(mod, buf);
		break;

	default:
		ri.Sys_Error(ERR_DROP, "Mod_NumForName: unknown fileid for %s", mod->name);
		break;
	}

	//loadmodel->extradatasize = Hunk_End();

	ri.FS_FreeFile(buf);

	return mod;
}


//void RegistrationManager::LoadAliasModel(model_t* mod, void* buffer) {
//	int					i, j;
//	dmdl_t* pinmodel, * pheader;
//	dstvert_t* pinst, * poutst;
//	dtriangle_t* pintri, * pouttri;
//	daliasframe_t* pinframe, * poutframe;
//	int* pincmd, * poutcmd;
//	int					version;
//
//	pinmodel = (dmdl_t*)buffer;
//
//	version = LittleLong(pinmodel->version);
//	if (version != ALIAS_VERSION)
//		ri.Sys_Error(ERR_DROP, "%s has wrong version number (%i should be %i)",
//			mod->name, version, ALIAS_VERSION);
//
//	pheader = (dmdl_t*)Hunk_Alloc(LittleLong(pinmodel->ofs_end));
//
//	// byte swap the header fields and sanity check
//	for (i = 0; i < sizeof(dmdl_t) / 4; i++)
//		((int*)pheader)[i] = LittleLong(((int*)buffer)[i]);
//
//	if (pheader->skinheight > MAX_LBM_HEIGHT)
//		ri.Sys_Error(ERR_DROP, "model %s has a skin taller than %d", mod->name,
//			MAX_LBM_HEIGHT);
//
//	if (pheader->num_xyz <= 0)
//		ri.Sys_Error(ERR_DROP, "model %s has no vertices", mod->name);
//
//	if (pheader->num_xyz > MAX_VERTS)
//		ri.Sys_Error(ERR_DROP, "model %s has too many vertices", mod->name);
//
//	if (pheader->num_st <= 0)
//		ri.Sys_Error(ERR_DROP, "model %s has no st vertices", mod->name);
//
//	if (pheader->num_tris <= 0)
//		ri.Sys_Error(ERR_DROP, "model %s has no triangles", mod->name);
//
//	if (pheader->num_frames <= 0)
//		ri.Sys_Error(ERR_DROP, "model %s has no frames", mod->name);
//
//	//
//	// load base s and t vertices (not used in gl version)
//	//
//	pinst = (dstvert_t*)((byte*)pinmodel + pheader->ofs_st);
//	poutst = (dstvert_t*)((byte*)pheader + pheader->ofs_st);
//
//	for (i = 0; i < pheader->num_st; i++) {
//		poutst[i].s = LittleShort(pinst[i].s);
//		poutst[i].t = LittleShort(pinst[i].t);
//	}
//
//	//
//	// load triangle lists
//	//
//	pintri = (dtriangle_t*)((byte*)pinmodel + pheader->ofs_tris);
//	pouttri = (dtriangle_t*)((byte*)pheader + pheader->ofs_tris);
//
//	for (i = 0; i < pheader->num_tris; i++) {
//		for (j = 0; j < 3; j++) {
//			pouttri[i].index_xyz[j] = LittleShort(pintri[i].index_xyz[j]);
//			pouttri[i].index_st[j] = LittleShort(pintri[i].index_st[j]);
//		}
//	}
//
//	//
//	// load the frames
//	//
//	for (i = 0; i < pheader->num_frames; i++) {
//		pinframe = (daliasframe_t*)((byte*)pinmodel
//			+ pheader->ofs_frames + i * pheader->framesize);
//		poutframe = (daliasframe_t*)((byte*)pheader
//			+ pheader->ofs_frames + i * pheader->framesize);
//
//		memcpy(poutframe->name, pinframe->name, sizeof(poutframe->name));
//		for (j = 0; j < 3; j++) {
//			poutframe->scale[j] = LittleFloat(pinframe->scale[j]);
//			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
//		}
//		// verts are all 8 bit, so no swapping needed
//		memcpy(poutframe->verts, pinframe->verts,
//			pheader->num_xyz * sizeof(dtrivertx_t));
//
//	}
//
//	mod->type = mod_alias;
//
//	//
//	// load the glcmds
//	//
//	pincmd = (int*)((byte*)pinmodel + pheader->ofs_glcmds);
//	poutcmd = (int*)((byte*)pheader + pheader->ofs_glcmds);
//	for (i = 0; i < pheader->num_glcmds; i++)
//		poutcmd[i] = LittleLong(pincmd[i]);
//
//
//	// register all skins
//	memcpy((char*)pheader + pheader->ofs_skins, (char*)pinmodel + pheader->ofs_skins,
//		pheader->num_skins * MAX_SKINNAME);
//	for (i = 0; i < pheader->num_skins; i++) {
//		mod->skins[i] = FindImage((char*)pheader + pheader->ofs_skins + i * MAX_SKINNAME
//			, it_skin);
//	}
//
//	mod->mins[0] = -32;
//	mod->mins[1] = -32;
//	mod->mins[2] = -32;
//	mod->maxs[0] = 32;
//	mod->maxs[1] = 32;
//	mod->maxs[2] = 32;
//
//	auto realModel = ConvertAliasModel(mod);
//}




// precalculated normal vectors
vec3_t	CMD2Model::anorms[NUMVERTEXNORMALS] = {
#include	"anorms.h"
};

// precalculated dot product results
float	CMD2Model::anorms_dots[SHADEDOT_QUANT][256] = {
#include	"anormtab.h"
};


static float* shadedots = CMD2Model::anorms_dots[0];
static vec3_t	lcolor;


/////////////////////////////////////////////////

vec3_t			g_lightcolor = { 1.0, 1.0, 1.0 };
int				g_ambientlight = 32;
float			g_shadelight = 128;
float			g_angle = 0.0;

/////////////////////////////////////////////////



// ----------------------------------------------
// constructor - reset all data.
// ----------------------------------------------

CMD2Model::CMD2Model(void) {
	m_vertices = 0;
	m_glcmds = 0;
	m_lightnormals = 0;

	num_frames = 0;
	num_xyz = 0;
	num_glcmds = 0;

	m_scale = 1.0;

	SetAnim(0);
}



// ----------------------------------------------
// destructeur - free allocated memory.
// ----------------------------------------------

CMD2Model::~CMD2Model(void) {
	delete[] m_vertices;
	delete[] m_glcmds;
	delete[] m_lightnormals;
	delete[] frameSkins;
}



// ----------------------------------------------
// LoadModel() - load model from file.
// ----------------------------------------------

bool CMD2Model::LoadModel(void* file) {
	//std::ifstream	file;			// file stream
	md2_t			header;			// md2 header
	char* buffer = (char*)file;		// buffer storing frame data
	frame_t* frame;			// temporary variable
	vec3_t* ptrverts;		// pointer on m_vertices
	int* ptrnormals;	// pointer on m_lightnormals

	dmdl_t* pinmodel = (dmdl_t*)buffer;

	// try to open filename
	//file.open(filename, std::ios::in | std::ios::binary);

	//if (file.fail())
	//	return false;

	// read header file
	//for (int i = 0; i < sizeof(dmdl_t) / 4; i++)
	//	((int*)&header)[i] = LittleLong(((int*)buffer)[i]);
	memcpy((char*)&header, file, sizeof(md2_t));


	/////////////////////////////////////////////
	//		verify that this is a MD2 file

	// check for the ident and the version number

	if ((header.ident != MD2_IDENT) && (header.version != MD2_VERSION)) {
		// this is not a MD2 model
		//file.close();
		return false;
	}

	/////////////////////////////////////////////


	// initialize member variables
	num_frames = header.num_frames;
	num_xyz = header.num_xyz;
	num_glcmds = header.num_glcmds;


	// allocate memory
	m_vertices = new vec3_t[num_xyz * num_frames];
	m_glcmds = new int[num_glcmds];
	m_lightnormals = new int[num_xyz * num_frames];
	buffer = new char[num_frames * header.framesize];


	/////////////////////////////////////////////
	//			reading file data

	// read frame data...
	//file.seekg(header.ofs_frames, std::ios::beg);
	memcpy((char*)buffer, (char*)file + header.ofs_frames, num_frames * header.framesize);

	// read opengl commands...
	//file.seekg(header.ofs_glcmds, std::ios::beg);
	//file.read((char*)m_glcmds, num_glcmds * sizeof(int));
	memcpy((char*)m_glcmds, (char*)file + header.ofs_glcmds, num_glcmds * sizeof(int));

	/////////////////////////////////////////////

		// register all skins
	//char* skinBuff = new char[header.num_skins * MAX_SKINNAME];
	frameSkins = new frameInfo[header.num_skins];
	for (int i = 0; i < header.num_skins; i++) {
		memcpy(frameSkins[i].name, (char*)file + header.ofs_skins + i * MAX_SKINNAME, MAX_SKINNAME);
		frameSkins[i].m_texid = RM.FindImage(frameSkins[i].name, it_skin);
	}
	//for (int i = 0; i < header.num_skins; i++) {
	//	mod->skins[i] = GL_FindImage((char*)pheader + pheader->ofs_skins + i * MAX_SKINNAME
	//		, it_skin);
	//}


	//frames = new frameInfo[num_frames];

	int index = 0;
	dx11Model.pt = Renderer::PrimitiveType::PRIMITIVETYPE_TRIANGLELIST;
	// vertex array initialization
	for (int frameIndex = 0; frameIndex < num_frames; frameIndex++) {
		// ajust pointers
		frame = (frame_t*)&buffer[header.framesize * frameIndex];
		ptrverts = &m_vertices[num_xyz * frameIndex];
		ptrnormals = &m_lightnormals[num_xyz * frameIndex];
		for (int i = 0; i < num_xyz; i++) {
			ptrverts[i][0] = (frame->verts[i].v[0] * frame->scale[0]) + frame->translate[0];
			ptrverts[i][1] = (frame->verts[i].v[1] * frame->scale[1]) + frame->translate[1];
			ptrverts[i][2] = (frame->verts[i].v[2] * frame->scale[2]) + frame->translate[2];

			ptrnormals[i] = frame->verts[i].lightnormalindex;
		}
		dx11Model.frames.resize(dx11Model.frames.size() + 1);

		int* ptricmds = m_glcmds;
		int isTriangleStrip = 0;
		// draw each triangle!
		while (int i = *(ptricmds++)) {
			if (i < 0) {
				//glBegin(GL_TRIANGLE_FAN);
				i = -i;
			}
			else {
				//glBegin(GL_TRIANGLE_STRIP);
				isTriangleStrip = 1;
			}
			ModelVertex lastLastVertex;
			int lastLastIndex = index++;
			ModelVertex lastVertex;
			int lastIndex = index++;
			{
				float l = shadedots[m_lightnormals[ptricmds[2]]];
				lastLastVertex = ModelVertex{
				float3(ptrverts[ptricmds[2]][0], ptrverts[ptricmds[2]][1], ptrverts[ptricmds[2]][2]),
				float2(l,l),
				float2(((float*)ptricmds)[0], ((float*)ptricmds)[1])
				};
				ptricmds += 3;
				i--;
				//dx11Model.frames[frameIndex].push_back(lastLastVertex.position);

				l = shadedots[m_lightnormals[ptricmds[2]]];
				lastVertex = ModelVertex{
				float3(ptrverts[ptricmds[2]][0], ptrverts[ptricmds[2]][1], ptrverts[ptricmds[2]][2]),
				float2(l,l),
				float2(((float*)ptricmds)[0], ((float*)ptricmds)[1])
				};

				//dx11Model.frames[frameIndex].push_back(lastVertex.position);

				ptricmds += 3;
				i--;
			}

			for ( /* nothing */; i > 0; i--, ptricmds += 3) {

				// ptricmds[0] : texture coordinate s
				// ptricmds[1] : texture coordinate t
				// ptricmds[2] : vertex index to render
				float l = shadedots[m_lightnormals[ptricmds[2]]];
				ModelVertex vertex = ModelVertex{
					float3(ptrverts[ptricmds[2]][0], ptrverts[ptricmds[2]][1], ptrverts[ptricmds[2]][2]),
					float2(l,l),
					float2(((float*)ptricmds)[0], ((float*)ptricmds)[1])
				};
				if (isTriangleStrip) {

					dx11Model.frames[frameIndex].push_back(lastLastVertex.position);
					dx11Model.frames[frameIndex].push_back(lastVertex.position);
					dx11Model.frames[frameIndex].push_back(vertex.position);

					if (frameIndex == 0) {
						if (index & 1) {
							dx11Model.verticies.push_back(FramedModelVertex{ lastLastVertex.normal, lastLastVertex.texcoord });
							dx11Model.verticies.push_back(FramedModelVertex{ lastVertex.normal, lastVertex.texcoord });
							dx11Model.verticies.push_back(FramedModelVertex{ vertex.normal, vertex.texcoord });
							dx11Model.indexes.push_back(lastLastIndex);
							dx11Model.indexes.push_back(index);
							dx11Model.indexes.push_back(lastIndex);
						}
						else {
							dx11Model.verticies.push_back(FramedModelVertex{ lastLastVertex.normal, lastLastVertex.texcoord });
							dx11Model.verticies.push_back(FramedModelVertex{ lastVertex.normal, lastVertex.texcoord });
							dx11Model.verticies.push_back(FramedModelVertex{ vertex.normal, vertex.texcoord });
							dx11Model.indexes.push_back(lastLastIndex);
							dx11Model.indexes.push_back(lastIndex);
							dx11Model.indexes.push_back(index);

						}
					}

					lastLastIndex = ++index;
					lastIndex = ++index;
					index++;

					lastLastVertex = lastVertex;
					lastVertex = vertex;
				}
				else {
					dx11Model.frames[frameIndex].push_back(lastLastVertex.position);
					dx11Model.frames[frameIndex].push_back(lastVertex.position);
					dx11Model.frames[frameIndex].push_back(vertex.position);

					if (frameIndex == 0) {
						dx11Model.verticies.push_back(FramedModelVertex{ lastLastVertex.normal, lastLastVertex.texcoord });
						dx11Model.verticies.push_back(FramedModelVertex{ lastVertex.normal, lastVertex.texcoord });
						dx11Model.verticies.push_back(FramedModelVertex{ vertex.normal, vertex.texcoord });
						dx11Model.indexes.push_back(lastLastIndex);
						dx11Model.indexes.push_back(lastIndex);
						dx11Model.indexes.push_back(index);
					}
					lastLastIndex += 3;
					lastIndex = index + 3;
					index += 3;
					lastVertex = vertex;
				}
			}

			//glEnd();
		}
	}

	dx11Model.primitiveCount = dx11Model.indexes.size() / 3;
	dx11Model.pt = Renderer::PrimitiveType::PRIMITIVETYPE_TRIANGLELIST;


	// free buffer's memory
	delete[] buffer;

	// close the file and return
	//file.close();
	return true;
}


// ----------------------------------------------
// DrawModel() - draw the model.
// ----------------------------------------------

void CMD2Model::DrawModel(float time, Transform position) {
	// animate. calculate current frame and next frame
	if (time > 0.0)
		Animate(time);

	//glPushMatrix();
	// rotate the model
	//glRotatef(-90.0, 1.0, 0.0, 0.0);
	//glRotatef(-90.0, 0.0, 0.0, 1.0);

	// render it on the screen
	RenderFrame(position);
	//glPopMatrix();
}



// ----------------------------------------------
// Animate() - calculate the current frame, next
// frame and interpolation percent.
// ----------------------------------------------

void CMD2Model::Animate(float time) {
	m_anim.curr_time = time;

	// calculate current and next frames
	if (m_anim.curr_time - m_anim.old_time > (1.0 / m_anim.fps)) {
		m_anim.curr_frame = m_anim.next_frame;
		m_anim.next_frame++;

		if (m_anim.next_frame > m_anim.endframe)
			m_anim.next_frame = m_anim.startframe;

		m_anim.old_time = m_anim.curr_time;
	}

	// prevent having a current/next frame greater
	// than the total number of frames...
	if (m_anim.curr_frame > (num_frames - 1))
		m_anim.curr_frame = 0;

	if (m_anim.next_frame > (num_frames - 1))
		m_anim.next_frame = 0;

	m_anim.interpol = m_anim.fps * (m_anim.curr_time - m_anim.old_time);
}



// ----------------------------------------------
// ProcessLighting() - process all lighting calculus.
// ----------------------------------------------

void CMD2Model::ProcessLighting(void) {
	float lightvar = (float)((g_shadelight + g_ambientlight) / 256.0);

	lcolor[0] = g_lightcolor[0] * lightvar;
	lcolor[1] = g_lightcolor[1] * lightvar;
	lcolor[2] = g_lightcolor[2] * lightvar;

	shadedots = anorms_dots[((int)(g_angle * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
}



// ----------------------------------------------
// Interpolate() - interpolate and scale vertices
// from the current and the next frame.
// ----------------------------------------------

void CMD2Model::Interpolate(vec3_t* vertlist) {

}



// ----------------------------------------------
// RenderFrame() - draw the current model frame
// using OpenGL commands.
// ----------------------------------------------

void CMD2Model::RenderFrame(Transform position) {
	static vec3_t	vertlist1[MAX_MD2_VERTS];	// interpolated vertices
	static vec3_t	vertlist2[MAX_MD2_VERTS];	// interpolated vertices
	int* ptricmds = m_glcmds;		// pointer on gl commands


	// reverse the orientation of front-facing
	// polygons because gl command list's triangles
	// have clockwise winding
	//glPushAttrib(GL_POLYGON_BIT);
	//glFrontFace(GL_CW);

	// enable backface culling
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);


	// process lighting
	ProcessLighting();

	// interpolate
	vec3_t* curr_v;	// pointeur to current frame vertices
	vec3_t* next_v;	// pointeur to next frame vertices

	// create current frame and next frame's vertex list
	// from the whole vertex list
	curr_v = &m_vertices[num_xyz * m_anim.curr_frame];
	next_v = &m_vertices[num_xyz * m_anim.next_frame];

	// interpolate and scale vertices to avoid ugly animation
	for (int i = 0; i < num_xyz; i++) {
		vertlist1[i][0] = (curr_v[i][0]) * m_scale;
		vertlist1[i][1] = (curr_v[i][1]) * m_scale;
		vertlist1[i][2] = (curr_v[i][2]) * m_scale;

		vertlist2[i][0] = (next_v[i][0]) * m_scale;
		vertlist2[i][1] = (next_v[i][1]) * m_scale;
		vertlist2[i][2] = (next_v[i][2]) * m_scale;

	}

	// bind model's texture
	//glBindTexture(GL_TEXTURE_2D, m_texid);

	FramedModelData model;

	// draw each triangle!
	//while (int i = *(ptricmds++)) {
	//	if (i < 0) {
	//		glBegin(GL_TRIANGLE_FAN);
	//		i = -i;
	//	}
	//	else {
	//		glBegin(GL_TRIANGLE_STRIP);
	//	}
	//
	//
	//	for ( /* nothing */; i > 0; i--, ptricmds += 3) {
	//		// ptricmds[0] : texture coordinate s
	//		// ptricmds[1] : texture coordinate t
	//		// ptricmds[2] : vertex index to render
	//
	//		float l = shadedots[m_lightnormals[ptricmds[2]]];
	//
	//		// set the lighting color
	//		glColor3f(l * lcolor[0], l * lcolor[1], l * lcolor[2]);
	//
	//		// parse texture coordinates
	//		glTexCoord2f(((float*)ptricmds)[0], ((float*)ptricmds)[1]);
	//
	//		// parse triangle's normal (for the lighting)
	//		// >>> only needed if using OpenGL lighting
	//		glNormal3fv(anorms[m_lightnormals[ptricmds[2]]]);
	//
	//		// draw the vertex
	//		glVertex3fv(vertlist1[ptricmds[2]]);
	//	}
	//
	//	glEnd();
	//}
	//
	//glDisable(GL_CULL_FACE);
	//glPopAttrib();
}



// ----------------------------------------------
// RenderFrame() - draw one frame of the model
// using gl commands.
// ----------------------------------------------

void CMD2Model::DrawFrame(int frame) {
	// set new animation parameters...
	m_anim.startframe = frame;
	m_anim.endframe = frame;
	m_anim.next_frame = frame;
	m_anim.fps = 1;
	m_anim.type = -1;

	// draw the model
	DrawModel(1.0, {});
}



// ----------------------------------------------
// initialize the 21 MD2 model animations.
// ----------------------------------------------

CMD2Model::anim_t CMD2Model::animlist[21] =
{
	// first, last, fps

	{   0,  39,  9 },	// STAND
	{  40,  45, 10 },	// RUN
	{  46,  53, 10 },	// ATTACK
	{  54,  57,  7 },	// PAIN_A
	{  58,  61,  7 },	// PAIN_B
	{  62,  65,  7 },	// PAIN_C
	{  66,  71,  7 },	// JUMP
	{  72,  83,  7 },	// FLIP
	{  84,  94,  7 },	// SALUTE
	{  95, 111, 10 },	// FALLBACK
	{ 112, 122,  7 },	// WAVE
	{ 123, 134,  6 },	// POINT
	{ 135, 153, 10 },	// CROUCH_STAND
	{ 154, 159,  7 },	// CROUCH_WALK
	{ 160, 168, 10 },	// CROUCH_ATTACK
	{ 196, 172,  7 },	// CROUCH_PAIN
	{ 173, 177,  5 },	// CROUCH_DEATH
	{ 178, 183,  7 },	// DEATH_FALLBACK
	{ 184, 189,  7 },	// DEATH_FALLFORWARD
	{ 190, 197,  7 },	// DEATH_FALLBACKSLOW
	{ 198, 198,  5 },	// BOOM
};



// ----------------------------------------------
// SetAnim() - initialize m_anim from the specified
// animation.
// ----------------------------------------------

void CMD2Model::SetAnim(int type) {
	if ((type < 0) || (type > MAX_ANIMATIONS))
		type = 0;

	m_anim.startframe = animlist[type].first_frame;
	m_anim.endframe = animlist[type].last_frame;
	m_anim.next_frame = animlist[type].first_frame + 1;
	m_anim.fps = animlist[type].fps;
	m_anim.type = type;
}

//
//#define SHADEDOT_QUANT 16
//float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
//{
//{1.23,1.30,1.47,1.35,1.56,1.71,1.37,1.38,1.59,1.60,1.79,1.97,1.88,1.92,1.79,1.02,0.93,1.07,0.82,0.87,0.88,0.94,0.96,1.14,1.11,0.82,0.83,0.89,0.89,0.86,0.94,0.91,1.00,1.21,0.98,1.48,1.30,1.57,0.96,1.07,1.14,1.60,1.61,1.40,1.37,1.72,1.78,1.79,1.93,1.99,1.90,1.68,1.71,1.86,1.60,1.68,1.78,1.86,1.93,1.99,1.97,1.44,1.22,1.49,0.93,0.99,0.99,1.23,1.22,1.44,1.49,0.89,0.89,0.97,0.91,0.98,1.19,0.82,0.76,0.82,0.71,0.72,0.73,0.76,0.79,0.86,0.83,0.72,0.76,0.76,0.89,0.82,0.89,0.82,0.89,0.91,0.83,0.96,1.14,0.97,1.40,1.19,0.98,0.94,1.00,1.07,1.37,1.21,1.48,1.30,1.57,1.61,1.37,0.86,0.83,0.91,0.82,0.82,0.88,0.89,0.96,1.14,0.98,0.87,0.93,0.94,1.02,1.30,1.07,1.35,1.38,1.11,1.56,1.92,1.79,1.79,1.59,1.60,1.72,1.90,1.79,0.80,0.85,0.79,0.93,0.80,0.85,0.77,0.74,0.72,0.77,0.74,0.72,0.70,0.70,0.71,0.76,0.73,0.79,0.79,0.73,0.76,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.26,1.26,1.48,1.23,1.50,1.71,1.14,1.19,1.38,1.46,1.64,1.94,1.87,1.84,1.71,1.02,0.92,1.00,0.79,0.85,0.84,0.91,0.90,0.98,0.99,0.77,0.77,0.83,0.82,0.79,0.86,0.84,0.92,0.99,0.91,1.24,1.03,1.33,0.88,0.94,0.97,1.41,1.39,1.18,1.11,1.51,1.61,1.59,1.80,1.91,1.76,1.54,1.65,1.76,1.70,1.70,1.85,1.85,1.97,1.99,1.93,1.28,1.09,1.39,0.92,0.97,0.99,1.18,1.26,1.52,1.48,0.83,0.85,0.90,0.88,0.93,1.00,0.77,0.73,0.78,0.72,0.71,0.74,0.75,0.79,0.86,0.81,0.75,0.81,0.79,0.96,0.88,0.94,0.86,0.93,0.92,0.85,1.08,1.33,1.05,1.55,1.31,1.01,1.05,1.27,1.31,1.60,1.47,1.70,1.54,1.76,1.76,1.57,0.93,0.90,0.99,0.88,0.88,0.95,0.97,1.11,1.39,1.20,0.92,0.97,1.01,1.10,1.39,1.22,1.51,1.58,1.32,1.64,1.97,1.85,1.91,1.77,1.74,1.88,1.99,1.91,0.79,0.86,0.80,0.94,0.84,0.88,0.74,0.74,0.71,0.82,0.77,0.76,0.70,0.73,0.72,0.73,0.70,0.74,0.85,0.77,0.82,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.34,1.27,1.53,1.17,1.46,1.71,0.98,1.05,1.20,1.34,1.48,1.86,1.82,1.71,1.62,1.09,0.94,0.99,0.79,0.85,0.82,0.90,0.87,0.93,0.96,0.76,0.74,0.79,0.76,0.74,0.79,0.78,0.85,0.92,0.85,1.00,0.93,1.06,0.81,0.86,0.89,1.16,1.12,0.97,0.95,1.28,1.38,1.35,1.60,1.77,1.57,1.33,1.50,1.58,1.69,1.63,1.82,1.74,1.91,1.92,1.80,1.04,0.97,1.21,0.90,0.93,0.97,1.05,1.21,1.48,1.37,0.77,0.80,0.84,0.85,0.88,0.92,0.73,0.71,0.74,0.74,0.71,0.75,0.73,0.79,0.84,0.78,0.79,0.86,0.81,1.05,0.94,0.99,0.90,0.95,0.92,0.86,1.24,1.44,1.14,1.59,1.34,1.02,1.27,1.50,1.49,1.80,1.69,1.86,1.72,1.87,1.80,1.69,1.00,0.98,1.23,0.95,0.96,1.09,1.16,1.37,1.63,1.46,0.99,1.10,1.25,1.24,1.51,1.41,1.67,1.77,1.55,1.72,1.95,1.89,1.98,1.91,1.86,1.97,1.99,1.94,0.81,0.89,0.85,0.98,0.90,0.94,0.75,0.78,0.73,0.89,0.83,0.82,0.72,0.77,0.76,0.72,0.70,0.71,0.91,0.83,0.89,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.46,1.34,1.60,1.16,1.46,1.71,0.94,0.99,1.05,1.26,1.33,1.74,1.76,1.57,1.54,1.23,0.98,1.05,0.83,0.89,0.84,0.92,0.87,0.91,0.96,0.78,0.74,0.79,0.72,0.72,0.75,0.76,0.80,0.88,0.83,0.94,0.87,0.95,0.76,0.80,0.82,0.97,0.96,0.89,0.88,1.08,1.11,1.10,1.37,1.59,1.37,1.07,1.27,1.34,1.57,1.45,1.69,1.55,1.77,1.79,1.60,0.93,0.90,0.99,0.86,0.87,0.93,0.96,1.07,1.35,1.18,0.73,0.76,0.77,0.81,0.82,0.85,0.70,0.71,0.72,0.78,0.73,0.77,0.73,0.79,0.82,0.76,0.83,0.90,0.84,1.18,0.98,1.03,0.92,0.95,0.90,0.86,1.32,1.45,1.15,1.53,1.27,0.99,1.42,1.65,1.58,1.93,1.83,1.94,1.81,1.88,1.74,1.70,1.19,1.17,1.44,1.11,1.15,1.36,1.41,1.61,1.81,1.67,1.22,1.34,1.50,1.42,1.65,1.61,1.82,1.91,1.75,1.80,1.89,1.89,1.98,1.99,1.94,1.98,1.92,1.87,0.86,0.95,0.92,1.14,0.98,1.03,0.79,0.84,0.77,0.97,0.90,0.89,0.76,0.82,0.82,0.74,0.72,0.71,0.98,0.89,0.97,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.60,1.44,1.68,1.22,1.49,1.71,0.93,0.99,0.99,1.23,1.22,1.60,1.68,1.44,1.49,1.40,1.14,1.19,0.89,0.96,0.89,0.97,0.89,0.91,0.98,0.82,0.76,0.82,0.71,0.72,0.73,0.76,0.79,0.86,0.83,0.91,0.83,0.89,0.72,0.76,0.76,0.89,0.89,0.82,0.82,0.98,0.96,0.97,1.14,1.40,1.19,0.94,1.00,1.07,1.37,1.21,1.48,1.30,1.57,1.61,1.37,0.86,0.83,0.91,0.82,0.82,0.88,0.89,0.96,1.14,0.98,0.70,0.72,0.73,0.77,0.76,0.79,0.70,0.72,0.71,0.82,0.77,0.80,0.74,0.79,0.80,0.74,0.87,0.93,0.85,1.23,1.02,1.02,0.93,0.93,0.87,0.85,1.30,1.35,1.07,1.38,1.11,0.94,1.47,1.71,1.56,1.97,1.88,1.92,1.79,1.79,1.59,1.60,1.30,1.35,1.56,1.37,1.38,1.59,1.60,1.79,1.92,1.79,1.48,1.57,1.72,1.61,1.78,1.79,1.93,1.99,1.90,1.86,1.78,1.86,1.93,1.99,1.97,1.90,1.79,1.72,0.94,1.07,1.00,1.37,1.21,1.30,0.86,0.91,0.83,1.14,0.98,0.96,0.82,0.88,0.89,0.79,0.76,0.73,1.07,0.94,1.11,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.74,1.57,1.76,1.33,1.54,1.71,0.94,1.05,0.99,1.26,1.16,1.46,1.60,1.34,1.46,1.59,1.37,1.37,0.97,1.11,0.96,1.10,0.95,0.94,1.08,0.89,0.82,0.88,0.72,0.76,0.75,0.80,0.80,0.88,0.87,0.91,0.83,0.87,0.72,0.76,0.74,0.83,0.84,0.78,0.79,0.96,0.89,0.92,0.98,1.23,1.05,0.86,0.92,0.95,1.11,0.98,1.22,1.03,1.34,1.42,1.14,0.79,0.77,0.84,0.78,0.76,0.82,0.82,0.89,0.97,0.90,0.70,0.71,0.71,0.73,0.72,0.74,0.73,0.76,0.72,0.86,0.81,0.82,0.76,0.79,0.77,0.73,0.90,0.95,0.86,1.18,1.03,0.98,0.92,0.90,0.83,0.84,1.19,1.17,0.98,1.15,0.97,0.89,1.42,1.65,1.44,1.93,1.83,1.81,1.67,1.61,1.36,1.41,1.32,1.45,1.58,1.57,1.53,1.74,1.70,1.88,1.94,1.81,1.69,1.77,1.87,1.79,1.89,1.92,1.98,1.99,1.98,1.89,1.65,1.80,1.82,1.91,1.94,1.75,1.61,1.50,1.07,1.34,1.27,1.60,1.45,1.55,0.93,0.99,0.90,1.35,1.18,1.07,0.87,0.93,0.96,0.85,0.82,0.77,1.15,0.99,1.27,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.86,1.71,1.82,1.48,1.62,1.71,0.98,1.20,1.05,1.34,1.17,1.34,1.53,1.27,1.46,1.77,1.60,1.57,1.16,1.38,1.12,1.35,1.06,1.00,1.28,0.97,0.89,0.95,0.76,0.81,0.79,0.86,0.85,0.92,0.93,0.93,0.85,0.87,0.74,0.78,0.74,0.79,0.82,0.76,0.79,0.96,0.85,0.90,0.94,1.09,0.99,0.81,0.85,0.89,0.95,0.90,0.99,0.94,1.10,1.24,0.98,0.75,0.73,0.78,0.74,0.72,0.77,0.76,0.82,0.89,0.83,0.73,0.71,0.71,0.71,0.70,0.72,0.77,0.80,0.74,0.90,0.85,0.84,0.78,0.79,0.75,0.73,0.92,0.95,0.86,1.05,0.99,0.94,0.90,0.86,0.79,0.81,1.00,0.98,0.91,0.96,0.89,0.83,1.27,1.50,1.23,1.80,1.69,1.63,1.46,1.37,1.09,1.16,1.24,1.44,1.49,1.69,1.59,1.80,1.69,1.87,1.86,1.72,1.82,1.91,1.94,1.92,1.95,1.99,1.98,1.91,1.97,1.89,1.51,1.72,1.67,1.77,1.86,1.55,1.41,1.25,1.33,1.58,1.50,1.80,1.63,1.74,1.04,1.21,0.97,1.48,1.37,1.21,0.93,0.97,1.05,0.92,0.88,0.84,1.14,1.02,1.34,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.94,1.84,1.87,1.64,1.71,1.71,1.14,1.38,1.19,1.46,1.23,1.26,1.48,1.26,1.50,1.91,1.80,1.76,1.41,1.61,1.39,1.59,1.33,1.24,1.51,1.18,0.97,1.11,0.82,0.88,0.86,0.94,0.92,0.99,1.03,0.98,0.91,0.90,0.79,0.84,0.77,0.79,0.84,0.77,0.83,0.99,0.85,0.91,0.92,1.02,1.00,0.79,0.80,0.86,0.88,0.84,0.92,0.88,0.97,1.10,0.94,0.74,0.71,0.74,0.72,0.70,0.73,0.72,0.76,0.82,0.77,0.77,0.73,0.74,0.71,0.70,0.73,0.83,0.85,0.78,0.92,0.88,0.86,0.81,0.79,0.74,0.75,0.92,0.93,0.85,0.96,0.94,0.88,0.86,0.81,0.75,0.79,0.93,0.90,0.85,0.88,0.82,0.77,1.05,1.27,0.99,1.60,1.47,1.39,1.20,1.11,0.95,0.97,1.08,1.33,1.31,1.70,1.55,1.76,1.57,1.76,1.70,1.54,1.85,1.97,1.91,1.99,1.97,1.99,1.91,1.77,1.88,1.85,1.39,1.64,1.51,1.58,1.74,1.32,1.22,1.01,1.54,1.76,1.65,1.93,1.70,1.85,1.28,1.39,1.09,1.52,1.48,1.26,0.97,0.99,1.18,1.00,0.93,0.90,1.05,1.01,1.31,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.97,1.92,1.88,1.79,1.79,1.71,1.37,1.59,1.38,1.60,1.35,1.23,1.47,1.30,1.56,1.99,1.93,1.90,1.60,1.78,1.61,1.79,1.57,1.48,1.72,1.40,1.14,1.37,0.89,0.96,0.94,1.07,1.00,1.21,1.30,1.14,0.98,0.96,0.86,0.91,0.83,0.82,0.88,0.82,0.89,1.11,0.87,0.94,0.93,1.02,1.07,0.80,0.79,0.85,0.82,0.80,0.87,0.85,0.93,1.02,0.93,0.77,0.72,0.74,0.71,0.70,0.70,0.71,0.72,0.77,0.74,0.82,0.76,0.79,0.72,0.73,0.76,0.89,0.89,0.82,0.93,0.91,0.86,0.83,0.79,0.73,0.76,0.91,0.89,0.83,0.89,0.89,0.82,0.82,0.76,0.72,0.76,0.86,0.83,0.79,0.82,0.76,0.73,0.94,1.00,0.91,1.37,1.21,1.14,0.98,0.96,0.88,0.89,0.96,1.14,1.07,1.60,1.40,1.61,1.37,1.57,1.48,1.30,1.78,1.93,1.79,1.99,1.92,1.90,1.79,1.59,1.72,1.79,1.30,1.56,1.35,1.38,1.60,1.11,1.07,0.94,1.68,1.86,1.71,1.97,1.68,1.86,1.44,1.49,1.22,1.44,1.49,1.22,0.99,0.99,1.23,1.19,0.98,0.97,0.97,0.98,1.19,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.94,1.97,1.87,1.91,1.85,1.71,1.60,1.77,1.58,1.74,1.51,1.26,1.48,1.39,1.64,1.99,1.97,1.99,1.70,1.85,1.76,1.91,1.76,1.70,1.88,1.55,1.33,1.57,0.96,1.08,1.05,1.31,1.27,1.47,1.54,1.39,1.20,1.11,0.93,0.99,0.90,0.88,0.95,0.88,0.97,1.32,0.92,1.01,0.97,1.10,1.22,0.84,0.80,0.88,0.79,0.79,0.85,0.86,0.92,1.02,0.94,0.82,0.76,0.77,0.72,0.73,0.70,0.72,0.71,0.74,0.74,0.88,0.81,0.85,0.75,0.77,0.82,0.94,0.93,0.86,0.92,0.92,0.86,0.85,0.79,0.74,0.79,0.88,0.85,0.81,0.82,0.83,0.77,0.78,0.73,0.71,0.75,0.79,0.77,0.74,0.77,0.73,0.70,0.86,0.92,0.84,1.14,0.99,0.98,0.91,0.90,0.84,0.83,0.88,0.97,0.94,1.41,1.18,1.39,1.11,1.33,1.24,1.03,1.61,1.80,1.59,1.91,1.84,1.76,1.64,1.38,1.51,1.71,1.26,1.50,1.23,1.19,1.46,0.99,1.00,0.91,1.70,1.85,1.65,1.93,1.54,1.76,1.52,1.48,1.26,1.28,1.39,1.09,0.99,0.97,1.18,1.31,1.01,1.05,0.90,0.93,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.86,1.95,1.82,1.98,1.89,1.71,1.80,1.91,1.77,1.86,1.67,1.34,1.53,1.51,1.72,1.92,1.91,1.99,1.69,1.82,1.80,1.94,1.87,1.86,1.97,1.59,1.44,1.69,1.05,1.24,1.27,1.49,1.50,1.69,1.72,1.63,1.46,1.37,1.00,1.23,0.98,0.95,1.09,0.96,1.16,1.55,0.99,1.25,1.10,1.24,1.41,0.90,0.85,0.94,0.79,0.81,0.85,0.89,0.94,1.09,0.98,0.89,0.82,0.83,0.74,0.77,0.72,0.76,0.73,0.75,0.78,0.94,0.86,0.91,0.79,0.83,0.89,0.99,0.95,0.90,0.90,0.92,0.84,0.86,0.79,0.75,0.81,0.85,0.80,0.78,0.76,0.77,0.73,0.74,0.71,0.71,0.73,0.74,0.74,0.71,0.76,0.72,0.70,0.79,0.85,0.78,0.98,0.92,0.93,0.85,0.87,0.82,0.79,0.81,0.89,0.86,1.16,0.97,1.12,0.95,1.06,1.00,0.93,1.38,1.60,1.35,1.77,1.71,1.57,1.48,1.20,1.28,1.62,1.27,1.46,1.17,1.05,1.34,0.96,0.99,0.90,1.63,1.74,1.50,1.80,1.33,1.58,1.48,1.37,1.21,1.04,1.21,0.97,0.97,0.93,1.05,1.34,1.02,1.14,0.84,0.88,0.92,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.74,1.89,1.76,1.98,1.89,1.71,1.93,1.99,1.91,1.94,1.82,1.46,1.60,1.65,1.80,1.79,1.77,1.92,1.57,1.69,1.74,1.87,1.88,1.94,1.98,1.53,1.45,1.70,1.18,1.32,1.42,1.58,1.65,1.83,1.81,1.81,1.67,1.61,1.19,1.44,1.17,1.11,1.36,1.15,1.41,1.75,1.22,1.50,1.34,1.42,1.61,0.98,0.92,1.03,0.83,0.86,0.89,0.95,0.98,1.23,1.14,0.97,0.89,0.90,0.78,0.82,0.76,0.82,0.77,0.79,0.84,0.98,0.90,0.98,0.83,0.89,0.97,1.03,0.95,0.92,0.86,0.90,0.82,0.86,0.79,0.77,0.84,0.81,0.76,0.76,0.72,0.73,0.70,0.72,0.71,0.73,0.73,0.72,0.74,0.71,0.78,0.74,0.72,0.75,0.80,0.76,0.94,0.88,0.91,0.83,0.87,0.84,0.79,0.76,0.82,0.80,0.97,0.89,0.96,0.88,0.95,0.94,0.87,1.11,1.37,1.10,1.59,1.57,1.37,1.33,1.05,1.08,1.54,1.34,1.46,1.16,0.99,1.26,0.96,1.05,0.92,1.45,1.55,1.27,1.60,1.07,1.34,1.35,1.18,1.07,0.93,0.99,0.90,0.93,0.87,0.96,1.27,0.99,1.15,0.77,0.82,0.85,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.60,1.78,1.68,1.93,1.86,1.71,1.97,1.99,1.99,1.97,1.93,1.60,1.68,1.78,1.86,1.61,1.57,1.79,1.37,1.48,1.59,1.72,1.79,1.92,1.90,1.38,1.35,1.60,1.23,1.30,1.47,1.56,1.71,1.88,1.79,1.92,1.79,1.79,1.30,1.56,1.35,1.37,1.59,1.38,1.60,1.90,1.48,1.72,1.57,1.61,1.79,1.21,1.00,1.30,0.89,0.94,0.96,1.07,1.14,1.40,1.37,1.14,0.96,0.98,0.82,0.88,0.82,0.89,0.83,0.86,0.91,1.02,0.93,1.07,0.87,0.94,1.11,1.02,0.93,0.93,0.82,0.87,0.80,0.85,0.79,0.80,0.85,0.77,0.72,0.74,0.71,0.70,0.70,0.71,0.72,0.77,0.74,0.72,0.76,0.73,0.82,0.79,0.76,0.73,0.79,0.76,0.93,0.86,0.91,0.83,0.89,0.89,0.82,0.72,0.76,0.76,0.89,0.82,0.89,0.82,0.89,0.91,0.83,0.96,1.14,0.97,1.40,1.44,1.19,1.22,0.99,0.98,1.49,1.44,1.49,1.22,0.99,1.23,0.98,1.19,0.97,1.21,1.30,1.00,1.37,0.94,1.07,1.14,0.98,0.96,0.86,0.91,0.83,0.88,0.82,0.89,1.11,0.94,1.07,0.73,0.76,0.79,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.46,1.65,1.60,1.82,1.80,1.71,1.93,1.91,1.99,1.94,1.98,1.74,1.76,1.89,1.89,1.42,1.34,1.61,1.11,1.22,1.36,1.50,1.61,1.81,1.75,1.15,1.17,1.41,1.18,1.19,1.42,1.44,1.65,1.83,1.67,1.94,1.81,1.88,1.32,1.58,1.45,1.57,1.74,1.53,1.70,1.98,1.69,1.87,1.77,1.79,1.92,1.45,1.27,1.55,0.97,1.07,1.11,1.34,1.37,1.59,1.60,1.35,1.07,1.18,0.86,0.93,0.87,0.96,0.90,0.93,0.99,1.03,0.95,1.15,0.90,0.99,1.27,0.98,0.90,0.92,0.78,0.83,0.77,0.84,0.79,0.82,0.86,0.73,0.71,0.73,0.72,0.70,0.73,0.72,0.76,0.81,0.76,0.76,0.82,0.77,0.89,0.85,0.82,0.75,0.80,0.80,0.94,0.88,0.94,0.87,0.95,0.96,0.88,0.72,0.74,0.76,0.83,0.78,0.84,0.79,0.87,0.91,0.83,0.89,0.98,0.92,1.23,1.34,1.05,1.16,0.99,0.96,1.46,1.57,1.54,1.33,1.05,1.26,1.08,1.37,1.10,0.98,1.03,0.92,1.14,0.86,0.95,0.97,0.90,0.89,0.79,0.84,0.77,0.82,0.76,0.82,0.97,0.89,0.98,0.71,0.72,0.74,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.34,1.51,1.53,1.67,1.72,1.71,1.80,1.77,1.91,1.86,1.98,1.86,1.82,1.95,1.89,1.24,1.10,1.41,0.95,0.99,1.09,1.25,1.37,1.63,1.55,0.96,0.98,1.16,1.05,1.00,1.27,1.23,1.50,1.69,1.46,1.86,1.72,1.87,1.24,1.49,1.44,1.69,1.80,1.59,1.69,1.97,1.82,1.94,1.91,1.92,1.99,1.63,1.50,1.74,1.16,1.33,1.38,1.58,1.60,1.77,1.80,1.48,1.21,1.37,0.90,0.97,0.93,1.05,0.97,1.04,1.21,0.99,0.95,1.14,0.92,1.02,1.34,0.94,0.86,0.90,0.74,0.79,0.75,0.81,0.79,0.84,0.86,0.71,0.71,0.73,0.76,0.73,0.77,0.74,0.80,0.85,0.78,0.81,0.89,0.84,0.97,0.92,0.88,0.79,0.85,0.86,0.98,0.92,1.00,0.93,1.06,1.12,0.95,0.74,0.74,0.78,0.79,0.76,0.82,0.79,0.87,0.93,0.85,0.85,0.94,0.90,1.09,1.27,0.99,1.17,1.05,0.96,1.46,1.71,1.62,1.48,1.20,1.34,1.28,1.57,1.35,0.90,0.94,0.85,0.98,0.81,0.89,0.89,0.83,0.82,0.75,0.78,0.73,0.77,0.72,0.76,0.89,0.83,0.91,0.71,0.70,0.72,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
//{1.26,1.39,1.48,1.51,1.64,1.71,1.60,1.58,1.77,1.74,1.91,1.94,1.87,1.97,1.85,1.10,0.97,1.22,0.88,0.92,0.95,1.01,1.11,1.39,1.32,0.88,0.90,0.97,0.96,0.93,1.05,0.99,1.27,1.47,1.20,1.70,1.54,1.76,1.08,1.31,1.33,1.70,1.76,1.55,1.57,1.88,1.85,1.91,1.97,1.99,1.99,1.70,1.65,1.85,1.41,1.54,1.61,1.76,1.80,1.91,1.93,1.52,1.26,1.48,0.92,0.99,0.97,1.18,1.09,1.28,1.39,0.94,0.93,1.05,0.92,1.01,1.31,0.88,0.81,0.86,0.72,0.75,0.74,0.79,0.79,0.86,0.85,0.71,0.73,0.75,0.82,0.77,0.83,0.78,0.85,0.88,0.81,0.88,0.97,0.90,1.18,1.00,0.93,0.86,0.92,0.94,1.14,0.99,1.24,1.03,1.33,1.39,1.11,0.79,0.77,0.84,0.79,0.77,0.84,0.83,0.90,0.98,0.91,0.85,0.92,0.91,1.02,1.26,1.00,1.23,1.19,0.99,1.50,1.84,1.71,1.64,1.38,1.46,1.51,1.76,1.59,0.84,0.88,0.80,0.94,0.79,0.86,0.82,0.77,0.76,0.74,0.74,0.71,0.73,0.70,0.72,0.82,0.77,0.85,0.74,0.70,0.73,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00}
//}
//;
//
//float* shadedots = r_avertexnormal_dots[0];
//vec3_t	shadevector;
//float	shadelight[3];
//
//typedef float vec4_t[4];
//
//FramedModelData  RegistrationManager::ConvertAliasModel(model_t* model) {
//	int			i;
//	dmdl_t* paliashdr;
//	float		an;
//	vec3_t		bbox[8];
//	image_t* skin;
//
//	paliashdr = (dmdl_t*)model->extradata;
//
//
//
//
//	shadedots = r_avertexnormal_dots[((int)(0 * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
//
//	an = 0;
//	shadevector[0] = cos(-an);
//	shadevector[1] = sin(-an);
//	shadevector[2] = 1;
//
//	VectorNormalize(shadevector);
//
//	FramedModelData directXmodel;
//
//
//	for (int frameIndex = 0; frameIndex < paliashdr->num_frames; frameIndex++) {
//		directXmodel.frames.push_back({});
//		bool texFlag = directXmodel.verticies.size() == 0;
//
//		float 	l;
//		daliasframe_t* frame;
//		dtrivertx_t* v, * ov, * verts;
//		int* order;
//		int		count;
//		float	frontlerp;
//		float	alpha;
//		vec3_t	move, delta, vectors[3];
//		vec3_t	frontv, backv;
//		int		i;
//		int		index_xyz;
//		float* lerp;
//
//		frame = (daliasframe_t*)((byte*)paliashdr + paliashdr->ofs_frames
//			+ frameIndex * paliashdr->framesize);
//		verts = v = frame->verts;
//
//		order = (int*)((byte*)paliashdr + paliashdr->ofs_glcmds);
//
//
//		// move should be the delta back to the previous frame * backlerp
//		//VectorSubtract(currententity->oldorigin, currententity->origin, delta);
//		//float angles[3] = { 0,0,0 };
//		//AngleVectors(angles, vectors[0], vectors[1], vectors[2]);
//
//		//move[0] = 0DotProduct(delta, vectors[0]);	// forward
//		//move[1] = 0-DotProduct(delta, vectors[1]);	// left
//		//move[2] = 0DotProduct(delta, vectors[2]);	// up
//
//		//VectorAdd(move, oldframe->translate, move);
//
//		for (i = 0; i < 3; i++) {
//			//move[i] = backlerp * move[i] + frontlerp * frame->translate[i];
//		}
//
//		for (i = 0; i < 3; i++) {
//			frontv[i] = frame->scale[i];
//		}
//		static	vec4_t	s_lerped[MAX_VERTS];
//
//		lerp = s_lerped[0];
//
//		{
//			{
//				for (int i = 0; i < paliashdr->num_xyz; i++, v++, lerp += 4) {
//					lerp[0] = v->v[0] * frontv[0];
//					lerp[1] = v->v[1] * frontv[1];
//					lerp[2] = v->v[2] * frontv[2];
//				}
//			}
//
//		}
//		while (1) {
//			// get the vertex count and primitive type
//			count = *order++;
//			if (!count)
//				break;		// done
//			if (count < 0) {
//				count = -count;
//				//qglBegin(GL_TRIANGLE_FAN);
//				//TODO
//			}
//			else {
//				//qglBegin(GL_TRIANGLE_STRIP);
//				directXmodel.pt = Renderer::PrimitiveType::PRIMITIVETYPE_TRIANGLESTRIP;
//
//				do {
//					// texture coordinates come from the draw list
//					if (texFlag) {
//						directXmodel.verticies.push_back(FramedModelVertex{ float3{0.0,0.0,0.0}, float2{((float*)order)[0],((float*)order)[1] } });
//						directXmodel.indexes.push_back(order[2]);
//					}
//					index_xyz = order[2];
//					order += 3;
//
//					// normals and vertexes come from the frame list
//					//l = shadedots[verts[index_xyz].lightnormalindex];
//
//					//qglColor4f(l * shadelight[0], l * shadelight[1], l * shadelight[2], alpha);
//					directXmodel.frames[frameIndex].push_back(float3{ s_lerped[index_xyz][0], s_lerped[index_xyz][1], s_lerped[index_xyz][2] });
//					//qglVertex3fv(s_lerped[index_xyz]);
//				} while (--count);
//			}
//
//		}
//
//
//	}
//	return directXmodel;
//
//
//}

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
	strcpy(image->name, name);
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
		//image->width = tex.GetWidth();		// after power of 2 and scales
		//image->height = tex.GetHeight();
		//image->paletted = uploaded_paletted;
		//image->sl = 0;
		//image->sh = 1;
		//image->tl = 0;
		//image->th = 1;
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
