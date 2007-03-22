HPB bot - Version 2.1 (April 28th, 2002)

The HPB bot currently works with Half-Life (deathmatch), Opposing Force
(deathmatch and CTF), Team Fortress 1.5, Counter-Strike (Version 1.4),
Front Line Force (Version 1.5), Holy Wars (Beta 2) and Valve's Deathmatch
Classic.  Half-Life version 1.1.0.9 or higher is required to use this bot.
If you haven't updated your Half-Life version to 1.1.0.9 (or higher) you
will HAVE to do so before using the HPB bot.


How to install the HPB bot:

Each MOD supported by the HPB bot has it's own folder within the HPB bot
folder.  In each of these folders there are 2 MS-DOS batch files.  One is
called "Install" and one is called "Remove".  Browse to the folder that
corresponds to the MOD you wish to install the HPB bot for (i.e. use the
tfc folder if you want to install the HPB bot for TFC).  Double click on
the "Install" batch file in this folder to install the HPB bot.  If you
wish to uninstall the HPB bot, double click on the "Remove" batch file.

Below is a list of the folders and the MOD that goes along with them...

cstrike   - Counter-Strike
dmc       - Deathmatch Classic
frontline - Front Line Force
gearbox   - Gearbox's Opposing Force
holywars  - Holy Wars
tfc       - Team Fortress 1.5
valve     - Half-Life deathmatch


How to uninstall the HPB bot:

Use Windows explorer to browse to the MOD folder where you clicked on the
"Install" batch file and double-click on the "Remove" batch file.


How to start the game:

For Half-Life deathmatch, just select "Multiplayer", then "LAN game", then
"Create game".  Select the map you wish to play and click on "OK" to start
the game.

For Opposing Force, Team Fortress 1.5, Counter-Strike, Front Line Force,
Holy Wars, or Deathmatch Classic, click on "Custom game", then click on
"Opposing Force", "Team Fortress", "Counter-Strike", "Front Line Force",
"Holy Wars", or "Deathmatch Classic" then click on "Activate".  Click on
"Done", then click on "Multiplayer".  Click on "LAN game", then click on
"Create game".  Click on the map name that you wish to play then click on
"OK" to start the game.

Bots will automatically be added to the game using the "HPB_bot.cfg" file
found in the directory of the game you are playing.  Half-Life is in the
"valve" directory.  Opposing Force is in the "gearbox" directory.  Team
Fortress 1.5 is in the "tfc" directory, Counter-Strike is in the "cstrike"
directory, Front Line Force is in the "frontline" directory, Holy Wars is
in the "holywars" directory and Deathmatch Classic is in the "dmc" directory.
If you wish to change the number of bots that are created automatically you
can edit the HPB_bot.cfg file using any text editor (like Notepad).  There
are instructions in the bot.cfg file each MOD showing what the valid
parameters are.


How to change the skill level of the bots:

Within each MOD folder in the Half-Life folder you will find a "HPB_bot.cfg"
file.  This is a text file that can be edited with Notepad or any other text
editor.  Open the HPB_bot.cfg file in the MOD folder that corresponds to the
MOD you wish to play (for example the Half-Life HPB_bot.cfg file is in
Half-Life\valve).  Find the line that says "botskill 2".  Change the 2 to a
higher or lower skill level setting.  Values from 1 to 5 are allowed (where
1 is the best skilled and the hardest to play against and 5 is the worst
skilled and easiest to play against).  Save the change to the HPB_bot.cfg
file and start the game as shown above.


How to change the default names of the bots:

The names come from a text file called "HPB_bot_names.txt".  You will find
this file in the MOD folder (contained within the Half-Life folder)
corresponding to the MOD you wish to play.  For example the HPB_bot_names.txt
file for TFC is found in Half-Life\tfc.  Edit this file with any text editor
(like Notepad) and add or delete names.  The bot names can be up to 32
characters long and you can have a maximum of 100 names total.  Each name
should be on a separate line with no leading spaces.


How to create map specific HPB_bot.cfg files:

You can create a HPB_bot.cfg file that has specific commands for each map by
creating a text file in the "maps" directory.  The file name should begin with
the same name as the map followed by "_HPB_bot.cfg" (for example
maps\2fort_HPB_bot.cfg or maps\stalkyard_HPB_bot.cfg).  The commands contained
in the map specific HPB_bot.cfg file are the same as the ones found in the
default HPB_bot.cfg file (the one found in the MOD directory).  If a map
specific bot cfg file does not exist then bots will be respawned based on how
many bots were in the previous level.  You can specify the team and class of
the bots for each map using these map specific HPB_bot.cfg files (by
specifying them in the "addbot" command).  When the server is started, if a
map specific config file exists for the first map then it will be used to
spawn the bots.  If a map specific HPB_bot.cfg file does not exist for the
first map then the HPB_bot.cfg file (in the MOD directory) will be used to
spawn the bots.  It is important to note that the HPB_bot.cfg file found in
the MOD directory will only be executed ONCE (on the first map if no map
specific HPB_bot.cfg file exists).  In other words, the HPB_bot.cfg file is
only executed once for the first map, from then on only map specific
HPB_bot.cfg files will be executed.


