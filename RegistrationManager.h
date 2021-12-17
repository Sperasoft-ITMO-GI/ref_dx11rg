#pragma once
#include <map>
#include "Texture.h"

class RegistrationManager;
extern RegistrationManager RM;

typedef enum {
	//it_skin,
	//it_sprite,
	//it_wall,
	//it_pic,
	//it_sky
} imagetype_t;

typedef struct image_s {
	char	name[256];			// game path, including extension
	imagetype_t	type;
	int		width, height;				// source image
	//int		upload_width, upload_height;	// after power of two and picmip
	int		registration_sequence;		// 0 = free
	//struct msurface_s* texturechain;	// for sort-by-texture world drawing
	int		texnum;						// gl texture binding
	//float	sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	//qboolean	scrap;
	//qboolean	has_alpha;

	//qboolean paletted;
} image_t;

class RegistrationManager {



public:

	void BeginRegistration(const char* map);

	size_t registration_sequence = 0;
	//size_t r_oldviewcluster = 0;

};

void R_BeginRegistration(char* model);
struct model_s* R_RegisterModel(char* name);
struct image_s* R_RegisterSkin(char* name);
image_t* R_RegisterPic(char* name);
void R_SetSky(char* name, float rotate, float axis[3]);
void R_EndRegistration(void);