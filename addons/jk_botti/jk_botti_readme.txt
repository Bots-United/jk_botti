jk_botti 1.50
-------------

1. Intro
2. What's new
3. Installing
4. Config, Commands

--------------------
1. Intro
--------------------

This is 1.50 release of jk_botti, by Jussi Kivilinna <jussi.kivilinna@iki.fi>
You are free to use code for any of your needs.

jk_botti is computer gamer for multiplayer mode of Half-Life (HLDM) and has 
strong support for various submods of HLDM, modifications that change game on 
serverside but doesn't require client modifications.

jk_botti aims to:
 * Lower CPU usage compared to other HLDM bots.
 * Be more interesting opponent than other bots found there
    * More humanlike aiming, hard to keep track on target making movement changes
    * More random combat movement, random jumping, better strafing
    * Hear player footsteps/firing weapons
 * Be dedicated server targeted bot
    * Listenserver client interfaces removed.
    * Automatic waypoints creation.

jk_botti is mostly tested with Severian's, but works with original HLDM and
Bubblemod. As of version 1.40 jk_botti also supports Opposing Force.
On Severian's and Bubblemod, gluon is disabled. Bot might work
with XDM but this hasn't been tested.

One of my major goals with jk_botti was to lower CPU usage compared to
HPB_bot. I used to run Severian's server on 400Mhz Celeron (later updated 
800Mhz Athlon) and even few HPB_bots would rise cpu usage too high.
(Update: I have new server on P3-550Mhz, with default small/medium maps
like datacore/stalkyard with three bots I have see 15% cpu usage).

Trick(s) used to lower CPU usage:
 * bot_think_fps settings for limiting number of thinks per second 
   (default: 30fps)

Aiming system on jk_botti does not use currently available and latest player
locations for tracking and shooting enemy but instead use old data (level 1
bot 60ms, level 5 bot 300ms old) to guess position of player. This makes 
bot-aiming worser when player changes movement vector rapidly (jump, duck,
change strafe direction rapidly). 

jk_botti creates waypoints automatically when 'autowaypoint' setting is
enabled in config. Autowaypointing collects data on map start (weapon/item
locations) and creates new waypoints on those locations. Then jk_botti waits
real player(s) to join server and start moving around. New waypoints are
added to new places where there isn't old waypoints already present. Newly
added waypoints will not be effective until map change. This is because
jk_botti needs to recreate route matrixes with new waypoints and since this is
CPU intensive operation this can only be done on map change.

Credits:
 * Based on HPB bot 4.0 by botman.
 * Uses BotAim code by Pierre-Marie Baty.
 * Uses code from GraveBot by Caleb 'Ghoul' Delnay. (part of goal selection
   system, longjumping)

