jk_botti - Be More Human
--------------------------------0.30

1. Intro
2. What's new
3. Installing
4. Config, Commands

--------------------
1. Intro
--------------------

This is 0.30 release of jk_botti, by Jussi Kivilinna <jussi.kivilinna@mbnet.fi>
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
See jk_botti.cfg
