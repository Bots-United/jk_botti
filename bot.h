//
// HPB_bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot.h
//

#ifndef BOT_H
#define BOT_H

#include <ctype.h>

// stuff for Win32 vs. Linux builds

#ifdef __linux__
#define Sleep sleep
#define stricmp strcasecmp
#define strcmpi strcasecmp 
#define strnicmp strncasecmp
typedef int BOOL;
#endif


// for detecting submods
#define SUBMOD_HLDM 0
#define SUBMOD_SEVS 1
#define SUBMOD_BUBBLEMOD 2
#define SUBMOD_XDM 3


// global defines
#define MAX_HEALTH 100
#define PLAYER_SEARCH_RADIUS     40.0


// define a new bit flag for bot identification
#define FL_THIRDPARTYBOT (1 << 27)


// define some function prototypes...
void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3);


#define LADDER_UNKNOWN  0
#define LADDER_UP       1
#define LADDER_DOWN     2

#define WANDER_LEFT  1
#define WANDER_RIGHT 2

#define RESPAWN_IDLE             1
#define RESPAWN_NEED_TO_RESPAWN  2
#define RESPAWN_IS_RESPAWNING    3

// waypoint goal types
#define WPT_GOAL_NONE		0
#define WPT_GOAL_HEALTH		1
#define WPT_GOAL_ARMOR		2
#define WPT_GOAL_WEAPON		3
#define WPT_GOAL_AMMO		4
#define WPT_GOAL_ITEM		5
#define WPT_GOAL_LOCATION	8
#define WPT_GOAL_ENEMY		9

// instant damage (from cbase.h)
#define DMG_CRUSH			(1 << 0)	// crushed by falling or moving object
#define DMG_BURN			(1 << 3)	// heat burned
#define DMG_FREEZE			(1 << 4)	// frozen
#define DMG_FALL			(1 << 5)	// fell too far
#define DMG_SHOCK			(1 << 8)	// electric shock
#define DMG_DROWN			(1 << 14)	// Drowning
#define DMG_NERVEGAS		(1 << 16)	// nerve toxins, very bad
#define DMG_RADIATION		(1 << 18)	// radiation exposure
#define DMG_DROWNRECOVER	(1 << 19)	// drowning recovery
#define DMG_ACID			(1 << 20)	// toxic chemicals or acid burns
#define DMG_SLOWBURN		(1 << 21)	// in an oven
#define DMG_SLOWFREEZE		(1 << 22)	// in a subzero freezer


#define BOT_SKIN_LEN 32
#define BOT_NAME_LEN 32

#define MAX_BOT_CHAT 100


typedef struct trigger_sound_s {
	int used;
	Vector origin;
	float volume;
	float attenuation;
	float time;
} trigger_sound_t;


extern trigger_sound_t trigger_sounds[32];
extern void SaveSound(edict_t * pPlayer, float time, const Vector & origin, float volume, float attenuation, int empty);


typedef struct
{
   int  iId;     // weapon ID
   int  iClip;   // amount of ammo in the clip
   int  iAmmo1;  // amount of ammo in primary reserve
   int  iAmmo2;  // amount of ammo in secondary reserve
} bot_current_weapon_t;


