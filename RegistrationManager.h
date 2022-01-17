#pragma once
#include <map>
#include "Texture.h"

class RegistrationManager;
extern RegistrationManager RM;

typedef enum {
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
} imagetype_t;

typedef struct image_s {
	int		texnum = -1;						// gl texture binding
	char	name[256];			// game path, including extension
	imagetype_t	type;
	int		width, height;				// source image
	//int		upload_width, upload_height;	// after power of two and picmip
	int		registration_sequence;		// 0 = free
	//struct msurface_s* texturechain;	// for sort-by-texture world drawing
	//float	sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	//qboolean	scrap;
	//qboolean	has_alpha;

	//qboolean paletted;
} image_t;
#define MAX_DXTEXTURES 2000

class RegistrationManager {
	size_t registration_sequence = 0;


	size_t numTextures = 0;
	std::array<image_t, MAX_DXTEXTURES> dxTextures; 

	uint32_t gammatable[256];;
	uint32_t d_8to24table[256];
	byte* d_16to8table;


	TextureData GL_Upload32(uint32_t* data, int width, int height, bool mipmap);
	TextureData GL_Upload8(byte* data, int width, int height, bool mipmap, bool is_sky);
	image_t* LoadPic(char* name, byte* pic, int width, int height, imagetype_t type, int bits);

public:

	void InitImages();
	void InitLocal();
	void BeginRegistration(const char* map);
	void SetPalette(const unsigned char* palette);
	void EndRegistration();


	image_t* FindImage(char* name, imagetype_t type);

	static void LoadPCX(char* filename, byte** pic, byte** palette, int* width, int* height);
	static void LoadTGA(char* name, byte** pic, int* width, int* height);
	void BuildPalettedTexture(unsigned char* paletted_texture, unsigned char* scaled, int scaled_width, int scaled_height);



	image_t* draw_chars;
	uint32_t rawPalette[256];

};

void R_BeginRegistration(char* model);
struct model_s* R_RegisterModel(char* name);
struct image_s* R_RegisterSkin(char* name);
image_t* R_RegisterPic(char* name);
void R_SetSky(char* name, float rotate, float axis[3]);
void R_EndRegistration(void); 
void R_SetPalette(const unsigned char* palette);
