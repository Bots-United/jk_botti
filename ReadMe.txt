botman's HPB bot source code - January 7th, 2001

This source code is provided as an example of how to create a bot for MODs
for the game Half-Life by Valve software.  The use of this source code is
unrestricted as long as you provide credit to me in any ReadMe.txt file that
is distributed with your bot or MOD and also on any website that is used to
distribute your bot or MOD.  The source code included here will allow you to
add a bot to Half-Life deathmatch, Gearbox's Opposing Force MOD, Counter-
Strike, Team Fortress Classic, Deathmatch Classic, the Holy Wars MOD and the
Front Line Force MOD.  This source code allows you to build a bot DLL file
for the Windows version of Half-Life as well as the Linux dedicated server
version of Half-Life.


What you need to run the bot:

To run the bot on a Windows machine, you will need the full retail version
of Half-Life, Counter-Strike, or Opposing Force.  The demo version of Half-
Life will not allow you to run a bot or any MOD.  To run the bot on a Linux
machine you will need to download and install the Linux dedicated server
version of Half-Life and the Linux version of the MOD that you wish to use.
The Linux version of the Half-Life dedicated server can be found on
www.fileplanet.com (in the Half-Life Official section) as well as on many
other game download websites.


What you will need to build the bot:

You will need one of the following C++ compilers:

Microsoft Visual C++ 5.0 (with service pack 3) or Visual C++ 6.0
(available for purchase from www.microsoft.com)

When using Microsoft Visual C++, you will need to open the HPB_bot.dsw
workspace file in the HPB_bot\dlls directory then use "Build->Rebuild All"
to build the HPB_bot.dll file.

To build the HPB bot on a Linux machine, FTP the files to a Linux machine
(making SURE to use ASCII mode to transfer the files), then run "make"
to build the HPB bot shared library (.so file).  If you unzip the HPB bot
source code directly on a Linux box, make SURE to run dos2unix or some
other utility to convert the carriage return and linefeed pairs (CR/LF)
to newline (NL) format, otherwise the make utility and the C++ compiler
will complain.


"Gee, Mr. Wizard.  How does the HPB bot work?"

Traditionally MODs for Half-Life could not have bots unless the MOD authors
created bot code embedded in the MOD itself.  You were not simply able to
combine bots with MODs.  For example, you couldn't use The Jumbot with
Counter-Strike to get bots in Counter-Strike.  The Jumbot is a MOD.  Counter-
Strike is a MOD.  Two MODs can not be loaded by Half-Life at the same time.
This would be like trying to load Counter-Strike and Team Fortress Classic
at the same time.  It just don't work.  In order to add bots to a MOD you
needed to have access to the source code to that MOD.

The HPB bot allows you to add a bot to Half-Life MODs without requiring you
to have access to the source code for the MOD.  In fact, for most MODs, you
will not be able to get access to the source code of the MOD.  Most MOD teams
do not make the source code to the MOD available to the general public.  The
source code for many MODs (like Counter-Strike, Team Fortress Classic,
Opposing Force, or Gunman) is owned by a company or corporation and is
considered proprietary and confidential and can not be distributed to the
general public.  Even though you don't have access to the source code of the
MOD you can still create a bot for the MOD.  The reason you can do this is
that the Half-Life engine has an internal structure used for entities.  This
same structure is used for all MODs when they create entities in a game.
When a player connects to a server, the Half-Life engine creates an entity
for this player and calls functions in the MOD when actions occur with that
entity.  Bots are created in exactly the same way that players are created,
except that bots don't have a network connection since they don't exist
outside of the server.

I started trying to develop a method of adding bots to Team Fortress Classic
and Counter-Strike in early 2000 (late January/early February).  I started
by trying to remove things from the Valve Standard SDK that weren't needed
by bots and somehow get a modified SDK and MOD to load at the same time.
After many weeks of crashing my Half-Life server over and over and over
again, I decided it would be easier to start almost completely from scratch
and create 99% of the code myself.  After working on my Half-Life deathmatch
bot for many months, I had a pretty good understanding of how the Half-Life
engine interfaced to a MOD.  There was a list of functions in the Half-Life
engine that the MOD could call and there was a list of functions in the MOD
that the Half-Life engine would call.  All of the other code in the MOD
was pretty well self-contained and didn't need any direct hooks into the
Half-Life engine (other than the previously mentioned list of functions).

I felt that it should be possible to create a DLL that sits between the
Half-Life engine and the MOD and pass function calls from one to the other.
When the Half-Life engine wanted to call a MOD function it would call this
intermediate DLL and the intermediate DLL would pass the call along to the
MOD.  If the MOD needed to call a Half-Life engine function, it would call
the intermediate MOD version of this function and the intermediate MOD
would pass the call along to the real Half-Life engine function.  After
some weeks with playing around with this idea, I was able to create my
Stripper Add-On which allowed you to select weapons that you did not want
to spawn in a level (strip them out), without having to modify the map
file itself.  You could strip out just the gauss gun and crossbow if you
didn't want those weapons to appear and every map that you loaded would
not have those weapons available.  This was done by intercepting the
engine "spawn" request to the MOD.  I would compare the name of the item
trying to be spawned and if it was one of the ones that I didn't want,
I would simply return to the engine and not pass this request on to
the MOD DLL thus the item would never be spawned in the game.

Perhaps this would be a good time to explain how entities get spawned in
Half-Life.  When you create a map (using Worldcraft or other map editing
tools), you specify the name of an entity that you want to create at a
specific location in the level.  These names are stored as ASCII text
strings (like "weapon_shotgun", "item_healthkit", or "ammo_crossbow").
When the Half-Life engine wants to create one of these items, it calls
a function in the MOD DLL that has the same name as the item.  For
example, in Half-Life deathmatch one of the functions exported from the
DLL file is called "weapon_shotgun".  You won't find this function in
the Standard SDK source code if you go looking for it.  It actually gets
created by a macro called "LINK_ENTITY_TO_CLASS()".  This macro is defined
in the util.h file in the Standard SDK.  This macro creates a function
that calls GetClassPtr().  GetClassPtr() is defined in the cbase.h file
in the Standard SDK.  GetClassPtr() will allocate space for an entity if
it does not exist yet and then will return a pointer to the private data
portion of that entity.  What this means is that whenever you have an
entity in a map, you will have to have an exported function in the MOD DLL
with the same name as that entity.  This becomes important later on when
you create the intermediate DLL that I referred to above.  You will have
to create one function in your bot DLL for each exported function in the
MOD DLL.  This way when the engine tries to create a "weapon_shotgun",
it will call the weapon_shotgun() function in the bot DLL and the bot
DLL weapon_shotgun() will call the MOD DLL's weapon_shotgun() function
(thus completing the chain).

