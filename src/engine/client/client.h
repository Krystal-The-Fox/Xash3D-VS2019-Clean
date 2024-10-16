/*
client.h - primary header for client
Copyright (C) 2009 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef CLIENT_H
#define CLIENT_H

#include "mathlib.h"
#include "cdll_int.h"
#include "menu_int.h"
#include "cl_entity.h"
#include "mod_local.h"
#include "pm_defs.h"
#include "pm_movevars.h"
#include "ref_params.h"
#include "render_api.h"
#include "cdll_exp.h"
#include "screenfade.h"
#include "protocol.h"
#include "netchan.h"
#include "net_api.h"
#include "world.h"

#define MAX_DEMOS		32
#define MAX_MOVIES		8
#define MAX_CDTRACKS	32
#define MAX_CLIENT_SPRITES	256	// SpriteTextures
#define MAX_EFRAGS		8192	// Arcane Dimensions required
#define MAX_REQUESTS	64

// screenshot types
#define VID_SCREENSHOT	0
#define VID_LEVELSHOT	1
#define VID_MINISHOT	2
#define VID_MAPSHOT		3	// special case for overview layer
#define VID_SNAPSHOT	4	// save screenshot into root dir and no gamma correction

// client sprite types
#define SPR_CLIENT		0	// client sprite for temp-entities or user-textures
#define SPR_HUDSPRITE	1	// hud sprite
#define SPR_MAPSPRITE	2	// contain overview.bmp that diced into frames 128x128

typedef int		sound_t;

typedef enum
{
	DEMO_INACTIVE = 0,
	DEMO_XASH3D,
	DEMO_QUAKE1
} demo_mode;

//=============================================================================
typedef struct netbandwithgraph_s
{
	word		client;
	word		players;
	word		entities;		// entities bytes, except for players
	word		tentities;	// temp entities
	word		sound;
	word		event;
	word		usr;
	word		msgbytes;
	word		voicebytes;
} netbandwidthgraph_t;

typedef struct frame_s
{
	// received from server
	double		receivedtime;	// time message was received, or -1
	double		latency;
	double		time;		// server timestamp
	qboolean		valid;		// cleared if delta parsing was invalid
	qboolean		choked;

	clientdata_t	clientdata;	// local client private data
	entity_state_t	playerstate[MAX_CLIENTS];
	weapon_data_t	weapondata[MAX_LOCAL_WEAPONS];
	netbandwidthgraph_t graphdata;
	byte		flags[MAX_VISIBLE_PACKET_VIS_BYTES];
	int		num_entities;
	int		first_entity;	// into the circular cl_packet_entities[]
} frame_t;

typedef struct runcmd_s
{
	double		senttime;
	double		receivedtime;
	float		frame_lerp;

	usercmd_t		cmd;

	qboolean		processedfuncs;
	qboolean		heldback;
	int		sendsize;
} runcmd_t;

// add angles
typedef struct
{
	float		starttime;
	float		total;
} pred_viewangle_t;

#define ANGLE_BACKUP	16
#define ANGLE_MASK		(ANGLE_BACKUP - 1)

#define CL_UPDATE_MASK	(CL_UPDATE_BACKUP - 1)
extern int CL_UPDATE_BACKUP;

#define SIGNONS		2		// signon messages to receive before connected
#define INVALID_HANDLE	0xFFFF		// for XashXT cache system

#define MIN_UPDATERATE	10.0f
#define MAX_UPDATERATE	102.0f

#define MIN_EX_INTERP	50.0f
#define MAX_EX_INTERP	100.0f

#define CL_MIN_RESEND_TIME	1.5f		// mininum time gap (in seconds) before a subsequent connection request is sent.    
#define CL_MAX_RESEND_TIME	20.0f		// max time.  The cvar cl_resend is bounded by these.

#define cl_serverframetime()	(cl.mtime[0] - cl.mtime[1])
#define cl_clientframetime()	(cl.time - cl.oldtime)

typedef struct
{
	// got from prediction system
	vec3_t		predicted_origins[CMD_BACKUP];
	vec3_t		prediction_error;
	vec3_t		lastorigin;
	int		lastground;

	// interp info
	float		interp_amount;

	// misc local info
	qboolean		repredicting;	// repredicting in progress
	qboolean		thirdperson;
	qboolean		apply_effects;	// local player will not added but we should apply their effects: flashlight etc
	float		idealpitch;
	int		viewmodel;
	int		health;		// client health
	int		onground;
	int		light_level;
	int		waterlevel;
	int		usehull;
	int		moving;
	int		pushmsec;
	int		weapons;
	float		maxspeed;
	float		scr_fov;

	// weapon predict stuff
	int		weaponsequence;
	float		weaponstarttime;
} cl_local_data_t;

typedef struct
{
	char		name[MAX_OSPATH];
	char		modelname[MAX_OSPATH];
	model_t		*model;
} player_model_t;

typedef struct
{
	qboolean		bUsed;
	float		fTime;
	int		nBytesRemaining;
} downloadtime_t;

typedef struct
{
	qboolean		doneregistering;
	int		percent;
	qboolean		downloadrequested;
	downloadtime_t	rgStats[8];
	int		nCurStat;
	int		nTotalSize;
	int		nTotalToTransfer;
	int		nRemainingToTransfer;
	float		fLastStatusUpdate;
	qboolean		custom;
} incomingtransfer_t;

// the client_t structure is wiped completely
// at every server map change
typedef struct
{
	int		servercount;		// server identification for prespawns
	int		validsequence;		// this is the sequence number of the last good
						// world snapshot/update we got.  If this is 0, we can't
						// render a frame yet
	int		parsecount;		// server message counter
	int		parsecountmod;		// modulo with network window
									
	qboolean		video_prepped;		// false if on new level or new ref dll
	qboolean		audio_prepped;		// false if on new level or new snd dll
	qboolean		paused;

	int		delta_sequence;		// acknowledged sequence number

	double		mtime[2];			// the timestamp of the last two messages
	float		lerpFrac;

	int		last_command_ack;
	int		last_incoming_sequence;

	qboolean		send_reply;
	qboolean		background;		// not real game, just a background
	qboolean		first_frame;		// first rendering frame
	qboolean		proxy_redirect;		// spectator stuff
	qboolean		skip_interp;		// skip interpolation this frame

	uint		checksum;			// for catching cheater maps

	frame_t		frames[MULTIPLAYER_BACKUP];		// alloced on svc_serverdata
	runcmd_t		commands[MULTIPLAYER_BACKUP];		// each mesage will send several old cmds
	local_state_t	predicted_frames[MULTIPLAYER_BACKUP];	// local client state

	double		time;			// this is the time value that the client
						// is rendering at.  always <= cls.realtime
						// a lerp point for other data
	double		oldtime;			// previous cl.time, time-oldtime is used
						// to decay light values and smooth step ups
	float		timedelta;		// floating delta between two updates

	char		serverinfo[MAX_SERVERINFO_STRING];
	player_model_t	player_models[MAX_CLIENTS];	// cache of player models
	player_info_t	players[MAX_CLIENTS];	// collected info about all other players include himself
	double		lastresourcecheck;
	string		downloadUrl;
	event_state_t	events;

	// predicting stuff but not only...
	cl_local_data_t	local;

	// player final info
	usercmd_t		*cmd;			// cl.commands[outgoing_sequence].cmd
	int		viewentity;
	vec3_t		viewangles;
	vec3_t		viewheight;
	vec3_t		punchangle;

	int		intermission;		// don't change view angle, full screen, et
	vec3_t		crosshairangle;

	pred_viewangle_t	predicted_angle[ANGLE_BACKUP];// accumulate angles from server
	int		angle_position;
	float		addangletotal;
	float		prevaddangletotal;

	// predicted origin and velocity
	vec3_t		simorg;
	vec3_t		simvel;

	// server state information
	int		playernum;
	int		maxclients;

	entity_state_t	instanced_baseline[MAX_CUSTOM_BASELINES];
	int		instanced_baseline_count;

	char		sound_precache[MAX_SOUNDS][MAX_QPATH];
	char		event_precache[MAX_EVENTS][MAX_QPATH];
	char		files_precache[MAX_CUSTOM][MAX_QPATH];
	lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	model_t		*models[MAX_MODELS+1];		// precached models (plus sentinel slot)
	int		nummodels;
	int		numfiles;

	consistency_t	consistency_list[MAX_MODELS];
	int		num_consistency;

	qboolean		need_force_consistency_response;
	resource_t	resourcesonhand;
	resource_t	resourcesneeded;
	resource_t	resourcelist[MAX_RESOURCES];
	int		num_resources;

	short		sound_index[MAX_SOUNDS];
	short		decal_index[MAX_DECALS];

	model_t		*worldmodel;			// pointer to world

	double frametime_remainder;
} client_t;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/
typedef enum
{
	ca_disconnected = 0,// not talking to a server
	ca_connecting,	// sending request packets to the server
	ca_connected,	// netchan_t established, waiting for svc_serverdata
	ca_validate,	// download resources, validating, auth on server
	ca_active,	// game views should be displayed
	ca_cinematic,	// playing a cinematic, not connected to a server
} connstate_t;

typedef enum
{
	scrshot_inactive,
	scrshot_normal,	// in-game screenshot
	scrshot_snapshot,	// in-game snapshot
	scrshot_plaque,  	// levelshot
	scrshot_savegame,	// saveshot
	scrshot_mapshot	// overview layer
} scrshot_t;

// client screen state
typedef enum
{
	CL_LOADING = 1,	// draw loading progress-bar
	CL_ACTIVE,	// draw normal hud
	CL_PAUSED,	// pause when active
	CL_CHANGELEVEL,	// draw 'loading' during changelevel
} scrstate_t;

typedef struct
{
	char		name[32];
	int		number;	// svc_ number
	int		size;	// if size == -1, size come from first byte after svcnum
	pfnUserMsgHook	func;	// user-defined function	
} cl_user_message_t;

typedef void (*pfnEventHook)( event_args_t *args );

typedef struct
{
	char		name[MAX_QPATH];
	word		index;	// event index
	pfnEventHook	func;	// user-defined function
} cl_user_event_t;

#define FONT_FIXED		0
#define FONT_VARIABLE	1

typedef struct
{
	int		hFontTexture;		// handle to texture
	int		secFontTexture;
	int		boldFontTexture;
	wrect_t		fontRc[256];		// rectangles
	byte		charWidths[256];
	int		charHeight;
	int		type;
	qboolean		valid;			// all rectangles are valid
} cl_font_t;

typedef struct
{
	// temp handle
	const model_t	*pSprite;			// pointer to current SpriteTexture

	// scissor test
	int		scissor_x;
	int		scissor_y;
	int		scissor_width;
	int		scissor_height;
	qboolean		scissor_test;
	qboolean		adjust_size;		// allow to adjust scale for fonts

	int		renderMode;		// override kRenderMode from TriAPI
	int		cullMode;			// override CULL FACE from TriAPI

	// holds text color
	rgba_t		textColor;
	rgba_t		spriteColor;
	vec4_t		triRGBA;

	// crosshair members
	const model_t	*pCrosshair;
	wrect_t		rcCrosshair;
	rgba_t		rgbaCrosshair;
} client_draw_t;

typedef struct cl_predicted_player_s
{
	int		movetype;
	int		solid;
	int		usehull;
	qboolean		active;
	vec3_t		origin;		// interpolated origin
	vec3_t		angles;
} predicted_player_t;

typedef struct
{
	int		gl_texturenum;	// this is a real texnum

	// scissor test
	int		scissor_x;
	int		scissor_y;
	int		scissor_width;
	int		scissor_height;
	qboolean		scissor_test;

	// holds text color
	rgba_t		textColor;
} gameui_draw_t;

typedef struct
{
	char		szListName[MAX_QPATH];
	client_sprite_t	*pList;
	int		count;
} cached_spritelist_t;

typedef struct
{
	// centerprint stuff
	float		time;
	int		y, lines;
	char		message[2048];
	int		totalWidth;
	int		totalHeight;
} center_print_t;

typedef struct
{
	float		time;
	float		duration;
	float		amplitude;
	float		frequency;
	float		next_shake;
	vec3_t		offset;
	float		angle;
	vec3_t		applied_offset;
	float		applied_angle;
} screen_shake_t;

typedef struct
{
	unsigned short	textures[MAX_SKINS];// alias textures
	struct mstudiotex_s	*ptexture;	// array of textures with local copy of remapped textures
	short		numtextures;	// textures count
	short		topcolor;		// cached value
	short		bottomcolor;	// cached value
	model_t		*model;		// for catch model changes
} remap_info_t;

typedef enum
{
	NET_REQUEST_CANCEL = 0,	// request was cancelled for some reasons
	NET_REQUEST_GAMEUI,		// called from GameUI
	NET_REQUEST_CLIENT,		// called from Client
} net_request_type_t;

typedef struct
{
	net_response_t		resp;
	net_api_response_func_t	pfnFunc;
	double			timeout;
	double			timesend;	// time when request was sended
	int			flags;	// FNETAPI_MULTIPLE_RESPONSE etc
} net_request_t;

// new versions of client dlls have a single export with all callbacks
typedef void (*CL_EXPORT_FUNCS)( void *pv );

typedef struct
{
	void		*hInstance;		// pointer to client.dll
	cldll_func_t	dllFuncs;			// dll exported funcs
	render_interface_t	drawFuncs;		// custom renderer support
	byte		*mempool;			// client edicts pool
	string		mapname;			// map name
	string		maptitle;			// display map title
	string		itemspath;		// path to items description for auto-complete func

	cl_entity_t	*entities;		// dynamically allocated entity array
	cl_entity_t	*static_entities;		// dynamically allocated static entity array
	remap_info_t	**remap_info;		// store local copy of all remap textures for each entity

	int		maxEntities;
	int		maxRemapInfos;		// maxEntities + cl.viewEnt; also used for catch entcount
	int		numStatics;		// actual static entity count
	int		maxModels;

	// movement values from server
	movevars_t	movevars;
	movevars_t	oldmovevars;
	playermove_t	*pmove;			// pmove state

	qboolean		pushed;			// used by PM_Push\Pop state
	int		oldviscount;		// used by PM_Push\Pop state
	int		oldphyscount;		// used by PM_Push\Pop state

	cl_user_message_t	msg[MAX_USER_MESSAGES];	// keep static to avoid fragment memory
	cl_user_event_t	*events[MAX_EVENTS];

	string		cdtracks[MAX_CDTRACKS];	// 32 cd-tracks read from cdaudio.txt

	model_t		sprites[MAX_CLIENT_SPRITES];	// client spritetextures
	int		viewport[4];		// viewport sizes

	client_draw_t	ds;			// draw2d stuff (hud, weaponmenu etc)
	screenfade_t	fade;			// screen fade
	screen_shake_t	shake;			// screen shake
	center_print_t	centerPrint;		// centerprint variables
	SCREENINFO	scrInfo;			// actual screen info
	ref_overview_t	overView;			// overView params
	color24		palette[256];		// palette used for particle colors

	cached_spritelist_t	sprlist[MAX_CLIENT_SPRITES];	// client list sprites

	client_textmessage_t *titles;			// title messages, not network messages
	int		numTitles;

	net_request_type_t	request_type;		// filter the requests
	net_request_t	net_requests[MAX_REQUESTS];	// no reason to keep more
	net_request_t	*master_request;		// queued master request

	efrag_t		*free_efrags;		// linked efrags
	cl_entity_t	viewent;			// viewmodel
} clgame_static_t;

typedef struct
{
	void		*hInstance;		// pointer to client.dll
	UI_FUNCTIONS	dllFuncs;			// dll exported funcs
	byte		*mempool;			// client edicts pool

	cl_entity_t	playermodel;		// uiPlayerSetup drawing model
	player_info_t	playerinfo;		// local playerinfo

	gameui_draw_t	ds;			// draw2d stuff (menu images)
	GAMEINFO		gameInfo;			// current gameInfo
	GAMEINFO		*modsInfo[MAX_MODS];	// simplified gameInfo for MainUI

	ui_globalvars_t	*globals;

	qboolean		drawLogo;			// set to TRUE if logo.avi missed or corrupted
	long		logo_xres;
	long		logo_yres;
	float		logo_length;
} gameui_static_t;

typedef struct
{
	connstate_t	state;
	qboolean		initialized;
	qboolean		changelevel;		// during changelevel
	qboolean		changedemo;		// during changedemo
	double		timestart;		// just for profiling

	// screen rendering information
	float		disable_screen;		// showing loading plaque between levels
						// or changing rendering dlls
						// if time gets > 30 seconds ahead, break it
	int		disable_servercount;	// when we receive a frame and cl.servercount
						// > cls.disable_servercount, clear disable_screen

	qboolean		draw_changelevel;		// draw changelevel image 'Loading...'

	keydest_t		key_dest;

	byte		*mempool;			// client premamnent pool: edicts etc

	netadr_t		hltv_listen_address;

	int		signon;			// 0 to SIGNONS, for the signon sequence.	
	int		quakePort;		// a 16 bit value that allows quake servers
						// to work around address translating routers
						// g-cont. this port allow many copies of engine in multiplayer game
	// connection information
	char		servername[MAX_QPATH];	// name of server from original connect
	double		connect_time;		// for connection retransmits
	int		max_fragment_size;		// we needs to test a real network bandwidth
	int		connect_retry;		// how many times we send a connect packet to the server
	qboolean		spectator;		// not a real player, just spectator

	local_state_t	spectator_state;		// init as client startup

	char		userinfo[MAX_INFO_STRING];
	char		physinfo[MAX_INFO_STRING];	// read-only

	sizebuf_t		datagram;			// unreliable stuff. gets sent in CL_Move about cl_cmdrate times per second.
	byte		datagram_buf[MAX_DATAGRAM];

	netchan_t		netchan;
	int		challenge;		// from the server to use for connecting

	float		packet_loss;
	double		packet_loss_recalc_time;
	int		starting_count;		// message num readed bits

	float		nextcmdtime;		// when can we send the next command packet?                
	int		lastoutgoingcommand;	// sequence number of last outgoing command
	int		lastupdate_sequence;	// prediction stuff

	int		td_lastframe;		// to meter out one message a frame
	int		td_startframe;		// host_framecount at start
	double		td_starttime;		// realtime at second frame of timedemo
	int		forcetrack;		// -1 = use normal cd track

	// game images
	int		pauseIcon;		// draw 'paused' when game in-pause
	int		tileImage;		// for draw any areas not covered by the refresh
	int		loadingBar;		// 'loading' progress bar
	cl_font_t		creditsFont;		// shared creditsfont

	float		latency;			// rolling average of frame latencey (receivedtime - senttime) values.

	int		num_client_entities;	// cl.maxclients * CL_UPDATE_BACKUP * MAX_PACKET_ENTITIES
	int		next_client_entities;	// next client_entity to use
	entity_state_t	*packet_entities;		// [num_client_entities]

	predicted_player_t	predicted_players[MAX_CLIENTS];
	double		correction_time;

	scrshot_t		scrshot_request;		// request for screen shot
	scrshot_t		scrshot_action;		// in-action
	string		shotname;

	// download info
	incomingtransfer_t	dl;

	// demo loop control
	int		demonum;			// -1 = don't play demos
	int		olddemonum;		// restore playing
	char		demos[MAX_DEMOS][MAX_QPATH];	// when not playing
	qboolean		demos_pending;

	// movie playlist
	int		movienum;
	string		movies[MAX_MOVIES];

	// demo recording info must be here, so it isn't clearing on level change
	qboolean		demorecording;
	qboolean		demoplayback;
	qboolean		demowaiting;		// don't record until a non-delta message is received
	qboolean		timedemo;
	string		demoname;			// for demo looping
	double		demotime;			// recording time
	qboolean		set_lastdemo;		// store name of last played demo into the cvar

	file_t		*demofile;
	file_t		*demoheader;		// contain demo startup info in case we record a demo on this level
} client_static_t;

#ifdef __cplusplus
extern "C" {
#endif

extern client_t		cl;
extern client_static_t	cls;
extern clgame_static_t	clgame;
extern gameui_static_t	gameui;

#ifdef __cplusplus
}
#endif

//
// cvars
//
extern convar_t	mp_decals;
extern convar_t	cl_logofile;
extern convar_t	cl_logocolor;
extern convar_t	cl_allow_download;
extern convar_t	cl_allow_upload;
extern convar_t	cl_download_ingame;
extern convar_t	*cl_nopred;
extern convar_t	*cl_showfps;
extern convar_t	*cl_timeout;
extern convar_t	*cl_nodelta;
extern convar_t	*cl_interp;
extern convar_t	*cl_showerror;
extern convar_t	*cl_nosmooth;
extern convar_t	*cl_smoothtime;
extern convar_t	*cl_crosshair;
extern convar_t	*cl_testlights;
extern convar_t	*cl_cmdrate;
extern convar_t	*cl_updaterate;
extern convar_t	*cl_solid_players;
extern convar_t	*cl_idealpitchscale;
extern convar_t	*cl_allow_levelshots;
extern convar_t	*cl_lightstyle_lerping;
extern convar_t	*cl_draw_particles;
extern convar_t	*cl_draw_tracers;
extern convar_t	*cl_levelshot_name;
extern convar_t	*cl_draw_beams;
extern convar_t	*cl_clockreset;
extern convar_t	*cl_fixtimerate;
extern convar_t	*gl_showtextures;
extern convar_t	*cl_bmodelinterp;
extern convar_t	*cl_lw;		// local weapons
extern convar_t	*cl_showevents;
extern convar_t	*scr_centertime;
extern convar_t	*scr_viewsize;
extern convar_t	*scr_loading;
extern convar_t	*v_dark;	// start from dark
extern convar_t	*net_graph;
extern convar_t	*rate;

//=============================================================================

void CL_SetLightstyle( int style, const char* s, float f );
void CL_RunLightStyles( void );
void CL_DecayLights( void );

//=================================================

//
// cl_cmds.c
//
void CL_Quit_f( void );
void CL_ScreenShot_f( void );
void CL_SnapShot_f( void );
void CL_PlayCDTrack_f( void );
void CL_SaveShot_f( void );
void CL_LevelShot_f( void );
void CL_SetSky_f( void );
void SCR_Viewpos_f( void );
void SCR_TimeRefresh_f( void );

//
// cl_custom.c
//
qboolean CL_CheckFile( sizebuf_t *msg, resource_t *pResource );
void CL_AddToResourceList( resource_t *pResource, resource_t *pList );
void CL_RemoveFromResourceList( resource_t *pResource );
void CL_MoveToOnHandList( resource_t *pResource );
void CL_ClearResourceLists( void );

//
// cl_debug.c
//
void CL_Parse_Debug( qboolean enable );
void CL_Parse_RecordCommand( int cmd, int startoffset );
void CL_ResetFrame( frame_t *frame );
void CL_WriteMessageHistory( void );
const char *CL_MsgInfo( int cmd );

//
// cl_main.c
//
void CL_Init( void );
void CL_SendCommand( void );
void CL_Disconnect_f( void );
void CL_ProcessFile( qboolean successfully_received, const char *filename );
void CL_WriteUsercmd( sizebuf_t *msg, int from, int to );
int CL_GetFragmentSize( void *unused );
qboolean CL_PrecacheResources( void );
void CL_SetupOverviewParams( void );
void CL_UpdateFrameLerp( void );
int CL_IsDevOverviewMode( void );
void CL_PingServers_f( void );
void CL_SignonReply( void );
void CL_ClearState( void );

//
// cl_demo.c
//
void CL_StartupDemoHeader( void );
void CL_DrawDemoRecording( void );
void CL_WriteDemoUserCmd( int cmdnumber );
void CL_WriteDemoMessage( qboolean startup, int start, sizebuf_t *msg );
void CL_WriteDemoUserMessage( const byte *buffer, size_t size );
qboolean CL_DemoReadMessage( byte *buffer, size_t *length );
void CL_DemoInterpolateAngles( void );
void CL_CheckStartupDemos( void );
void CL_WriteDemoJumpTime( void );
void CL_CloseDemoHeader( void );
void CL_DemoCompleted( void );
void CL_StopPlayback( void );
void CL_StopRecord( void );
void CL_PlayDemo_f( void );
void CL_TimeDemo_f( void );
void CL_StartDemos_f( void );
void CL_Demos_f( void );
void CL_DeleteDemo_f( void );
void CL_Record_f( void );
void CL_Stop_f( void );

//
// cl_events.c
//
void CL_ParseEvent( sizebuf_t *msg );
void CL_ParseReliableEvent( sizebuf_t *msg );
void CL_SetEventIndex( const char *szEvName, int ev_index );
void CL_QueueEvent( int flags, int index, float delay, event_args_t *args );
void CL_PlaybackEvent( int flags, const edict_t *pInvoker, word eventindex, float delay, float *origin,
	float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
void CL_RegisterEvent( int lastnum, const char *szEvName, pfnEventHook func );
void CL_BatchResourceRequest( qboolean initialize );
int CL_EstimateNeededResources( void );
void CL_ResetEvent( event_info_t *ei );
word CL_EventIndex( const char *name );
void CL_FireEvents( void );

//
// cl_game.c
//
void CL_UnloadProgs( void );
qboolean CL_LoadProgs( const char *name );
void CL_ParseUserMessage( sizebuf_t *msg, int svc_num );
void CL_LinkUserMessage( char *pszName, const int svc_num, int iSize );
void CL_ParseFinaleCutscene( sizebuf_t *msg, int level );
void CL_ParseTextMessage( sizebuf_t *msg );
void CL_DrawHUD( int state );
void CL_InitEdicts( void );
void CL_FreeEdicts( void );
void CL_ClearWorld( void );
void CL_DrawCenterPrint( void );
void CL_ClearSpriteTextures( void );
void CL_FreeEntity( cl_entity_t *pEdict );
void CL_CenterPrint( const char *text, float y );
void CL_TextMessageParse( byte *pMemFile, int fileSize );
client_textmessage_t *CL_TextMessageGet( const char *pName );
int pfnDecalIndexFromName( const char *szDecalName );
int pfnIndexFromTrace( struct pmtrace_s *pTrace );
model_t *CL_ModelHandle( int modelindex );
void NetAPI_CancelAllRequests( void );
int CL_FindModelIndex( const char *m );
cl_entity_t *CL_GetLocalPlayer( void );
model_t *CL_LoadClientSprite( const char *filename );
model_t *CL_LoadModel( const char *modelname, int *index );
HLSPRITE pfnSPR_Load( const char *szPicName );
HLSPRITE pfnSPR_LoadExt( const char *szPicName, uint texFlags );
void PicAdjustSize( float *x, float *y, float *w, float *h );
void CL_FillRGBA( int x, int y, int width, int height, int r, int g, int b, int a );
void CL_PlayerTrace( float *start, float *end, int traceFlags, int ignore_pe, pmtrace_t *tr );
void CL_PlayerTraceExt( float *start, float *end, int traceFlags, int (*pfnIgnore)( physent_t *pe ), pmtrace_t *tr );
void CL_SetTraceHull( int hull );

_inline cl_entity_t *CL_EDICT_NUM( int n )
{
	if(( n >= 0 ) && ( n < clgame.maxEntities ))
		return clgame.entities + n;

	Host_Error( "CL_EDICT_NUM: bad number %i\n", n );
	return NULL;	
}

//
// cl_parse.c
//
void CL_ParseServerMessage( sizebuf_t *msg, qboolean normal_message );
void CL_ParseTempEntity( sizebuf_t *msg );
void CL_StartResourceDownloading( const char *pszMessage, qboolean bCustom );
qboolean CL_DispatchUserMessage( const char *pszName, int iSize, void *pbuf );
qboolean CL_RequestMissingResources( void );
void CL_RegisterResources ( sizebuf_t *msg );
void CL_ParseViewEntity( sizebuf_t *msg );
void CL_ParseServerTime( sizebuf_t *msg );

//
// cl_scrn.c
//
void SCR_VidInit( void );
void SCR_TileClear( void );
void SCR_DirtyScreen( void );
void SCR_AddDirtyPoint( int x, int y );
void SCR_InstallParticlePalette( void );
void SCR_EndLoadingPlaque( void );
void SCR_RegisterTextures( void );
void SCR_LoadCreditsFont( void );
void SCR_MakeScreenShot( void );
void SCR_MakeLevelShot( void );
void SCR_NetSpeeds( void );
void SCR_RSpeeds( void );
void SCR_DrawFPS( int height );

//
// cl_netgraph.c
//
void CL_InitNetgraph( void );
void SCR_DrawNetGraph( void );

//
// cl_view.c
//

void V_Init (void);
void V_Shutdown( void );
qboolean V_PreRender( void );
void V_PostRender( void );
void V_RenderView( void );

//
// cl_pmove.c
//
void CL_SetSolidEntities( void );
void CL_SetSolidPlayers( int playernum );
void CL_InitClientMove( void );
void CL_PredictMovement( qboolean repredicting );
void CL_CheckPredictionError( void );
qboolean CL_IsPredicted( void );
int CL_TruePointContents( const vec3_t p );
int CL_PointContents( const vec3_t p );
int CL_WaterEntity( const float *rgflPos );
cl_entity_t *CL_GetWaterEntity( const float *rgflPos );
void CL_SetupPMove( playermove_t *pmove, local_state_t *from, usercmd_t *ucmd, qboolean runfuncs, double time );
int CL_TestLine( const vec3_t start, const vec3_t end, int flags );
pmtrace_t *CL_VisTraceLine( vec3_t start, vec3_t end, int flags );
pmtrace_t CL_TraceLine( vec3_t start, vec3_t end, int flags );
void CL_PushTraceBounds( int hullnum, const float *mins, const float *maxs );
void CL_PopTraceBounds( void );
void CL_MoveSpectatorCamera( void );
void CL_SetLastUpdate( void );
void CL_RedoPrediction( void );
void CL_ClearPhysEnts( void );
void CL_PushPMStates( void );
void CL_PopPMStates( void );
void CL_SetUpPlayerPrediction( int dopred, int bIncludeLocalClient );

//
// cl_qparse.c
//
void CL_ParseQuakeMessage( sizebuf_t *msg, qboolean normal_message );

//
// cl_studio.c
//
void CL_InitStudioAPI( void );

//
// cl_frame.c
//
int CL_ParsePacketEntities( sizebuf_t *msg, qboolean delta );
qboolean CL_AddVisibleEntity( cl_entity_t *ent, int entityType );
void CL_ResetLatchedVars( cl_entity_t *ent, qboolean full_reset );
qboolean CL_GetEntitySpatialization( struct channel_s *ch );
qboolean CL_GetMovieSpatialization( struct rawchan_s *ch );
void CL_ProcessPlayerState( int playerindex, entity_state_t *state );
void CL_ComputePlayerOrigin( cl_entity_t *clent );
void CL_ProcessPacket( frame_t *frame );
void CL_MoveThirdpersonCamera( void );
qboolean CL_IsPlayerIndex( int idx );
void CL_SetIdealPitch( void );
void CL_EmitEntities( void );

//
// cl_remap.c
//
remap_info_t *CL_GetRemapInfoForEntity( cl_entity_t *e );
void CL_AllocRemapInfo( int topcolor, int bottomcolor );
void CL_FreeRemapInfo( remap_info_t *info );
void R_StudioSetRemapColors( int top, int bottom );
void CL_UpdateRemapInfo( int topcolor, int bottomcolor );
void CL_ClearAllRemaps( void );

//
// cl_tent.c
//
int CL_AddEntity( int entityType, cl_entity_t *pEnt );
void CL_WeaponAnim( int iAnim, int body );
void CL_ClearEffects( void );
void CL_ClearEfrags( void );
void CL_TestLights( void );
void CL_DrawParticlesExternal( const ref_viewpass_t *rvp, qboolean trans_pass, float frametime );
void CL_FireCustomDecal( int textureIndex, int entityIndex, int modelIndex, float *pos, int flags, float scale );
void CL_DecalShoot( int textureIndex, int entityIndex, int modelIndex, float *pos, int flags );
void CL_PlayerDecal( int playerIndex, int textureIndex, int entityIndex, float *pos );
void R_FreeDeadParticles( struct particle_s **ppparticles );
void CL_AddClientResource( const char *filename, int type );
void CL_AddClientResources( void );
int CL_FxBlend( cl_entity_t *e );
void CL_InitParticles( void );
void CL_ClearParticles( void );
void CL_FreeParticles( void );
void CL_DrawParticles( double frametime );
void CL_DrawTracers( double frametime );
void CL_InitTempEnts( void );
void CL_ClearTempEnts( void );
void CL_FreeTempEnts( void );
void CL_TempEntUpdate( void );
void CL_InitViewBeams( void );
void CL_ClearViewBeams( void );
void CL_FreeViewBeams( void );
void CL_DrawBeams( int fTrans );
void CL_AddCustomBeam( cl_entity_t *pEnvBeam );
void CL_KillDeadBeams( cl_entity_t *pDeadEntity );
void CL_ParseViewBeam( sizebuf_t *msg, int beamType );
void CL_LoadClientSprites( void );
void CL_ReadPointFile_f( void );
void CL_ReadLineFile_f( void );
void CL_RunLightStyles( void );

//
// console.c
//
extern convar_t *con_fontsize;
qboolean Con_Visible( void );
qboolean Con_FixedFont( void );
void Con_VidInit( void );
void Con_Shutdown( void );
void Con_ToggleConsole_f( void );
void Con_ClearNotify( void );
void Con_DrawDebug( void );
void Con_RunConsole( void );
void Con_DrawConsole( void );
void Con_DrawVersion( void );
void Con_DrawStringLen( const char *pText, int *length, int *height );
void Con_DrawSecLen(const char* pText, int* length, int* height);
int Con_DrawString( int x, int y, const char *string, rgba_t setColor );
int Con_DrawSecString(int x, int y, const char* string, rgba_t setColor);
int Con_DrawBoldString(int x, int y, const char* string, rgba_t setColor);
int Con_DrawCharacter( int x, int y, int number, rgba_t color );
void Con_DrawCharacterLen( int number, int *width, int *height );
void Con_DefaultColor( int r, int g, int b );
void Con_InvalidateFonts( void );
void Con_SetFont( int fontNum );
void Con_SetSecFont(int fontNum);
void Con_SetBoldFont(int fontNum);
void Con_CharEvent( int key );
void Con_RestoreFont( void );
void Con_RestoreSecFont(void);
void Key_Console( int key );
void Key_Message( int key );
void Con_FastClose( void );

//
// s_main.c
//
void S_StreamRawSamples( int samples, int rate, int width, int channels, const byte *data );
void S_StreamAviSamples( void *Avi, int entnum, float fvol, float attn, float synctime );
void S_StartBackgroundTrack( const char *intro, const char *loop, long position, qboolean fullpath );
void S_StopBackgroundTrack( void );
void S_StreamSetPause( int pause );
void S_StartStreaming( void );
void S_StopStreaming( void );
void S_BeginRegistration( void );
sound_t S_RegisterSound( const char *sample );
void S_EndRegistration( void );
void S_RestoreSound( const vec3_t pos, int ent, int chan, sound_t handle, float fvol, float attn, int pitch, int flags, double sample, double end, int wordIndex );
void S_StartSound( const vec3_t pos, int ent, int chan, sound_t sfx, float vol, float attn, int pitch, int flags );
void S_AmbientSound( const vec3_t pos, int ent, sound_t handle, float fvol, float attn, int pitch, int flags );
void S_FadeClientVolume( float fadePercent, float fadeOutSeconds, float holdTime, float fadeInSeconds );
void S_FadeMusicVolume( float fadePercent );
void S_StartLocalSound( const char *name, float volume, qboolean reliable );
void SND_UpdateSound( void );
void S_ExtraUpdate( void );

//
// cl_gameui.c
//
void UI_UnloadProgs( void );
qboolean UI_LoadProgs( void );
void UI_UpdateMenu( float realtime );
void UI_KeyEvent( int key, qboolean down );
void UI_MouseMove( int x, int y );
void UI_SetActiveMenu( qboolean fActive );
void UI_AddServerToList( netadr_t adr, const char *info );
void UI_GetCursorPos( int *pos_x, int *pos_y );
void UI_SetCursorPos( int pos_x, int pos_y );
void UI_ShowCursor( qboolean show );
qboolean UI_CreditsActive( void );
void UI_CharEvent( int key );
qboolean UI_MouseInRect( void );
qboolean UI_IsVisible( void );
void pfnPIC_Set( HIMAGE hPic, int r, int g, int b, int a );
void pfnPIC_Draw( int x, int y, int width, int height, const wrect_t *prc );
void pfnPIC_DrawTrans( int x, int y, int width, int height, const wrect_t *prc );
void pfnPIC_DrawHoles( int x, int y, int width, int height, const wrect_t *prc );
void pfnPIC_DrawAdditive( int x, int y, int width, int height, const wrect_t *prc );

//
// cl_video.c
//
void SCR_InitCinematic( void );
void SCR_FreeCinematic( void );
qboolean SCR_PlayCinematic( const char *name );
qboolean SCR_DrawCinematic( void );
qboolean SCR_NextMovie( void );
void SCR_RunCinematic( void );
void SCR_StopCinematic( void );
void CL_PlayVideo_f( void );

#endif//CLIENT_H