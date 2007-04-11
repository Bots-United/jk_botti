/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//
// JK_Botti - be more human!
//
// bot_sound.cpp
//

#include <malloc.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "bot_weapon_select.h"
#include "waypoint.h"
#include "bot_sound.h"

typedef struct player_sound_list_s 
{ 
	int body_volume, weapon_volume, target_volume; 
} player_sound_list_t;

const Vector g_vecZero = Vector(0,0,0);

static CSoundEnt sSoundEnt;
CSoundEnt *pSoundEnt = &sSoundEnt;

static player_sound_list_t PlayerSoundList[32];


//
void SaveSound(edict_t * pEdict, const Vector & origin, int volume, int channel) 
{
   // save player sounds to player list
   int idx = ENTINDEX(pEdict) - 1;
   if(idx >= 0 && idx < gpGlobals->maxClients)
   {
      if(channel == CHAN_WEAPON)
      {
         if(PlayerSoundList[idx].weapon_volume < volume)
            PlayerSoundList[idx].weapon_volume = volume;
      }
      else
      {
         if(PlayerSoundList[idx].body_volume < volume)
            PlayerSoundList[idx].body_volume = volume;
      }
      
      return;
   }
   
   CSoundEnt::InsertSound(bits_SOUND_COMBAT, origin, volume, 5.0);
}

// reset player sounds list and global sound list for player
void ResetSound(edict_t * pEdict)
{
   // reset player sounds list
   int idx = ENTINDEX(pEdict) - 1;
   if(idx >= 0 && idx < gpGlobals->maxClients)
   {
      PlayerSoundList[idx].body_volume = 0;
      PlayerSoundList[idx].weapon_volume = 0;
      PlayerSoundList[idx].target_volume = 0;
   }
   
   // reset global sound list for player
   CSound * pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex ( pEdict ) );
   if(pSound)
      pSound->Reset();
}

