//
// JK_Botti - be more human!
//
// util.h
//

void null_terminate_buffer(char *buf, const size_t maxlen);

double UTIL_GetSecs(void);

float UTIL_WrapAngle(float angle);
Vector UTIL_WrapAngles(const Vector & angles);

Vector UTIL_AnglesToForward(const Vector &angles);
Vector UTIL_AnglesToRight(const Vector &angles);
void UTIL_MakeVectorsPrivate( const Vector &angles, Vector &v_forward, Vector &v_right, Vector &v_up );
Vector UTIL_VecToAngles(const Vector &forward);

int UTIL_GetBotIndex(const edict_t *pEdict);
bot_t *UTIL_GetBotPointer(const edict_t *pEdict);

Vector UTIL_AdjustOriginWithExtent(bot_t &pBot, const Vector & v_target_origin, edict_t *pTarget);

void fast_random_seed(unsigned int seed);
int RANDOM_LONG2(int lLow, int lHigh);
float RANDOM_FLOAT2(float flLow, float flHigh);

void ClientPrint( edict_t *pEdict, int msg_dest, const char *msg_name);
void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message );
char* UTIL_GetTeam(edict_t *pEntity, char *teamstr, size_t slen);

qboolean FInShootCone( const Vector & Origin, edict_t *pEdict, float distance, float target_radius, float min_angle);
qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict, edict_t ** pHit);
qboolean FVisibleEnemy( const Vector &vecOrigin, edict_t *pEdict, edict_t *pEnemy );

void UTIL_BuildFileName_N(char *filename, int size, char *arg1, char *arg2);
void GetGameDir (char *game_dir);

void UTIL_ConsolePrintf( char *fmt, ... );
void UTIL_AssertConsolePrintf(const char *file, const char *str, int line);
char* UTIL_VarArgs2( char * string, size_t strlen, char *format, ... );

void UTIL_DrawBeam(edict_t *pEnemy, const Vector &start, const Vector &end, int width, int noise, int red, int green, int blue, int brightness, int speed);

int UTIL_GetClientCount(void);
int UTIL_GetBotCount(void);

int UTIL_PickRandomBot(void);

breakable_list_t * UTIL_FindBreakable(breakable_list_t * pbreakable);
breakable_list_t * UTIL_LookupBreakable(edict_t *pEdict);
void UTIL_FreeFuncBreakables(void);
void UTIL_UpdateFuncBreakable(edict_t *pEdict, const char * setting, const char * value);

void SaveAliveStatus(edict_t * pPlayer);
float UTIL_GetTimeSinceRespawn(edict_t * pPlayer);

void CheckPlayerChatProtection(edict_t * pPlayer);
qboolean IsPlayerChatProtected(edict_t * pPlayer);

Vector VecBModelOrigin(edict_t *pEdict);

qboolean IsAlive(const edict_t *pEdict);

qboolean FInViewCone(const Vector & Origin, edict_t *pEdict);
