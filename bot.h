//
// HPB_bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot.h
//

#ifndef BOT_H
#define BOT_H

// stuff for Win32 vs. Linux builds

#ifdef __linux__
#define Sleep sleep
typedef int BOOL;
#endif


// define a new bit flag for bot identification

#define FL_THIRDPARTYBOT (1 << 27)


// define constants used to identify the MOD we are playing...

#define VALVE_DLL      1
#define TFC_DLL        2
#define CSTRIKE_DLL    3
#define GEARBOX_DLL    4
#define FRONTLINE_DLL  5
#define HOLYWARS_DLL   6
#define DMC_DLL        7


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

// game start messages for TFC...
#define MSG_TFC_IDLE          1
#define MSG_TFC_TEAM_SELECT   2
#define MSG_TFC_CLASS_SELECT  3

// game start messages for CS...
#define MSG_CS_IDLE         1
#define MSG_CS_TEAM_SELECT  2
#define MSG_CS_CT_SELECT    3
#define MSG_CS_T_SELECT     4

// game start messages for OpFor...
#define MSG_OPFOR_IDLE          1
#define MSG_OPFOR_TEAM_SELECT   2
#define MSG_OPFOR_CLASS_SELECT  3

// game start messages for FrontLineForce...
#define MSG_FLF_IDLE            1
#define MSG_FLF_TEAM_SELECT     2
#define MSG_FLF_CLASS_SELECT    3
#define MSG_FLF_PISTOL_SELECT   4
#define MSG_FLF_WEAPON_SELECT   5
#define MSG_FLF_RIFLE_SELECT    6
#define MSG_FLF_SHOTGUN_SELECT  7
#define MSG_FLF_SUBMACHINE_SELECT   8
#define MSG_FLF_HEAVYWEAPONS_SELECT 9


#define TFC_CLASS_CIVILIAN  0
#define TFC_CLASS_SCOUT     1
#define TFC_CLASS_SNIPER    2
#define TFC_CLASS_SOLDIER   3
#define TFC_CLASS_DEMOMAN   4
#define TFC_CLASS_MEDIC     5
#define TFC_CLASS_HWGUY     6
#define TFC_CLASS_PYRO      7
#define TFC_CLASS_SPY       8
#define TFC_CLASS_ENGINEER  9

// halo status for holy wars
#define HW_WAIT_SPAWN  0
#define HW_NEW_SAINT   2


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


typedef struct
{
   int  iId;     // weapon ID
   int  iClip;   // amount of ammo in the clip
   int  iAmmo1;  // amount of ammo in primary reserve
   int  iAmmo2;  // amount of ammo in secondary reserve
} bot_current_weapon_t;


