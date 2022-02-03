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

class CMD2Model {

public:
	typedef float vec3_t[3];



	// md2 header
	typedef struct {
		int		ident;				// magic number. must be equal to "IPD2"
		int		version;			// md2 version. must be equal to 8

		int		skinwidth;			// width of the texture
		int		skinheight;			// height of the texture
		int		framesize;			// size of one frame in bytes

		int		num_skins;			// number of textures
		int		num_xyz;			// number of vertices
		int		num_st;				// number of texture coordinates
		int		num_tris;			// number of triangles
		int		num_glcmds;			// number of opengl commands
		int		num_frames;			// total number of frames

		int		ofs_skins;			// offset to skin names (64 bytes each)
		int		ofs_st;				// offset to s-t texture coordinates
		int		ofs_tris;			// offset to triangles
		int		ofs_frames;			// offset to frame data
		int		ofs_glcmds;			// offset to opengl commands
		int		ofs_end;			// offset to the end of file

	} md2_t;



	// vertex
	typedef struct {
		unsigned char	v[3];				// compressed vertex' (x, y, z) coordinates
		unsigned char	lightnormalindex;	// index to a normal vector for the lighting

	} vertex_t;



	// frame
	typedef struct {
		float		scale[3];		// scale values
		float		translate[3];	// translation vector
		char		name[16];		// frame name
		vertex_t	verts[1];		// first vertex of this frame
		image_s* m_texid;
	} frame_t;

	// frameInfo
	typedef struct {
		char		name[16];		// frame name
		image_s*    m_texid;
	} frameInfo;



	// animation
	typedef struct {
		int		first_frame;			// first frame of the animation
		int		last_frame;				// number of frames
		int		fps;					// number of frames per second

	} anim_t;



	// status animation
	typedef struct {
		int		startframe;				// first frame
		int		endframe;				// last frame
		int		fps;					// frame per second for this animation

		float	curr_time;				// current time
		float	old_time;				// old time
		float	interpol;				// percent of interpolation

		int		type;					// animation type

		int		curr_frame;				// current frame
		int		next_frame;				// next frame

	} animState_t;



	// animation list
	typedef enum {
		STAND,
		RUN,
		ATTACK,
		PAIN_A,
		PAIN_B,
		PAIN_C,
		JUMP,
		FLIP,
		SALUTE,
		FALLBACK,
		WAVE,
		POINT,
		CROUCH_STAND,
		CROUCH_WALK,
		CROUCH_ATTACK,
		CROUCH_PAIN,
		CROUCH_DEATH,
		DEATH_FALLBACK,
		DEATH_FALLFORWARD,
		DEATH_FALLBACKSLOW,
		BOOM,

		MAX_ANIMATIONS

	} animType_t;




public:
	// constructor/destructor
	CMD2Model(void);
	~CMD2Model(void);


	// functions
	bool	LoadModel(void* file);

	void	DrawModel(float time, Transform position);
	void	DrawFrame(int frame);

	void	SetAnim(int type);
	void	ScaleModel(float s) { m_scale = s; }


private:
	void	Animate(float time);
	void	ProcessLighting(void);
	void	Interpolate(vec3_t* vertlist);
	void	RenderFrame(Transform position);


public:
	// member variables
	static vec3_t	anorms[NUMVERTEXNORMALS];
	static float	anorms_dots[SHADEDOT_QUANT][256];

	static anim_t	animlist[21];		// animation list


public:
	int				num_frames;			// number of frames
	int				num_xyz;			// number of vertices
	int				num_glcmds;			// number of opengl commands

	vec3_t* m_vertices;		// vertex array
	int* m_glcmds;			// opengl command array
	int* m_lightnormals;	// normal index array

	frameInfo*      frameSkins;			// texture id
	animState_t		m_anim;				// animation
	float			m_scale;			// scale value


	FramedModelData dx11Model;

};

typedef enum { mod_bad, mod_brush, mod_sprite, mod_alias } modtype_t;

typedef struct model_s {
	CMD2Model* realModel;
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
	uint32_t d_8to24table[256];
	byte* d_16to8table;


	TextureData GL_Upload32(uint32_t* data, int width, int height, bool mipmap);
	TextureData GL_Upload8(byte* data, int width, int height, bool mipmap, bool is_sky);
	image_t* LoadPic(char* name, byte* pic, int width, int height, imagetype_t type, int bits);

	void LoadAliasModel(model_t* mod, void* buffer);

	FramedModelData ConvertAliasModel(model_t* model);

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

	static void LoadPCX(const char* filename, byte** pic, byte** palette, int* width, int* height);
	static void LoadTGA(const char* name, byte** pic, int* width, int* height);
	void BuildPalettedTexture(unsigned char* paletted_texture, unsigned char* scaled, int scaled_width, int scaled_height);



	image_t* draw_chars;
	uint32_t rawPalette[256];


	~RegistrationManager();

};

void R_BeginRegistration(char* model);
struct model_s* R_RegisterModel(char* name);
struct image_s* R_RegisterSkin(char* name);
image_t* R_RegisterPic(char* name);
void R_SetSky(char* name, float rotate, float axis[3]);
void R_EndRegistration(void); 
void R_SetPalette(const unsigned char* palette);