Once I was able to intercept calls from the Half-Life engine to the MOD
DLL, I began working on trying to intercept calls from the MOD DLL to the
Half-Life engine.  This turned out to be fairly easy to do since the
Half-Life engine calls a function in the MOD with a list of engine functions
that the MOD can call.  The function that does this in the Standard SDK is
"GiveFnptrsToDll()", which can be found in the h_export.cpp file.  In the
Standard SDK, all this function does is copy this structure to a globally
accessible structure called "g_engfuncs".  In the HPB bot version of
GiveFnptrsToDll (also found in h_export.cpp), it does a little more than
this.  It make a local copy of the Half-Life engine's functions (also stored
in a variable called g_engfuncs) so that the HPB bot DLL can call the
engine's version of these function when needed.  The HPB bot version of
GiveFnptrsToDll then determines which MOD is being loaded (by calling an
engine function to get the game directory and using that to determine which
MOD is being played).  Once the MOD is known, the HPB bot code loads the DLL
file that is required for this MOD (for example TFC uses a DLL file called
tfc.dll).  Once the MOD DLL has been loaded by the HPB bot code, it needs to
load the address of 2 functions exported by the MOD DLL (GiveFnptrsToDll and
GetEntityAPI).  The HPB bot code overrides the function addresses in the
engine's function table with HPB bot versions of these functions (so that
the MOD DLL will call the HPB bot version of these functions instead of
calling the Half-Life engine's version) and passes this "engine" function
table on to the newly loaded MOD DLL by calling it's version of the
GiveFnptrsToDll() function.  (You may want to re-read the previous paragraph
again SLOWLY if you don't fully understand what I just said).

After the Half-Life engine has called GiveFnptrsToDll(), it calls a function
named "GetEntityAPI()".  GetEntityAPI() will copy the list of MOD functions
that can be called by the engine, into a memory block pointed to by a pointer
passed into this function.  This allows the Half-Life engine to call these
MOD DLL functions when it needs to spawn an entity, connect or disconnect a
player, call Think() functions, Touch() functions, or Use() functions, etc.
The HPB bot passes it's list of these functions back to the Half-Life
engine and then calls the MOD DLL's version of GetEntityAPI (passing in a
pointer to the HPB bot's structure that will hold the MOD DLL's function
list).

After GiveFnptrsToDll() and GetEntityAPI() have been called, the Half-Life
engine will call GameDLLInit() which can be used by the MOD DLL to initialize
any variables that it needs.  Each of these functions (GiveFnptrsToDll,
GetEntityAPI, and GameDLLInit) will only be called one time by the Half-Life
engine (as long as that MOD is running).  They don't get called each time a
map is loaded.

You may be wondering "How does Half-Life know what the DLL file for a MOD is
anyway?".  Each MOD exists in a subdirectory (or folder) inside the Half-Life
directory.  For example all of TFC's files will be found in the Half-Life\tfc
folder.  All of Counter-Strike's files will be found in the Half-Life\cstrike
folder.  Within this MOD folder is a file called "liblist.gam".  This file is
used by Half-Life to load a MOD.  You can view or edit this file with any
text editor (like Notepad).  One of the entries in this file is called
"gamedll".  The gamedll entry tells Half-Life the directory and filename of
the MOD DLL file.  For example, in the tfc directory the liblist.gam file has
"dlls\tfc.dll" as the gamedll entry.  When Half-Life starts to load the TFC
MOD, it scans this liblist.gam file and loads the MOD DLL file indicated by
the "gamedll" entry (in this case dlls\tfc.dll which would actually be
"C:\SIERRA\Half-Life\tfc\dlls\tfc.dll" if you have Half-Life installed in the
default directory of C:\SIERRA\Half-Life).

So, the engine loads the DLL specified by "gamedll", then calls the
GiveFnptrsToDll() function in this DLL.  The engine then calls GetEntityAPI()
from this DLL and then it calls GameDLLInit() in this DLL.  Once this is done,
the engine proceeds to load the map and begins spawning entities that are
contained within that map.  It does this by calling the functions in the DLL
with the same name as the entity (like in the "weapon_shotgun" example above).

One of the entities that will be found in every map is called "worldspawn".
This entity is always the first entity created and is "the world" in a map.
The worldspawn entity is handled in world.cpp in the Standard SDK.  In the
Standard SDK, the Spawn() function for the worldspawn entity will precache
all of the entities needed by the MOD as well as read the sv_cheats CVAR and
store the value of sv_cheats.  This worldspawn entity will only be created
once and there will only be one worldspawn entity for each map.  You can see
in the HPB bot source code where I check if the entity being spawned is this
worldspawn entity (in the DispatchSpawn function found in dll.cpp).  I use
this time to initialize map specific variables (like waypoint file loading
and global variable initialization) and also precache some additional sounds
and sprites needed by my waypoint editing code.  Precaching an entity more
than once does not cause any problems so it's okay if I precache it and then
the MOD precaches the same sound, model or sprite in it's Spawn() function.

Once all of the entities in the .bsp map file have been spawned, the Half-Life
engine allows clients to connect to the game.  The Half-Life engine will call
a function called "StartFrame()" at the beginning of every video frame while
the MOD is running.  So if you have a video card that gives you 30 frames per
second (FPS), the StartFrame() function will get called 30 times every second.
You can use this StartFrame() function to perform periodic functions that you
want to execute.  One of the functions that you will want to execute every
frame is to call a Think() function for the bots.

In the Standard SDK, when you create an entity, you can assign a Think()
function for that entity that will get called periodically by the Half-Life
engine.  For fake clients (bots) the engine will not call the Think() function
and you must do this yourself.  That's why this is done in the StartFrame()
function found in the HPB bot dll.cpp file.  There are lots of other functions
that are executed in the HPB bot StartFrame() function, but I won't go into
any detail about each one of those.  You can read through the code and figure
these things out on your own.

Okay, so now we know how things are started up and we know how entities get
created.  We know how to get the bot's Think() function to be called during
every frame.  How do you actually get a bot into the game?

Look at the BotCreate() function in the bot.cpp file of the HPB bot source
code.  In it's simplest form, you need to do the following...

void BotCreate(void)
{
   edict_t *BotEnt;  // this is a pointer to the engine's edict structure
   char ptr[128];  // allocate space for message from ClientConnect
   char *infobuffer;  // pointer to buffer for key/value data
   int clientIndex;  // engine's player index (1-32)

   BotEnt = g_engfuncs.pfnCreateFakeClient)( "Bot" );
   player( VARS(BotEnt) );
   infobuffer = g_engfuncs.pfnGetInfoKeyBuffer)( BotEnt )
   clientIndex = ENTINDEX( BotEnt );
   g_engfuncs.pfnSetClientKeyValue)( clientIndex, infobuffer, "model", "gina" );
   ClientConnect( BotEnt, "BotName", "127.0.0.1", ptr );
   ClientPutInServer( BotEnt );
   BotEnt->v.flags |= FL_FAKECLIENT;
}

Let's analyze this line by line to understand what it does...

The pfnCreateFakeClient() function is an engine function that creates a fake
client (bot) on the server.

The player() function is one of the functions exported by the MOD DLL that
initializes the player entity.  You have to pass in an entvars_t pointer to
this function and that's what the VARS() macros does for you (it converts an
edict_t pointer to the associated entvars_t pointer).

The pfnGetInfoKeyBuffer() function is an engine function that returns a
pointer to a text string that contains key value information about this
entity (things like left handed or right handed models in Counter-Strike use
the infobuffer key "lefthand" to store which model version to use).

The ENTINDEX() function is an engine function that returns the client index
for the player (the client index ranges from 1 to 32 depending on which
client slot this player is using).

The pfnSetClientKeyValue() function is an engine function that modifies a
key/value pair in the infobuffer that was retrieved previously.  Here we
are setting the player model to use the "gina" Half-Life deathmatch model.
This model may get changed by the MOD once the player joins the game.

