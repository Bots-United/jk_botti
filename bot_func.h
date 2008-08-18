//
// JK_Botti - be more human!
//
// bot_func.h
//

#ifndef BOT_FUNC_H
#define BOT_FUNC_H

//prototypes of bot functions...

void BotCreate( const char *skin, const char *name, int skill, int top_color, int bottom_color, int cfg_bot_index );
void BotThink( bot_t &pBot );

void BotPointGun(bot_t &pBot);
void BotAimPre( bot_t &pBot );
void BotAimPost( bot_t &pBot );
void LoadBotModels(void);

void LoadBotChat(void);
void BotChatTaunt(bot_t &pBot, edict_t *victim_edict);
void BotChatWhine(bot_t &pBot);
void BotChatTalk(bot_t &pBot);
void BotChatEndGame(bot_t &pBot);

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
qboolean BotCheckWallOnBack( bot_t &pBot );
qboolean BotCheckWallOnForward( bot_t &pBot );
void BotLookForDrop( bot_t &pBot );
qboolean BotEdgeForward( bot_t &pBot, const Vector &v_move_dir );
qboolean BotEdgeRight( bot_t &pBot, const Vector &v_move_dir );
qboolean BotEdgeLeft( bot_t &pBot, const Vector &v_move_dir );

qboolean BotUpdateTrackSoundGoal( bot_t &pBot );
int BotGetSoundWaypoint( bot_t &pBot, edict_t *pTrackSoundEdict, edict_t ** pNewTrackSoundEdict );
void BotUpdateHearingSensitivity(bot_t &pBot);

void BotRemoveEnemy( bot_t &pBot, qboolean b_keep_tracking );
void BotResetReactionTime(bot_t &pBot, qboolean have_slow_reaction = FALSE);
void BotKick(bot_t &pBot);
void BotCheckTeamplay(void);
void BotFindEnemy( bot_t &pBot );
Vector BotBodyTarget( edict_t *pBotEnemy, bot_t &pBot );
qboolean BotFireWeapon( const Vector & v_enemy, bot_t &pBot, int weapon_choice );
void BotShootAtEnemy( bot_t &pBot );
qboolean BotShootTripmine( bot_t &pBot );

#endif // BOT_FUNC_H

