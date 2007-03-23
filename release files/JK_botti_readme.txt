JK_botti - Be More Human
--------------------------------version 0.20

This is 0.20 release of JK_botti, by Jussi Kivilinna <jussi.kivilinna@mbnet.fi>
You are free to use code for any of your needs.

Credits:
 * Based on HPB bot 4.0 by botman.
 * Uses BotAim code by Pierre-Marie Baty.
 * Uses code from GraveBot by Caleb 'Ghoul' Delnay. (part of goal selection system)

This bot aims to:
 * Lower CPU usage compared to HPB_bot.
 * Be more interesting opponent than other bots found there (try it)
 
Notes:
 * (Auto)waypoints are saved to "addons/jk_botti/waypoints" directory. This directory
   must exists, otherwise waypoint-files WILL NOT BE SAVED!
 * Config command list can be found in jk_botti.cfg
 * Only command missing from jk_botti.cfg listing is 'botweapon':
   - On server console "jk_botti botweapon list" lists all botweapon options available.
   - Config-file example: "botweapon weapon_gauss use_percent 0"
     + this will disable Tau/Gauss
   - Config-file example: "botweapon weapon_gauss 