How to create custom spray logos for the bots:

Use Windows Explorer to browse to the Half-Life\HPB_bot folder.  Inside this
folder you will find an application called "bot_logo".  Double click on this
application to run it.  It will ask you which MOD you wish to use to create
custom spray logos for.  Then you will be allowed to add or remove bitmaps
from the decals.wad file used by the bots to spray logos.  Please read the
"Bot_logo_ReadMe.txt" file in the Half-Life\HPB_bot folder for more details.

The bots will RANDOMLY choose a logo from the list that you have created when
they are added to the game.  You can not force a bot to use a specific logo
unless that is the only logo that you have added to the decals.wad file.


Common questions:

Q: Will I be able to play a MOD on the Internet or on a LAN with the HPB bot
   installed?
A: Yes!  You can join a network game over the Internet or on a LAN without
   having to uninstall the HPB bot.  You cannot spawn bots when connected to
   an Internet server, but you can spawn bots on a LAN if you are running the
   server.

Q: Will I be able to play single player games with the HPB bot installed?
A: Yes, the HPB bot will not interfere with single player games.

Q: When will you be releasing the next version of the HPB bot?
A: I don't know yet and I am not going to provide a release date.  When
   another version is ready, I will post news on my main page and I will
   send e-mail to the various Half-Life news sites to let them know that
   something new is available.

Q: When are you going to provide <insert_your_favorite_feature_here> into
   the HPB bot?
A: See the answer to the previous question.


Running the HPB bot on Win32 or Linux Dedicated servers:

Install the HPB bot as described above for Win32 dedicated servers and start
the game the way you normally would.  For Linux dedicated servers, download
the .tgz file into your hlds_l directory then use "tar xvfz HPB_bot_X_X.tgz"
to extract everything to the HPB_bot directory.  Within the hlds_l/HPB_bot
directory will be a directory for each supported MOD.  To install the HPB bot
simply change to the directory corresponding to the MOD you want and run the
Install script found in that directory.  For example, if hlds was installed
in /usr/games/hlds_l, you would change to /usr/games/hlds_l/HPB_bot/cstrike
then run Install to install the HPB bot for Counter-Strike.

When running a dedicated server the HPB bot commands that are normally
available from the client console screen are disabled.  To create a bot you
must set the CVAR "HPB_bot" to the value "addbot" (you can also provide
additional parameters like as shown in the HPB_bot.cfg file).  To use the
"HPB_bot" CVAR you would use one of the following commands on the dedicated
server command line...

HPB_bot "addbot"
HPB_bot "addbot 2 4"
HPB_bot "addbot 1 3 Da_Killer 4"

You can use the "kick" command to remove a bot.  When using the dedicated
server, bot will also be automatically spawned using the HPB_bot.cfg file.

For Linux dedicated servers using custom player models, make sure that the
MOD's models/player directories are all lowercase and make sure that the .mdl
files within these directories are all lowercase as well.  Remember if the
client doesn't have the same player models as on the server, all they will
see is a bunch of "helmet" players running around.


The HPB bot currently does NOT understand any of the following:

1) The HPB bot has limited skills using the weapons.  Not all weapons are
supported.  Weapons are not always used efficiently or correctly.  The bots
LOVE using the crowbar when they are close enough.

2) The bots do not know how to use many of the special skills of each of the
classes.  They won't disguise as the enemy team or feign death.  Bot's don't
know that they need to call for a medic when infected.

3) The bots don't understand the concept of attacking in groups and don't know
how to defend an area in the map.

4) The bots don't communicate with each other (or other players) about what
they are doing.  They don't understand the concept of "teamplay".

5) Currently the bots don't "load up" on ammo or health.  The bot's don't know
when they are low on ammo or health.  In a future release I'll modify things
so that the bots go get health or ammo when the are low on these items.


Here are my plans for the future releases of the HPB bot.  Please don't e-mail
me and ask me when certain features will be available because I will just
ignore your e-mail and delete it.  I will put up a release when something is
ready to be released and not before then.  The order of the items listed below
may change depending on how easy or difficult something is to get working and
depending on how I feel at that particular time.

1) Teach the HPB bot how to use special skills of each class.  For example,
being disguised as the enemy team.

2) Teach the HPB bot the rules of the game.  Capture the flag is fairly
simple once you know how to navigate to the flag and then navigate back to
home base.  But teaching the bots to not ALL rush to the flag at once and
actually use some sort of team play tactics IS NOT EASY!!!

botman
