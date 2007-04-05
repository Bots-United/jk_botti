jk_botti - Be More Human
--------------------------------0.40

1. Intro
2. What's new
3. Installing
4. Config, Commands

--------------------
1. Intro
--------------------

This is 0.40 release of jk_botti, by Jussi Kivilinna <jussi.kivilinna@mbnet.fi>
You are free to use code for any of your needs.

Credits:
 * Based on HPB bot 4.0 by botman.
 * Uses BotAim code by Pierre-Marie Baty.
 * Uses code from GraveBot by Caleb 'Ghoul' Delnay. (part of goal selection
   system, longjumping)

This bot aims to:
 * Lower CPU usage compared to HPB_bot.
 * Be more interesting opponent than other bots found there (try it)
    * More humanlike aiming, hard to keep track on target making movement
      changes
    * More random combat movement, random jumping, better straifing
    * Hear player footsteps/firing weapons
 * Be dedicated server targeted bot
    * Listenserver client interfaces removed.
    * Automatic waypoints creation.

jk_botti is mostly tested with Severian's, but works with original HLDM and
Bubblemod. On Severian's and Bubblemod, gluon is disabled. Bot might work
with XDM but this isn't tested.

One of my major goals with jk_botti was to lower CPU usage compared to
HPB_bot. I used to run Severian's server on 400Mhz Celeron (later updated 
800Mhz Athlon) and even few HPB_bots would rise cpu usage too high.

Tricks used to lower CPU usage:
 * Bots think only every second frame (hardcoded limit)
 * bot_think_fps settings for limiting number of thinks per second 
   (default: 30fps)

Aiming system on jk_botti does not use currently available and latest player
locations for tracking and shooting enemy but instead use old data (level 1
bot 100ms, level 5 bot 500ms old) to guess position of player. This makes 
aiming worser when player changes movement vector rapidly (jump, duck,
change strafe direction rapidly). 

jk_botti creates waypoints automatically when 'autowaypoint' setting is
enabled in config. Autowaypointing collects data on map start (weapon/item
locations) and creates new waypoints on those locations. Then jk_botti waits
real player(s) to join server and start moving around. New waypoints are
added to new places where there isn't old waypoints already present. Newly
added waypoints will not be effective until map change. This is because
jk_botti needs to recreate route matrixes with new waypoints and since this is
CPU intensive operation this can only really be done on map change.

--------------------
2. What's new
--------------------
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

List of available settings:

 - pause_frequency
      How often out of 1000 times the bot will pause.
      Value: 0-1000
 
 - normal_strafe
      How much bot strafes when walking around. This value is percent.
      Value: 0-100
 
 - battle_strafe
      How much bot strafes when attacking enemy. This value is percent.
      Value: 0-100
 
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
      Value: 0.5-10 (0.5 = bad, 5 = good, 10 = god)

 - hear_frequency
      How often out of 100 times the bot will hear what happens around it.
      Value: 0-100
      
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