--------------------
2. What's new
--------------------
1.50:
 New features:
 * Add HL:Arena submod support with 3 new weapons (silenced Glock, auto
   shotgun, burst rifle) and 7 reused OP4 weapons (#77)
 * Add Opposing Force penguin weapon support (snark reskin) (#80)
 * Bots with only a weak weapon (Glock) now avoid combat and seek better
   weapons, but still fight back in self-defense (#79)

 Bug fixes:
 * Fix always-true condition in retreat logic making health/ammo retreat
   checks dead code (#78)
 * Fix BotCanJumpUp not setting duck flag for crouch-jumps, causing bots
   to get stuck on obstacles (#49)
 * Fix BotSecondaryAmmoLow checking wrong ammo index (#47)
 * Fix 3 copy-paste bugs in bot_navigate.cpp: duck-jump side check, wall
   detection timestamps, drop detection condition (#48)
 * Fix BotCanDuckUnder regression from refactoring (#72)
 * Fix WaypointFindRandomGoal always overwriting first result (#44)
 * Fix WaypointFindItem operator precedence bug (#44)
 * Fix SaveSound owner-match bug preventing sound owner tracking (#52)
 * Fix RANDOM_LONG2 off-by-one and fast_random_seed state corruption (#37)
 * Fix safe_strcopy buffer underflow on zero-size destination (#38)
 * Fix undefined behavior from left-shifting signed int (#34)
 * Fix IsPlayerFacingWall stale vector, JKASSERT argument swap,
   UTIL_HostSay buffer overflow, GetGameDir crash (#40)
 * Fix BotChatFillInName crash with no players present (#41)
 * Fix DIR handle leak in LoadBotModels (#64)
 * Fix BotFindItem not picking up weapons bot already owns (#65)
 * Fix AmmoPickup using assignment instead of accumulation (#65)
 * Fix BotAllWeaponsRunningOutOfAmmo being too aggressive (#65)
 * Fix weapon recoil check using wrong boolean operator (#65)
 * Fix MP5 grenade interpolation weights inverted (#65)
 * Fix uninitialized v_player position in BotFindEnemy (#65)
 * Fix sound enemy direction not set for v_newenemy (#65)
 * Fix BotUpdateTrackSoundGoal timeout never triggering (#65)
 * Fix ClientSoundIndex returning bogus out-of-range values (#65)
 * Fix GetEdictChannelSound silently dropping channel 0 sounds (#65)
 * Fix new_PM_PlaySound wrong volume formula (#65)
 * Fix BotDoStrafe ladder condition always-true tautology (#65)
 * Fix BotChangePitch/BotChangeYaw returning pre-turn diff (#65)
 * Fix FVisibleEnemy feet_offset going below bounding box (#65)
 * Fix BotHeadTowardWaypoint identical if/else branches (#65)
 * Fix WaypointFindRunawayPath checking wrong constant (#65)
 * Fix WaypointFindItem parenthesization bug (#65)
 * Fix WaypointReachable water surface detection dead code (#65)
 * Fix BotChatTalk setting cooldown before percent check (#65)
 * Fix BotChatFillInName comparing processed vs raw names (#65)
 * Fix bot_whine_percent/bot_endgame_percent wrong print type (#65)
 * Fix m_rgAmmo out-of-bounds array access (#65)
 * Fix GetSpecificTeam only_count_bots logic inverted (#65)
 * Fix BotPickLogo wrapping at wrong index (#65)
 * Fix BotSwapCharacter missing bounds guard (#65)
 * Fix ScreenFade integer truncation (#65)
 * Fix missing botMsgFunction NULL reset (#65)
 * Fix get_crossover_interleave off-by-one (#65)

 Improvements:
 * Improve PRNG quality: use rotates, better LCG constants (#37)
 * Add jk_botti_mm.so symlink to release directory (#46)

 Refactoring:
 * Refactor commands.cpp: replace macros with helper functions,
   extract sub-functions (#66)
 * Break up large functions across 8 source files into smaller
   sub-functions: bot.cpp, bot_combat.cpp, bot_navigate.cpp,
   bot_weapons.cpp, bot_chat.cpp, dll.cpp, waypoint.cpp (#67-#76)

 Internal:
 * Add comprehensive unit test suite with 95% line coverage and
   97% function coverage (#41, #44, #47, #50-#64, #68)
 * Add gcov coverage collection for unit tests (#54)
 * Separate linux and win32 build output directories (#45)
 * Require tests to pass before creating release artifact (#39)
 * Clean up whitespace, includes, and dead code (#36, #38, #42)
 * Improve neuralnet/geneticalg experimental code (#35)

1.45:
 New features:
 * Bots can now use satchel charges (issue #15)
 * New bot_shoot_breakables cvar to control breakable targeting (issue #19)
   * 0 = disabled, 1 = shoot all breakables, 2 = shoot only path-blocking
     breakables (default)

 Bug fixes:
 * Fix assertion fail when botdontshoot is enabled (issue #4)
 * Fix posdata linked list cycle causing NULL dereference (issue #10)
 * Fix SSE stack alignment crash on i686
 * Fix infinite loop and weapon selection bugs (jonatan1024, GoldSrc-one fork)
 * Fix OP4 grapple detection (anzz1)
 * Fix min_bots logic (anzz1)
 * Avoid calling BotChatTaunt when the bot kills self (josemam)
 * Fix WaypointFindRunawayPath always returning -1 (copy-paste bug)

 Improvements:
 * Improve weapon selection: allow Glock, fix avoided weapon reuse
   (jonatan1024, GoldSrc-one fork)
 * Improve melee weapon movement behavior (jonatan1024, GoldSrc-one fork)
 * Make grapple the least favorite weapon in OP4 (anzz1)
 * Add null safety checks to engine callback functions

 Security and stability:
 * Fix stack buffer overflow in config file parsing
 * Fix unchecked memory allocations across multiple subsystems
 * Add bounds validation for waypoint file loading
 * Fix null dereference in sound list traversal
 * Fix out-of-bounds reads in bot creation and combat code
 * Cap network-supplied alloca sizes in query hook handlers
 * Fix waypoint exclude list unable to exclude waypoint index 0

 Internal:
 * Update bundled zlib from 1.2.3 to 1.3.2
 * Derive version from git tags instead of hardcoded version.h
 * Keep debug symbols in release builds
 * Add unit test suite
 * Add GitHub Actions CI (artkirienko)
 * Update project URLs to GitHub

1.44β:
 * Fix rare crash when bot tries to shoot tripmine with MP5

1.43:
 * Bots do not attack breakable objects or monsters with huge amount
   of health
 * Support modifying new HL server query format

1.42:
 * [bugfix, linux] Fixed jk_botti to be linked with libm.so
 * Tweaked low level bot skills alot
    * They will be more active now (run around maps) now
    * Not too good at combat
 * Bot reaction times have been adjusted to more human scale (level 1 
   has 100ms and level 5 300ms). Times used to be less before.
 * Removal of bot_reaction_time setting.
   * You can still use bot_skill settings to adjust reaction times.
 * Internal changes
   * Compiled with newer version of gcc
   * Made large portion of inline functions uninlined as on modern 
     cpus/system small cache/code size is better than inlining 
     everything manually.

1.41:
 * Fixed low level bot aiming (were rapidly shifting right to left)
   * As result removed undocumented and broken setting 'aimangle_varitation'
 * Add skill setting updown_turn_ration, setting to make bot aim up-down worse
   than right-left.
 * Add bot endgame chatting.
   * New setting 'bot_endgame_percent', see jk_botti.cfg
   * New section [bot_endgame] in jk_botti_chat.txt
 * Bot 'write' long chat message for longer time now than short message.
 * Binaries now work on AMD K6 series CPUs.

1.40:
 * Add support for Opposing Force Deathmatch.
 * Tuned skill 1 bot to be more leet, skill 2 is now about same as old skill 1.
 * New sound system and sound tracking system, huge improvement in how bot
   finds enemy that isn't visible.
 * Bugfixes to goal selection and weapon selection, results huge improvement
   how bot work on when not given good weapons on spawn (which is typical on
   Severian's MOD). 
 * Bot now understands that it can get more ammo by picking up same weapon 
   again.
 * Bot avoids combat if it doesn't have good weapon or doesn't have enough
   health.
 * Output warning message when model given for bot creation is replaced by 
   team-balance code.
 * Changed to only check existance of player model file on listenserver 
   when creating bots.
 * Changed save order of waypoint .wpt and .matrix files so that matrix 
   doesn't have to be calculated one extra time.
 * Fixed instability problems: Replaced most of dynamic memory management 
   with static memory.
 * Fixed buggy creation of crouch waypoint over drop (it's recommended to
   replace server datacore.wpt with stock wpt since this bug hit most hard
   on datacore on one specific high traffic area).

1.30:
 * Fixed Severians MOD detection with teamplay.
 * Fixed bug with jk_botti only using first 3 letters of player/bot team name.
 * Fixed bots to get affected by weapon recoil. Somewhere along hldm releases
   recoil code was moved completely to client.dll, so now jk_botti will
   emulate client.dll function. Difference to MP5 aiming is most notable.
 * Removed hack to make bot aim worser than it should when using MP5, this 
   isn't needed anymore.
 * Added team autobalance for teamplay servers. Requirements: mp_teamplay is
   set to 1 and mp_teamlist is set with more than one team.
   * New commands 'team_balancetype' and 'team_blockedlist'. See jk_botti.cfg
     for details.
 * Disabled 'bot_conntimes 1' on listenserver.
 * NOTE: listenserver support might be completely dropped in 1.40 unless
   someone confirms that jk_botti actually works correctly on listenserver.
   Dropping support means that loading jk_botti on listenserver will be
   blocked with some error message.

1.24:
 * Rewrote min/max_bots code.
 * Increased config-file processing and bot adding speed.
 * Changed bot kicking to use 'kick # <userid>' instead of 'kick <name>'.
 * Fixed broken random bot selection code.

1.20:
 * Added support for lifts.
 * Fixed weapon selection code to honor weapon secondary/primary attack skill
   setting.
 * Fixed bot get up ladders better.
 * Fixed min_bots/max_bots to handle other metamod bots correctly.
 * More aggressive autowaypointing for linking separate areas.
 * Fixed autowaypointing for paths leading from water to dry.
 * Added missing ammunition type 'ammo_9mmbox'.
 * Fixed goal selection to use wall mounted health/battery rechargers.
 * Compiled with more aggressive optimization flags. This speeds up 
   path-matrix creation.
 * Fixed output for 'kickall'-command.
 * Fixed bot not to jump when coming near edge and bot is planing to go down 
   using ladders.
 * Workaround for loading map specific config for map 'logo'. 
   Use '_jk_botti_logo.cfg' for this map. 'jk_botti_logo.cfg' is already used 
   for bot spraypaints.
 * Fixed bot always ducking when next waypoint is crouch waypoint. This 
   caused bot not be able to jump through small window with crouch waypoint 
   inside.

1.10:
 * Fixed bots to use wall mounted health/battery rechargers and buttons.
 * Fixed bots to pick up items.
 * Fixed observer mode so that bots don't hear observers anymore. This caused
   bots to get interested about observer sounds and track theim.
 * Lowered default bot lookaround/pause frequency and times.
 * Fixed autowaypointing not to place waypoints midair.
 * Fixed autowaypointing not to create impossible upwards paths.
 * Fixed autowaypointing not to create crouch waypoints if there is room to 
   stand up.
 * Old waypoint files are automatically processed to fix above 
   autowaypointing errors.
 * Lots of tweaks to autowaypointing: better handling of ladders and stairs, 
   better linking of isolated areas.
 * 'autowaypoint' is now default on.
 * New command 'show_waypoints' for viewing/aiding waypoint creation.

1.01: 
 * Fixed 'bot_conntimes 1' crashing on Windows servers.

1.00:
 * Remembers bots from config when recreating bots by min_bots/max_bots.
 * Change bot connection times on server queries.
   - Set 'bot_conntimes 1' in config file to have different connection times
     for each bot on server query.
 * Tweaked code computing RunPlayerMove-msec.
 * Bots don't see targets too early behind corners.

0.57:
 * Waypoint files are now compressed with zlib.

0.56:
 * Fixed waypoint matrix creation being done right on map start on win32.
 * Added command 'jk_botti kickall' for clearing server from all bots.
 * Added waypoint files for default valve maps.

0.54:
 * Fixed the way bots react to sounds. Instead of making sound enemy, bot 
   finds it's way to the interesting sound.
 * New way of computing msec for RunPlayerMove.
 * Bots don't attack respawn players too fast (delay is skill depend).
 * New skill settings: pause_time_min, pause_time_max, react_delay_min[0-2],
   react_delay_max[0-2], weaponchange_rate_min, weaponchange_rate_max.

0.53:
 * Tweaked bot behavior now with fixed reaction times. Fixed bots see better.

0.52:
 * Config-file errors are now print to server console with config line number.
 * Fixed bug with bot reaction times not reseting when waypoints active.
 * Fixed 'botweapon reset' not working.
 * Fixed unknown command error output for 'jk_botti' server command.

0.50:
 * Config file is now reloaded on EVERY map change.
 * Fixed bot strafing, and other minor changes to movement.
 * Fixed xbow shooting.
 * Remake of sound/bot-hear system, based on hlsdk/CSoundEnt. Bot skill
   setting 'hear_frequency' replaced by 'hearing_sensitivity'.
 * Bots attack func_breakables and func_pushables.
 * Bots don't attack players facing wall (chatting/writing). Player
   must face wall 2 seconds to protection to activate.
 * Bots ignore snarks/grenades/ect when fighting.
 * Tweaked down MP5 and crossbow aiming.
 * New setting to tweak down aim skill for specific weapons (aim_speed).
 * Bot chat is logged to server logs now.
 * Fixed weapon selection bug (from HPB). Bot randomized weapons in 
   for loop causing weapons last in list to be used a lot less than
   weapon first in the list.
 * Added separate frame timer for aiming.
 * Multiple bots don't get same name anymore when using [lvlX] tags.

0.40:
 * Bots now use crossbow (with zoom).
 * Bots now know how to fire MP5 grenades at right angle depending target's
   height and distance.
 * Waypoint matrix is now calculated over time on first minutes after map 
   change with max 10ms time slices.
 * Waypoint matrix is saved at mapchange or server shutdown.
 * Bot skill can now be edited through jk_botti.cfg or 'jk_botti' server
   command.
 * Minor improvements and fixes.

0.31:
 * Improvements to aiming and combat movements

0.30:
 * HUGE amount of bug fixes, most aim related.
 * Better readme.
 * BubbleMod gluon is not disabled anymore if bm_gluon_mod cvar set 0.
 * Weapon/Ammo codes saved to waypoints for picking up specific ammo/weapon.
 * Bots will longjump now (code from GraveBot)

0.20
 * First release

--------------------
3. Installing
--------------------
jk_botti is Metamod plugin, so I except you have already installed Metamod
successfully. If not read: 
   https://github.com/Bots-United/jk_botti

Binary release file comes with correct directory structure:
addons/jk_botti/dlls/* - binaries
addons/jk_botti/waypoints/* - empty directory created for waypoint files.
addons/jk_botti/* - config files, readme files

1. If this is NOT first install, backup your old jk_botti config files.
2. Extract release file to your 'valve' directory. 
3. Edit 'addons/metamod/plugins.ini' and add like 'win32 addons/jk_botti/dlls/jk_botti_mm.dll'
4. If this is your first install edit 'addons/jk_botti/jk_botti.cfg', otherwise restore your backup config
5. Start server and enjoy.

--------------------
4. Config, Commands
--------------------
For commands 'bot_skill_setup' and 'botweapon' see below.
For other commands see jk_botti.cfg

Command - set_team

Usage:
 - set_team <teamname> "model1;model2;..."
      Map player models to a team name. Bots will treat all players using
      any of the listed models as members of the same team. Models not
      listed in any set_team command use the default HL1 behavior (model
      name = team name).

 - set_team clear
      Remove all custom team model mappings.

 - set_team
      Show number of active model mappings.

Example:
 In jk_botti.cfg:
   set_team team1 "gign;sas;gsg9"
   set_team team2 "arctic;terror;leet"

 With this config, a bot using the "gign" model will recognize players
 using "sas" or "gsg9" models as teammates, and treat "arctic", "terror",
 and "leet" players as enemies. Players using models not listed (e.g.
 "scientist") are treated as their own team.

 This is useful for custom mods that assign multiple player
 models to the same team.

Command - bot_shoot_breakables

Usage:
 - bot_shoot_breakables <value>

Controls whether bots shoot func_breakable and func_pushable entities.

Values:
 0 - Never shoot breakables.
 1 - Always shoot visible breakables.
 2 - Only shoot breakables that block the bot's movement path. (default)
     Bot traces forward in its facing direction and only targets
     breakables in the way. Decorative breakables (ceiling lights,
     side windows) are ignored.

Command - bot_skill_setup

Usage:
 - bot_skill_setup <skill> <setting> <value>
      Set setting value for skill.

 - bot_skill_setup <skill> <setting>
      Shows setting value for skill.

 - bot_skill_setup <skill>
      Shows all setting values for skill.
 
 - bot_skill_setup reset
      Reset all skill settings to default.

List of available settings:

 - pause_frequency
      How often out of 1000 times the bot will pause.
      Value: 0-1000

 - pause_time_min
   pause_time_max
      Set range of time bot stays paused. Exact time is randomized value
      between these two. If min is larger than max, then min is used always.
      Value: 0.0-10.0
 
 - normal_strafe
      How much bot strafes when walking around. This value is percent.
      Value: 0-100
 
 - battle_strafe
      How much bot strafes when attacking enemy. This value is percent.
      Value: 0-100
 
 - react_delay_min
   react_delay_max
      Set range of bot reaction time. Exact time is randomized value between
      these two. If min is larger than max, then min is used always.
      Value: 0.0-1.0
 
 - weaponchange_rate_min
   weaponchange_rate_max
      Set range of time bot tries to keep currect weapon. Exact time is 
      randomized value between these two. If min is larger than max, then min 
      is used always.
      Value: 0.0-10.0
      
 - keep_optimal_distance
      How often bot out of 1000 times the bot tries to keep at optimum distance
      of weapon when attacking.
      Value: 0-999
 
 - shootcone_diameter
      Bot guesses if it's ok to fire target by thinking of virtual cone which 
      tip is on bot's weapon and base of diameter set here on target position. 
      When bot is aiming inside base area it will
      think that target can be shot.
      Value: 100~400
      
 - shootcone_minangle
      If above condition doesn't apply, bot will check if angle between bot aim
      and line to target is less than angle set here.
      Value: 0-180 (0 = no angle, 180 = shoot at everything even if behind us)
      
 - turn_skill
      BotAim turn_skill, how good bot is at aiming on enemy.
      Value: 0.5-4 (0.5 = bad, 4 = very fast)

 - updown_turn_ration
      How much slower bots aims up and down than side ways?
      Value: 1-10 (1 = bot aims up/downwards as fast as sideways, 10 = bot aim 
                   almost level, default for level 1 bot = 2.0, level 5 = 3.0)

 - hearing_sensitivity
      How far away bot will hear. 0.0 hear nothing, 1.5 used for skill 1.
      Value: 0.0-1.5
 
 - track_sound_time_min
 - track_sound_time_max
      How long time in seconds bot tries to track one sound?
 
 - can_longjump
      Can bot at this skill level use longjump.
      Value: 0/1 (0 = no, 1 = yes)
 
 - random_jump_frequency
      How often out of 100 times the bot will do random jump.
      Value: 0-100
      
 - random_jump_duck_frequency
      How often out of 100 times the bot will do random duck when random 
      jumping (midair). This is subsetting for 'random_jump_frequency'.
      Value: 0-100
 
 - random_duck_frequency
      How often (out of 100 times) the bot will do random duck.
      Value: 0-100
      
 - random_longjump_frequency
      How often out of 100 times the bot will do random longjump.
      Value: 0-100

Command - botweapon 

Usage:
 - botweapon <weapon-name> <setting> <value>
      Set setting value for weapon.

 - botweapon <weapon-name> <setting>
      Shows setting value for weapon.

 - botweapon <weapon-name>
      Shows all setting values for weapon.
      
 - botweapon reset
      Reset all weapon settings to default.

List of weapon-namess:

  weapon_crowbar
  weapon_handgrenade
  weapon_snark
  weapon_egon
  weapon_gauss
  weapon_shotgun
  weapon_357
  weapon_hornetgun
  weapon_9mmAR
  weapon_crossbow
  weapon_rpg
  weapon_9mmhandgun

List of available settings:
 
 - primary_skill_level
      Bot skill must be less than or equal to this value for bot to use 
      primary attack.
      Value: 0-5 (0 = disabled, 1 highest level, 5 lowest)
   
 - secondary_skill_level
      Bot skill must be less than or equal to this value for bot to use 
      secondary attack.
      Value: 0-5 (0 = disabled, 1 highest level, 5 lowest)

 - aim_speed
      Aim speed that will be used for this gun. This is relative to bot's
      aim_skill. 1.0 = use bot's highest turn skill, 0.0 = lowest.
      Value: 0.0-1.0

 - avoid_this_gun
      Bot avoids using this weapon if possible.
      Value: 0/1 (0 = bot doesn't avoid weapon, 1 avoid)
 
 - prefer_higher_skill_attack
      Bot uses higher skill attack of primary and secondary attacks if both 
      available.
      Value: 0/1 (0 = use both attacks, 1 only use better one)
 
 - primary_min_distance
      Minimum distance for using primary attack of this weapon.
      Value: 0-9999 (0 = no minimum)
 
 - primary_max_distance
      Maximum distance for using primary attack of this weapon.
      Value: 0-9999 (9999 = no maximum)
      
 - secondary_min_distance
      Minimum distance for using secondary attack of this weapon.
      Value: 0-9999 (0 = no minimum)
   
 - secondary_max_distance
      Maximum distance for secondary attack of using this weapon.
      Value: 0-9999 (9999 = no maximum)
   
 - opt_distance
      Optimal distance from target when using this weapon.
      Value: 0-9999
 
 - use_percent
      Times out of 100 to use this weapon when available.
      Value: 0-100
 
 - can_use_underwater
      Can use this weapon underwater.
      Value: 0/1 (0 = no, 1 = yes)
 
 - primary_fire_percent
      Times out of 100 to use primary fire when both attacks are available 
      to use.
      Value: 0-100
         
 - low_ammo_primary
      Ammo-level at which bot thinks weapon is running out of primary ammo.
      Value: 1-255
   
 - low_ammo_secondary
      Ammo-level at which bot thinks weapon is running out of secondary ammo.
      Value: 1-255
