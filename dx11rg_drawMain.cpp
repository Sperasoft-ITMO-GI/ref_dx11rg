#include "dx11rg_drawData.h"
#include "RegistrationManager.h"


model_t* r_worldmodel;

float gldepthmin, gldepthmax;

image_t* r_notexture; // use for bad textures
image_t* r_particletexture; // little dot for particles

entity_t* currententity;


cplane_t frustum[4];

int r_visframecount; // bumped when going to a new PVS
int r_framecount; // used for dlight push checking

int c_brush_polys, c_alias_polys;

float v_blend[4]; // final blending color

void GL_Strings_f(void);

//
// view origin
//
vec3_t vup;
vec3_t vpn;
vec3_t vright;
vec3_t r_origin;

float r_world_matrix[16];
float r_base_world_matrix[16];

//
// screen size info
//
refdef_t r_newrefdef;

int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t* r_norefresh;
cvar_t* r_drawentities;
cvar_t* r_drawworld;
cvar_t* r_speeds;
cvar_t* r_fullbright;
cvar_t* r_novis;
cvar_t* r_nocull;
cvar_t* r_lerpmodels;
cvar_t* r_lefthand;

cvar_t* r_lightlevel; // FIXME: This is a HACK to get the client's light level


cvar_t* dx11_mode;

cvar_t* vid_gamma;
cvar_t* vid_ref;

//dx11config_t dx11_config;
//dx11state_t  dx11_state;


cvar_t* gl_round_down;


/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame(refdef_t* fd)
{
    //if (r_norefresh->value)
    //	return;

    r_newrefdef = *fd;

    //if (!r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
    //	ri.Sys_Error(ERR_DROP, "R_RenderView: NULL worldmodel");

    if (r_speeds->value)
    {
        c_brush_polys = 0;
        c_alias_polys = 0;
    }

    R_PushDlights();

    //if (gl_finish->value)
    //	qglFinish();

    R_SetupFrame();
    //
    R_SetFrustum();
    //
    R_SetupDX();
    //
    R_MarkLeaves(); // done here so we know if we're in water
    //
    R_DrawWorld();

    R_DrawEntitiesOnList();
    //
    R_RenderDlights();
    //
    R_DrawParticles();
    //
    R_DrawAlphaSurfaces();
    //
    R_Flash();
    //
    //if (r_speeds->value) {
    //	ri.Con_Printf(PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
    //		c_brush_polys,
    //		c_alias_polys,
    //		c_visible_textures,
    //		c_visible_lightmaps);
    //}
    //RD.Present();
    return;
}


/*
=============
R_SetupFrame
=============
*/
void R_SetupFrame(void)
{
    int i;
    mleaf_t* leaf;

    r_framecount++;

    // build the transformation matrix for the given view angles
    VectorCopy(r_newrefdef.vieworg, r_origin);

    AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

    if (!r_worldmodel->firstFrame)
        if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
        {
            r_oldviewcluster = r_viewcluster;
            r_oldviewcluster2 = r_viewcluster2;
            leaf = Mod_PointInLeaf(r_origin, r_worldmodel);
            r_viewcluster = r_viewcluster2 = leaf->cluster;

            // check above and below so crossing solid water doesn't draw wrong
            if (!leaf->contents)
            {
                // look down a bit
                vec3_t temp;

                VectorCopy(r_origin, temp);
                temp[2] -= 16;
                leaf = Mod_PointInLeaf(temp, r_worldmodel);
                if (!(leaf->contents & CONTENTS_SOLID) &&
                    (leaf->cluster != r_viewcluster2))
                    r_viewcluster2 = leaf->cluster;
            } else
            {
                // look up a bit
                vec3_t temp;

                VectorCopy(r_origin, temp);
                temp[2] += 16;
                leaf = Mod_PointInLeaf(temp, r_worldmodel);
                if (!(leaf->contents & CONTENTS_SOLID) &&
                    (leaf->cluster != r_viewcluster2))
                    r_viewcluster2 = leaf->cluster;
            }
        }


    for (i = 0; i < 4; i++)
        v_blend[i] = r_newrefdef.blend[i];

    c_brush_polys = 0;
    c_alias_polys = 0;

    // clear out the portion of the screen that the NOWORLDMODEL defines
    if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
    {
        //qglEnable(GL_SCISSOR_TEST);
        //qglClearColor(0.3, 0.3, 0.3, 1);
        //qglScissor(r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height);
        //qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //qglClearColor(1, 0, 0.5, 0.5);
        //qglDisable(GL_SCISSOR_TEST);
    }
}


