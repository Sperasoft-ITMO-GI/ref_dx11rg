#include "RegistrationManager.h"
#include "dx11rg_local.h"

void RegistrationManager::BeginRegistration(const char* map) {
	char	fullname[MAX_QPATH];
	cvar_t* flushmap;

	registration_sequence++;
	//r_oldviewcluster = -1;		// force markleafs

	Com_sprintf(fullname, sizeof(fullname), "maps/%s.bsp", map);

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	flushmap = ri.Cvar_Get("flushmap", "0", 0);
	//if (strcmp(mod_known[0].name, fullname) || flushmap->value)
	//	Mod_Free(&mod_known[0]);
	//r_worldmodel = Mod_ForName(fullname, True);
	//
	//r_viewcluster = -1;
}