The ClientConnect() function is a MOD DLL function that normally gets called
by the Half-Life engine when a human player is connecting to the server.  The
"BotName" parameter is the name of the bot and this is the name that will show
up in the scoreboard.  The "127.0.0.1" parameter is the IP address of the bot.
Since bots don't have a network connection, I use the standard TCP/IP loopback
address to indicate to the engine that this is a bot connecting to the server.
The bot will appear on the scoreboard at this point, but the bot is not quite
connected to the game yet.

ClientPutInServer() is another MOD DLL function that we call to actually
put the bot into the game.  At this point the bot will appear like any other
player would appear.  If the MOD does not require the bot to select a team
or class or weapon before joining the game, the bot will be spawned into
the level and will be visible to other players.

The last thing we do is set the FL_FAKECLIENT bit in the entity's flags
variable.  We do this to let the engine know that this player is a bot and
does not have a network connection.  If the engine tries to send a network
message to a bot it will crash.  The FL_FAKECLIENT bit tells the engine "Do
not send any network messages to this entity".

Now we've got the bot into the game, we need to be able to get the bot to
run around and shoot at stuff.  We do this by calling an engine function
named "pfnRunPlayerMove()".  This function is only used by fake clients (bots)
and is required to get the engine to update the bot's origin (location) as
well as to get the bot to be able to fire weapons, jump, duck, use items,
and perform all of the other functions that normal human players can do.

The pfnRunPlayerMove() function is called during every frame for every bot
that is connected to the server.  The HPB bot StartFrame() function will
loop through a global array of bot structures (called "bots") to determine
which bots are connect to the server and will call the BotThink() function
for each one of these bots.  The BotThink() function (found in bot.cpp) will
call pfnRunPlayerMove() when it is ready to have the engine update the bot's
location and "keyboard" keys that have been pressed.  The pfnRunPlayerMove()
function takes the following arguments...

void pfnRunPlayerMove( edict_t *fakeclient, const float *viewangles,
 float forwardmove, float sidemove, float upmove, unsigned short buttons,
 byte impulse, byte msec );

...this function prototype can be found in the engine.h file included with
the HPB bot or in the Standard SDK source code.  Here's a description of
the arguments to the pfnRunPlayerMove() function...

edict_t *fakeclient - This is the edict pointer of the fake client (bot) that
was created with the pfnCreateFakeClient() function in BotCreate().

float *viewangles - This is the view angles for the bot.  The view angles
are the direction in which the bot is facing (viewing and aiming).

float forwardmove - This is the speed which you wish the bot to use to move
forward during the next frame.  The maximum default forward speed in
Half-Life deathmatch is 270 units/second.  Other MODs will have different
maximum speeds (sometimes the maximum speed depends on which class the bot
is playing).  Passing in a negative value for the speed will cause the bot
to move backwards.

float sidemove - This is the speed to use when moving sideways (strafing).
Again, different MODs will have different maximum strafe speeds (possibly
depending on the player class).  Positive values will move the bot to its
right, and negative values will move the bot to its left.

float upmove - From what I have found, this parameter seems to do nothing.
I assumed it was used to move up and down ladders, but this is actually
handled by getting the bot to look in the direction it wants to go (using
the view angles) and "pressing" the IN_FORWARD key to move in that direction.

unsigned short buttons - This is what is used by the bot to press "keys" on
the "keyboard".  The button variable is a bit mask of values that can be set
to get the bot to perform various actions (like shooting, ducking, jumping,
etc.).  These values are all defined in the common\in_buttons.h file found
in the HPB bot template source code or the Standard SDK source code.

byte impulse - There are "impulse commands" that can be set by the player
which get handled by the MOD DLL code.  In Half-Life deathmatch, for example,
the flashlight is an impulse command (100) setting impulse to 100 will turn
the flashlight on or off.  See the CBasePlayer::ImpulseCommands() in the
player.cpp file from the Standard SDK for more details.  The HPB bot template
source code doesn't use this parameter for anything.

byte msec - This value (in milliseconds) is used to tell the engine what the
"duration" of the pfnRunPlayerMove() command should be.  The duration of the
movement for a bot needs to be based on what the frame rate of the server is.
Faster frame rates means less time per frame is spent moving a player at X
units per second.  Ideally you want to send a total of 1000 milliseconds for
every second of real time.  This means if you are getting exactly 10 frames
per second, and the BotThink function is getting called 10 times a second,
every pfnRunPlayerMove() command should use 100 as the msec value (since 10
times 100 equals 1000).  What you are trying to do is predict how long you
think the next frame will actually take for the engine to render so that the
bot moves at the desired speed during that period of rendering time.  Since
you can't predict the future, you have to guess at what this value will be.
The msec value that I am using comes from modified code from TheFatal's
(author of The Jumbot) advanced bot framework.  You can see this code at the
top of the BotThink() function in the bot.cpp file.

Okay, so now we understand how to get a bot into the game and how to get it
to move around in a level.  What to we need to do to get the HPB bot code to
work for a MOD that isn't already supported by the HPB bot?

I mentioned above that the MOD DLL exports a list of functions that have the
same name as the entity.  In the HPB bot template source code "linkfunc.cpp"
contains a list of functions that are the hooks between the Half-Life engine
and the MOD DLL.  The HPB bot DLL will export these function so that the
Half-Life engine can call them when an entity is spawned.  The HPB bot version
of these functions will call the MOD DLL's version of these same functions
to get the MOD to create these entities.  I have included all of the entities
from Half-Life deathmatch, Team Fortress 1.5, Counter-Strike 1.0, Opposing
Force, and Front Line Force 1.1.  To support any other MODs (or to support
updates to the currently supported MODs) you may have to add entities to
the linkfunc.cpp file.  If you have Microsoft Visual C++ you can use the
"dumpbin" utility (found in the bin directory) to dump the exported symbols
in a DLL file.  Start up an MS-DOS window and run "dumpbin /exports mp.dll"
(or whatever the MOD DLL name is) and it will give you the list of exported
symbols (most of which will be the entities that you need).  You can ignore
any of these symbols that begin with a '?'.  These are C++ functions that
do not need to be exported from the bot DLL code (since they never will be
called by the Half-Life engine).  All of the other symbols need to be included
in linkfunc.cpp in order for entities in the MOD maps to be properly spawned.

If you don't have Microsoft Visual C++, I have written a small C utility
which will allow you to dump symbols from a DLL file called "exports.c".  You
can find this program here: http://planethalflife.com/botman/exports.c
Compile this using any C or C++ compiler and run exports.exe with the DLL
filename as an argument and it will print out a list of exported symbols.

You will notice that some of the exported symbols are the same as function
names that we have discussed previously (GetEntityAPI and GiveFnptrsToDll).
These and a few others (GetEntityAPI2 and GetNewDLLFunctions) are already
defined in the HPB bot source code and do not need to be included in the
linkfunc.cpp file.  If you include a symbol that has already been defined
(either in linkfunc.cpp or in any of the other HPB bot files) you will get
an error message from the compiler or linker complaining about duplicate
function names.  To fix this just remove the entry that you added to the
linkfunc.cpp file that caused the duplication.