/*
=============
R_SetupDX
=============
*/
void R_SetupDX(void)
{
    RenderData data;
    //RD.SetViewMatrix(currentView.ViewMatrix.Transpose()	);

    using namespace DirectX;
    matrix viewMatrix = XMMatrixIdentity();


    viewMatrix *= XMMatrixTranslation(-r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

    viewMatrix *= XMMatrixRotationZ(XMConvertToRadians(-r_newrefdef.viewangles[1]));
    viewMatrix *= XMMatrixRotationY(XMConvertToRadians(-r_newrefdef.viewangles[0]));
    viewMatrix *= XMMatrixRotationX(XMConvertToRadians(-r_newrefdef.viewangles[2]));

    viewMatrix *= XMMatrixRotationZ(XMConvertToRadians(90.0f));
    viewMatrix *= XMMatrixRotationX(XMConvertToRadians(-90.0f));

#
    data.time = r_newrefdef.time;
    data.view = viewMatrix;
    data.projection = matrix::CreatePerspectiveFieldOfView(DegToRad(r_newrefdef.fov_y),
                                                           (float)r_newrefdef.width / (float)r_newrefdef.height, 10.f,
                                                           1500.0f);;


    RD.SetRenderData(data);
}


int SignbitsForPlane(cplane_t* out)
{
    int bits, j;

    // for fast box on planeside test

    bits = 0;
    for (j = 0; j < 3; j++)
    {
        if (out->normal[j] < 0)
            bits |= 1 << j;
    }
    return bits;
}

void R_SetFrustum(void)
{
    int i;

    // rotate VPN right by FOV_X/2 degrees
    RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90 - r_newrefdef.fov_x / 2));
    // rotate VPN left by FOV_X/2 degrees
    RotatePointAroundVector(frustum[1].normal, vup, vpn, 90 - r_newrefdef.fov_x / 2);
    // rotate VPN up by FOV_X/2 degrees
    RotatePointAroundVector(frustum[2].normal, vright, vpn, 90 - r_newrefdef.fov_y / 2);
    // rotate VPN down by FOV_X/2 degrees
    RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90 - r_newrefdef.fov_y / 2));

    for (i = 0; i < 4; i++)
    {
        frustum[i].type = PLANE_ANYZ;
        frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
        frustum[i].signbits = SignbitsForPlane(&frustum[i]);
    }
}


