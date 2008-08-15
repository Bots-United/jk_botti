//
// JK_Botti - be more human!
//
// bot_func.h
//

#ifndef BOT_FUNC_H
#define BOT_FUNC_H

//prototypes of bot functions...

void BotSpawnInit( bot_t &pBot );
void BotCreate( const char *skin, const char *name, int skill, int top_color, int bottom_color, int cfg_bot_index );
void BotStartGame( bot_t &pBot );
angle_t BotInFieldOfView( bot_t &pBot, const Vector & dest );
qboolean BotEntityIsVisible( bot_t &pBot, const Vector & dest );
void BotPickLogo(bot_t &pBot);
void BotSprayLogo(bot_t &pBot);
void BotFindItem( bot_t &pBot );
qboolean BotLookForMedic( bot_t &pBot );
qboolean BotLookForGrenades( bot_t &pBot );
void BotThink( bot_t &pBot );
void BotPointGun(bot_t &pBot);
void BotAimPre( bot_t &pBot );
void BotAimPost( bot_t &pBot );
void LoadBotModels(void);

void BotDoStrafe(bot_t &pBot);
void BotDoRandomJumpingAndDuckingAndLongJumping(bot_t &pBot, float moved_distance);

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

void BotRemoveEnemy( bot_t &pBot, qboolean b_keep_tracking );
qboolean BotLowHealth( bot_t &pBot );
void BotResetReactionTime(bot_t &pBot, qboolean have_slow_reaction = FALSE);
void BotKick(bot_t &pBot);
void BotCheckTeamplay(void);
void BotFindEnemy( bot_t &pBot );
Vector BotBodyTarget( edict_t *pBotEnemy, bot_t &pBot );
qboolean BotFireWeapon( const Vector & v_enemy, bot_t &pBot, int weapon_choice );
void BotShootAtEnemy( bot_t &pBot );
qboolean BotShootTripmine( bot_t &pBot );

void LoadBotChat(void);
void BotTrimBlanks(const char *in_string, char *out_string, int sizeof_out_string);
int BotChatTrimTag(const char *original_name, char *out_name, int sizeof_out_name);
void BotDropCharacter(const char *in_string, char *out_string, int sizeof_out_string);
void BotSwapCharacter(const char *in_string, char *out_string, int sizeof_out_string);
void BotChatName(const char *original_name, char *out_name, int sizeof_out_name);
void BotChatText(const char *in_text, char *out_text, int sizeof_out_text);
void BotChatFillInName(char *bot_say_msg, int sizeof_msg, const char *chat_text, const char *chat_name, const char *bot_name);

#endif // BOT_FUNC_H

