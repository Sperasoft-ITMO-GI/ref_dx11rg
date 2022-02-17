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
	model_t* mod;
	int		i;
	//dsprite_t* sprout;
	dmdl_t* pheader;

	mod = RM.FindModel(name, False);
	if (mod) {
		mod->registration_sequence = RM.registration_sequence;

		// register any images used by the models
		if (mod->type == mod_sprite) {
			auto sprout = mod;
			for (i = 0; i < sprout->num_frames; i++)
				sprout->skins[i].m_texid = RM.FindImage(sprout->skins[i].name, it_sprite);
		}
		else if (mod->type == mod_alias) {
			auto sprout = mod;
			for (i = 0; i < sprout->num_skins; i++)
				sprout->skins[i].m_texid = RM.FindImage(sprout->skins[i].name, it_skin);
			//PGM
			//mod->numframes = pheader->num_frames;
			//PGM
		}
		else if (mod->type == mod_brush) {
			for (i = 0; i < mod->numtexinfo; i++)
				mod->texinfo[i].image->registration_sequence = RM.registration_sequence;
		}
	}
	return mod;
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