/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList(void)
{
    int i;

    for (int i = 0; i < MAX_LIGHTMAPS; i ++)
    {
        //nonDynamicLightMapUpdateFlag[i] = 0;
        //dynamicLightMapUpdateFlag2 = 0;
    }
    dynamicLightMapUpdateFlag = 0;
    //
    //if (!r_drawentities->value)
    //	return;

    // draw non-transparent first
    for (i = 0; i < r_newrefdef.num_entities; i++)
    {
        currententity = &r_newrefdef.entities[i];
        if (currententity->flags & RF_TRANSLUCENT)
            continue; // solid

        if (currententity->flags & RF_BEAM)
        {
            //R_DrawBeam(currententity);
        } else
        {
            currentmodel = currententity->model;
            if (!currentmodel)
            {
                //R_DrawNullModel();
                continue;
            }
            switch (currentmodel->type)
            {
            case mod_alias:
                R_DrawAliasModel(currententity);
                break;
            case mod_brush:
                R_DrawBrushModel(currententity);
                break;
            case mod_sprite:
                //R_DrawSpriteModel(currententity);
                break;
            default:
                ri.Sys_Error(ERR_DROP, "Bad modeltype");
                break;
            }
        }
    }

    // draw transparent entities
    // we could sort these if it ever becomes a problem...
    //qglDepthMask(0);		// no z writes
    for (i = 0; i < r_newrefdef.num_entities; i++)
    {
        currententity = &r_newrefdef.entities[i];
        if (!(currententity->flags & RF_TRANSLUCENT))
            continue; // solid

        if (currententity->flags & RF_BEAM)
        {
            //R_DrawBeam(currententity);
        } else
        {
            currentmodel = currententity->model;

            if (!currentmodel)
            {
                //R_DrawNullModel();
                continue;
            }
            switch (currentmodel->type)
            {
            case mod_alias:
                R_DrawAliasModel(currententity);
                break;
            case mod_brush:
                R_DrawBrushModel(currententity);
                break;
            case mod_sprite:
                //R_DrawSpriteModel(currententity);
                break;
            default:
                ri.Sys_Error(ERR_DROP, "Bad modeltype");
                break;
            }
        }
    }
    //qglDepthMask(1);		// back to writing
}


void R_DrawParticles(void)
{
    int i;
    unsigned char color[4];
    const particle_t* p;

    static ParticlesMesh particles;
    particles.vertixes.clear();
    particles.indexes.clear();
    ParticleVertex particle;
    particles.pt = PRIMITIVETYPE_POINTLIST_EXT;
    particles.primitiveCount = r_newrefdef.num_particles;
    for (i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++)
    {
        *(int*)color = RM.d_8to24table[p->color];
        color[3] = p->alpha * 255;

        particle.color = float4(color[0], color[1], color[2], color[3]) / 255;
        particle.position = {p->origin[0], p->origin[1], p->origin[2]};
        particles.vertixes.push_back(particle);
        particles.indexes.push_back(i);
    }
    ParticlesDrawData data;
    data.flags = 0;
    RD.DrawParticles(particles, data);
}


void R_Flash()
{
    if (!v_blend[3])
        return;
    UIDrawData data{
        0, 0, (int)windowState.width, (int)windowState.height,
        0, 0, 0, 0,
        float4(v_blend),
        UICOLORED
    };
    RD.DrawColor(data);
};

matrix R_RotateForEntity(entity_t* e, bool old)
{
    matrix result = matrix::CreateTranslation(0, 0, 0);
    //	qglTranslatef(e->origin[0], e->origin[1], e->origin[2]);
    //
    //	qglRotatef(e->angles[1], 0, 0, 1);
    //	qglRotatef(-e->angles[0], 0, 1, 0);
    //	qglRotatef(-e->angles[2], 1, 0, 0);

    // Q2 camera transform
    matrix RotZMat = matrix::CreateRotationZ(DegToRad(e->angles[1]));
    matrix RotYMat = matrix::CreateRotationY(DegToRad(-e->angles[0]));
    matrix RotXMat = matrix::CreateRotationX(DegToRad(-e->angles[2]));
    matrix TranMat;
    if (old)
    {
        //TranMat = matrix::CreateTranslation(
        //    e->origin[0] * (1 - currententity->backlerp) + currententity->backlerp * e->oldorigin[0],
        //    e->origin[1] * (1 - currententity->backlerp) + currententity->backlerp * e->oldorigin[1],
        //    e->origin[2] * (1 - currententity->backlerp) + currententity->backlerp * e->oldorigin[2]);
        
        TranMat = matrix::CreateTranslation(float3(e->oldorigin));
    } else
    {
        TranMat = matrix::CreateTranslation(float3(e->origin));
    }

    result = RotXMat * RotYMat * RotZMat * TranMat;
    return result;
}
