# Microsoft Developer Studio Project File - Name="HPB_bot_mm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=HPB_bot_mm - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HPB_bot_mm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HPB_bot_mm.mak" CFG="HPB_bot_mm - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HPB_bot_mm - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SDKSrc/Public/dlls", NVGBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MT /W3 /WX /GX /O2 /I "../metamod" /I "../../devtools/hlsdk-2.3/multiplayer/dlls" /I "../../devtools/hlsdk-2.3/multiplayer/engine" /I "../../devtools/hlsdk-2.3/multiplayer/pm_shared" /I "../../devtools/hlsdk-2.3/multiplayer/common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /Fr /YX /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386 /def:".\HPB_bot_mm.def"
# SUBTRACT LINK32 /profile /incremental:yes /map /debug
# Begin Target

# Name "HPB_bot_mm - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\bot.cpp
DEP_CPP_BOT_C=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\bot_func.h"\
	".\bot_weapons.h"\
	".\waypoint.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_BOT_C=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bot_chat.cpp
DEP_CPP_BOT_CH=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_BOT_CH=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bot_client.cpp
DEP_CPP_BOT_CL=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\bot_client.h"\
	".\bot_func.h"\
	".\bot_weapons.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_BOT_CL=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bot_combat.cpp
DEP_CPP_BOT_CO=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\bot_func.h"\
	".\bot_weapons.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_BOT_CO=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bot_models.cpp
DEP_CPP_BOT_M=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_BOT_M=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bot_navigate.cpp
DEP_CPP_BOT_N=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\bot_func.h"\
	".\bot_weapons.h"\
	".\waypoint.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_BOT_N=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bot_start.cpp
DEP_CPP_BOT_S=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\bot_func.h"\
	".\bot_weapons.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_BOT_S=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\dll.cpp
DEP_CPP_DLL_C=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\bot_func.h"\
	".\waypoint.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_DLL_C=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\entity_state.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\common\weaponinfo.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	"..\..\devtools\sdk\single-player source\pm_shared\pm_info.h"\
	
# End Source File
# Begin Source File

SOURCE=.\engine.cpp
DEP_CPP_ENGIN=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\bot_client.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_ENGIN=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\h_export.cpp
DEP_CPP_H_EXP=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_H_EXP=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\util.cpp
DEP_CPP_UTIL_=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\bot_func.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_UTIL_=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\common\usercmd.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# Begin Source File

SOURCE=.\waypoint.cpp
DEP_CPP_WAYPO=\
	"..\metamod\dllapi.h"\
	"..\metamod\engine_api.h"\
	"..\metamod\h_export.h"\
	"..\metamod\log_meta.h"\
	"..\metamod\meta_api.h"\
	"..\metamod\mhook.h"\
	"..\metamod\mreg.h"\
	"..\metamod\mutil.h"\
	"..\metamod\osdep.h"\
	"..\metamod\plinfo.h"\
	"..\metamod\sdk_util.h"\
	"..\metamod\types_meta.h"\
	".\bot.h"\
	".\waypoint.h"\
	"c:\program files\microsoft visual studio\vc98\include\basetsd.h"\
	
NODEP_CPP_WAYPO=\
	"..\..\devtools\sdk\single-player source\common\const.h"\
	"..\..\devtools\sdk\single-player source\common\crc.h"\
	"..\..\devtools\sdk\single-player source\common\cvardef.h"\
	"..\..\devtools\sdk\single-player source\common\event_flags.h"\
	"..\..\devtools\sdk\single-player source\common\in_buttons.h"\
	"..\..\devtools\sdk\single-player source\dlls\activity.h"\
	"..\..\devtools\sdk\single-player source\dlls\cdll_dll.h"\
	"..\..\devtools\sdk\single-player source\dlls\enginecallback.h"\
	"..\..\devtools\sdk\single-player source\dlls\extdll.h"\
	"..\..\devtools\sdk\single-player source\dlls\vector.h"\
	"..\..\devtools\sdk\single-player source\engine\archtypes.h"\
	"..\..\devtools\sdk\single-player source\engine\custom.h"\
	"..\..\devtools\sdk\single-player source\engine\edict.h"\
	"..\..\devtools\sdk\single-player source\engine\eiface.h"\
	"..\..\devtools\sdk\single-player source\engine\progdefs.h"\
	"..\..\devtools\sdk\single-player source\engine\sequence.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\bot.h
# End Source File
# Begin Source File

SOURCE=.\bot_client.h
# End Source File
# Begin Source File

SOURCE=.\bot_func.h
# End Source File
# Begin Source File

SOURCE=.\bot_weapons.h
# End Source File
# Begin Source File

SOURCE=.\waypoint.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
