#pragma once
#include <map>
#include "TextureData.h"
#include "ModelData.h"
#include "../qcommon/qcommon.h"

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



// number of precalculated normals
#define NUMVERTEXNORMALS		162

// precalculated normal vectors
#define SHADEDOT_QUANT			16

// magic number "IDP2" or 844121161
#define MD2_IDENT				(('2'<<24) + ('P'<<16) + ('D'<<8) + 'I')

// model version
#define	MD2_VERSION				8

// maximum number of vertices for a MD2 model
#define MAX_MD2_VERTS			2048



// ==============================================
// CMD2Model - MD2 model class object.
// ==============================================

typedef struct {
	char		name[MAX_SKINNAME];		// frame name
	image_s* m_texid;
} skinInfo;



typedef enum { mod_bad, mod_brush, mod_sprite, mod_alias } modtype_t;

typedef struct model_s {
	skinInfo* skins;
	//FramedModelData realModel;

	int modelId;
	int num_frames;
	int num_skins;
	int num_tris;
	char name[255];
	int registration_sequence;
	modtype_t	type;
} model_t;




#define	MAX_MOD_KNOWN	512
#define	MAX_LBM_HEIGHT		480

class RegistrationManager {


	size_t numTextures = 0;
	size_t numModels = 0;
	std::array<image_t*, MAX_DXTEXTURES> dxTextures; 
	std::array<model_t*, MAX_MOD_KNOWN> dxModels;

	uint32_t gammatable[256];;
	byte* d_16to8table;


	TextureData GL_Upload32(uint32_t* data, int width, int height, bool mipmap);
	TextureData GL_Upload8(byte* data, int width, int height, bool mipmap, bool is_sky);
	image_t* LoadPic(char* name, byte* pic, int width, int height, imagetype_t type, int bits);

	FramedModelData  LoadAliasModel(model_t* mod, void* buffer);
	

public:
	RegistrationManager();

	void InitImages();
	void InitLocal();


	size_t registration_sequence = 0;
	void BeginRegistration(const char* map);
	void SetPalette(const unsigned char* palette);
	void EndRegistration();


	image_t* FindImage(const char* name, imagetype_t type);

	model_t* FindModel(const char* name, qboolean crash);
	model_t* FindModel(int id, qboolean crash);

	static void LoadPCX(const char* filename, byte** pic, byte** palette, int* width, int* height);
	static void LoadTGA(const char* name, byte** pic, int* width, int* height);
	void BuildPalettedTexture(unsigned char* paletted_texture, unsigned char* scaled, int scaled_width, int scaled_height);



	image_t* draw_chars;
	uint32_t rawPalette[256];
	uint32_t d_8to24table[256];


	~RegistrationManager();

};

void R_BeginRegistration(char* model);
struct model_s* R_RegisterModel(char* name);
struct image_s* R_RegisterSkin(char* name);
image_t* R_RegisterPic(char* name);
void R_SetSky(char* name, float rotate, float axis[3]);
void R_EndRegistration(void); 
void R_SetPalette(const unsigned char* palette);