typedef struct
{
   qboolean is_used;
   int respawn_state;
   edict_t *pEdict;
   qboolean need_to_initialize;
   char name[BOT_NAME_LEN+1];
   char skin[BOT_SKIN_LEN+1];
   int bot_skill;
   int not_started;
   int start_action;
   float f_kick_time;
   float f_create_time;
   float f_frame_time;
   Vector bot_v_angle;
   
   int chat_percent;
   int taunt_percent;
   int whine_percent;
   int chat_tag_percent;
   int chat_drop_percent;
   int chat_swap_percent;
   int chat_lower_percent;

// TheFatal - START
   int msecnum;
   float msecdel;
   float msecval;
// TheFatal - END

   // things from pev in CBasePlayer...
   int bot_team;
   int bot_class;
   float idle_angle;
   float idle_angle_time;  // for Front Line Force
   int round_end;        // round has ended (in round based games)
   float blinded_time;

   float bot_think_time;

   float f_max_speed;
   float f_prev_speed;
   float f_speed_check_time;
   Vector v_prev_origin;

   float f_find_item;
   edict_t *pBotPickupItem;

   int ladder_dir;
   float f_start_use_ladder_time;
   float f_end_use_ladder_time;
   qboolean  waypoint_top_of_ladder;

   float f_wall_check_time;
   float f_wall_on_right;
   float f_wall_on_left;
   float f_dont_avoid_wall_time;
   float f_look_for_waypoint_time;
   float f_jump_time;
   float f_drop_check_time;

   int wander_dir;
   //int strafe_percent;
   float f_strafe_direction;  // 0 = none, negative = left, positive = right
   float f_strafe_time;
   float f_exit_water_time;

   Vector waypoint_origin;
   float f_waypoint_time;
   int curr_waypoint_index;
   int prev_waypoint_index[5];
   float f_random_waypoint_time;
   int waypoint_goal;
   float f_waypoint_goal_time;
   float prev_waypoint_distance;
   int exclude_points[6];  // five item locations + 1 null

   float f_last_item_found;

   edict_t *pBotEnemy;
   float f_bot_see_enemy_time;
   float f_bot_find_enemy_time;

   int wpt_goal_type;
   float f_evaluate_goal_time;

   Vector v_enemy_previous_origin;
   float f_aim_tracking_time;
   float f_aim_x_angle_delta;
   float f_aim_y_angle_delta;

   edict_t *pBotUser;
   float f_bot_use_time;
   float f_bot_spawn_time;

   edict_t *killer_edict;
   qboolean  b_bot_say;
   float f_bot_say;
   char  bot_say_msg[256];
   float f_bot_chat_time;

   int   enemy_attack_count;
   float f_duck_time;
   int   ducking;
   
   float f_random_jump_time;
   float f_random_jump_duck_time;
   float f_random_jump_duck_end;

   float f_sniper_aim_time;

   float f_prediction_ammount;
   float f_shootcone_maxwidth;
   float f_shootcone_minangle;
   
   int   zooming;

   float f_shoot_time;
   float f_primary_charging;
   float f_secondary_charging;
   int   charging_weapon_id;
   float f_grenade_search_time;
   float f_grenade_found_time;
   int current_weapon_index;
   float current_opt_distance;
   
   float f_move_speed;
   float f_pause_time;
   float f_sound_update_time;

   qboolean  b_see_tripmine;
   qboolean  b_shoot_tripmine;
   Vector v_tripmine;

   qboolean  b_use_health_station;
   float f_use_health_time;
   qboolean  b_use_HEV_station;
   float f_use_HEV_time;

   qboolean  b_use_button;
   float f_use_button_time;
   qboolean  b_lift_moving;

   qboolean  b_use_capture;
   float f_use_capture_time;
   edict_t *pCaptureEdict;

   int   logo_percent;
   qboolean  b_spray_logo;
   float f_spray_logo_time;
   char  logo_name[16];
   int   top_color;
   int   bottom_color;

   float f_start_vote_time;
   qboolean  vote_in_progress;
   float f_vote_time;

   int   reaction_time;
   float f_reaction_target_time;  // time when enemy targeting starts

   bot_current_weapon_t current_weapon;  // one current weapon for each bot
   int m_rgAmmo[MAX_AMMO_SLOTS];  // total ammo amounts (1 array for each bot)

} bot_t;


#define MAX_TEAMS 32
#define MAX_TEAMNAME_LENGTH 16


#define MAX_FLAGS  5

typedef struct {
   edict_t *edict;
   int  team_no;
} FLAG_S;

#define MAX_BACKPACKS 100

typedef struct {
   edict_t *edict;
   int  armor;   // 0=none
   int  health;  // 0=none
   int  ammo;    // 0=none
   int  team;    // 0=all, 1=Blue, 2=Red, 3=Yellow, 4=Green
} BACKPACK_S;


#define MAX_SKINS 200

typedef struct
{
   qboolean skin_used;
   char model_name[32];
   char bot_name[32];
} skin_t;

typedef struct
{
   qboolean can_modify;
   char text[81];
} bot_chat_t;

typedef struct
{
   char identification[4];		// should be WAD2 (or 2DAW) or WAD3 (or 3DAW)
   int  numlumps;
   int  infotableofs;
} wadinfo_t;

typedef struct
{
   int  filepos;
   int  disksize;
   int  size;					// uncompressed
   char type;
   char compression;
   char pad1, pad2;
   char name[16];				// must be null terminated
} lumpinfo_t;


Vector GetPredictedPlayerPosition(edict_t * pPlayer, float time, const float globaltime);
qboolean GetPredictedIsAlive(edict_t * pPlayer, float time);
void GatherPlayerData(void);

qboolean AreTeamMates(edict_t * pOther, edict_t * pEdict);

float UTIL_WrapAngle (float angle_to_wrap);
Vector UTIL_WrapAngles (const Vector & angles_to_wrap);

void RegisterCvars (void);
void ThinkCvars (void);

// new UTIL.CPP functions...
edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector & vecCenter, float flRadius );
edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue );
edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName );
edict_t *UTIL_FindEntityByTarget( edict_t *pentStart, const char *szName );
edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName );

inline Vector GetGunPosition(edict_t *pEdict)
{
   return (pEdict->v.origin + pEdict->v.view_ofs);
}

inline void UTIL_SelectItem(edict_t *pEdict, char *item_name)
{
   FakeClientCommand(pEdict, item_name, NULL, NULL);
}

inline Vector VecBModelOrigin(edict_t *pEdict)
{
   return pEdict->v.absmin + (pEdict->v.size * 0.5);
}

inline Vector UTIL_VecToAngles( const Vector & vec )
{
   Vector VecOut;
   VEC_TO_ANGLES(vec, VecOut);
   return VecOut;
}

