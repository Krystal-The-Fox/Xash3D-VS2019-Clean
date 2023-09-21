/*
gl_downsample.c - take a shot of the current resolution and shrink it by a factor

Copyright (C) 2022 Magic_Nipples

9/20/2023
FIXED CRASH WITH GL_INVALID BY SETTING 'ENABLE INCREMENTAL LINKING' TO 'YES'
*/

#include "common.h"
#include "client.h"
#include "gl_local.h"

static int DOWNSAMPLE_SIZE_X;
static int DOWNSAMPLE_SIZE_Y;
static int DSCVAR_VAL;

static int screen_texture_width, screen_texture_height;

// current refdef size:
static int curView_x;
static int curView_y;
static int curView_width;
static int curView_height;

// texture coordinates of screen data inside screentexture
static float screenTex_tcw;
static float screenTex_tch;

static int sample_width;
static int sample_height;

// texture coordinates of adjusted textures
static float sampleText_tcw;
static float sampleText_tch;

/*int TexMgr_Pad(int s)
{
	int i;
	for (i = 1; i < s; i <<= 1)
		;
	return i;
}*/

/*
=================
R_Bloom_InitEffectTexture
=================
*/
void R_Bloom_InitEffectTexture(void)
{
	int texture = tr.r_sampleeffecttexture;
	rgbdata_t	r_screen;

	if (r_downsample->value < 0)
		r_downsample->value = 0;

	if (r_downsample->value > 4.0f)
		r_downsample->value = 4.0f;

	r_downsample->value = (int)r_downsample->value;

	/*if (cl.local.waterlevel >= 3)
	{
		DOWNSAMPLE_SIZE_X = screen_texture_width / (2.5);
		DOWNSAMPLE_SIZE_Y = screen_texture_width / (2.5);
	}*/

	if (r_downsample->value == 1)
	{
		if (glState.height > 480)
			DOWNSAMPLE_SIZE_X = 480;
		else
			DOWNSAMPLE_SIZE_X = 320;
	}
	else if (r_downsample->value == 2)
	{
		if (glState.height > 480)
			DOWNSAMPLE_SIZE_X = 320;
		else
			DOWNSAMPLE_SIZE_X = 240;
	}
	else if (r_downsample->value == 3)
	{
		if (glState.height > 480)
			DOWNSAMPLE_SIZE_X = 240;
		else
			DOWNSAMPLE_SIZE_X = 160;
	}
	else if (r_downsample->value == 4)
	{
		if (glState.height > 480)
			DOWNSAMPLE_SIZE_X = 160;
		else
			DOWNSAMPLE_SIZE_X = 80;
	}
	if (!glState.wideScreen) //fix for 4:3 resolutions having different scaling?
		DOWNSAMPLE_SIZE_X -= 80;

	DOWNSAMPLE_SIZE_Y = DOWNSAMPLE_SIZE_X;

	/*if (!GL_Support(GL_ARB_TEXTURE_NPOT_EXT))
	{
		DOWNSAMPLE_SIZE_X = TexMgr_Pad(DOWNSAMPLE_SIZE_X);
		DOWNSAMPLE_SIZE_Y = TexMgr_Pad(DOWNSAMPLE_SIZE_Y);
	}*/

	if (!texture) 	//tr.r_sampleeffecttexture = GL_CreateTexture("*sampleeffecttexture", DOWNSAMPLE_SIZE_X, DOWNSAMPLE_SIZE_Y, NULL, TF_NEAREST);
	{
		// not initialized ?
		memset(&r_screen, 0, sizeof(r_screen));

		r_screen.width = DOWNSAMPLE_SIZE_X;
		r_screen.height = DOWNSAMPLE_SIZE_Y;
		r_screen.type = PF_RGBA_32;
		r_screen.size = DOWNSAMPLE_SIZE_X * DOWNSAMPLE_SIZE_Y * 4;
		r_screen.flags = IMAGE_HAS_COLOR;
		r_screen.buffer = NULL; // create empty texture for now
		tr.r_sampleeffecttexture = GL_LoadTextureInternal("*sampleeffecttexture", &r_screen, TF_NEAREST);
		texture = tr.r_sampleeffecttexture;
	}
}

/*
=================
R_Sampling_InitTextures
=================
*/
void R_Sampling_InitTextures(void)
{
	int texture = tr.r_initsampletexture;
	rgbdata_t	r_screen;

	screen_texture_width = glState.width;
	screen_texture_height = glState.height;

	// disable downsample if we can't handle a texture of that size
	if (screen_texture_width > glConfig.max_2d_texture_size || screen_texture_height > glConfig.max_2d_texture_size)
	{
		screen_texture_width = screen_texture_height = 0;
		Con_Printf("'R_Sampling_InitTextures' too high of a resolution, effect disabled\n");
		return;
	}

	if (!texture) //tr.r_initsampletexture = GL_CreateTexture("*samplescreentexture", screen_texture_width, screen_texture_height, NULL, TF_NEAREST);
	{
		// not initialized ?
		memset(&r_screen, 0, sizeof(r_screen));

		r_screen.width = screen_texture_width;
		r_screen.height = screen_texture_height;
		r_screen.type = PF_RGBA_32;
		r_screen.size = screen_texture_width * screen_texture_height * 4;
		r_screen.flags = IMAGE_HAS_COLOR;
		r_screen.buffer = NULL; // create empty texture for now
		tr.r_initsampletexture = GL_LoadTextureInternal("*samplescreentexture", &r_screen, TF_NEAREST);
		texture = tr.r_initsampletexture;
	}

	// validate bloom size and init the bloom effect texture
	R_Bloom_InitEffectTexture();

	DSCVAR_VAL = r_downsample->value;
}