After updating linkfunc.cpp you will also need to change h_export.cpp to
let it know about the MOD directory that you are using.  Modify the
GiveFnptrsToDll() function to know what library (DLL file) needs to be
loaded based on the directory name for the MOD.  There are already examples
in this function for Half-Life deathmatch, Team Fortress Classic, Counter-
Strike, Opposing Force, and Front Line Force.

Once you have updated linkfunc.cpp with all of the necessary entity names
and modified h_export.cpp to load the MOD DLL file, you should rebuild the
HPB bot DLL, copy it to the MOD dlls directory, modify the liblist.gam file
in the MOD directory to use your bot DLL file instead of the MOD DLL file
(in the gamedll setting), and start up the MOD.  You should be able to create
a LAN game and spawn into the game just like you normally would without the
bot code.  Try running around in various maps to make sure that all entities
are showing up and make sure that everything behaves the way that it normally
would (i.e. you can use items, push buttons, fire weapons, open doors, climb
ladders, swim, jump, etc.)  If the game crashes at any point then you probably
haven't added all the entities that should be added.  It is a good idea to
turn on developer mode when starting up Half-Life (use "hl.exe -dev" in a
shortcut to start Half-Life in developer mode).  This will print out extra
information on the console when loading a level.  Any entities that you failed
to add to linkfunc.cpp will cause an error message to be printed out while
the Half-Life engine is loading the map.  You should make a note of these
missing entities, quit the game, and edit linkfunc.cpp to add these missing
entities, then rebuild the bot DLL file, start the MOD and try that map out
again.

Okay, you've got all the MOD entities added to the linkfunc.cpp file.  How
do you create a bot and get it to join a team?

For some MODs the bot will immediately spawn into the game without having to
choose anything from a menu (like Half-Life deathmatch).  For other MODs you
will have to create code so that the bot can make selections from any menus
that get presented to players when they join the game.  Now would be a good
time to talk about how network packets are sent between the server and the
client.

When the MOD DLL wants to display something on the client's HUD, it must send
a network packet to that client.  This is done using several Half-Life engine
functions.  Every network message from the MOD to the client will begin by
calling the pfnMessageBegin() engine function (found in the engine.cpp file
in the HPB bot template source code).  Every network message will end by
calling pfnMessageEnd() (also in engine.cpp).  Between the pfnMessageBegin()
and pfnMessageEnd(), the MOD will sent BYTEs, CHARacters, SHORT integers,
LONG integers, ANGLEs, COORDinates, STRINGs or ENTITY values.  Each of these
data types has a pfnWriteXXX() function (where XXX is Byte, Char, Short, Long,
Angle, etc.).  You can see the prototypes for these functions in the
engine\eiface.h file in the Standard SDK or the HPB bot template source code.

The network packet is made up of this data in the same order that the MOD
calls these functions.  For example if the MOD wants to send a network packet
with a byte, followed by a string, followed by a long, it would issue the
following Half-Life engine function calls...

pfnMessageBegin(MSG_ONE, gmsgShowIt, NULL, pEdict);
pfnWriteByte(1);
pfnWriteString("You've got the flag!");
pfnWriteLong(100000);
pfnMessageEnd();

Here's what the arguments to pfnMessageBegin() are:

int msg_dest - This is the destination of this message.  Messages can be sent to
all players (MSG_BROADCAST or MSG_ALL), one player (MSG_ONE_UNRELIABLE or
MSG_ONE), players in the potentially visible set (MSG_PVS or MSG_PVS_R), or
players in the potentially audible set (MSG_PAS or MSG_PAS_R).  There is a
"unreliable" and "reliable" version of each type of message.  Unreliable means
that the Half-Life engine sends the message once, but it is not guaranteed to
get to the client.  Reliable messages will keep begin sent to the client until
the client replies back that it has successfully received this message.  These
msg_dest types can be found in the common\const.h file.

int msg_type - The message type is a number assigned to the message when the
MOD is started up by the Half-Life engine.  For each custom message that the
MOD wants to send to the client, the MOD will have to "register" this message
with the Half-Life engine.  This is done in the MOD using the REG_USER_MSG()
macro (which calls the pfnRegUserMsg engine function).  You pass REG_USER_MSG
the network message name (for example "ShowIt") and the size of the message
in bytes (or -1 if the size will vary).  The size is the number of bytes that
will be sent between the pfnMessageBegin() and pfnMessageEnd().  You can find
many examples of the REG_USER_MSG in the Half-Life Standard SDK source code.

float *pOrigin - This parameter can be used to pass an origin Vector (an array
of 3 floating point values) to the client.  The origin is the location in 3D
space that you want to use for this network message (for example to display
a sprite at a specific location in a map).

edict_t *ed - The edict parameter informs the Half-Life engine which entity
(player) you want to send a message to when you use MSG_ONE_UNRELIABLE or
MSG_ONE.  The message will only get sent to this specific player.

When the MOD DLL sends a network message, the HPB bot engine.cpp code will
intercept these pfnMessageBegin(), pfnMessageEnd() and pfnWriteXXX() functions
before they are sent to the engine.  This allows bots to monitor what network
messages are being sent to other clients and to themselves.  Since the bots
don't have a client, the engine will simply throw away any messages that are
destined for a bot (as long as the FL_FAKECLIENT bit is set in the flags field
of that player).

