#include "RegistrationManager.h"
#include "dx11rg_local.h"

void RegistrationManager::InitImages() {

	registration_sequence = 1;

	// init intensity conversions
	//intensity = ri.Cvar_Get("intensity", "2", 0);
	//
	//if (intensity->value <= 1)
	//	ri.Cvar_Set("intensity", "1");
	//
	//dx11_state.inverse_intensity = 1 / intensity->value;
	{
		int		r, g, b;
		unsigned	v;
		byte* pic, * pal;
		int		width, height;

		// get the palette

		LoadPCX("pics/colormap.pcx", &pic, &pal, &width, &height);
		if (!pal)
			ri.Sys_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");

		for (int i = 0; i < 256; i++) {
			r = pal[i * 3 + 0];
			g = pal[i * 3 + 1];
			b = pal[i * 3 + 2];

			v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
			d_8to24table[i] = LittleLong(v);
		}

		d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

		free(pic);
		free(pal);
	}

	ri.FS_LoadFile("pics/16to8.dat", (void**)&(d_16to8table));
	if (!d_16to8table)
		ri.Sys_Error(ERR_FATAL, "Couldn't load pics/16to8.pcx");

	float g = 0;

	for (int i = 0; i < 256; i++) {
		if (g == 1) {
			gammatable[i] = i;
		} else {
			float inf;

			inf = 255 * pow((i + 0.5) / 255.5, g) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	//for (int i = 0; i < 256; i++) {
	//	int j = i * intensity->value;
	//	if (j > 255)
	//		j = 255;
	//	intensitytable[i] = j;
	//}
}

void RegistrationManager::InitLocal(void) {
	draw_chars = FindImage("pics/conchars.pcx", it_pic);
}

void RegistrationManager::BeginRegistration(const char* map) {
	char	fullname[MAX_QPATH];
	//cvar_t* flushmap;

	registration_sequence++;
	//r_oldviewcluster = -1;		// force markleafs

	//Com_sprintf(fullname, sizeof(fullname), "maps/%s.bsp", map);

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	//flushmap = ri.Cvar_Get("flushmap", "0", 0);
	//if (strcmp(mod_known[0].name, fullname) || flushmap->value)
	//	Mod_Free(&mod_known[0]);
	//r_worldmodel = Mod_ForName(fullname, True);
	//
	//r_viewcluster = -1;
}

void RegistrationManager::SetPalette(const unsigned char* palette) {
	int		i;

	byte* rp = (byte*)rawPalette;

	if (palette) {
		for (i = 0; i < 256; i++) {
			rp[i * 4 + 0] = palette[i * 3 + 0];
			rp[i * 4 + 1] = palette[i * 3 + 1];
			rp[i * 4 + 2] = palette[i * 3 + 2];
			rp[i * 4 + 3] = 0xff;
		}
	} else {
		for (i = 0; i < 256; i++) {
			rp[i * 4 + 0] = d_8to24table[i] & 0xff;
			rp[i * 4 + 1] = (d_8to24table[i] >> 8) & 0xff;
			rp[i * 4 + 2] = (d_8to24table[i] >> 16) & 0xff;
			rp[i * 4 + 3] = 0xff;
		}
	}
	// setting texture palette
	// 
	//int i;
	//unsigned char temptable[768];
	//
	//for (i = 0; i < 256; i++) {
	//	temptable[i * 3 + 0] = (palette[i] >> 0) & 0xff;
	//	temptable[i * 3 + 1] = (palette[i] >> 8) & 0xff;
	//	temptable[i * 3 + 2] = (palette[i] >> 16) & 0xff;
	//}

}

void RegistrationManager::EndRegistration() {

	image_t* image;

	// never free r_notexture or particle texture
	//r_notexture->registration_sequence = registration_sequence;
	//r_particletexture->registration_sequence = registration_sequence;

	for (int i = 0; i < numTextures; i++) {

		if (dxTextures[i].registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!dxTextures[i].registration_sequence)
			continue;		// free image_t slot
		if (dxTextures[i].type == it_pic)
			continue;		// don't free pics
		// free it
		RD.ReleaseTexture(dxTextures[i].texnum);
		dxTextures[i].texnum = -1;
	}
}

image_t* RegistrationManager::FindImage(char* name, imagetype_t type) {
	image_t* image = nullptr;
	int		i, len;
	byte* pic, * palette;
	int		width, height;

	if (!name)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name");
	len = strlen(name);
	if (len < 5)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

	for (i = 0; i < numTextures; i++) {
		if (!strcmp(name, dxTextures[i].name)) {
			dxTextures[i].registration_sequence = registration_sequence;
			return &dxTextures[i];
		}
	}



	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;
	if (!strcmp(name + len - 4, ".pcx")) {
		LoadPCX(name, &pic, &palette, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
		image = LoadPic(name, pic, width, height, type, 8);
	} else if (!strcmp(name + len - 4, ".wal")) {
		//image = GL_LoadWal(name);
	} else if (!strcmp(name + len - 4, ".tga")) {
		LoadTGA(name, &pic, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name);
		image = LoadPic(name, pic, width, height, type, 32);
	} else
		return NULL;	//	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name);


	if (pic)
		free(pic);
	if (palette)
		free(palette);

	return image;
}

const int		gl_solid_format = 3;
const int		gl_alpha_format = 4;
int		upload_width, upload_height;
bool uploaded_paletted;

void  RegistrationManager::BuildPalettedTexture(unsigned char* paletted_texture, unsigned char* scaled, int scaled_width, int scaled_height) {
	int i;

	for (i = 0; i < scaled_width * scaled_height; i++) {
		unsigned int r, g, b, c;

		r = (scaled[0] >> 3) & 31;
		g = (scaled[1] >> 2) & 63;
		b = (scaled[2] >> 3) & 31;

		c = r | (g << 5) | (b << 11);

		paletted_texture[i] = d_16to8table[c];

		scaled += 4;
	}
}



TextureData  RegistrationManager::GL_Upload32(uint32_t* data, int width, int height, bool mipmap) {
	int			samples;
	unsigned	scaled[256 * 256];
	unsigned char paletted_texture[256 * 256];
	int			scaled_width, scaled_height;
	int			i, c;
	byte* scan;
	int comp;

	uploaded_paletted = False;

	for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
		;
	//if (gl_round_down->value && scaled_width > width && mipmap)
	//	scaled_width >>= 1;
	for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
		;
	//if (gl_round_down->value && scaled_height > height && mipmap)
	//	scaled_height >>= 1;

	//// let people sample down the world textures for speed
	//if (mipmap) {
	//	scaled_width >>= (int)gl_picmip->value;
	//	scaled_height >>= (int)gl_picmip->value;
	//}

	// don't ever bother with >256 textures
	if (scaled_width > 256)
		scaled_width = 256;
	if (scaled_height > 256)
		scaled_height = 256;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled) / 4)
		ri.Sys_Error(ERR_DROP, "GL_Upload32: too big");

	// scan the texture for any non-255 alpha
	c = width * height;
	scan = ((byte*)data) + 3;
	samples = gl_solid_format;
	for (i = 0; i < c; i++, scan += 4) {
		if (*scan != 255) {
			samples = gl_alpha_format;
			break;
		}
	}

	//if (samples == gl_solid_format)
	//	comp = gl_tex_solid_format;
	//else if (samples == gl_alpha_format)
	//	comp = gl_tex_alpha_format;
	//else {
	//	ri.Con_Printf(PRINT_ALL,
	//		"Unknown number of texture components %i\n",
	//		samples);
	//	comp = samples;
	//}


	//if (scaled_width == width && scaled_height == height) {
	//
	//	Texture tex = Texture(scaled_width, scaled_height);
	//	for (size_t w = 0; w < scaled_width; w++) {
	//		for (size_t h = 0; h < scaled_height; h++) {
	//			tex.PutPixel(w, h, data[w + h * scaled_width]);
	//		}
	//	}
	//	return tex;
	//	//qglTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//
	//
	////memcpy(scaled, data, width * height * 4);
	//} else
	//	GL_ResampleTexture(data, width, height, scaled, scaled_width, scaled_height);
	//
	//GL_LightScaleTexture(scaled, scaled_width, scaled_height, !mipmap);

	//if (qglColorTableEXT && gl_ext_palettedtexture->value && (samples == gl_solid_format)) {
	//	uploaded_paletted = True;
	//	GL_BuildPalettedTexture(paletted_texture, (unsigned char*)scaled, scaled_width, scaled_height);
	//	qglTexImage2D(GL_TEXTURE_2D,
	//		0,
	//		GL_COLOR_INDEX8_EXT,
	//		scaled_width,
	//		scaled_height,
	//		0,
	//		GL_COLOR_INDEX,
	//		GL_UNSIGNED_BYTE,
	//		paletted_texture);
	//} else {
	//	qglTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	//}


	TextureData tex = TextureData(width, height);
	for (size_t w = 0; w < width; w++) {
		for (size_t h = 0; h < height; h++) {

			auto c = TextureData::Color(data[w + h * width]);
			uint8_t b = c.GetR();
			c.SetR(c.GetB());
			c.SetB(b);
			tex.PutPixel(w, h, data[w + h * width]);
			//tex.PutPixel(w, h, data[w + h*width]);
		}
	}
	return tex;
}

TextureData RegistrationManager::GL_Upload8(byte* data, int width, int height, bool mipmap, bool is_sky) {
	unsigned	trans[512 * 256];
	int			i, s;
	int			p;

	s = width * height;

	for (i = 0; i < s; i++) {
		p = data[i];
		trans[i] = d_8to24table[p];

		if (p == 255) {	// transparent, so scan around for another color
			// to avoid alpha fringes
			// FIXME: do a full flood fill so mips work...
			if (i > width && data[i - width] != 255)
				p = data[i - width];
			else if (i < s - width && data[i + width] != 255)
				p = data[i + width];
			else if (i > 0 && data[i - 1] != 255)
				p = data[i - 1];
			else if (i < s - 1 && data[i + 1] != 255)
				p = data[i + 1];
			else
				p = 0;
			// copy rgb components
			((byte*)&trans[i])[0] = ((byte*)&d_8to24table[p])[0];
			((byte*)&trans[i])[1] = ((byte*)&d_8to24table[p])[1];
			((byte*)&trans[i])[2] = ((byte*)&d_8to24table[p])[2];
		}
	}

	return GL_Upload32(trans, width, height, mipmap);

}


image_t* RegistrationManager::LoadPic(char* name, byte* pic, int width, int height, imagetype_t type, int bits) {
	image_t* image;
	int			i;

	// find a free image_t
	for (i = 0; i < numTextures; i++) {
		if (dxTextures[i].texnum == -1)
			break;
	}

	if (i == numTextures) {
		if (numTextures == MAX_DXTEXTURES)
			ri.Sys_Error(ERR_DROP, "MAX_GLTEXTURES");
		numTextures++;
	}
	image = &dxTextures[i];

	if (strlen(name) >= sizeof(image->name))
		ri.Sys_Error(ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	strcpy(image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	//if (type == it_skin && bits == 8)
	//	R_FloodFillSkin(pic, width, height);

	// load little pics into the scrap
	{
		image->texnum = i;
		//GL_Bind(image->texnum);
		TextureData tex = TextureData(0, 0);
		if (bits == 8)
			tex = GL_Upload8(pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_sky);
		else
			tex = GL_Upload32((unsigned*)pic, width, height, (image->type != it_pic && image->type != it_sky));
		//image->width = tex.GetWidth();		// after power of 2 and scales
		//image->height = tex.GetHeight();
		//image->paletted = uploaded_paletted;
		//image->sl = 0;
		//image->sh = 1;
		//image->tl = 0;
		//image->th = 1;
		RD.RegisterTexture(i, tex);
	}

	return image;
}

void RegistrationManager::LoadPCX(char* filename, byte** pic, byte** palette, int* width, int* height) {
	byte* raw;
	pcx_t* pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte* out, * pix;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = ri.FS_LoadFile(filename, (void**)&raw);
	if (!raw) {
		ri.Con_Printf(PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t*)raw;

	pcx->xmin = LittleShort(pcx->xmin);
	pcx->ymin = LittleShort(pcx->ymin);
	pcx->xmax = LittleShort(pcx->xmax);
	pcx->ymax = LittleShort(pcx->ymax);
	pcx->hres = LittleShort(pcx->hres);
	pcx->vres = LittleShort(pcx->vres);
	pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
	pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480) {
		ri.Con_Printf(PRINT_ALL, "Bad pcx file %s\n", filename);
		return;
	}

	out = (byte*)malloc((pcx->ymax + 1) * (pcx->xmax + 1));

	*pic = out;

	pix = out;

	if (palette) {
		*palette = (byte*)malloc(768);
		memcpy(*palette, (byte*)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax + 1;
	if (height)
		*height = pcx->ymax + 1;

	for (y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1) {
		for (x = 0; x <= pcx->xmax; ) {
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0) {
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			} else
				runLength = 1;

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if (raw - (byte*)pcx > len) {
		ri.Con_Printf(PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		free(*pic);
		*pic = NULL;
	}

	ri.FS_FreeFile(pcx);
}

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;

void RegistrationManager::LoadTGA(char* name, byte** pic, int* width, int* height) {
	int		columns, rows, numPixels;
	byte* pixbuf;
	int		row, column;
	byte* buf_p;
	byte* buffer;
	int		length;
	TargaHeader		targa_header;
	byte* targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_LoadFile(name, (void**)&buffer);
	if (!buffer) {
		ri.Con_Printf(PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort(*((short*)tmp));
	buf_p += 2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort(*((short*)tmp));
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.y_origin = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.width = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.height = LittleShort(*((short*)buf_p));
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type != 2
		&& targa_header.image_type != 10)
		ri.Sys_Error(ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");

	if (targa_header.colormap_type != 0
		|| (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
		ri.Sys_Error(ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = (byte*)malloc(numPixels * 4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment

	if (targa_header.image_type == 2) {  // Uncompressed, RGB images
		for (row = rows - 1; row >= 0; row--) {
			pixbuf = targa_rgba + row * columns * 4;
			for (column = 0; column < columns; column++) {
				unsigned char red, green, blue, alphabyte;
				switch (targa_header.pixel_size) {
				case 24:

					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;
				}
			}
		}
	} else if (targa_header.image_type == 10) {   // Runlength encoded RGB images
		unsigned char red, green, blue, alphabyte, packetHeader, packetSize, j;
		for (row = rows - 1; row >= 0; row--) {
			pixbuf = targa_rgba + row * columns * 4;
			for (column = 0; column < columns; ) {
				packetHeader = *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						break;
					}

					for (j = 0; j < packetSize; j++) {
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if (column == columns) { // run spans across rows
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				} else {                            // non run-length packet
					for (j = 0; j < packetSize; j++) {
						switch (targa_header.pixel_size) {
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
						}
						column++;
						if (column == columns) { // pixel packet run spans across rows
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
		breakOut:;
		}
	}

	ri.FS_FreeFile(buffer);
}
