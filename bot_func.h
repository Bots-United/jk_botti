//
// JK_Botti - be more human!
//
// bot_func.h
//

#ifndef BOT_FUNC_H
#define BOT_FUNC_H


#define AIM_RACC_OLD 1
#define AIM_RACC 2

//prototypes of bot functions...

void BotSpawnInit( bot_t &pBot );
void BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2,
                const char *arg3, const char *arg4, const char *arg5 );
void BotStartGame( bot_t &pBot );
int BotInFieldOfView( bot_t &pBot, const Vector & dest );
qboolean BotEntityIsVisible( bot_t &pBot, const Vector & dest );
void BotPickLogo(bot_t &pBot);
void BotSprayLogo(edict_t *pEntity, char *logo_name);
void BotFindItem( bot_t &pBot );
qboolean BotLookForMedic( bot_t &pBot );
qboolean BotLookForGrenades( bot_t &pBot );
void BotThink( bot_t &pBot );
void BotPointGun(bot_t &pBot);
void BotAimPre( bot_t &pBot );
void BotAimPost( bot_t &pBot );
void LoadBotModels(void);

float BotChangePitch( bot_t &pBot, float speed );
float BotChangeYaw( bot_t &pBot, float speed );
qboolean BotFindWaypoint( bot_t &pBot );
qboolean BotHeadTowardWaypoint( bot_t &pBot );
void BotOnLadder( bot_t &pBot, float moved_distance );
void BotUnderWater( bot_t &pBot );
void BotUseLift( bot_t &pBot, float moved_distance );
qboolean BotStuckInCorner( bot_t &pBot );
void BotTurnAtWall( bot_t &pBot, TraceResult *tr, qboolean negative );
qboolean BotCantMoveForward( bot_t &pBot, TraceResult *tr );
qboolean BotCanJumpUp( bot_t &pBot, qboolean *bDuckJump );
qboolean BotCanDuckUnder( bot_t &pBot );
void BotRandomTurn( bot_t &pBot );
qboolean BotFollowUser( bot_t &pBot );
qboolean BotCheckWallOnLeft( bot_t &pBot );
qboolean BotCheckWallOnRight( bot_t &pBot );
void BotLookForDrop( bot_t &pBot );

void BotCheckTeamplay(void);
edict_t *BotFindEnemy( bot_t &pBot );
Vector BotBodyTarget( edict_t *pBotEnemy, bot_t &pBot );
qboolean BotFireWeapon( const Vector & v_enemy, bot_t &pBot, int weapon_choice );
void BotShootAtEnemy( bot_t &pBot );
qboolean BotShootTripmine( bot_t &pBot );

void free_posdata_list(int idx);

#endif // BOT_FUNC_H

