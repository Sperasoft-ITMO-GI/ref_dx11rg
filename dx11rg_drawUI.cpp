#include "dx11rg_local.h"
#include "RegistrationManager.h"




void Draw_Char(int x, int y, int num) {
	int				row, col;
	float			frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	UIDrawData data{ x, y, 8, 8,
	col * 8, row * 8, 8, 8,
	float4(0.0f, 0.0f, 0.0f, 0.8f),
	UICHAR | UISCALED };

	RD.DrawImg(RM.draw_chars->texnum, data);
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize(int* w, int* h, char* pic) {
	image_t* gl;

	gl = R_RegisterPic(pic);
	if (!gl) {
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic(int x, int y, int w, int h, char* pic) {
	image_t* gl;

	gl = R_RegisterPic(pic);
	if (!gl) {
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}
	UIDrawData data{ x, y, w, h,
	0,0,0,0,
	float4(),
	0 };

	RD.DrawImg(gl->texnum, data);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, char* pic) {
	image_t* gl;

	gl = R_RegisterPic(pic);
	if (!gl) {
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	UIDrawData data{ x, y, gl->width, gl->height,
	0,0,0,0,
	float4(),
	0 };

	RD.DrawImg(gl->texnum, data);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h, char* pic) {
	image_t* image;

	image = R_RegisterPic(pic);
	if (!image) {
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	UIDrawData drawData{ x, y, w, h,
	0,0,0,0,
	float4(),
	0 };
	RD.DrawImg(image->texnum, drawData);

}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c) {

	TextureData::Color cb = TextureData::Color(RM.d_8to24table[c]);
	UIDrawData data{ x, y, w, h,
	0,0,0,0,
	float4(cb.ToFloat4()),
	UICOLORED };
	RD.DrawColor(data);

	//TODO
}

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void) {

	//
	//auto c = { 0.0f, 0.0f, 0.0f, 0.8f };
	UIDrawData data{ 0, 0, windowState.width, windowState.height,
	0,0,0,0,
	float4(0.0f, 0.0f, 0.0f, 0.8f),
	UICOLORED };
	RD.DrawColor(data);
	//TODO
}

/*
=============
Draw_StretchRaw
=============
*/

void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte* data) {
	static unsigned	image32[256 * 256];
	static unsigned char image8[256 * 256];
	int			i, j, trows;
	byte* source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;

	if (rows <= 256) {
		hscale = 1;
		trows = rows;
	}
	else {
		hscale = rows / 256.0;
		trows = 256;
	}
	t = rows * hscale / 256;

	unsigned* dest;

	for (i = 0; i < trows; i++) {
		row = (int)(i * hscale);
		if (row > rows)
			break;
		source = data + cols * row;
		dest = &image32[i * 256];
		fracstep = cols * 0x10000 / 256;
		frac = fracstep >> 1;
		for (j = 0; j < 256; j++) {
			dest[j] = RM.rawPalette[source[frac >> 16]];
			frac += fracstep;
		}
	}

	static TextureData tex(256, 256);
	for (size_t h = 0; h < 256; h++) {
		for (size_t w = 0; w < 256; w++) {
			auto c = TextureData::Color(image32[h * 256 + w]);
			uint8_t b = c.GetR();
			c.SetR(c.GetB());
			c.SetB(b);
			tex.PutPixel(w, h, image32[h * 256 + w]);
		}
	}

	RD.UpdateTexture(1300, tex);


	UIDrawData drawData{ x, y, windowState.width, windowState.height,
	0,0,0,0,
	float4(),
	0 };

	RD.DrawImg(1300, drawData);

}