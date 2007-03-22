NOTE: The bot_logo.exe application may cause some systems to hang or
crash or do other nasty things to your computer.  I have not throughly
tested out this application on various types of computers.  Use this
application at your own risk!

Please don't e-mail me about problems using the bot_logo.exe application
since it is completely unsupported and will probably not be improved on
in future HPB bot releases.

If you have problems using the bot_logo.exe application, you can also
use Wally to edit the decals.wad file.


bot_logo - HPB bot custom spray logo manager (March 18th, 2001)

The bot_logo.exe application will allow you to create custom spray logos
for the bots to use.  It will ask for folder name that corresponds to the
MOD you wish to create logos for.  See the HPB_bot_ReadMe.txt file for a
list of folder names and the MODs that they correspond with.

Some MODs use the decals.wad file found in the Half-Life valve directory
instead of having their own decals.wad file.  If this is the case, you
will see a warning message letting you know that you will be editing the
decals.wad file found in the valve directory (since it is used by that MOD).

To add spray logos to the decals.wad file, you will have to have the bitmap
file (.bmp extension) in the Half-Life\logos folder before starting the
bot_logo.exe application.  When the bot_logo.exe application starts, you
will see a list of bitmap files in the left hand list box.  Each bitmap
file MUST be an uncompressed, 8 bit bitmap with a width and height no
larger than 256 pixels.  The width and the height MUST be even multiples
of 16 (16, 32, 48, 64, 80, ..., 240, 256).  If any of these properties
are not correct in the bitmap file, the file name will not appear in the
list box on the left.

You can click on the filename on the left to display that bitmap file.
For monochrome bitmaps, you can select a color from the pull down menu
(just above the "Exit" button).  Once you have selected the bitmap that
you want to use and the color that you wish to use, you can add the bitmap
by clicking on the "Add" button.  The bitmap will now show up in the list
box on the right.  A number will be appended to the filename to create a
unique logo name (so that you can have several different copies of the
same bitmap in different colors).

You can keep selecting bitmap files and colors and adding them to the
bot logo list until you are done.  If you wish to remove a bot logo from
the list box on the right, just select the logo you wish to remove and
click on the "Remove" button.

When you are done, you can save the bot logos by clicking on the "Exit"
button.  The bot_logo.exe application will ask if you wish to save the
changes you have made.  If you select "No", no changes will be made and
the program will exit.  If you select "Yes", the program will write the
modified bot logo list out to the decals.wad file.

If this is the first time you have edited the decals.wad file, you will
be given a message stating that the decals.wad file has been copied to
"decals_old.wad".  This allows you to restore the original MOD's decals.wad
file in the event that one being modified by the bot_logo.exe application
becomes corrupted.

The decals.wad file can also be edited using a WAD editor (like Wally).
The lump name format that I am using for the bot logos is the filename
and number combination preceeded by "{__" for color logos and "__" for
monochrome logos.  For example, if your file is a color bitmap named
"splat2" and this is the 3rd copy of this bitmap, the lump name would be
"{__splat2_03".


Adding bot logos to a dedicated server:

If you want to add bot logos on a Win32 or Linux dedicated server you
will need to create a file in the MOD directory called "bot_logo.cfg".
I have created a sample bot_logo.cfg file for you in the HPB bot directory
if you want to use this one.  Just copy the bot_logo.cfg file into your
MOD directory (i.e. C:\SIERRA\Half-Life\tfc\bot_logo.cfg) and the bots
will read this file when they are created.  The bot_logo.cfg file should
contain WAD lump names from the decals.wad file.  You must have each lump
name on a separate line and you must make sure the lump name EXACTLY matches
the lump name in the decals.wad file.  If the MOD has it's own decals.wad
file you can use lump names from it.  You can also use lump names from the
valve\decals.wad file (like I have done).  You can't add your own custom
spray logos to decals.wad and get them to show up on other clients machines.
You can only pick logos from the existing decals.wad file and use those
lumps as bot logos.  Use Wally or another WAD file editor to view the lump
names and images from the decals.wad file.

botman