// update player sounds
void UpdatePlayerSound(edict_t * pEdict)
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;
	
	int idx = ENTINDEX(pEdict) - 1;
	if(idx < 0 || idx >= gpGlobals->maxClients)
	{
		return;
	}

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt :: ClientSoundIndex( pEdict ) );

	if ( !pSound )
	{
		UTIL_ConsolePrintf ( "Client lost reserved sound!\n" );
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;


	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.
	iBodyVolume = PlayerSoundList[idx].body_volume;
	
	// decay body volume over time 
	PlayerSoundList[idx].body_volume = (int)(PlayerSoundList[idx].body_volume - 250 * gpGlobals->frametime);
	if ( PlayerSoundList[idx].body_volume < 0 )
	{
		PlayerSoundList[idx].body_volume = 0;
	}

	// convert player move speed and actions into sound audible by monsters.
	if ( PlayerSoundList[idx].weapon_volume  > iBodyVolume )
	{
		PlayerSoundList[idx].target_volume = PlayerSoundList[idx].weapon_volume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player. 
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		PlayerSoundList[idx].target_volume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	PlayerSoundList[idx].weapon_volume = (int)(PlayerSoundList[idx].weapon_volume - 250 * gpGlobals->frametime);
	if ( PlayerSoundList[idx].weapon_volume < 0 )
	{
		PlayerSoundList[idx].weapon_volume = 0;
	}


	// if target volume is greater than the player sound's current volume, we paste the new volume in 
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if ( PlayerSoundList[idx].target_volume > iVolume )
	{
		iVolume = PlayerSoundList[idx].target_volume;
	}
	else if ( iVolume > PlayerSoundList[idx].target_volume )
	{
		iVolume = (int)(iVolume - 250 * gpGlobals->frametime);

		if ( iVolume < PlayerSoundList[idx].target_volume )
		{
			iVolume = PlayerSoundList[idx].target_volume;
		}
	}

	if ( pSound )
	{
		pSound->m_vecOrigin = pEdict->v.origin;
		pSound->m_iType |= bits_SOUND_PLAYER;
		pSound->m_iVolume = iVolume;
	}
}


//=========================================================
// CSound - Clear - zeros all fields for a sound
//=========================================================
void CSound :: Clear ( void )
{
	m_vecOrigin		= g_vecZero;
	m_iType			= 0;
	m_iVolume		= 0;
	m_flExpireTime	= 0;
	m_iNext			= SOUNDLIST_EMPTY;
	m_iNextAudible	= 0;
}

//=========================================================
// Reset - clears the volume, origin, and type for a sound,
// but doesn't expire or unlink it. 
//=========================================================
void CSound :: Reset ( void )
{
	m_vecOrigin		= g_vecZero;
	m_iType			= 0;
	m_iVolume		= 0;
	m_iNext			= SOUNDLIST_EMPTY;
}

//=========================================================
// FIsSound - returns TRUE if the sound is an Audible sound
//=========================================================
BOOL CSound :: FIsSound ( void )
{
	if ( m_iType & ( bits_SOUND_COMBAT | bits_SOUND_WORLD | bits_SOUND_PLAYER | bits_SOUND_DANGER ) )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// FIsScent - returns TRUE if the sound is actually a scent
//=========================================================
BOOL CSound :: FIsScent ( void )
{
	if ( m_iType & ( bits_SOUND_CARCASS | bits_SOUND_MEAT | bits_SOUND_GARBAGE ) )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// Spawn 
//=========================================================
void CSoundEnt :: Spawn( void )
{
	Initialize();

	m_nextthink = gpGlobals->time + 1; 
}

//=========================================================
// Think - at interval, the entire active sound list is checked
// for sounds that have ExpireTimes less than or equal
// to the current world time, and these sounds are deallocated.
//=========================================================
void CSoundEnt :: Think ( void )
{
	int iSound;
	int iPreviousSound;

	m_nextthink = gpGlobals->time + 0.3;// how often to check the sound list.

	iPreviousSound = SOUNDLIST_EMPTY;
	iSound = m_iActiveSound; 

	while ( iSound != SOUNDLIST_EMPTY )
	{
		if ( m_SoundPool[ iSound ].m_flExpireTime <= gpGlobals->time && m_SoundPool[ iSound ].m_flExpireTime != SOUND_NEVER_EXPIRE )
		{
			int iNext = m_SoundPool[ iSound ].m_iNext;

			// move this sound back into the free list
			FreeSound( iSound, iPreviousSound );

			iSound = iNext;
		}
		else
		{
			iPreviousSound = iSound;
			iSound = m_SoundPool[ iSound ].m_iNext;
		}
	}

	if ( m_fShowReport )
	{
		UTIL_ConsolePrintf( "Soundlist: %d / %d  (%d)\n", ISoundsInList( SOUNDLISTTYPE_ACTIVE ),ISoundsInList( SOUNDLISTTYPE_FREE ), ISoundsInList( SOUNDLISTTYPE_ACTIVE ) - m_cLastActiveSounds );
		m_cLastActiveSounds = ISoundsInList ( SOUNDLISTTYPE_ACTIVE );
	}

}

//=========================================================
// Precache - dummy function
//=========================================================
void CSoundEnt :: Precache ( void )
{
}

//=========================================================
// FreeSound - clears the passed active sound and moves it 
// to the top of the free list. TAKE CARE to only call this
// function for sounds in the Active list!!
//=========================================================
void CSoundEnt :: FreeSound ( int iSound, int iPrevious )
{
	if ( !pSoundEnt )
	{
		// no sound ent!
		return;
	}

	if ( iPrevious != SOUNDLIST_EMPTY )
	{
		// iSound is not the head of the active list, so
		// must fix the index for the Previous sound
//		pSoundEnt->m_SoundPool[ iPrevious ].m_iNext = m_SoundPool[ iSound ].m_iNext;
		pSoundEnt->m_SoundPool[ iPrevious ].m_iNext = pSoundEnt->m_SoundPool[ iSound ].m_iNext;
	}
	else 
	{
		// the sound we're freeing IS the head of the active list.
		pSoundEnt->m_iActiveSound = pSoundEnt->m_SoundPool [ iSound ].m_iNext;
	}

	// make iSound the head of the Free list.
	pSoundEnt->m_SoundPool[ iSound ].m_iNext = pSoundEnt->m_iFreeSound;
	pSoundEnt->m_iFreeSound = iSound;
}

//=========================================================
// IAllocSound - moves a sound from the Free list to the 
// Active list returns the index of the alloc'd sound
//=========================================================
int CSoundEnt :: IAllocSound( void )
{
	int iNewSound;

	if ( m_iFreeSound == SOUNDLIST_EMPTY )
	{
		// no free sound!
		UTIL_ConsolePrintf( "Free Sound List is full!\n" );
		return SOUNDLIST_EMPTY;
	}

	// there is at least one sound available, so move it to the
	// Active sound list, and return its SoundPool index.
	
	iNewSound = m_iFreeSound;// copy the index of the next free sound

	m_iFreeSound = m_SoundPool[ m_iFreeSound ].m_iNext;// move the index down into the free list. 

	m_SoundPool[ iNewSound ].m_iNext = m_iActiveSound;// point the new sound at the top of the active list.

	m_iActiveSound = iNewSound;// now make the new sound the top of the active list. You're done.

	return iNewSound;
}

//=========================================================
// InsertSound - Allocates a free sound and fills it with 
// sound info.
//=========================================================
void CSoundEnt :: InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration )
{
	int	iThisSound;

	if ( !pSoundEnt )
	{
		// no sound ent!
		return;
	}

	iThisSound = pSoundEnt->IAllocSound();

	if ( iThisSound == SOUNDLIST_EMPTY )
	{
		UTIL_ConsolePrintf( "Could not AllocSound() for InsertSound() (DLL)\n" );
		return;
	}

	pSoundEnt->m_SoundPool[ iThisSound ].m_vecOrigin = vecOrigin;
	pSoundEnt->m_SoundPool[ iThisSound ].m_iType = iType;
	pSoundEnt->m_SoundPool[ iThisSound ].m_iVolume = iVolume;
	pSoundEnt->m_SoundPool[ iThisSound ].m_flExpireTime = gpGlobals->time + flDuration;
}

//=========================================================
// Initialize - clears all sounds and moves them into the 
// free sound list.
//=========================================================
void CSoundEnt :: Initialize ( void )
{
  	int i;
	int iSound;

	m_cLastActiveSounds = 0;
	m_iFreeSound = 0;
	m_iActiveSound = SOUNDLIST_EMPTY;

	for ( i = 0 ; i < MAX_WORLD_SOUNDS ; i++ )
	{// clear all sounds, and link them into the free sound list.
		m_SoundPool[ i ].Clear();
		m_SoundPool[ i ].m_iNext = i + 1;
	}

	m_SoundPool[ i - 1 ].m_iNext = SOUNDLIST_EMPTY;// terminate the list here.

	
	// now reserve enough sounds for each client
	for ( i = 0 ; i < gpGlobals->maxClients ; i++ )
	{
		iSound = pSoundEnt->IAllocSound();

		if ( iSound == SOUNDLIST_EMPTY )
		{
			UTIL_ConsolePrintf( "Could not AllocSound() for Client Reserve! (DLL)\n" );
			return;
		}

		pSoundEnt->m_SoundPool[ iSound ].m_flExpireTime = SOUND_NEVER_EXPIRE;
	}

	if ( CVAR_GET_FLOAT("displaysoundlist") == 1 )
	{
		m_fShowReport = TRUE;
	}
	else
	{
		m_fShowReport = FALSE;
	}
}

//=========================================================
// ISoundsInList - returns the number of sounds in the desired
// sound list.
//=========================================================
int CSoundEnt :: ISoundsInList ( int iListType )
{
	int i;
	int iThisSound;

	if ( iListType == SOUNDLISTTYPE_FREE )
	{
		iThisSound = m_iFreeSound;
	}
	else if ( iListType == SOUNDLISTTYPE_ACTIVE )
	{
		iThisSound = m_iActiveSound;
	}
	else
	{
		UTIL_ConsolePrintf( "Unknown Sound List Type!\n" );
		return 0;
	}

	if ( iThisSound == SOUNDLIST_EMPTY )
	{
		return 0;
	}

	i = 0;

	while ( iThisSound != SOUNDLIST_EMPTY )
	{
		i++;

		iThisSound = m_SoundPool[ iThisSound ].m_iNext;
	}

	return i;
}

//=========================================================
// ActiveList - returns the head of the active sound list
//=========================================================
int CSoundEnt :: ActiveList ( void )
{
	if ( !pSoundEnt )
	{
		return SOUNDLIST_EMPTY;
	}

	return pSoundEnt->m_iActiveSound;
}

//=========================================================
// FreeList - returns the head of the free sound list
//=========================================================
int CSoundEnt :: FreeList ( void )
{
	if ( !pSoundEnt )
	{
		return SOUNDLIST_EMPTY;
	}

	return pSoundEnt->m_iFreeSound;
}

//=========================================================
// SoundPointerForIndex - returns a pointer to the instance
// of CSound at index's position in the sound pool.
//=========================================================
CSound*	CSoundEnt :: SoundPointerForIndex( int iIndex )
{
	if ( !pSoundEnt )
	{
		return NULL;
	}

	if ( iIndex > ( MAX_WORLD_SOUNDS - 1 ) )
	{
		UTIL_ConsolePrintf( "SoundPointerForIndex() - Index too large!\n" );
		return NULL;
	}

	if ( iIndex < 0 )
	{
		UTIL_ConsolePrintf( "SoundPointerForIndex() - Index < 0!\n" );
		return NULL;
	}

	return &pSoundEnt->m_SoundPool[ iIndex ];
}

//=========================================================
// Clients are numbered from 1 to MAXCLIENTS, but the client
// reserved sounds in the soundlist are from 0 to MAXCLIENTS - 1,
// so this function ensures that a client gets the proper index
// to his reserved sound in the soundlist.
//=========================================================
int CSoundEnt :: ClientSoundIndex ( edict_t *pClient )
{
	int iReturn = ENTINDEX( pClient ) - 1;

#ifdef _DEBUG
	if ( iReturn < 0 || iReturn > gpGlobals->maxClients )
	{
		UTIL_ConsolePrintf( "** ClientSoundIndex returning a bogus value! **\n" );
	}
#endif // _DEBUG

	return iReturn;
}