/*
=================
R_InitDownSampleTextures
=================
*/
void R_InitDownSampleTextures(void)
{
	DOWNSAMPLE_SIZE_X = 0;
	DOWNSAMPLE_SIZE_Y = 0;

	if (tr.r_initsampletexture)
		GL_FreeTexture(tr.r_initsampletexture);
	if (tr.r_sampleeffecttexture)
		GL_FreeTexture(tr.r_sampleeffecttexture);

	tr.r_initsampletexture = tr.r_sampleeffecttexture = 0;

	//if ((!r_downsample->value) && (cl.local.waterlevel < 3))
	if (!r_downsample->value)
		return;

	R_Sampling_InitTextures();
}

/*
=================
R_Sampling_Quad
=================
*/
static _inline void R_Sampling_Quad(int x, int y, int w, int h, float texwidth, float texheight)
{
	pglBegin(GL_QUADS);
	pglTexCoord2f(0, texheight);
	pglVertex2f(x, y);
	pglTexCoord2f(0, 0);
	pglVertex2f(x, y + h);
	pglTexCoord2f(texwidth, 0);
	pglVertex2f(x + w, y + h);
	pglTexCoord2f(texwidth, texheight);
	pglVertex2f(x + w, y);
	pglEnd();
}

/*
=================
R_DownSampling
=================
*/
void R_DownSampling(void)
{
	//if ( (!r_downsample->value) && (cl.local.waterlevel < 3))
	if (!r_downsample->value)
		return;

	//if (!DOWNSAMPLE_SIZE_X && !DOWNSAMPLE_SIZE_Y)
	//	R_Sampling_InitTextures();

	if (DSCVAR_VAL != r_downsample->value)
		R_InitDownSampleTextures();

	if (screen_texture_width < DOWNSAMPLE_SIZE_X || screen_texture_height < DOWNSAMPLE_SIZE_Y)
		return;

	// set up full screen workspace
	pglScissor(0, 0, glState.width, glState.height);
	pglViewport(0, 0, glState.width, glState.height);
	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();

	pglOrtho(0, glState.width, glState.height, 0, -10, 100);

	pglMatrixMode(GL_MODELVIEW);
	pglLoadIdentity();

	pglDisable(GL_DEPTH_TEST);
	pglDisable(GL_ALPHA_TEST);
	pglDepthMask(GL_FALSE);
	//pglDisable(GL_BLEND);

	GL_Cull(GL_NONE);
	pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// set up current sizes
	curView_x = RI.viewport[0];
	curView_y = RI.viewport[1];
	curView_width = RI.viewport[2];
	curView_height = RI.viewport[3];

	screenTex_tcw = ((float)curView_width / (float)screen_texture_width);
	screenTex_tch = ((float)curView_height / (float)screen_texture_height);
	if (curView_height > curView_width)
	{
		sampleText_tcw = ((float)curView_width / (float)curView_height);
		sampleText_tch = 1.0f;
	}
	else
	{
		sampleText_tcw = 1.0f;
		sampleText_tch = ((float)curView_height / (float)curView_width);
	}

	sample_width = (DOWNSAMPLE_SIZE_X * sampleText_tcw);
	sample_height = (DOWNSAMPLE_SIZE_Y * sampleText_tch);

	// copy the screen and draw resized
	GL_Bind(GL_TEXTURE0, tr.r_initsampletexture);
	pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, curView_x, glState.height - (curView_y + curView_height), curView_width, curView_height);
	R_Sampling_Quad(0, glState.height - sample_height, sample_width, sample_height, screenTex_tcw, screenTex_tch);

	// copy small scene into r_sampleeffecttexture
	GL_Bind(GL_TEXTURE0, tr.r_sampleeffecttexture);
	pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height);

	pglBegin(GL_QUADS);
	pglTexCoord2f(0, sampleText_tch);
	pglVertex2f(curView_x, curView_y);
	pglTexCoord2f(0, 0);
	pglVertex2f(curView_x, curView_y + curView_height);
	pglTexCoord2f(sampleText_tcw, 0);
	pglVertex2f(curView_x + curView_width, curView_y + curView_height);
	pglTexCoord2f(sampleText_tcw, sampleText_tch);
	pglVertex2f(curView_x + curView_width, curView_y);
	pglEnd();

	pglEnable(GL_DEPTH_TEST);
	pglDepthMask(GL_TRUE);
	//pglDisable(GL_BLEND);
	GL_Cull(GL_FRONT);
}