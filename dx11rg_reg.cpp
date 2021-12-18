#include "dx11rg_local.h"
#include "RegistrationManager.h"

RegistrationManager RM;

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration(char* model) {
	RM.BeginRegistration(model);
	return;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/
struct model_s* R_RegisterModel(char* name) {
	return NULL;
}

/*
===============
R_RegisterSkin
===============
*/
struct image_s* R_RegisterSkin(char* name) {
	return NULL;
}

/*
=============
Draw_FindPic
=============
*/
image_t* R_RegisterPic(char* name) {
	image_t* gl = nullptr;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\') {
		Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
		gl = RM.FindImage(fullname, it_pic);
	} else
		gl = RM.FindImage(name + 1, it_pic);

	return gl;
}

/*
============
R_SetSky
============
*/
// 3dstudio environment map names
char* suf[6] = { (char*)"rt", (char*)"bk", (char*)"lf", (char*)"ft", (char*)"up", (char*)"dn" };
void R_SetSky(char* name, float rotate, vec3_t axis) {
	return;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration(void) {
	RM.EndRegistration();
	return;
}


void R_SetPalette(const unsigned char* palette) {
	RM.SetPalette(palette);
	return;
}
