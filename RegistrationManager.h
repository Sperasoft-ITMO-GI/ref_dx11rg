#pragma once
#include <array>

#include "../qcommon/qcommon.h"
#include "dx11rg_model.h"
#include "dx11rg_drawData.h"


class RegistrationManager;


#define MAX_DXTEXTURES 2000
#define	MAX_MOD_KNOWN	512



class RegistrationManager {

#define NUMVERTEXNORMALS	162

	static float r_avertexnormals[NUMVERTEXNORMALS][3];

#define SHADEDOT_QUANT 16
	static float	r_avertexnormal_dots[SHADEDOT_QUANT][256];
	static float* shadedots;

	std::array<image_t*, MAX_DXTEXTURES> dxTextures;
	std::array<model_t*, MAX_MOD_KNOWN> dxModels;
	model_t	mod_inline[MAX_MOD_KNOWN];

	uint32_t gammatable[256];;
	byte* d_16to8table;


	TextureData GL_Upload32(uint32_t* data, int width, int height, bool mipmap);
	TextureData GL_Upload8(byte* data, int width, int height, bool mipmap, bool is_sky);
	image_t* LoadPic(char* name, byte* pic, int width, int height, imagetype_t type, int bits);
	image_t* LoadWal(char* name);

	FramedModelData  LoadAliasModel(model_t* mod, void* buffer);
	void LoadBrushModel(model_t* mod, void* buffer);


	void Mod_Free(model_t* mod);

	int sideSequence[6] = { 4, 1, 5, 0, 2, 3 };

public:
	RegistrationManager();

	void InitImages();
	void InitLocal();


	size_t registration_sequence = 0;
	size_t numTextures = 0;
	size_t numModels = 0;

	void BeginRegistration(const char* map);
	void SetPalette(const unsigned char* palette);
	void EndRegistration();


	image_t* FindImage(const char* name, imagetype_t type);
	image_t* FindImage(int id);

	model_t* FindModel(const char* name, qboolean crash);
	model_t* FindModel(int id, qboolean crash);

	static void LoadPCX(const char* filename, byte** pic, byte** palette, int* width, int* height);
	static void LoadTGA(const char* name, byte** pic, int* width, int* height);
	void BuildPalettedTexture(unsigned char* paletted_texture, unsigned char* scaled, int scaled_width, int scaled_height);


	void CreateLightmap(int width, int height, int bits, unsigned char* data, int texNum, bool mipmap);


	image_t* draw_chars;
	uint32_t rawPalette[256];
	uint32_t d_8to24table[256];
	int skySide = 0;


	~RegistrationManager();

};


extern RegistrationManager RM;

void R_BeginRegistration(char* model);
struct model_s* R_RegisterModel(char* name);
struct image_s* R_RegisterSkin(char* name);
image_t* R_RegisterPic(char* name);
void R_SetSky(char* name, float rotate, float axis[3]);
void R_EndRegistration(void);
void R_SetPalette(const unsigned char* palette);