typedef struct
{
   bool is_used;
   int respawn_state;
   edict_t *pEdict;
   bool need_to_initialize;
   char name[BOT_NAME_LEN+1];
   char skin[BOT_SKIN_LEN+1];
   int bot_skill;
   int not_started;
   int start_action;
   float f_kick_time;
   float f_create_time;
   float f_frame_time;

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
   int bot_money;        // for Counter-Strike
   int primary_weapon;   // for Front Line Force
   int secondary_weapon; // for Front Line Force
   int defender;         // for Front Line Force
   int warmup;           // for Front Line Force
   float idle_angle;
   float idle_angle_time;  // for Front Line Force
   int round_end;        // round has ended (in round based games)
   float blinded_time;
   int gren1;            // primary grenade total for TFC
   int gren2;            // secondary grenade total for TFC

   float f_max_speed;
   float f_prev_speed;
   float f_speed_check_time;
   Vector v_prev_origin;

   float f_find_item;
   edict_t *pBotPickupItem;

   int ladder_dir;
   float f_start_use_ladder_time;
   float f_end_use_ladder_time;
   bool  waypoint_top_of_ladder;

   float f_wall_check_time;
   float f_wall_on_right;
   float f_wall_on_left;
   float f_dont_avoid_wall_time;
   float f_look_for_waypoint_time;
   float f_jump_time;
   float f_drop_check_time;

   int wander_dir;
   int strafe_percent;
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
   bool waypoint_near_flag;
   Vector waypoint_flag_origin;
   float prev_waypoint_distance;
   int weapon_points[6];  // five weapon locations + 1 null

   edict_t *pBotEnemy;
   float f_bot_see_enemy_time;
   float f_bot_find_enemy_time;

   Vector v_enemy_previous_origin;
   float f_aim_tracking_time;
   float f_aim_x_angle_delta;
   float f_aim_y_angle_delta;

   edict_t *pBotUser;
   float f_bot_use_time;
   float f_bot_spawn_time;

   edict_t *killer_edict;
   bool  b_bot_say;
   float f_bot_say;
   char  bot_say_msg[256];
   float f_bot_chat_time;

   int   enemy_attack_count;
   float f_duck_time;

   float f_sniper_aim_time;

   float f_shoot_time;
   float f_primary_charging;
   float f_secondary_charging;
   int   charging_weapon_id;
   int   grenade_time;  // min time between grenade throws
   float f_gren_throw_time;
   float f_gren_check_time;
   bool  b_grenade_primed;
   int   grenade_type;  // 0 = primary, 1 = secondary
   float f_grenade_search_time;
   float f_grenade_found_time;
   
   float f_medic_check_time;
   float f_medic_pause_time;
   float f_medic_yell_time;

   float f_move_speed;
   float f_pause_time;
   float f_sound_update_time;
   bool  bot_has_flag;

   bool  b_see_tripmine;
   bool  b_shoot_tripmine;
   Vector v_tripmine;

   bool  b_use_health_station;
   float f_use_health_time;
   bool  b_use_HEV_station;
   float f_use_HEV_time;

   bool  b_use_button;
   float f_use_button_time;
   bool  b_lift_moving;

   bool  b_use_capture;
   float f_use_capture_time;
   edict_t *pCaptureEdict;

   int   logo_percent;
   bool  b_spray_logo;
   float f_spray_logo_time;
   char  logo_name[16];
   int   top_color;
   int   bottom_color;

   float f_engineer_build_time;

   int   sentrygun_waypoint;
   bool  b_build_sentrygun;
   int   sentrygun_level;
   int   sentrygun_attack_count;
   float f_other_sentry_time;
   bool  b_upgrade_sentry;

   int   dispenser_waypoint;
   bool  b_build_dispenser;
   int   dispenser_built;
   int   dispenser_attack_count;

   float f_medic_check_health_time;
   float f_heal_percent;

   float f_start_vote_time;
   bool  vote_in_progress;
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
   bool skin_used;
   char model_name[32];
   char bot_name[32];
} skin_t;

typedef struct
{
   bool can_modify;
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


// new UTIL.CPP functions...
edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius );
edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue );
edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName );
edict_t *UTIL_FindEntityByTarget( edict_t *pentStart, const char *szName );
edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName );
void ClientPrint( edict_t *pEdict, int msg_dest, const char *msg_name);
void UTIL_SayText( const char *pText, edict_t *pEdict );
void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message );
int UTIL_GetTeam(edict_t *pEntity);
int UTIL_GetClass(edict_t *pEntity);
int UTIL_GetBotIndex(edict_t *pEdict);
bot_t *UTIL_GetBotPointer(edict_t *pEdict);
bool IsAlive(edict_t *pEdict);
bool FInViewCone(Vector *pOrigin, edict_t *pEdict);
bool FVisible( const Vector &vecOrigin, edict_t *pEdict );
Vector Center(edict_t *pEdict);
Vector GetGunPosition(edict_t *pEdict);
void UTIL_SelectItem(edict_t *pEdict, char *item_name);
void UTIL_SelectWeapon(edict_t *pEdict, int weapon_index);
Vector VecBModelOrigin(edict_t *pEdict);
bool UpdateSounds(edict_t *pEdict, edict_t *pPlayer);
void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText );
void UTIL_BuildFileName(char *filename, char *arg1, char *arg2);
void GetGameDir (char *game_dir);

void LoadBotChat(void);
void BotTrimBlanks(char *in_string, char *out_string);
int BotChatTrimTag(char *original_name, char *out_name);
void BotDropCharacter(char *in_string, char *out_string);
void BotSwapCharacter(char *in_string, char *out_string);
void BotChatName(char *original_name, char *out_name);
void BotChatText(char *in_text, char *out_text);
void BotChatFillInName(char *bot_say_msg, char *chat_text, char *chat_name, const char *bot_name);

#endif // BOT_H