So, in order to know when things are being displayed on the bot's "HUD", we
must intercept the MOD network messages and recognize what data is being sent
as part of the network packet.  The first thing we must do is know what all
of the "msg_type" values are going to be.  Since each message has a unique
msg_type value and since no two MODs will use the same msg_type values
(because each MOD has it's own list of custom network messages), we need some
way to know which msg_types correspond to which messages.  This is done in
the pfnRegUserMsg() function in the HPB bot engine.cpp file.  pfnRegUserMsg()
will get passed a text string (the network message name) and the number of
bytes (which we don't really care about).  I have a global integer variable
used to store the msg_type assigned to each message as it is created by the
Half-Life engine.  Notice that I first call the Half-Life engine's version
of pfnRegUserMsg() and store the return value for my use like this...

   msg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);

Then I compare the pszName parameter to specific strings that I know the MOD
will use for certain types of network messages.  How do I know what these
strings are?  I have to start the MOD up and let the HPB bot write out all
of these values to a text file that I can go back and look at later.  If you
are using Microsoft Visual C++, this will be done automatically for you if
you build the HPB bot DLL in Debug mode.  Building in Debug mode will define
the _DEBUG symbol which will cause this code in pfnRegUserMsg() to be added
to the HPB bot DLL...

#ifdef _DEBUG
   fp=fopen("bot.txt","a");
   fprintf(fp,"pfnRegUserMsg: pszName=%s msg=%d\n",pszName,msg);
   fclose(fp);
#endif

If you don't have Microsoft Visual C++ you can just comment out the "#ifdef
_DEBUG" and "#endif" lines and rebuild the bot to always print this data
out.

This creates a text file in the Half-Life directory called "bot.txt" and
will write out a text string each time pfnRegUserMsg() gets called by the
MOD DLL.  For example, here's some of the output from Team Fortress Classic
with Debug mode enabled...

pfnRegUserMsg: pszName=SelAmmo msg=64
pfnRegUserMsg: pszName=CurWeapon msg=65
pfnRegUserMsg: pszName=Geiger msg=66
pfnRegUserMsg: pszName=Flashlight msg=67
pfnRegUserMsg: pszName=FlashBat msg=68
pfnRegUserMsg: pszName=Health msg=69
pfnRegUserMsg: pszName=Damage msg=70
pfnRegUserMsg: pszName=Battery msg=71
pfnRegUserMsg: pszName=SecAmmoVal msg=72
pfnRegUserMsg: pszName=SecAmmoIcon msg=73
pfnRegUserMsg: pszName=Train msg=74
pfnRegUserMsg: pszName=Items msg=75
pfnRegUserMsg: pszName=Concuss msg=76
pfnRegUserMsg: pszName=HudText msg=77
pfnRegUserMsg: pszName=SayText msg=78

Team Fortress Classic has about 50 of these custom network messages.  What
do they all do?  We don't know that yet.  You have to figure out what they
do by looking to see where they get used in the game.  Finding out where they
appear in the game will be explained a little later on in this ReadMe.txt
file.  You'll also notice that the msg_type assigned by the Half-Life engine
doesn't begin with 1 (or 0).  It starts with 64.  There are some network
messages that are reserved by the Half-Life engine.  Some of these can be
seen in the util.h file in the Standard SDK or the HPB bot template source
code.  All of the reserved msg_types begin with "SVC_".  The other reserved
messages are unknown (at least by me).

Most MODs, that use the same network messages that are defined in the Valve
Standard SDK, also use the same format for the data part of those network
messages.  For example the "Health" message shown above is used to tell
the HUD to change the current health value displayed on the HUD.  This network
message can be found in the player.cpp file in the Standard SDK...

   MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
      WRITE_BYTE( m_iClientHealth );
   MESSAGE_END();

...this is the same as...

   pfnMessageBegin( MSG_ONE, gmsgHealth, NULL, pev );
   pfnWriteByte( m_iClientHealth );
   pfnMessageEnd();

...in the case of TFC, gmsgHealth will always be 69 (unless Valve modifies
Team Fortress Classic to include a new custom network command that shuffles
all of these value around).  It's best not to "hard-code" these values since
it makes things much more difficult to debug if a new version of a MOD is
released.  By saving the value that pfnRegUserMsg() returns and comparing
that to the value passed into pfnMessageBegin(), we can know which message
is being sent to the client (in this case, the health of a player is being
adjusted up or down and needs to be displayed on the HUD).

Now that we know how to intercept network messages, we can begin to try to
figure out how to get a bot to select something in a menu so that it can
choose a team or class or weapon before joining the game.  How do we go about
doing this?  We need to know what msg_type is used when a menu is displayed.
The easiest way to find this out is to start a game with the "engine_debug"
flag enabled and join a game ourselves, then go back and look at the "bot.txt"
file to see what network messages got sent to our HUD.  The "engine_debug"
flag can be found in the engine.cpp file and is normally enabled by pulling
down the console and using the command "debug_engine" to turn it on.  From
that point on, any messages going from the MOD to the engine will be logged
in this bot.txt file.  Since we want to display messages that occur before
we get connected to the game (and have a chance to pull the console down),
we will temporarily hard code the engine_debug flag to be enabled at startup
time.  In engine.cpp, change this line...

int debug_engine = 0;

...to this...

int debug_engine = 1;

...and rebuild the HPB bot DLL file.  Copy the HPB bot DLL file to the MOD
dlls directory and start a LAN game up like you did before.  Join a game by
picking one of the options in a menu then exit from the game and look at the
bot.txt file.  You will see a BUNCH of output in there.  This is why you don't
want to leave the debug_engine flag set to 1 all the time (because it will
wind up filling up the user's hard disk with a bunch of data).

Here's an example of me joining a Team Fortress game by selecting the Red Team
from the VGUI menu...

Skip down past the point where the pfnRegUserMsg output occurs.  You'll see
some entities being created and some stuff being precached.  You want to find
a pfnMessageBegin() with a msg_type that is 64 or larger.  The first one I see
looks like this...

pfnMessageBegin: edict=0 dest=2 type=89
pfnWriteByte: 1
pfnWriteString: 
pfnMessageEnd:

...msg_type 89 is a "TeamInfo" message.  This probably isn't what I want.
Skip on down to the next msg_type that is 64 or larger...

pfnMessageBegin: edict=1df7410 dest=1 type=80
pfnWriteByte: 1
pfnWriteString: #Observer
pfnMessageEnd:

...msg_type 80 is "TextMsg".  This is probably displaying some kind of text
on the HUD.  This isn't what we want yet so keep looking...

pfnMessageBegin: edict=1df7410 dest=1 type=84
pfnWriteByte: 0
pfnMessageEnd:

...msg_type 84 is "ResetHUD".  This isn't what we want...

pfnMessageBegin: edict=1df7410 dest=1 type=85
pfnMessageEnd:

...msg_type 85 is "InitHUD".  This isn't what we want...

pfnMessageBegin: edict=1df7410 dest=1 type=73
pfnWriteString: grenade
pfnMessageEnd:

...msg_type 73 is "SecAmmoIcon".  Something to do with secondary ammo.  Not
what we want so keep searching...

pfnMessageBegin: edict=0 dest=2 type=80
pfnWriteByte: 1
pfnWriteString: #Game_playerjoin
pfnWriteString: botman
pfnMessageEnd:

...msg_type 80 is "TextMsg" (we've seen this before).  I see "#Game_playerjoin"
and then "botman" (hey!, that's me!).  I'll bet this message is displaying
text on player's HUDs letting them know that I have just joined the game!  Keep
searching...

pfnMessageBegin: edict=1df7410 dest=1 type=91
pfnWriteByte: 1
pfnMessageEnd:

...msg_type 91 is "GameMode".  Hmmm, I'm not sure what that is.  Let's keep
searching...

pfnMessageBegin: edict=1df7410 dest=1 type=88
pfnWriteByte: 1
pfnWriteShort: 0
pfnWriteShort: 0
pfnWriteShort: 0
pfnWriteShort: 0
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=88
pfnWriteByte: 1
pfnWriteShort: 0
pfnWriteShort: 0
pfnWriteShort: 0
pfnWriteShort: 0
pfnMessageEnd:

...we get 2 msg_type 88's in a row.  88 is "ScoreInfo".  This is sending
something to the HUD to update the score information.  Probably since I
just joined the game the MOD is letting all the clients know that my score
is 0.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=104
pfnWriteByte: 1
pfnWriteByte: 0
pfnMessageEnd:

...msg_type 104 is "Spectator".  Yes, I'm in spectator mode until I choose
a team and a class.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=89
pfnWriteByte: 1
pfnWriteString: 
pfnMessageEnd:

...msg_type 89 is "TeamInfo" again.  I'm not sure what that is so I'll keep
searching...

pfnMessageBegin: edict=0 dest=2 type=90
pfnWriteString: Blue
pfnWriteShort: 0
pfnWriteShort: 0
pfnMessageEnd:
pfnMessageBegin: edict=0 dest=2 type=90
pfnWriteString: Red
pfnWriteShort: 0
pfnWriteShort: 0
pfnMessageEnd:

...msg_type 90 is "TeamScore".  Hey, there's something in there I recognize!
It's got "Blue" and "Red", those are the team names.  Not quite what we want
so we keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=97
pfnWriteByte: 6
pfnMessageEnd:

...msg_type 97 is "HideWeapon".  I don't even have any weapons yet, so I'll
just ignore this.  This is probably something to prevent some weapon cheat
when switching teams or something.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=98
pfnWriteByte: 0
pfnMessageEnd:

...msg_type 98 is "SetFOV".  I know from the Standard SDK that the FOV is
the field of view (used to zoom in when using the crossbow).  Setting the
FOV to zero (the pfnWriteByte value) resets the FOV back to the normal 90
degrees.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=69
pfnWriteByte: 1
pfnMessageEnd:

...msg_type 69 is "Health".  This is resetting my health to 1!  I guess
I've only got 1 unit of health while I'm in observer mode.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=70
pfnWriteByte: 0
pfnWriteByte: 0
pfnWriteLong: 0
pfnWriteCoord: 384.000000
pfnWriteCoord: -640.000000
pfnWriteCoord: -255.000000
pfnMessageEnd:

...msg_type 70 is "Damage".  I know from the Standard SDK that this message
is sent when the player is damaged by another player, explosions or by the
world (like falling damage).  It looks like it's just resetting things to
zero (the pfnWriteBytes and pfnWriteLong).  The Coord is probably not used
here.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=71
pfnWriteShort: 0
pfnMessageEnd:

...msg_type 71 is "Battery".  I know in the Standard SDK that the Battery
message is used to increase (or decrease) the armor.  I guess this must be
setting my armor to zero.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=72
pfnWriteByte: 0
pfnWriteByte: 0
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=72
pfnWriteByte: 1
pfnWriteByte: 0
pfnMessageEnd:

...Here we have 2 msg_type 72's in a row.  msg_type 72 is "SecAmmoVal".  This
is probably resetting my secondary ammo value to zero.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=74
pfnWriteByte: 0
pfnMessageEnd:

msg_type 72 is a "Train" message.  I know from the Standard SDK that sending
a zero here means that this player is not currently using a Train (so the HUD
won't display the little forward and backward arrows in the middle of the
display).  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=83
pfnWriteString: tf_weapon_medikit
pfnWriteByte: -1
pfnWriteByte: -1
pfnWriteByte: -1
pfnWriteByte: -1
pfnWriteByte: 0
pfnWriteByte: 1
pfnWriteByte: 3
pfnWriteByte: 0
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=83
pfnWriteString: tf_weapon_spanner
pfnWriteByte: 2
pfnWriteByte: -1
pfnWriteByte: -1
pfnWriteByte: -1
pfnWriteByte: 0
pfnWriteByte: 2
pfnWriteByte: 4
pfnWriteByte: 0
pfnMessageEnd:

<a bunch more type=83 messages left out here>

...Whoa!  What's this?  msg_type 83 is "WeaponList".  I see the names of
weapons in each of these packets with a bunch of numbers.  I know from the
Standard SDK that the weapon list messages contain the weapon name, weapon
ID, max amount of primary ammo for the weapon, max amount of secondary
ammo, and the slot and position in the slot to display this weapon on the
HUD.  This is interesting, but it's not what we want.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=102
pfnWriteByte: 0
pfnWriteByte: 0
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=102
pfnWriteByte: 1
pfnWriteByte: 0
pfnMessageEnd:

<a bunch more type=102 messages left out here>

...msg_type 102 is "AmmoX" messages.  I know from the Standard SDK that AmmoX
messages are sent when the ammo amount is increased or decreased.  The first
byte in the message is the ammo index value and the second byte in the message
is the current amount of ammo available.  All of these are being reset to
zero.  Not what we want, keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=106
pfnWriteShort: 0
pfnWriteShort: 0
pfnWriteShort: 0
pfnWriteShort: 0
pfnWriteShort: 0
pfnMessageEnd:

...msg_type 106 is a "ValClass" message.  I'm not sure what this is so I'll
just ignore it for now.  Keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=107
pfnWriteByte: 2
pfnWriteString: #Team_Blue
pfnWriteString: #Team_Red
pfnWriteString: #Team_Yellow
pfnWriteString: #Team_Green
pfnMessageEnd:

...msg_type 107 is a "TeamNames" message.  This must be telling my HUD what
the names of the teams are for this map.  Getting closer, keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=105
pfnWriteByte: 1
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=108
pfnWriteByte: 0
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=109
pfnWriteByte: 0
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=111
pfnWriteByte: 0
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=112
pfnWriteByte: 0
pfnMessageEnd:

msg_types 105, 108, 109, 111, and 112 are "AllowSpec", "Feign", "Detpack",
"BuildSt", and "RandomPC" respectively.  I'm not sure what these are used
for so I'll keep searching...

pfnMessageBegin: edict=1df7410 dest=1 type=93
pfnWriteString: botman
pfnMessageEnd:

...msg_type 93 is "ServerName".  I guess this is storing the name of the
server on the HUD.  In this case, since I created the server, the server
has my name.  Shortly after this we find something...

pfnMessageBegin: edict=1df7410 dest=1 type=110
pfnWriteByte: 2
pfnMessageEnd:
pfnMessageBegin: edict=1df7410 dest=1 type=110
pfnWriteByte: 2
pfnMessageEnd:

ClientCommand: jointeam 2

pfnMessageBegin: edict=0 dest=2 type=80
pfnWriteByte: 1
pfnWriteString: #Game_joinedteam
pfnWriteString: botman
pfnWriteString: 2
pfnMessageEnd:

...we have 2 msg_type 110s "VGUIMenu".  Hey!  That's what we've been looking
for!  The MOD is displaying a menu and sending a 2 as the first byte.  We'll
have to figure out what that 2 is used for a little later on.  Right after
the "VGUIMenu" messages we see a ClientCommand() function being called.  What
has happened here is the client has sent a command "jointeam 2".  This was
sent by my client when I selected the Red team from the menu.  At this point
I'm guessing that "jointeam" assigns me to a team and "2" is used to indicate
which team I want (1=Blue, 2=Red, 3=Yellow, 4=Green).  We can verify this by
starting a game and choosing other teams to see how this command changes.

After receiving the "jointeam" command, the MOD DLL sends a network packet to
the client with a text string (msg_type 80 = "TextMsg") that says
"#Game_joinedteam" and has my player name and a "2".  This must be displaying
a message on player's HUDs to let them know that I have joined the Red team.

So we now know that when we see msg_type = 110 we need to send "jointeam"
followed by the team number that we wish to join.  Since the bot doesn't have
a client, it can't issue client commands in the same way that players do.  I
have created a function called "FakeClientCommand()" that allows you to pass
a command to the MOD DLL just as though the command had come from a real
client.  Here's an example of FakeClientCommand() being used in the BotStart()
function found in the bot_start.cpp file...

   // select the team the bot wishes to join...
   if (pBot->bot_team == 1)
      strcpy(c_team, "1");
   else if (pBot->bot_team == 2)
      strcpy(c_team, "2");
   else if (pBot->bot_team == 3)
      strcpy(c_team, "3");
   else if (pBot->bot_team == 4)
      strcpy(c_team, "4");
   else
      strcpy(c_team, "5");

   FakeClientCommand(pEdict, "jointeam", c_team, NULL);

This fills in the c_team with the team number then calls FakeClientCommand()
passing in the command and 2 optional arguments.  If additional arguments
aren't needed for the command you should pass NULL instead.

You can go through a similar process to find the commands needed to select
a class or select a weapon as I did above for joining a team.

I never did explain what the byte data for msg_type 110 (VGUIMenu) was used
to indicate.  This byte value selects what type of VGUI menu is going to be
displayed on the HUD.  For example there is one menu for selecting a team,
there is a completely different menu for selecting a class, and another
completely different menu for selecting a weapon (or special skill, etc.)
Each of these VGUI menus will have a byte value assigned to them and they
won't be the same between different MODs.  TFC uses 2 as the byte value to
identify the team selection menu.  TFC uses 3 as the byte value to identify
the class selection menu.  Anytime a msg_type 110 is sent with a byte value
of 3 means the player (or bot) should send a command to select a class.

Okay so now we know what network messages to look for.  How does the HPB bot
code intercept these messages and interpret what is being sent?

Let's take the case of the "VGUIMenu" message.  The first thing we want to
do is store what the msg_type value will be for this network message.  This
is done in the pfnRegUserMsg() function in the engine.cpp file.  I store this
in a global variable called "message_VGUI".  Whenever pfnMessageBegin() gets
called, I compare the msg_type to message_VGUI.  If they are the same then I
know that the MOD is sending a VGUI message to one of the clients.  I want
to make sure that I only intercept messages that are being sent to bots,
since I don't want to interfere with message being sent to real players.  I
mentioned above that one of the parameters to pfnMessageBegin() was the player
edict.  This is the same value that gets returned by CreateFakeClient() when
the bot was created.  If we store these edict values in some global array,
we can scan this array to see if a network message is being sent to a bot.
This is done by a function called UTIL_GetBotIndex() which is found in the
util.cpp file.  This function will return the index in a global array (called
"bots") that stores the bot edict values.  If the edict value passed into
UTIL_GetBotIndex() is not found in this array then UTIL_GetBotIndex() will 
return -1 indicating that this edict is NOT a bot.  If the return value is
zero or greater than this network message is being sent to a bot.  I will
need this index value (from the bots array) in several other places during
the network command so I store this value in a global variable called
"botMsgIndex".

When I know that a network message is being sent to a bot (index >= 0),
and I know which type of message is being sent (msg_type == message_VGUI),
I store a pointer to a function that will be used to handle all of the data
sent during this network message.  For example, for TFC, this function is
called "BotClient_TFC_VGUI".  This function can be found in "bot_client.cpp".
I store a pointer to this function in the variable "botMsgFunction".  The
functions in bot_client.cpp work like state machines.  The function is called
over and over for every value passed between the pfnMessageBegin() and the
pfnMessageEnd().  The BotClient_XXX() function must keep track of what data
has already been sent by incrementing a "state" variable that indicates what
type of value it is expecting next.  When the last data field in a network
packet is sent, the state variable is reset back to 0 so that the next packet
of this type will start with the first data field again.  For example, the
"BotClient_Valve_WeaponList()" function has 9 states (0-8).  Here's an example
of a WeaponList network message so that you can see the 8 different data
fields...

pfnMessageBegin: edict=1e01e40 dest=1 type=76
pfnWriteString: weapon_9mmhandgun   // state = 0
pfnWriteByte: 2                     // state = 1
pfnWriteByte: 250                   // state = 2
pfnWriteByte: -1                    // state = 3
pfnWriteByte: -1                    // state = 4
pfnWriteByte: 1                     // state = 5
pfnWriteByte: 0                     // state = 6
pfnWriteByte: 2                     // state = 7
pfnWriteByte: 0                     // state = 8
pfnMessageEnd:

Once the pfnMessageBegin() function has been called, and the botMsgFunction
variable has been initialized, I will call this "botMsgFunction" function
each time pfnWriteByte(), pfnWriteChar(), pfnWriteShort(), pfnWriteLong(),
pfnWriteAngle(), pfnWriteCoord(), pfnWriteString(), or pfnWriteEntity() is
called.  It is up to the state machine code in the bot_client.cpp function
to remember what type of data is being passed into this function (byte, char,
string, angle, etc.).  The state machine code will copy the value from the
network message into a local variable and store away anything that is
needed in the bots array (using the botMsgIndex as the index).  This can be
used to set a "flag" in the bots structure to indicate that a certain
network message has been sent to the bot.

For example, this is done in the BotClient_TFC_VGUI() function.  When it sees
a VGUIMenu message with a 2 in the byte field, it sets "start_action" to
MSG_TFC_TEAM_SELECT.  This indicates to the bot that it needs to use the
FakeClientCommand() to send the "teamselect" command to select a team.  The
BotClient_TFC_VGUI() function will set "start_action" to MSG_TFC_CLASS_SELECT
when it sees a 3 in the byte field, indicating to the bot that it needs use
the FakeClientCommand() to select the class.

You will have to add (or modify) functions in the bot_client.cpp file to match
the types of network messages being sent by the MOD you wish to add a bot to.
There should be enough examples in engine.cpp and bot_client.cpp to figure out
how to intercept and store fields from most types of network messages.

Okay, let's get on to another area.  Let's figure out what weapons are used
in a MOD and how to select them and how to get the bot to fire those weapons.

The first thing we will need is the Weapon ID for each of the weapons in the
MOD.  You saw in one of the network packets a "WeaponList" message.  This
contains the weapon name, weapon ID, and other stuff.  Let's break one of
these network messages down into it's components.  Here's one from Half-Life
deathmatch...

pfnMessageBegin: edict=1e01e40 dest=1 type=76
pfnWriteString: weapon_9mmAR
pfnWriteByte: 2
pfnWriteByte: 250
pfnWriteByte: 3
pfnWriteByte: 10
pfnWriteByte: 2
pfnWriteByte: 0
pfnWriteByte: 4
pfnWriteByte: 0
pfnMessageEnd:

The string "weapon_9mmAR" is the weapon name.  You will need to use this to
select the weapon when you want to change from one weapon to another.  Just
use the weapon name as the command like this...

FakeClientCommand(pEdict, "weapon_9mmAR", NULL, NULL);

...and the MOD will switch to that weapon (as long as you are carrying it
and have enough ammo, and are allowed to switch to it).

The first byte value is the primary ammo index value.  Ammo is represented
using an integer for the type of ammo (for example 1 might be 9mm bullets,
2 might be AR grenades, 3 might be handgrenades, etc.).  In this case, the
primary ammo type for the 9mmAR is type 2 (we know it's 9mm bullets because
we can see that when we play the game).  The next byte is the maximum number
of primary ammo that can be carried by the player at one time.  In this case
it's 250 rounds of ammo.  The next byte is the secondary ammo type.  This is
3 in this case (which is the AR grenades that we see in the game).  The next
byte is the maximum number of secondary rounds that the player can carry (in
this case it's 10 rounds).  The next byte is the HUD slot where this weapon
will be displayed (slot number 2).  Since the bots don't have a HUD they
don't care what the slot is (this is ignored).  The next byte is the position
within the slot (how far down in the slot) the weapon will be displayed.  In
this case it's 0 which means at the top of the slot.  The next byte (the 4)
is the weapon ID.  You will need to keep track of this number since some of
the other network messages will only use the weapon ID (not the weapon name)
when weapon messages are sent.  The last byte is a flags field which does
not get used by the bot.

As you receive each one of these WeaponList messages you can store the
weapon name, primary and secondary ammo type, maximum value for the primary
and secondary ammo (if you need them for anything), and the weapon ID.

For the MOD that are already supported by the HPB bot, each of these will
be handled by the BotClient_Valve_WeaponList() function (in bot_client.cpp).
Each MOD actually has it's own function (BotClient_TFC_WeaponList,
BotClient_CS_WeaponList, BotClient_Gearbox_WeaponList, etc.), but since
they all pass the same network data, they all call the Valve version.  This
is true for many of the bot_client.cpp functions.  There is a separate one
for each MOD but most of them call the Valve version since they all use the
same data in the network packets (it just makes the code a little smaller).
If the MOD you are adding a bot to has the same network data as one of the
existing functions you can just have it call the Valve version as well.

Once you know what the weapon ID is for each of the weapons in the MOD, you
can add these to the bot_weapons.h file.  These weapon IDs are defined as
constants in this file to make it easier to identify what weapon a certain
weapon ID corresponds to.  The weapon IDs are rarely (if ever) changed when
a new version of a MOD is released.  If new weapons are added the MOD authors
will usually just pick one of the unused IDs to assign to this new weapon
and not shuffle all the other weapons IDs around, but it is possible that
you might encounter a MOD that does shuffle the IDs around when a new MOD is
released.  For this reason you may want to store the weapon IDs in global
variables (using the weapon name string to identify the weapon) so that if
they do change you won't have to modify the bot_weapons.h file later on.

The weapon IDs are used by the bot to keep track of which weapon it is
currently carrying.  You will see in the bot_combat.cpp code where I check
the weapon ID before trying to switch weapons so that I don't attempt to
switch to a weapon that the bot has already previously selected.

So now you know how to find out the weapon IDs for a MOD and how to get the
bot to switch weapons.  The only other thing left when using a weapon is
getting the bot to fire the weapon.  There is a primary fire and secondary
fire on weapons (for most MODs).  You get the bot to use the primary or
secondary fire by setting one of the bits in the "buttons" argument of
pfnRunPlayerMove().  By setting the IN_ATTACK bit, the bot will use the
primary fire button.  By setting the IN_ATTACK2 bit, the bot will use the
secondary fire button.  You can set these bits by ORing the IN_ATTACK or
IN_ATTACK2 value with the current pEdict->v.button value like this...

   pEdict->v.button = pEdict->v.button | IN_ATTACK;

This will set this bit without changing any of the other bits already set
in the "button" field.  If you just set pEdict->v.button equal to IN_ATTACK,
you would clear any previously set bits.  For example if you wanted to jump
AND shoot you can't do this...

   pEdict->v.button = IN_JUMP;
   pEdcit->v.button = IN_ATTACK;

The second statement would wipe out the value set by the first statement.
You would need to do this instead...

   pEdict->v.button = pEdict->v.button | IN_JUMP;
   pEdict->v.button = pEdict->v.button | IN_ATTACK;

(or use this shorter format...)

   pEdict->v.button |= IN_JUMP;
   pEdict->v.button |= IN_ATTACK;

(it does the same thing.)

Some weapons need to have the fire button held down while they are being
used (like the egon gun in Half-Life deathmatch).  To do this you must set
the IN_ATTACK or IN_ATTACK2 bit on every frame.  Which means that you must
keep setting this bit every time the BotThink() function is called.

Some weapons may require you to charge the weapon up and then fire the
weapon.  To do this set the IN_ATTACK or IN_ATTACK2 bit on every frame for
some small amount of time then don't set the IN_ATTACK or IN_ATTACK2 bit
to get the bot to fire the weapon.

Another topic that comes up frequently is "How do I get the bot to detect
<fill in you favorite thing here> in a level?"

This can usually be done one of two ways.  But they both have advantages
and disadvantages.  When an entity is created in a level, you have several
different techniques to "find" that entity.  One of the simplest methods
is to search a radius around the bot for entities.  This can be done like
this...

float radius = 100.0;
edict_t *pent = NULL;

while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin,
                                        radius )) != NULL)
{
   if (strcmp(STRING(pent->v.classname), "entity_name") == 0)
   {
      // found an "entity_name" entity near the bot...
   }
}

UTIL_FindEntityInSphere() will return a pointer to an entity that is within
"radius" units of the origin (location) of the bot.  You pass in NULL for
the pent value on the first call to tell the engine to start the search and
you pass in each returned value on the next call to tell it to return the
next one in the list.  The engine will return NULL when no more entities
are within "radius" units of the bot.

The advantage to this is that it is fairly simple to use.  The disadvantage
to this is that it can be very slow if the search radius is very large or
if there are lots of entities near the bot.  This will especially slow things
down if you are doing this type of search during every single BotThink() call.
If you are going to use this technique, you may want to limit the searches to
once every 0.1 seconds or so to reduce the load on slower CPUs.

Another technique for keeping track of entities is to watch for them as they
are spawned (by intercepting the engine messages) and keep track of them in
a global array.  For example, here's a handgrende being thrown in Half-Life
deathmatch...

pfnCreateEntity: 1e11ca4
pfnPvAllocEntPrivateData:
pfnSetModel: edict=1e11ca4 models/grenade.mdl
pfnSetSize: 1e11ca4

We see the entity being created (the edict is 1e11ca4 hexadecimal).  Then we
see the model of this edict being set to "models/grenade.mdl" so we know that
it is a hand grenade.  The only thing we don't see is a network message when
this entity gets removed from the game.  The Half-Life engine actually deletes
entities without the MOD having to call a function to do this.  One of the
bits that can be set in the edict's flags field is "FL_KILLME".  When this
flag is set it tells the engine that this entity needs to be removed from the
level.  So you would have to constantly scan through any entities that you
are keeping track of to see if any of them have the FL_KILLME bit set.  This
would indicate that the MOD has "removed" them.  This is obviously more
complicated than the previous method of searching for entities, but might
be more CPU efficient in some cases.  The more CPU time you can save the more
bots somebody with a slower CPU will be able to create on their machine.

One of the problems that you may run into is that some events don't have any
network messages at all.  With the SDK 2.0 release Valve provided the ability
to create effects on the client.  This means that the server may send one
network message to the client when something is created (like a smoke bomb),
but the client may be the one to create the sprites, sounds, and decals.
You may never see a network message telling the client to create a smoke
screen.  The client may do this on it's own several seconds after the smoke
grenade is actually thrown.  However, one thing that is always true is that
if there is something on the HUD that changes then there will be a network
message that tells the client to display that thing or turn that thing off
or change the sprite used to display that thing.  For example in Counter-
Strike, when you enter or leave a buy zone, a message will get sent to the
HUD to tell it to display the buy zone shopping cart or to turn off the buy
zone shopping cart.

Also don't assume that something will be created or sent to the client in
a way that makes the most sense to you.  Sometimes you have to be creative
about how things are detected or intercepted.  Sometimes the MOD developers
will use a TextMsg network message to play a sound on the HUD, you might be
looking for a sound message for something and never find it, because it was
never actually sent.

I hope this document helps you understand how some things are done in the
HPB bot.  However, don't try to understand everything at one time.  You will
very quickly get overwhelmed by the amount of things that you must know to
get the bot to perform the same tasks as a human player.  Take things one
step at a time.  Work on one section until you understand how that one little
piece works, then move on to another section.  Remember that the HPB bot
wasn't created in a single day (or week or month) and any bot that you are
working on won't be created in a single day either.  Take things slow, try
to learn as you go and HAVE FUN!!!

botman