inline void WRAP_TraceLine(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr) {
   TRACE_LINE(v1, v2, fNoMonsters, pentToSkip, ptr);
}

// Overloaded to add IGNORE_GLASS
inline void UTIL_TraceLine( const Vector & vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
   WRAP_TraceLine( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr );
}

inline void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
   WRAP_TraceLine( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr );
}

inline void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr );
}

inline void UTIL_TraceMove( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, /*int ducking,*/ edict_t *pentIgnore, TraceResult *ptr ) {
	TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), /*((!ducking) ? head_hull :*/ point_hull/*)*/, pentIgnore, ptr );
}

inline edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius )
{
   edict_t  *pentEntity;

   pentEntity = FIND_ENTITY_IN_SPHERE( pentStart, vecCenter, flRadius);

   if (!fast_FNullEnt(pentEntity))
      return pentEntity;

   return NULL;
}

inline edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue )
{
   edict_t *pentEntity;

   pentEntity = FIND_ENTITY_BY_STRING( pentStart, szKeyword, szValue );

   if (!fast_FNullEnt(pentEntity))
      return pentEntity;
   return NULL;
}

inline edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName )
{
	return UTIL_FindEntityByString( pentStart, "classname", szName );
}

inline edict_t *UTIL_FindEntityByTarget( edict_t *pentStart, const char *szName )
{
	return UTIL_FindEntityByString( pentStart, "target", szName );
}

inline edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName )
{
	return UTIL_FindEntityByString( pentStart, "targetname", szName );
}

inline double deg2rad(double deg) {
	return(deg * (M_PI / 180));
}

void ClientPrint( edict_t *pEdict, int msg_dest, const char *msg_name);
void UTIL_SayText( const char *pText, edict_t *pEdict );
void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message );
char* UTIL_GetTeam(edict_t *pEntity, char teamstr[32]);
int UTIL_GetBotIndex(edict_t *pEdict);
bot_t *UTIL_GetBotPointer(edict_t *pEdict);

//qboolean IsAlive(edict_t *pEdict);
inline qboolean IsAlive(edict_t *pEdict) {
   return ((pEdict->v.deadflag == DEAD_NO) &&
           (pEdict->v.health > 0) &&
           !(pEdict->v.flags & FL_NOTARGET) &&
           (pEdict->v.takedamage != 0));
}

qboolean FHearable(bot_t &pBot, edict_t *pPlayer);
qboolean FInViewCone( const Vector & Origin, edict_t *pEdict);
qboolean FInShootCone( const Vector & Origin, edict_t *pEdict, float distance, float target_radius, float min_angle);
qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict, edict_t ** pHit = 0);
Vector Center(edict_t *pEdict);
Vector GetGunPosition(edict_t *pEdict);
void UTIL_SelectItem(edict_t *pEdict, char *item_name);
void UTIL_SelectWeapon(edict_t *pEdict, int weapon_index);
Vector VecBModelOrigin(edict_t *pEdict);
qboolean UpdateSounds(bot_t &pBot, edict_t *pPlayer);
void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, qboolean needmore, char *pText );
void UTIL_BuildFileName_N(char *filename, int size, char *arg1, char *arg2);
void GetGameDir (char *game_dir);
void UTIL_PrintBotInfo(void(*printfunc)(void *, char*), void * arg);
void UTIL_ServerPrintf( char *fmt, ... );
Vector UTIL_GetOrigin(edict_t *pEdict);

void LoadBotChat(void);
void BotTrimBlanks(char *in_string, char *out_string);
int BotChatTrimTag(char *original_name, char *out_name);
void BotDropCharacter(char *in_string, char *out_string);
void BotSwapCharacter(char *in_string, char *out_string);
void BotChatName(char *original_name, char *out_name);
void BotChatText(char *in_text, char *out_text);
void BotChatFillInName(char *bot_say_msg, char *chat_text, char *chat_name, const char *bot_name);

qboolean GetGoodWeaponCount(bot_t &pBot);
qboolean AllWeaponsRunningOutOfAmmo(bot_t &pBot);

inline int RANDOM_LONG2(int lLow, int lHigh) 
{ 
	if(lLow==lHigh)
		return(lLow);
	
	int tmp[2];
	tmp[0] = RANDOM_LONG(lLow, lHigh);
	tmp[1] = RANDOM_LONG(lLow, lHigh);
	
	return(tmp[(!RANDOM_LONG(0, 2) ? 0 : 1)]);
}

inline float RANDOM_FLOAT2(float flLow, float flHigh) 
{
	if(flLow==flHigh)
		return(flLow);
	
	float tmp[2];
	tmp[0] = RANDOM_FLOAT(flLow, flHigh);
	tmp[1] = RANDOM_FLOAT(flLow, flHigh);
	
	return(tmp[(!RANDOM_LONG(0, 2) ? 0 : 1)]);
}

#endif // BOT_H
