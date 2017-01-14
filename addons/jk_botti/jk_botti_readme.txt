jk_botti 1.44β
-------------

1. Intro
2. What's new
3. Installing
4. Config, Commands

--------------------
1. Intro
--------------------

This is 1.44β release of jk_botti, by Jussi Kivilinna <jussi.kivilinna@iki.fi>
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
   http://wiki.bots-united.com/index.php/How_to_install_metamod
   http://wiki.bots-united.com/index.php/How_to_install_a_metamod_bot_%28or_any_metamod_plugin%29

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
